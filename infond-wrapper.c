/* 
 * This program is intended as a wrapper that starts up infond in its 
 * own chroot environment.
 *
 * You'll need the statically compiled server (infond-static).
 */

#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define must_not_fail(c) do {   \
    if ((c) != 0) {             \
        perror(#c " failed");   \
        exit(EXIT_FAILURE);     \
    }                           \
} while (0)

int main() {
    const char *uid_s = getenv("INFOND_UID");
    const char *gid_s = getenv("INFOND_GID");
    if (!uid_s || !gid_s) {
        fprintf(stderr, "both INFOND_UID and INFOND_GID must be set\n");
        exit(EXIT_FAILURE);
    }
    int fd;
    for (fd = 3; fd < 1024; fd++) close(fd);
    uid_t uid = atol(uid_s);
    gid_t gid = atol(gid_s);
    must_not_fail(chroot("."));
    must_not_fail(chdir("/"));
    must_not_fail(setgid(gid));
    must_not_fail(setuid(uid));
    if (getuid() == 0) {
        fprintf(stderr, "uid is 0. won't continue\n");
        exit(EXIT_FAILURE);
    }
    if (getgid() == 0) {
        fprintf(stderr, "gid is 0. won't continue\n");
        exit(EXIT_FAILURE);
    }
    char *argv[] = { "infond-static", NULL };
    char *envp[] = { NULL };
    execve("/infond-static", argv, envp);
    perror("execve failed");
    return EXIT_FAILURE;
}
