/*
** $Id: ldo.c,v 2.38 2006/06/05 19:36:14 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in lua.h
*/


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#define ldo_c
#define LUA_CORE

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"
#include "lzio.h"




/*
** {======================================================
** Error-recovery functions
** =======================================================
*/


/* chain list of long jump buffers */
struct lua_longjmp {
  struct lua_longjmp *previous;
  luai_jmpbuf b;
  volatile int status;  /* error code */
};


void luaD_seterrorobj (lua_State *L, int errcode, StkId oldtop) {
  switch (errcode) {
    case LUA_ERRMEM: {
      setsvalue2s(L, oldtop, luaS_newliteral(L, MEMERRMSG));
      break;
    }
    case LUA_ERRERR: {
      setsvalue2s(L, oldtop, luaS_newliteral(L, "error in error handling"));
      break;
    }
    case LUA_ERREXC: {
      setsvalue2s(L, oldtop, luaS_newliteral(L, "unhandled C++ exception"));
      break;
    }
    case LUA_ERRSYNTAX:
    case LUA_ERRRUN: {
      setobjs2s(L, oldtop, L->top - 1);  /* error message on current top */
      break;
    }
  }
  L->top = oldtop + 1;
}


static void restore_stack_limit (lua_State *L) {
  lua_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  if (L->size_ci > LUAI_MAXCALLS) {  /* there was an overflow? */
    int inuse = cast_int(L->ci - L->base_ci);
    if (inuse + 1 < LUAI_MAXCALLS)  /* can `undo' overflow? */
      luaD_reallocCI(L, LUAI_MAXCALLS);
  }
}


static void resetstack (lua_State *L, int status) {
  L->ci = L->base_ci;
  L->base = L->ci->base;
  luaF_close(L, L->base);  /* close eventual pending closures */
  luaD_seterrorobj(L, status, L->base);
  L->nCcalls = LUA_NOYIELD | LUA_NOVPCALL;
  restore_stack_limit(L);
  L->errorJmp = NULL;
}


/* search for an error handler in the frame stack and call it */
static int call_errfunc (lua_State *L) {
  CallInfo *ci;
  for (ci = L->ci; ci > L->base_ci && ci->errfunc == 0; ci--) ;
  if (ci->errfunc >= 2) {
    StkId errfunc = ci->base + (ci->errfunc - 2);
    if (!ttisfunction(errfunc)) return LUA_ERRERR;
    setobjs2s(L, L->top, L->top - 1);  /* move argument */
    setobjs2s(L, L->top - 1, errfunc);  /* push function */
    incr_top(L);
    luaD_call(L, L->top - 2, 1, LUA_NOYIELD | LUA_NOVPCALL);  /* call it */
  }
  return LUA_ERRRUN;
}

void luaD_throw (lua_State *L, int errcode) {
  struct lua_longjmp *lj = L->errorJmp;
  if (lj) {
    if (errcode == LUA_ERRRUN)
      errcode = call_errfunc(L);
    lj->status = errcode;
    LUAI_THROW(L, lj);
  }
  else {
    L->status = cast_byte(errcode);
    if (G(L)->panic) {
      resetstack(L, errcode);
      lua_unlock(L);
      G(L)->panic(L);
    }
    exit(EXIT_FAILURE);
  }
}


int luaD_rawrunprotected (lua_State *L, Pfunc f, void *ud) {
  struct lua_longjmp lj;
  lj.status = 0;
  lj.previous = L->errorJmp;  /* chain new error handler */
  L->errorJmp = &lj;
  LUAI_TRY(L, &lj,
    lj.status = (*f)(L, ud);
  );
  L->errorJmp = lj.previous;  /* restore old error handler */
  return lj.status;
}


static int f_continue (lua_State *L, void *ud) {
  ptrdiff_t stop_ci = (ptrdiff_t)ud;
  while (L->ci > restoreci(L, stop_ci)) {  /* continue frames, top to bottom */
    CClosure *cc = &ci_func(L->ci)->c;
    L->ci->errfunc = 0;
    if (cc->isC) {  /* continue C function */
      int n;
      lua_assert(L->ctx != NULL);
      if (L->top > L->ci->top) L->ci->top = L->top;
      lua_unlock(L);
      n = (*cc->f)(L);  /* (re-)call it */
      lua_lock(L);
      if (n < 0) return LUA_YIELD;
      lua_assert(L->base + n <= L->top);
      luaD_poscall(L, L->top - n);
    }
    else {  /* continue Lua function */
      luaV_resume(L);
      if (luaV_execute(L)) return LUA_YIELD;
    }
  }
  return 0;
}


static int unwind_frames (lua_State *L, CallInfo *stop, ptrdiff_t old_top,
                          int status) {
  CallInfo *ci;
  StkId otop;
  for (ci = L->ci; ci > stop; ci--)
    if (ci->errfunc) {  /* found vpcall catch frame */
      L->ctx = (void *)(ptrdiff_t)status;
      otop = L->ci > ci ? (ci+1)->func : L->top - 1;
      goto found;
    }
  if (old_top == 0) return -1;  /* no unwind? */
  otop = restorestack(L, old_top);
found:
  luaF_close(L, otop);  /* close eventual pending closures */
  luaD_seterrorobj(L, status, otop);  /* sets L->top, too */
  L->ci = ci;
  L->base = ci->base;
  L->hookmask = ci->hookmask;
  restore_stack_limit(L);
  return ci == stop;
}


int luaD_pcall (lua_State *L, Pfunc func, void *ud, ptrdiff_t old_top,
                int ef, unsigned int flagmask) {
  int status;
  ptrdiff_t stop_ci = saveci(L, L->ci);
  unsigned int old_nCcalls = L->nCcalls;
  luaD_catch(L, ef);
  for (;;) {
    L->nCcalls = old_nCcalls & flagmask;
    status = luaD_rawrunprotected(L, func, ud);
    if (status == 0)
      break;
    if (unwind_frames(L, restoreci(L, stop_ci), old_top, status))
      break;
    func = f_continue;
    ud = (void *)stop_ci;
  }
  lua_assert(L->ci == restoreci(L, stop_ci));
  L->ci->errfunc = 0;
  L->nCcalls = old_nCcalls;
  return status;
}


/* }====================================================== */


static void correctstack (lua_State *L, TValue *oldstack) {
  CallInfo *ci;
  GCObject *up;
  L->top = (L->top - oldstack) + L->stack;
  for (up = L->openupval; up != NULL; up = up->gch.next)
    gco2uv(up)->v = (gco2uv(up)->v - oldstack) + L->stack;
  for (ci = L->base_ci; ci <= L->ci; ci++) {
    ci->top = (ci->top - oldstack) + L->stack;
    ci->base = (ci->base - oldstack) + L->stack;
    ci->func = (ci->func - oldstack) + L->stack;
  }
  L->base = (L->base - oldstack) + L->stack;
}


void luaD_reallocstack (lua_State *L, int newsize) {
  TValue *oldstack = L->stack;
  int realsize = newsize + 1 + EXTRA_STACK;
  lua_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  luaM_reallocvector(L, L->stack, L->stacksize, realsize, TValue);
  L->stacksize = realsize;
  L->stack_last = L->stack+newsize;
  correctstack(L, oldstack);
}


void luaD_reallocCI (lua_State *L, int newsize) {
  CallInfo *oldci = L->base_ci;
  luaM_reallocvector(L, L->base_ci, L->size_ci, newsize, CallInfo);
  L->size_ci = newsize;
  L->ci = (L->ci - oldci) + L->base_ci;
  L->end_ci = L->base_ci + L->size_ci - 1;
}


void luaD_growstack (lua_State *L, int n) {
  if (n <= L->stacksize)  /* double size is enough? */
    luaD_reallocstack(L, 2*L->stacksize);
  else
    luaD_reallocstack(L, L->stacksize + n);
}


static CallInfo *growCI (lua_State *L) {
  if (L->size_ci > LUAI_MAXCALLS)  /* overflow while handling overflow? */
    luaD_throw(L, LUA_ERRERR);
  else {
    luaD_reallocCI(L, 2*L->size_ci);
    if (L->size_ci > LUAI_MAXCALLS)
      luaG_runerror(L, "stack overflow");
  }
  return ++L->ci;
}


void luaD_callhook (lua_State *L, int event, int line) {
  lua_Hook hook = L->hook;
  if (hook && !nohooks(L)) {
    ptrdiff_t top = savestack(L, L->top);
    ptrdiff_t ci_top = savestack(L, L->ci->top);
    lua_Debug ar;
    unsigned short old_nCcalls;
    ar.event = event;
    ar.currentline = line;
    if (event == LUA_HOOKTAILRET)
      ar.i_ci = 0;  /* tail call; no debug information about it */
    else
      ar.i_ci = cast_int(L->ci - L->base_ci);
    luaD_checkstack(L, LUA_MINSTACK);  /* ensure minimum stack size */
    L->ci->top = L->top + LUA_MINSTACK;
    lua_assert(L->ci->top <= L->stack_last);
    old_nCcalls = L->nCcalls;
    L->nCcalls = old_nCcalls | (event >= LUA_HOOKLINE ?
                 (LUA_NOVPCALL | LUA_NOHOOKS) : /* line+count hook can yield */
                 (LUA_NOVPCALL | LUA_NOHOOKS | LUA_NOYIELD));
    lua_unlock(L);
    (*hook)(L, &ar);
    lua_lock(L);
    lua_assert(nohooks(L));
    L->nCcalls = old_nCcalls;  /* restore call flags */
    L->ci->top = restorestack(L, ci_top);
    L->top = restorestack(L, top);
    if (L->status == LUA_YIELD) {  /* handle hook yield here, after restore */
      L->base = L->top;  /* protect Lua frame, undo this in f_coresume */
      SAVEPC(L, GETPC(L) - 1);  /* correct pc */
      luaD_throw(L, LUA_YIELD);
    }
  }
}


static StkId adjust_varargs (lua_State *L, Proto *p, int actual) {
  int i;
  int nfixargs = p->numparams;
  Table *htab = NULL;
  StkId base, fixed;
  for (; actual < nfixargs; ++actual)
    setnilvalue(L->top++);
#if defined(LUA_COMPAT_VARARG)
  if (p->is_vararg & VARARG_NEEDSARG) { /* compat. with old-style vararg? */
    int nvar = actual - nfixargs;  /* number of extra arguments */
    lua_assert(p->is_vararg & VARARG_HASARG);
    luaC_checkGC(L);
    htab = luaH_new(L, nvar, 1);  /* create `arg' table */
    for (i=0; i<nvar; i++)  /* put extra arguments into `arg' table */
      setobj2n(L, luaH_setnum(L, htab, i+1), L->top - nvar + i);
    /* store counter in field `n' */
    setnvalue(luaH_setstr(L, htab, luaS_newliteral(L, "n")), cast_num(nvar));
  }
#endif
  /* move fixed parameters to final position */
  fixed = L->top - actual;  /* first fixed argument */
  base = L->top;  /* final position of first argument */
  for (i=0; i<nfixargs; i++) {
    setobjs2s(L, L->top++, fixed+i);
    setnilvalue(fixed+i);
  }
  /* add `arg' parameter */
  if (htab) {
    sethvalue(L, L->top++, htab);
    lua_assert(iswhite(obj2gco(htab)));
  }
  return base;
}


static StkId tryfuncTM (lua_State *L, StkId func) {
  const TValue *tm = luaT_gettmbyobj(L, func, TM_CALL);
  StkId p;
  ptrdiff_t funcr = savestack(L, func);
  if (!ttisfunction(tm))
    luaG_typeerror(L, func, "call");
  /* Open a hole inside the stack at `func' */
  for (p = L->top; p > func; p--) setobjs2s(L, p, p-1);
  incr_top(L);
  func = restorestack(L, funcr);  /* previous call may change stack */
  setobj2s(L, func, tm);  /* tag method is the new function to be called */
  return func;
}



#define inc_ci(L) \
  ((L->ci == L->end_ci) ? growCI(L) : \
   (condhardstacktests(luaD_reallocCI(L, L->size_ci)), ++L->ci))


int luaD_precall (lua_State *L, StkId func, int nresults) {
  Closure *cl;
  ptrdiff_t funcr;
  if (!ttisfunction(func)) /* `func' is not a function? */
    func = tryfuncTM(L, func);  /* check the `function' tag method */
  funcr = savestack(L, func);
  cl = clvalue(func);
  L->ci->ctx = L->ctx;
  if (!cl->l.isC) {  /* Lua function? prepare its call */
    CallInfo *ci;
    StkId st, base;
    Proto *p = cl->l.p;
    luaD_checkstack(L, p->maxstacksize);
    func = restorestack(L, funcr);
    if (!p->is_vararg) {  /* no varargs? */
      base = func + 1;
      if (L->top > base + p->numparams)
        L->top = base + p->numparams;
    }
    else {  /* vararg function */
      int nargs = cast_int(L->top - func) - 1;
      base = adjust_varargs(L, p, nargs);
      func = restorestack(L, funcr);  /* previous call may change the stack */
    }
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = func;
    L->base = ci->base = base;
    ci->top = L->base + p->maxstacksize;
    lua_assert(ci->top <= L->stack_last);
    SAVEPC(L, p->code);  /* starting point */
    ci->tailcalls = 0;
    ci->nresults = cast(short, nresults);
    ci->errfunc = 0;
    for (st = L->top; st < ci->top; st++)
      setnilvalue(st);
    L->top = ci->top;
    if (L->hookmask & LUA_MASKCALL) {
      const Instruction* pc = GETPC(L);
      SAVEPC(L, pc+1);  /* hooks assume 'pc' is already incremented */
      luaD_callhook(L, LUA_HOOKCALL, -1);
      SAVEPC(L, pc);  /* correct 'pc' */
    }
    return PCRLUA;
  }
  else {  /* if is a C function, call it */
    CallInfo *ci;
    int n;
    luaD_checkstack(L, LUA_MINSTACK);  /* ensure minimum stack size */
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = restorestack(L, funcr);
    L->base = ci->base = ci->func + 1;
    ci->top = L->top + LUA_MINSTACK;
    lua_assert(ci->top <= L->stack_last);
    L->ctx = NULL;  /* initial vcontext */
    ci->nresults = cast(short, nresults);
    ci->errfunc = 0;
    if (L->hookmask & LUA_MASKCALL)
      luaD_callhook(L, LUA_HOOKCALL, -1);
    lua_unlock(L);
    n = (*cl->c.f)(L);  /* do the actual call */
    lua_lock(L);
    if (n < 0)  /* yielding? */
      return PCRYIELD;
    else {
      luaD_poscall(L, L->top - n);
      return PCRC;
    }
  }
}


static StkId callrethooks (lua_State *L, StkId firstResult) {
  ptrdiff_t fr = savestack(L, firstResult);  /* next call may change stack */
  luaD_callhook(L, LUA_HOOKRET, -1);
  if (f_isLua(L->ci)) {  /* Lua function? */
    while (L->ci->tailcalls--)  /* call hook for eventual tail calls */
      luaD_callhook(L, LUA_HOOKTAILRET, -1);
  }
  return restorestack(L, fr);
}


int luaD_poscall (lua_State *L, StkId firstResult) {
  StkId res;
  int wanted, i;
  CallInfo *ci;
  if (L->hookmask & LUA_MASKRET)
    firstResult = callrethooks(L, firstResult);
  ci = L->ci--;
  res = ci->func;  /* res == final position of 1st result */
  wanted = ci->nresults;
  L->base = (ci - 1)->base;  /* restore base */
  L->ctx = (ci - 1)->ctx;  /* restore ctx */
  /* move results to correct place */
  for (i = wanted; i != 0 && firstResult < L->top; i--)
    setobjs2s(L, res++, firstResult++);
  while (i-- > 0)
    setnilvalue(res++);
  L->top = res;
  return (wanted - LUA_MULTRET);  /* 0 iff wanted == LUA_MULTRET */
}


/*
** Call a function (C or Lua). The function to be called is at *func.
** The arguments are on the stack, right after the function.
** When returns, all the results are on the stack, starting at the original
** function position.
*/ 
void luaD_call (lua_State *L, StkId func, int nresults, int callflags) {
  unsigned short old_nCcalls = L->nCcalls;
  int pcr;
  G(L)->cycles -= 100;
  L->nCcalls = (old_nCcalls + 8) | callflags;
  if (L->nCcalls >= LUAI_MAXCCALLS*8) {
    if (L->nCcalls < (LUAI_MAXCCALLS+1)*8)
      luaG_runerror(L, "C stack overflow");
    else if (L->nCcalls >= (LUAI_MAXCCALLS + (LUAI_MAXCCALLS>>3))*8)
      luaD_throw(L, LUA_ERRERR);  /* error while handing stack error */
  }
  pcr = luaD_precall(L, func, nresults);
  if ((pcr == PCRLUA && luaV_execute(L)) || pcr == PCRYIELD)
    luaD_throw(L, LUA_YIELD);  /* need to break C call boundary */
  luaC_checkGC(L);
  L->nCcalls = old_nCcalls;
}


static int f_coresume (lua_State *L, void *ud) {
  StkId base = cast(StkId, ud);
  if (L->ctx == NULL)  /* tail yield from C */
    luaD_poscall(L, base);  /* finish C call */
  else if (f_isLua(L->ci)) {  /* yield from Lua hook */
    L->base = L->ci->base;  /* restore invariant */
    if (luaV_execute(L)) return LUA_YIELD;
  }
  else {  /* resumable yield from C */
    StkId rbase = L->base;
    if (rbase < base) {
      while (base < L->top)
        setobjs2s(L, rbase++, base++);  /* move results down */
      L->top = rbase;
    }
    L->base = L->ci->base;  /* restore invariant */
  }
  return f_continue(L, (void *)(ptrdiff_t)0);  /* resume remaining frames */
}


static int f_costart (lua_State *L, void *ud) {
  int pcr = luaD_precall(L, cast(StkId, ud), LUA_MULTRET);
  if ((pcr == PCRLUA && luaV_execute(L)) || pcr == PCRYIELD)
    return LUA_YIELD;
  else
    return 0;
}


static int resume_error (lua_State *L, const char *msg) {
  L->top = L->ci->base;
  setsvalue2s(L, L->top, luaS_new(L, msg));
  incr_top(L);
  lua_unlock(L);
  return LUA_ERRRUN;
}


LUA_API int lua_resume (lua_State *L, int nargs) {
  Pfunc pf;
  void *ud;
  int status;
  lua_lock(L);
  switch (L->status) {
  case LUA_YIELD:
    pf = f_coresume;
    ud = L->top - nargs;
    L->status = 0;
    break;
  case 0:
    if (L->ci != L->base_ci)
      return resume_error(L, "cannot resume non-suspended coroutine");
    pf = f_costart;
    ud = L->top - (nargs + 1);
    break;
  default:
    return resume_error(L, "cannot resume dead coroutine");
  }
  lua_assert(cast(StkId, ud) >= L->base);
  for (;;) {
    L->nCcalls = 0;
    status = luaD_rawrunprotected(L, pf, ud);
    if (status <= LUA_YIELD)
      break;
    if (unwind_frames(L, L->base_ci, 0, status)) {  /* fallthrough? */
      /* keep frames for traceback, but mark coroutine as `dead' */
      luaD_seterrorobj(L, status, L->top);
      break;
    }
    pf = f_continue;
    ud = (void *)(ptrdiff_t)0;  /* (void *)saveci(L, L->base_ci); */
  }
  L->nCcalls = LUA_NOYIELD | LUA_NOVPCALL;
  L->status = status;
  lua_unlock(L);
  return status;
}


LUA_API int lua_vyield (lua_State *L, int nresults, void *ctx) {
  lua_lock(L);
  if (noyield(L))
    luaG_runerror(L, "attempt to yield across non-resumable call boundary");
  lua_assert(L->ci > L->base_ci);
  if (!f_isLua(L->ci)) {  /* usual yield */
    L->ctx = ctx;
    L->base = L->top - nresults;  /* no longer in sync with L->ci->base */
  }
  L->status = LUA_YIELD;  /* marker for luaD_callhook */
  lua_unlock(L);
  return -1;
}


/*
** Execute a protected parser.
*/
struct SParser {  /* data to `f_parser' */
  ZIO *z;
  Mbuffer buff;  /* buffer to be used by the scanner */
  const char *name;
};

static int f_parser (lua_State *L, void *ud) {
  int i;
  Proto *tf;
  Closure *cl;
  struct SParser *p = cast(struct SParser *, ud);
  int c = luaZ_lookahead(p->z);
  L->nCcalls |= (LUA_NOYIELD | LUA_NOVPCALL);  /* parser is not resumable */
  luaC_checkGC(L);
  tf = ((c == LUA_SIGNATURE[0]) ? luaU_undump : luaY_parser)(L, p->z,
                                                             &p->buff, p->name);
  cl = luaF_newLclosure(L, tf->nups, hvalue(gt(L)));
  cl->l.p = tf;
  for (i = 0; i < tf->nups; i++)  /* initialize eventual upvalues */
    cl->l.upvals[i] = luaF_newupval(L);
  setclvalue(L, L->top, cl);
  incr_top(L);
  return 0;
}


int luaD_protectedparser (lua_State *L, ZIO *z, const char *name) {
  struct SParser p;
  int status;
  p.z = z; p.name = name;
  luaZ_initbuffer(L, &p.buff);
  status = luaD_pcall(L, f_parser, &p, savestack(L, L->top),
                      -1, ~0);  /* -1 = inherit errfunc */
  luaZ_freebuffer(L, &p.buff);
  return status;
}


