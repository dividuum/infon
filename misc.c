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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#endif

void die(char *fmt, ...) {
#ifdef WIN32
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    MessageBoxA(GetActiveWindow(), buf, "Fatal Error", MB_ICONSTOP);
#else
    va_list ap;
    va_start(ap, fmt);
    printf("--[ FATAL ERROR ]-----------\n");
    vprintf(fmt, ap);
    printf("\n----------------------------\n");
    va_end(ap);
#endif
    exit(1);
}
