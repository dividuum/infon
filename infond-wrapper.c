/* 
 * This program is intended as a wrapper that starts up infond in its 
 * own chroot environment.
 *
 * You'll need the statically compiled server (infond-static).
 *
 * Be sure to define INFOND_UID and INFOND_GID (the user and group id
 * that infond should run as) while compiling this program. For example:
 *
 * gcc infond-wrapper.c -o infond-wrapper -DINFOND_UID=1000 -DINFOND_GID=1000
 */

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#if !defined(INFOND_UID) || !defined(INFOND_GID)
#error "INFOND_UID and INFOND_GID must be defined"
#endif

#define must_not_fail(c) do {   \
    if ((c) != 0) {             \
        perror(#c " failed");   \
        exit(EXIT_FAILURE);     \
    }                           \
} while (0)

int main() {
    int fd;
    for (fd = 3; fd < 1024; fd++) close(fd);
    must_not_fail(chroot("."));
    must_not_fail(chdir("/"));
    must_not_fail(setgid(INFOND_UID));
    must_not_fail(setuid(INFOND_GID));
    char *argv[] = { "infond-static", NULL };
    char *envp[] = { NULL };
    execve("/infond-static", argv, envp);
    perror("execve failed");
    return EXIT_FAILURE;
}
