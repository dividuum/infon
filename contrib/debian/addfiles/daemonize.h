/*
   
    Copyright (c) 2006 Joachim Breitner <nomeata@debian.org>. All Rights Reserved.
    
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>


#define PIDFILE "/var/run/infond.pid"

void undaemonize() {
	int ret;

    	ret = unlink(PIDFILE);

	if (ret == -1) {
	    perror("Could not delete " PIDFILE);
	   // exit(1);
	}
}

void daemonize(int argc, char *argv[]) {
	int ret;

	if ( ! (argc > 1 && strcmp(argv[1], "-d") == 0) ) {
		return;
	}
	
	/* init script has to change directory */
	/*
	ret = chdir("/usr/share/infon-server/");
	if (ret == -1) {
		perror("Could not change to data directory");
		exit(1);
	}
	*/

        int pid = fork();

        if (pid == -1) {
            // error
            perror("Could not fork");
            exit(1);
        }

        if (pid) {
            // parent process
            FILE *pidfile = fopen(PIDFILE,"w");
            if (pidfile == NULL) {
                perror("Could not open pidfile");
                exit(1);
            }

            ret = fprintf(pidfile,"%d\n",pid);
            if (ret < 0) {
                perror("Could not write to pidfile");
            }

            ret = fclose(pidfile);
            if (ret != 0) {
                perror("Could not close pidfile");
            }

            exit(EXIT_SUCCESS);
        } else {
            struct passwd *pwinfo = getpwnam("nobody");

            if (pwinfo == NULL) {
                perror("Could not find user nobody");
                exit (1);
            }

            if (setuid(pwinfo->pw_uid) == -1) {
                perror("Could not change userid");
                exit(1);
            }
    
            freopen("/dev/null","r",stdin); 
            freopen("/dev/null","w",stdout); 
            freopen("/dev/null","w",stderr); 

        }
      	        
	atexit(undaemonize);
}


