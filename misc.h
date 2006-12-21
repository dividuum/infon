/*
   
    Copyright (c) 2006 Florian Wesch <fw@dividuum.de>. All Rights Reserved.
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

#ifndef MISC_H
#define MISC_H

void die(const char *fmt, ...);
int yesno(const char *fmt, ...);
void infomsg(const char *fmt, ...);
#ifdef WIN32
const char *ErrorString(int error);
#endif

#ifndef abs
#define abs(a) ((a)<0?-(a):(a))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define limit(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

#define lua_register_constant(L, name) \
    do { lua_pushnumber(L, name); lua_setglobal(L, #name); } while (0)

#define lua_register_string_constant(L, name) \
    do { lua_pushliteral(L, name); lua_setglobal(L, #name); } while (0)

#define save_lua_stack(L, diff)         \
    lua_State *check_L = L;             \
    int lua_stack = lua_gettop(check_L) - diff;\
    fprintf(stderr, "%s> %d\n", __FUNCTION__, lua_stack);

#define check_lua_stack()               \
    fprintf(stderr, "%s< %d\n", __FUNCTION__, lua_gettop(check_L)); \
    assert(lua_gettop(check_L) == lua_stack);

#endif
