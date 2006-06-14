/*
** $Id: lvm.c,v 1.284b 2003/04/03 13:35:34 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* needed only when `lua_number2str' uses `sprintf' */
#include <stdio.h>

#define lvm_c

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lvm.h"



/* function to convert a lua_Number to a string */
#ifndef lua_number2str
#define lua_number2str(s,n)     sprintf((s), LUA_NUMBER_FMT, (n))
#endif


/* limit for table tag-method chains (to avoid loops) */
#define MAXTAGLOOP	100


const TObject *luaV_tonumber (const TObject *obj, TObject *n) {
  lua_Number num;
  if (ttisnumber(obj)) return obj;
  if (ttisstring(obj) && luaO_str2d(svalue(obj), &num)) {
    setnvalue(n, num);
    return n;
  }
  else
    return NULL;
}


int luaV_tostring (lua_State *L, StkId obj) {
  if (!ttisnumber(obj))
    return 0;
  else {
    char s[32];  /* 16 digits, sign, point and \0  (+ some extra...) */
    lua_number2str(s, nvalue(obj));
    setsvalue2s(obj, luaS_new(L, s));
    return 1;
  }
}


static void traceexec (lua_State *L) {
  lu_byte mask = L->hookmask;
  if (mask & LUA_MASKCOUNT) {  /* instruction-hook set? */
    if (L->hookcount == 0) {
      resethookcount(L);
      luaD_callhook(L, LUA_HOOKCOUNT, -1);
      return;
    }
  }
  if (mask & LUA_MASKLINE) {
    CallInfo *ci = L->ci;
    Proto *p = ci_func(ci)->l.p;
    int newline = getline(p, pcRel(*ci->u.l.pc, p));
    if (!L->hookinit) {
      luaG_inithooks(L);
      return;
    }
    lua_assert(ci->state & CI_HASFRAME);
    if (pcRel(*ci->u.l.pc, p) == 0)  /* tracing may be starting now? */
      ci->u.l.savedpc = *ci->u.l.pc;  /* initialize `savedpc' */
    /* calls linehook when enters a new line or jumps back (loop) */
    if (*ci->u.l.pc <= ci->u.l.savedpc ||
        newline != getline(p, pcRel(ci->u.l.savedpc, p))) {
      luaD_callhook(L, LUA_HOOKLINE, newline);
      ci = L->ci;  /* previous call may reallocate `ci' */
    }
    ci->u.l.savedpc = *ci->u.l.pc;
  }
}


static int callTMres (lua_State *L, const TObject *f,
                       const TObject *p1, const TObject *p2) {
  int ret;                       
  setobj2s(L->top, f);  /* push function */
  setobj2s(L->top+1, p1);  /* 1st argument */
  setobj2s(L->top+2, p2);  /* 2nd argument */
  luaD_checkstack(L, 3);  /* cannot check before (could invalidate p1, p2) */
  L->top += 3;
  ret = luaD_call_yp(L, L->top - 3, 1);
  if (ret < 0) {	/* mm yielded */
     return -1;
  }
  L->top--;  /* result will be in L->top */
  return 0;
}



static int callTM (lua_State *L, const TObject *f,
                    const TObject *p1, const TObject *p2, const TObject *p3) {
  setobj2s(L->top, f);  /* push function */
  setobj2s(L->top+1, p1);  /* 1st argument */
  setobj2s(L->top+2, p2);  /* 2nd argument */
  setobj2s(L->top+3, p3);  /* 3th argument */
  luaD_checkstack(L, 4);  /* cannot check before (could invalidate p1...p3) */
  L->top += 4;
  return luaD_call_yp(L, L->top - 4, 0);
}


static const TObject *luaV_index (lua_State *L, const TObject *t,
                                  TObject *key, int loop) {
  const TObject *tm = fasttm(L, hvalue(t)->metatable, TM_INDEX);
  if (tm == NULL) return &luaO_nilobject;  /* no TM */
  if (ttisfunction(tm)) {
    if (callTMres(L, tm, t, key) < 0) {		/* index metamethod yielded */
      return 0;
    }
    return L->top;
  }
  else return luaV_gettable(L, tm, key, loop);
}

static const TObject *luaV_getnotable (lua_State *L, const TObject *t,
                                       TObject *key, int loop) {
  const TObject *tm = luaT_gettmbyobj(L, t, TM_INDEX);
  if (ttisnil(tm))
    luaG_typeerror(L, t, "index");
  if (ttisfunction(tm)) {
    if (callTMres(L, tm, t, key) < 0) {		/* index metamethod yielded */
      return 0;
    }
    return L->top;
  }
  else return luaV_gettable(L, tm, key, loop);
}


/*
** Function to index a table.
** Receives the table at `t' and the key at `key'.
** leaves the result at `res'.
*/
const TObject *luaV_gettable (lua_State *L, const TObject *t, TObject *key,
                              int loop) {
  if (loop > MAXTAGLOOP)
    luaG_runerror(L, "loop in gettable");
  if (ttistable(t)) {  /* `t' is a table? */
    Table *h = hvalue(t);
    const TObject *v = luaH_get(h, key);  /* do a primitive get */
    if (!ttisnil(v)) return v;
    else return luaV_index(L, t, key, loop+1);
  }
  else return luaV_getnotable(L, t, key, loop+1);
}


/*
** Receives table at `t', key at `key' and value at `val'.
*/
int luaV_settable (lua_State *L, const TObject *t, TObject *key, StkId val) {
  const TObject *tm;
  int loop = 0;
  do {
    if (ttistable(t)) {  /* `t' is a table? */
      Table *h = hvalue(t);
      TObject *oldval = luaH_set(L, h, key); /* do a primitive set */
      if (!ttisnil(oldval) ||  /* result is no nil? */
          (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) { /* or no TM? */
        setobj2t(oldval, val);  /* write barrier */
        return 0;
      }
      /* else will try the tag method */
    }
    else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_NEWINDEX)))
      luaG_typeerror(L, t, "index");
    if (ttisfunction(tm)) {
      return callTM(L, tm, t, key, val);
    }
    t = tm;  /* else repeat with `tm' */ 
  } while (++loop <= MAXTAGLOOP);
  luaG_runerror(L, "loop in settable");
  return 0;
}


static int call_binTM (lua_State *L, const TObject *p1, const TObject *p2,
                       StkId res, TMS event) {
  ptrdiff_t result = savestack(L, res);
  const TObject *tm = luaT_gettmbyobj(L, p1, event);  /* try first operand */
  if (ttisnil(tm))
    tm = luaT_gettmbyobj(L, p2, event);  /* try second operand */
  if (!ttisfunction(tm)) return 0;
  if (callTMres(L, tm, p1, p2) < 0) {	/* mm yielded */
     return -1;
  }
  res = restorestack(L, result);  /* previous call may change stack */
  setobjs2s(res, L->top);
  return 1;
}


static const TObject *get_compTM (lua_State *L, Table *mt1, Table *mt2,
                                  TMS event) {
  const TObject *tm1 = fasttm(L, mt1, event);
  const TObject *tm2;
  if (tm1 == NULL) return NULL;  /* no metamethod */
  if (mt1 == mt2) return tm1;  /* same metatables => same metamethods */
  tm2 = fasttm(L, mt2, event);
  if (tm2 == NULL) return NULL;  /* no metamethod */
  if (luaO_rawequalObj(tm1, tm2))  /* same metamethods? */
    return tm1;
  return NULL;
}


static int call_orderTM (lua_State *L, const TObject *p1, const TObject *p2,
                         TMS event) {
  const TObject *tm1 = luaT_gettmbyobj(L, p1, event);
  const TObject *tm2;
  if (ttisnil(tm1)) return -1;  /* no metamethod? */
  tm2 = luaT_gettmbyobj(L, p2, event);
  if (!luaO_rawequalObj(tm1, tm2))  /* different metamethods? */
    return -1;
  if (callTMres(L, tm1, p1, p2) < 0) 	/* mm yielded */
    return -2;
  return !l_isfalse(L->top);
}


static int luaV_strcmp (const TString *ls, const TString *rs) {
  const char *l = getstr(ls);
  size_t ll = ls->tsv.len;
  const char *r = getstr(rs);
  size_t lr = rs->tsv.len;
  for (;;) {
    int temp = strcoll(l, r);
    if (temp != 0) return temp;
    else {  /* strings are equal up to a `\0' */
      size_t len = strlen(l);  /* index of first `\0' in both strings */
      if (len == lr)  /* r is finished? */
        return (len == ll) ? 0 : 1;
      else if (len == ll)  /* l is finished? */
        return -1;  /* l is smaller than r (because r is not finished) */
      /* both strings longer than `len'; go on comparing (after the `\0') */
      len++;
      l += len; ll -= len; r += len; lr -= len;
    }
  }
}


int luaV_lessthan (lua_State *L, const TObject *l, const TObject *r) {
  int res;
  if (ttype(l) != ttype(r))
    return luaG_ordererror(L, l, r);
  else if (ttisnumber(l))
    return nvalue(l) < nvalue(r);
  else if (ttisstring(l))
    return luaV_strcmp(tsvalue(l), tsvalue(r)) < 0;
  else if ((res = call_orderTM(L, l, r, TM_LT)) != -1) 
    return res;
  return luaG_ordererror(L, l, r);
}


static int luaV_lessequal (lua_State *L, const TObject *l, const TObject *r) {
  int res;
  if (ttype(l) != ttype(r))
    return luaG_ordererror(L, l, r);
  else if (ttisnumber(l))
    return nvalue(l) <= nvalue(r);
  else if (ttisstring(l))
    return luaV_strcmp(tsvalue(l), tsvalue(r)) <= 0;
  else if ((res = call_orderTM(L, l, r, TM_LE)) != -1)  /* first try `le' */
    return res;
  else if ((res = call_orderTM(L, r, l, TM_LT)) != -1)  /* else try `lt' */
    return !res;
  return luaG_ordererror(L, l, r);
}


int luaV_equalval (lua_State *L, const TObject *t1, const TObject *t2) {
  const TObject *tm;
  lua_assert(ttype(t1) == ttype(t2));
  switch (ttype(t1)) {
    case LUA_TNIL: return 1;
    case LUA_TNUMBER: return nvalue(t1) == nvalue(t2);
    case LUA_TBOOLEAN: return bvalue(t1) == bvalue(t2);  /* true must be 1 !! */
    case LUA_TLIGHTUSERDATA: return pvalue(t1) == pvalue(t2);
    case LUA_TUSERDATA: {
      if (uvalue(t1) == uvalue(t2)) return 1;
      tm = get_compTM(L, uvalue(t1)->uv.metatable, uvalue(t2)->uv.metatable,
                         TM_EQ);
      break;  /* will try TM */
    }
    case LUA_TTABLE: {
      if (hvalue(t1) == hvalue(t2)) return 1;
      tm = get_compTM(L, hvalue(t1)->metatable, hvalue(t2)->metatable, TM_EQ);
      break;  /* will try TM */
    }
    default: return gcvalue(t1) == gcvalue(t2);
  }
  if (tm == NULL) return 0;  /* no TM? */
  if (callTMres(L, tm, t1, t2) < 0)  /* call TM */
    return -1;			/* yield */
  return !l_isfalse(L->top);
}


int luaV_concat (lua_State *L, int total, int last) {
  do {
    StkId top = L->base + last + 1;
    int n = 2;  /* number of elements handled in this pass (at least 2) */
    if (!tostring(L, top-2) || !tostring(L, top-1)) {
      int res = call_binTM(L, top-2, top-1, top-2, TM_CONCAT);
      if (res < 0)	/* mm yielded */
        return -1;
      else if (!res)
        luaG_concaterror(L, top-2, top-1);
    } else if (tsvalue(top-1)->tsv.len > 0) {  /* if len=0, do nothing */
      /* at least two string values; get as many as possible */
      lu_mem tl = cast(lu_mem, tsvalue(top-1)->tsv.len) +
                  cast(lu_mem, tsvalue(top-2)->tsv.len);
      char *buffer;
      int i;
      while (n < total && tostring(L, top-n-1)) {  /* collect total length */
        tl += tsvalue(top-n-1)->tsv.len;
        n++;
      }
      if (tl > MAX_SIZET) luaG_runerror(L, "string size overflow");
      buffer = luaZ_openspace(L, &G(L)->buff, tl);
      tl = 0;
      for (i=n; i>0; i--) {  /* concat all strings */
        size_t l = tsvalue(top-i)->tsv.len;
        memcpy(buffer+tl, svalue(top-i), l);
        tl += l;
      }
      setsvalue2s(top-n, luaS_newlstr(L, buffer, tl));
    }
    total -= n-1;  /* got `n' strings to create 1 new */
    last -= n-1;
  } while (total > 1);  /* repeat until only 1 result left */
  return 0;
}


static int Arith (lua_State *L, StkId ra,
                   const TObject *rb, const TObject *rc, TMS op) {
  TObject tempb, tempc;
  const TObject *b, *c;
  if ((b = luaV_tonumber(rb, &tempb)) != NULL &&
      (c = luaV_tonumber(rc, &tempc)) != NULL) {
    switch (op) {
      case TM_ADD: setnvalue(ra, nvalue(b) + nvalue(c)); break;
      case TM_SUB: setnvalue(ra, nvalue(b) - nvalue(c)); break;
      case TM_MUL: setnvalue(ra, nvalue(b) * nvalue(c)); break;
      case TM_DIV: setnvalue(ra, nvalue(b) / nvalue(c)); break;
      case TM_POW: {
        const TObject *f = luaH_getstr(hvalue(gt(L)), G(L)->tmname[TM_POW]);
        ptrdiff_t res = savestack(L, ra);
        if (!ttisfunction(f))
          luaG_runerror(L, "`__pow' (`^' operator) is not a function");
        if (callTMres(L, f, b, c) < 0)		/* mm yielded */
          return -1;
        ra = restorestack(L, res);  /* previous call may change stack */
        setobjs2s(ra, L->top);
        break;
      }
      default: lua_assert(0); break;
    }
  }
  else {
    int mmstat = call_binTM(L, rb, rc, ra, op);
    if (mmstat < 0)		/* mm yielded */
      return -1;
    luaG_aritherror(L, rb, rc);
  }
  return 0;
}



/*
** some macros for common tasks in `luaV_execute'
*/

#define runtime_check(L, c)	{ if (!(c)) return 0; }

#define RA(i)	(base+GETARG_A(i))
/* to be used after possible stack reallocation */
#define XRA(i)	(L->base+GETARG_A(i))
#define RB(i)	(base+GETARG_B(i))
#define RKB(i)	((GETARG_B(i) < MAXSTACK) ? RB(i) : k+GETARG_B(i)-MAXSTACK)
#define RC(i)	(base+GETARG_C(i))
#define RKC(i)	((GETARG_C(i) < MAXSTACK) ? RC(i) : k+GETARG_C(i)-MAXSTACK)
#define KBx(i)	(k+GETARG_Bx(i))


#define dojump(pc, i)	((pc) += (i))


/* This function handles returning to an opcode (or C function) that has been partially executed.
 * It is used in the implementation of OP_RETURN and lua_resume. */

StkId luaV_return (lua_State *L, CallInfo *ci, StkId firstResult, int *yielded)
{
  int nresults;
  *yielded = 0;		/* return flag to indicate whether the C function yielded when we resumed it */
  
  while ((ci->state & CI_C) && (ci->state & CI_CALLING)) {
    int n;

    /* We're returning into a C function. Complete the function call. */
    
    nresults = L->top - firstResult;	/* number of actual return values for LUA_MULTRET */
    luaD_poscall(L, ci->u.c.nresults, firstResult);
    ci->state &= ~CI_CALLING;		/* API checks want this flag off */
    
    if (ci->state & CI_CTAILCALL) {
      /* C function tail called. */

      ci->state &= ~CI_CTAILCALL;
      L->nCcalls--;
      if (ci->u.c.nresults >= 0)
        firstResult = L->top - ci->u.c.nresults;
      else
        firstResult = L->top - nresults;
      ci = L->ci - 1;
      continue;
    }
    
    lua_unlock(L);
    n = (*clvalue(L->base - 1)->c.f)(L);  /* do the actual call */
    lua_lock(L);
    if (n >= 0) {		/* function returned */
      L->nCcalls--;
      firstResult = L->top - n;
      ci = L->ci - 1;
    } else {			/* function yielded */
      if (ci->u.c.savetop) {
        L->base = L->ci->base;	/* restore stack ptrs */
        L->top = ci->u.c.savetop;
      }
      *yielded = 1;
      return NULL;
    }
  }
  
  /* See if our C frame is supposed to do the luaD_poscall(), or someone else up the chain. */

  if (!(ci->state & CI_CALLING)) {
    return firstResult;
  }

  lua_assert(ci->state & CI_SAVEDPC);
  switch (GET_OPCODE(*(ci->u.l.savedpc - 1)))
  {
    /* Normal function calls */
    
    case OP_CALL:
    case OP_TAILCALL:
      /* finish execution of call */
      nresults = GETARG_C(*(ci->u.l.savedpc - 1)) - 1;
      luaD_poscall(L, nresults, firstResult);  /* complete it */
      if (nresults >= 0) L->top = L->ci->top;
      break;
    
    /* Metamethod invoked via callTMres() yielded */
        
    case OP_GETTABLE:
    case OP_GETGLOBAL:
    case OP_SELF:
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_POW:
    case OP_CONCAT:
    case OP_UNM:
      /* finish execution of metamethod call */
      luaD_poscall(L, 1, firstResult);  /* complete it */
      L->top = L->ci->top;
        
      /* finish execution of get instruction */
      setobj2s(XRA(*(ci->u.l.savedpc - 1)), L->top);
      break;
      
    case OP_LT:
    case OP_LE:
    case OP_EQ:
      /* finish execution of metamethod call */
      luaD_poscall(L, 1, firstResult);  /* complete it */
      L->top = L->ci->top;
        
      /* finish execution of branch */
      if (!l_isfalse(L->top) != GETARG_A(*(ci->u.l.savedpc - 1)))
        ci->u.l.savedpc++;
      else
        dojump(ci->u.l.savedpc, GETARG_sBx(*(ci->u.l.savedpc)) + 1);
      break;
      
    /* Metamethod invoked via callTM() yielded */
      
    case OP_SETTABLE:
    case OP_SETGLOBAL:
      /* finish execution of metamethod call */
      luaD_poscall(L, 0, firstResult);  /* complete it */
      L->top = L->ci->top;
      break;
      
    /* Other resumable opcodes */
    
    case OP_TFORLOOP:
    {
      Instruction i = *(ci->u.l.savedpc - 1);
      int nvar = GETARG_C(i) + 1;
      StkId ra;
      StkId cb = ra + nvar + 2;
      
      /* finish execution of iterator call */
      luaD_poscall(L, 3, firstResult);  /* complete it */
      L->top = L->ci->top;
      
      ra = XRA(i) + 2;  /* final position of first result */
      cb = ra + nvar;
      do {  /* move results to proper positions */
        nvar--;
        setobjs2s(ra+nvar, cb+nvar);
      } while (nvar > 0);
      if (ttisnil(ra))  /* break loop? */
        ci->u.l.savedpc++;  /* skip jump (break loop) */
      else
        dojump(ci->u.l.savedpc, GETARG_sBx(*(ci->u.l.savedpc)) + 1);  /* jump back */
      break;
    }
        
    default:
      printf("returning to unexpected opcode\n");
      abort();
  }
  return NULL;
}

/* These macros handle calling a function which may invoke a metamethod that yields. If a yield occurs,
 * the PC is saved, and luaV_execute() returns. Note that if you change a new opcode to use one of
 * these macros, then the opcode must be patched into luaV_return() above to complete the setobj2s
 * (or whatever operation needs to occur after the metamethod returns) when the coroutine resumes.
 */

/* This macro takes an argument which is a pointer to a TObject (if call completed) or NULL if call yielded. */

#define SETOBJ2S_YP(src, val) do { \
	const TObject *ri; \
	int ci_off = L->ci - L->base_ci; /* save our ci pointer, because we don't know how much it will change if the metamethod yields */ \
	ri = (val); \
	if (ri) { \
	  setobj2s((src), ri); \
	} else { \
	  CallInfo *ci = L->base_ci + ci_off; \
          ci->u.l.savedpc = pc;  /* save `pc' to return later */ \
          ci->state = (CI_SAVEDPC | CI_CALLING); \
          return NULL; \
        } \
} while (0)      

/* This macro takes an argument which is the return value from luaD_call_yp (0 if completed, -1 if yield). */

#define YP(res) do { \
	int ci_off = L->ci - L->base_ci; /* save our ci pointer, because we don't know how much it will change if the metamethod yields */ \
	if (res < 0) { \
	  CallInfo *ci = L->base_ci + ci_off; \
          ci->u.l.savedpc = pc;  /* save `pc' to return later */ \
          ci->state = (CI_SAVEDPC | CI_CALLING); \
          return NULL; \
        } \
} while (0)      
	  

StkId luaV_execute (lua_State *L) {
  LClosure *cl;
  TObject *k;
  const Instruction *pc;
 callentry:  /* entry point when calling new functions */
  if (L->hookmask & LUA_MASKCALL) {
    L->ci->u.l.pc = &pc;
    luaD_callhook(L, LUA_HOOKCALL, -1);
  }
 retentry:  /* entry point when returning to old functions */
  L->ci->u.l.pc = &pc;
  lua_assert(L->ci->state == CI_SAVEDPC ||
             L->ci->state == (CI_SAVEDPC | CI_CALLING));
  L->ci->state = CI_HASFRAME;  /* activate frame */
  pc = L->ci->u.l.savedpc;
  cl = &clvalue(L->base - 1)->l;
  k = cl->p->k;
  /* main loop of interpreter */
  for (;;) {
    const Instruction i = *pc++;
    if (G(L)->cyclesleft) {
        if (--G(L)->cyclesleft <= 0) {
            G(L)->cyclesleft = -1;
            luaG_runerror(L, "CPU limit exceeded");
        }
    }
    StkId base, ra;
    if ((L->hookmask & (LUA_MASKLINE | LUA_MASKCOUNT)) &&
        (--L->hookcount == 0 || L->hookmask & LUA_MASKLINE)) {
      traceexec(L);
      if (L->ci->state & CI_YIELD) {  /* did hook yield? */
        L->ci->u.l.savedpc = pc - 1;
        L->ci->state = CI_YIELD | CI_SAVEDPC;
        return NULL;
      }
    }
    /* warning!! several calls may realloc the stack and invalidate `ra' */
    base = L->base;
    ra = RA(i);
    lua_assert(L->ci->state & CI_HASFRAME);
    lua_assert(base == L->ci->base);
    lua_assert(L->top <= L->stack + L->stacksize && L->top >= base);
    lua_assert(L->top == L->ci->top ||
         GET_OPCODE(i) == OP_CALL ||   GET_OPCODE(i) == OP_TAILCALL ||
         GET_OPCODE(i) == OP_RETURN || GET_OPCODE(i) == OP_SETLISTO);
    switch (GET_OPCODE(i)) {
      case OP_MOVE: {
        setobjs2s(ra, RB(i));
        break;
      }
      case OP_LOADK: {
        setobj2s(ra, KBx(i));
        break;
      }
      case OP_LOADBOOL: {
        setbvalue(ra, GETARG_B(i));
        if (GETARG_C(i)) pc++;  /* skip next instruction (if C) */
        break;
      }
      case OP_LOADNIL: {
        TObject *rb = RB(i);
        do {
          setnilvalue(rb--);
        } while (rb >= ra);
        break;
      }
      case OP_GETUPVAL: {
        int b = GETARG_B(i);
        setobj2s(ra, cl->upvals[b]->v);
        break;
      }
      case OP_GETGLOBAL: {
        TObject *rb = KBx(i);
        const TObject *v;
        lua_assert(ttisstring(rb) && ttistable(&cl->g));
        v = luaH_getstr(hvalue(&cl->g), tsvalue(rb));
        if (!ttisnil(v)) { setobj2s(ra, v); }
        else
          SETOBJ2S_YP(XRA(i), luaV_index(L, &cl->g, rb, 0));
        break;
      }
      case OP_GETTABLE: {
        StkId rb = RB(i);
        TObject *rc = RKC(i);
        if (ttistable(rb)) {
          const TObject *v = luaH_get(hvalue(rb), rc);
          if (!ttisnil(v)) { setobj2s(ra, v); }
          else {
            SETOBJ2S_YP(XRA(i), luaV_index(L, rb, rc, 0));
          }
        }
        else {
          SETOBJ2S_YP(XRA(i), luaV_getnotable(L, rb, rc, 0));
        }
        break;
      }
      case OP_SETGLOBAL: {
        lua_assert(ttisstring(KBx(i)) && ttistable(&cl->g));
        YP(luaV_settable(L, &cl->g, KBx(i), ra));
        break;
      }
      case OP_SETUPVAL: {
        int b = GETARG_B(i);
        setobj(cl->upvals[b]->v, ra);  /* write barrier */
        break;
      }
      case OP_SETTABLE: {
        YP(luaV_settable(L, ra, RKB(i), RKC(i)));
        break;
      }
      case OP_NEWTABLE: {
        int b = GETARG_B(i);
        b = fb2int(b);
        sethvalue(ra, luaH_new(L, b, GETARG_C(i)));
        luaC_checkGC(L);
        break;
      }
      case OP_SELF: {
        StkId rb = RB(i);
        TObject *rc = RKC(i);
        runtime_check(L, ttisstring(rc));
        setobjs2s(ra+1, rb);
        if (ttistable(rb)) {
          const TObject *v = luaH_getstr(hvalue(rb), tsvalue(rc));
          if (!ttisnil(v)) { setobj2s(ra, v); }
          else
            SETOBJ2S_YP(XRA(i), luaV_index(L, rb, rc, 0));
        }
        else
          SETOBJ2S_YP(XRA(i), luaV_getnotable(L, rb, rc, 0));
        break;
      }
      case OP_ADD: {
        TObject *rb = RKB(i);
        TObject *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) + nvalue(rc));
        }
        else
          YP(Arith(L, ra, rb, rc, TM_ADD));
        break;
      }
      case OP_SUB: {
        TObject *rb = RKB(i);
        TObject *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) - nvalue(rc));
        }
        else
          YP(Arith(L, ra, rb, rc, TM_SUB));
        break;
      }
      case OP_MUL: {
        TObject *rb = RKB(i);
        TObject *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) * nvalue(rc));
        }
        else
          YP(Arith(L, ra, rb, rc, TM_MUL));
        break;
      }
      case OP_DIV: {
        TObject *rb = RKB(i);
        TObject *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) / nvalue(rc));
        }
        else
          YP(Arith(L, ra, rb, rc, TM_DIV));
        break;
      }
      case OP_POW: {
        YP(Arith(L, ra, RKB(i), RKC(i), TM_POW));
        break;
      }
      case OP_UNM: {
        const TObject *rb = RB(i);
        TObject temp;
        if (tonumber(rb, &temp)) {
          setnvalue(ra, -nvalue(rb));
        }
        else {
          int ci_off = L->ci - L->base_ci;
          int rv;
          setnilvalue(&temp);
          rv = call_binTM(L, RB(i), &temp, ra, TM_UNM);
          if (rv < 0) {
            CallInfo *ci = L->base_ci + ci_off;
            ci->u.l.savedpc = pc;  /* save `pc' to return later */
            ci->state = (CI_SAVEDPC | CI_CALLING);
            return NULL;
          }
          if (!rv)
            luaG_aritherror(L, RB(i), &temp);
        }
        break;
      }
      case OP_NOT: {
        int res = l_isfalse(RB(i));  /* next assignment may change this value */
        setbvalue(ra, res);
        break;
      }
      case OP_CONCAT: {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        YP(luaV_concat(L, c-b+1, c));  /* may change `base' (and `ra') */
        base = L->base;
        setobjs2s(RA(i), base+b);
        luaC_checkGC(L);
        break;
      }
      case OP_JMP: {
        dojump(pc, GETARG_sBx(i));
        break;
      }
      case OP_EQ: {
        int ci_off = L->ci - L->base_ci;
        int rv = 0; /* default to false, if type tags are not equal */
        if (ttype(RKB(i)) == ttype(RKC(i))) {
          rv = luaV_equalval(L, RKB(i), RKC(i));
          if (rv < 0) {
            CallInfo *ci = L->base_ci + ci_off;
            ci->u.l.savedpc = pc;  /* save `pc' to return later */
            ci->state = (CI_SAVEDPC | CI_CALLING);
            return NULL;
          }
        }
        if (rv != GETARG_A(i)) pc++;
        else dojump(pc, GETARG_sBx(*pc) + 1);
        break;
      }
      case OP_LT: {
        int ci_off = L->ci - L->base_ci;
        int rv = luaV_lessthan(L, RKB(i), RKC(i));
        if (rv < 0) {
          CallInfo *ci = L->base_ci + ci_off;
          ci->u.l.savedpc = pc;  /* save `pc' to return later */
          ci->state = (CI_SAVEDPC | CI_CALLING);
          return NULL;
        }
        if (rv != GETARG_A(i)) pc++;
        else dojump(pc, GETARG_sBx(*pc) + 1);
        break;
      }
      case OP_LE: {
        int ci_off = L->ci - L->base_ci;
        int rv = luaV_lessequal(L, RKB(i), RKC(i));
        if (rv < 0) {
          CallInfo *ci = L->base_ci + ci_off;
          ci->u.l.savedpc = pc;  /* save `pc' to return later */
          ci->state = (CI_SAVEDPC | CI_CALLING);
          return NULL;
        }
        if (rv != GETARG_A(i)) pc++;
        else dojump(pc, GETARG_sBx(*pc) + 1);
        break;
      }
      case OP_TEST: {
        TObject *rb = RB(i);
        if (l_isfalse(rb) == GETARG_C(i)) pc++;
        else {
          setobjs2s(ra, rb);
          dojump(pc, GETARG_sBx(*pc) + 1);
        }
        break;
      }
      case OP_CALL:
      case OP_TAILCALL: {
        int ci_off = L->ci - L->base_ci; /* memorize current CallInfo, as luaD_precall can add arbitrary stack frames */
        StkId firstResult;
        int b = GETARG_B(i);
        int nresults;
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        nresults = GETARG_C(i) - 1;
        firstResult = luaD_precall(L, ra);
        if (firstResult) {
          if (firstResult > L->top) {  /* yield? */
            CallInfo *ci = L->base_ci + ci_off;
            lua_assert(L->ci->state == (CI_C | CI_YIELD));
            ci->u.l.savedpc = pc;
            ci->state = (CI_SAVEDPC | CI_CALLING);	/* set CI_CALLING as we may have to return through this frame */
            return NULL;
          }
          /* it was a C function (`precall' called it); adjust results */
          luaD_poscall(L, nresults, firstResult);
          if (nresults >= 0) L->top = L->ci->top;
        }
        else {  /* it is a Lua function */
          CallInfo *ci = L->base_ci + ci_off;
          if (GET_OPCODE(i) == OP_CALL) {  /* regular call? */
            ci->u.l.savedpc = pc;  /* save `pc' to return later */
            ci->state = (CI_SAVEDPC | CI_CALLING);
          }
          else {  /* tail call: put new frame in place of previous one */
            int aux;
            base = ci->base;  /* `luaD_precall' may change the stack */
            ra = RA(i);
            if (L->openupval) luaF_close(L, base);
            for (aux = 0; ra+aux < L->top; aux++)  /* move frame down */
              setobjs2s(base+aux-1, ra+aux);
            ci->top = L->top = base+aux;  /* correct top */
            lua_assert(L->ci->state & CI_SAVEDPC);
            ci->u.l.savedpc = L->ci->u.l.savedpc;
            ci->u.l.tailcalls++;  /* one more call lost */
            ci->state = CI_SAVEDPC;
            L->ci--;  /* remove new frame */
            L->base = L->ci->base;
          }
          goto callentry;
        }
        break;
      }
      case OP_RETURN: {
        CallInfo *ci = L->ci - 1;  /* previous function frame */
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b-1;
        lua_assert(L->ci->state & CI_HASFRAME);
        if (L->openupval) luaF_close(L, base);
        L->ci->state = CI_SAVEDPC;  /* deactivate current function */
        L->ci->u.l.savedpc = pc;
        /* previous function was running `here'? */
        if (!(ci->state & CI_CALLING)) {
          lua_assert((ci->state & CI_C) || ci->u.l.pc != &pc);
          return ra;  /* no: return */
        }
        else {  /* yes: continue its execution */
          int yielded;
          StkId res;
          
          lua_assert(ttisfunction(ci->base - 1));
          res = luaV_return(L, ci, ra, &yielded);
          if (res)
            return res;
          if (yielded)
            return NULL;

          goto retentry;
        }
      }
      case OP_FORLOOP: {
        lua_Number step, idx, limit;
        const TObject *plimit = ra+1;
        const TObject *pstep = ra+2;
        if (!ttisnumber(ra))
          luaG_runerror(L, "`for' initial value must be a number");
        if (!tonumber(plimit, ra+1))
          luaG_runerror(L, "`for' limit must be a number");
        if (!tonumber(pstep, ra+2))
          luaG_runerror(L, "`for' step must be a number");
        step = nvalue(pstep);
        idx = nvalue(ra) + step;  /* increment index */
        limit = nvalue(plimit);
        if (step > 0 ? idx <= limit : idx >= limit) {
          dojump(pc, GETARG_sBx(i));  /* jump back */
          chgnvalue(ra, idx);  /* update index */
        }
        break;
      }
      case OP_TFORLOOP: {
        int nvar = GETARG_C(i) + 1;
        StkId cb = ra + nvar + 2;  /* call base */
        setobjs2s(cb, ra);
        setobjs2s(cb+1, ra+1);
        setobjs2s(cb+2, ra+2);
        L->top = cb+3;  /* func. + 2 args (state and index) */
        YP(luaD_call_yp(L, cb, nvar));
        L->top = L->ci->top;
        ra = XRA(i) + 2;  /* final position of first result */
        cb = ra + nvar;
        do {  /* move results to proper positions */
          nvar--;
          setobjs2s(ra+nvar, cb+nvar);
        } while (nvar > 0);
        if (ttisnil(ra))  /* break loop? */
          pc++;  /* skip jump (break loop) */
        else
          dojump(pc, GETARG_sBx(*pc) + 1);  /* jump back */
        break;
      }
      case OP_TFORPREP: {  /* for compatibility only */
        if (ttistable(ra)) {
          setobjs2s(ra+1, ra);
          setobj2s(ra, luaH_getstr(hvalue(gt(L)), luaS_new(L, "next")));
        }
        dojump(pc, GETARG_sBx(i));
        break;
      }
      case OP_SETLIST:
      case OP_SETLISTO: {
        int bc;
        int n;
        Table *h;
        runtime_check(L, ttistable(ra));
        h = hvalue(ra);
        bc = GETARG_Bx(i);
        if (GET_OPCODE(i) == OP_SETLIST)
          n = (bc&(LFIELDS_PER_FLUSH-1)) + 1;
        else {
          n = L->top - ra - 1;
          L->top = L->ci->top;
        }
        bc &= ~(LFIELDS_PER_FLUSH-1);  /* bc = bc - bc%FPF */
        for (; n > 0; n--)
          setobj2t(luaH_setnum(L, h, bc+n), ra+n);  /* write barrier */
        break;
      }
      case OP_CLOSE: {
        luaF_close(L, ra);
        break;
      }
      case OP_CLOSURE: {
        Proto *p;
        Closure *ncl;
        int nup, j;
        p = cl->p->p[GETARG_Bx(i)];
        nup = p->nups;
        ncl = luaF_newLclosure(L, nup, &cl->g);
        ncl->l.p = p;
        for (j=0; j<nup; j++, pc++) {
          if (GET_OPCODE(*pc) == OP_GETUPVAL)
            ncl->l.upvals[j] = cl->upvals[GETARG_B(*pc)];
          else {
            lua_assert(GET_OPCODE(*pc) == OP_MOVE);
            ncl->l.upvals[j] = luaF_findupval(L, base + GETARG_B(*pc));
          }
        }
        setclvalue(ra, ncl);
        luaC_checkGC(L);
        break;
      }
    }
  }
}
