 ___        __               ____        _   _   _          .--..-.
|_ _|_ __  / _| ___  _ __   | __ )  __ _| |_| |_| | ___    /   / \ \'-.  _
 | || '_ \| |_ / _ \| '_ \  |  _ \ / _` | __| __| |/ _ \  /   /  _( o)_ ; \
 | || | | |  _| (_) | | | | | |_) | (_| | |_| |_| |  __/  O  O  /      ' -.\
|___|_| |_|_|  \___/|_| |_| |____/ \__,_|\__|\__|_|\___|        \___;_/-__ ;)
                _                                                 __/   \ //-.
               / \   _ __ ___ _ __   __ _                        ; /     //   \
              / _ \ | '__/ _ \ '_ \ / _` |                       ' (INFON  )   )
             / ___ \| | |  __/ | | | (_| |                        \(       )   )
            /_/   \_\_|  \___|_| |_|\__,_|                         '      /   /
                                                                    '.__.- -'
    For more information on the game, see the website at          _ \\   // _
               http://infon.dividuum.de/                         (_'_/   \_'_)

Playing:

  Please visit http://infon.dividuum.de/
  There you'll find tutorials, a function reference and 
  example code.

Compiling the Game:

  Requirements:

  Server:
    libevent (tested with 1.1a)
    zlib

  Client:
    libevent
    zlib

  2D SDL based renderer:
    SDL (tested with 1.2.8) 
    SDL_image
    SDL_gfx
    SGE

  3D OpenGL based renderer:
    SDL
    OpenGL

  Type 'make', edit config.lua, run ./infond 
  then ./infon localhost. If you need help, please
  visit the channel #infon on irc.freenode.net.

Contact:  

  Florian 'dividuum' Wesch <fw@dividuum.de>
  Please send me your bots, images of events, patches etc..

License:

  The game itself is released under the GPL.

    See http://www.gnu.org/licenses/gpl.txt for details

  The binary release of the 2D renderer (sdl_gui.so / sdl_gui.dll) is 
  linked against:

    SDL         http://www.libsdl.org
    SDL_image   http://www.libsdl.org/projects/SDL_image/
    SDL_gfx     http://www.ferzkopp.net/Software/SDL_gfx-2.0/
    SGE         http://www.etek.chalmers.se/~e8cal1/sge/

      All of them are covered by the LGPL. 
      See http://www.gnu.org/licenses/lgpl.txt for details

  The binary release of the 3D renderer (gl_gui.so / gl_gui.dll) is 
  linked against:

    SDL         http://www.libsdl.org

      This library is covered by the LGPL. 
      See http://www.gnu.org/licenses/lgpl.txt for details

  The binary releases of both client (infon / infon.exe) and 
  server (infond) include libevent:

    libevent    http://www.monkey.org/~provos/libevent/        

      Copyright (c) 2002, 2003 Niels Provos <provos@citi.umich.edu>
      All rights reserved.

      Redistribution and use in source and binary forms, with or without
      modification, are permitted provided that the following conditions
      are met:
      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
      2. Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.
      3. The name of the author may not be used to endorse or promote products
         derived from this software without specific prior written permission.

      THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
      IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
      OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
      IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
      INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
      NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
      THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  
  The server uses a modified lua 5.1.1:

    lua         http://www.lua.org/

      Copyright (C) 1994-2006 Lua.org, PUC-Rio.

      Permission is hereby granted, free of charge, to any person obtaining a copy
      of this software and associated documentation files (the "Software"), to deal
      in the Software without restriction, including without limitation the rights
      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
      copies of the Software, and to permit persons to whom the Software is
      furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included in
      all copies or substantial portions of the Software.

      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
      THE SOFTWARE.

  The Datafiles included in client and server distribution:

    gfx/5x7.fnt      - From SDL_gfx (LGPL)
    gfx/font.png     - From SDL_sge (LGPL)
    gfx/creature.mdl - hm!? 
