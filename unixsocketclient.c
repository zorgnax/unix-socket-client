#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <errno.h>

void die (const char *mesg) {
    perror(mesg);
    exit(1);
}

int transfer (int fromfd, int tofd) {
    char buf[4096];
    int got = 0;
    int sent = 0;
    int retval;
    got = read(fromfd, buf, sizeof buf);
    if (got < 0 && errno != EAGAIN)
        die("read");
    if (!got)
        return 0;
    while (got - sent > 0) {
        retval = write(tofd, buf + sent, got - sent);
        if (retval < 0)
            die("write");
        sent += retval;
    }
    return got;
}

int unixsocket (const char *path) {
    struct sockaddr_un addr = {0};
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        die("socket");
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);
    if (connect(sock, &addr, sizeof addr) < 0)
        die("connect");
    return sock;
}

void init (const char *path) {
    fd_set allrfds, rfds;
    int sock = unixsocket(path);
    FD_ZERO(&allrfds);
    FD_SET(0, &allrfds);
    FD_SET(sock, &allrfds);
    while (1) {
        rfds = allrfds;
        if (!FD_ISSET(0, &rfds) && !FD_ISSET(sock, &rfds))
            break;
        if (select(sock + 1, &rfds, NULL, NULL, NULL) < 0)
            die("select");
        if (FD_ISSET(0, &rfds)) {
            if (!transfer(0, sock)) {
                shutdown(sock, SHUT_WR);
                FD_CLR(0, &allrfds);
            }
        }
        if (FD_ISSET(sock, &rfds)) {
            if (!transfer(sock, 1)) {
                shutdown(sock, SHUT_RD);
                FD_CLR(sock, &allrfds);
            }
        }
    }
    close(sock);
}

int main (int ac, char **av) {
    char *path;
    if (ac < 2) {
        fprintf(stderr, "usage: %s path\n", av[0]);
        exit(1);
    }
    path = av[1];
    init(path);
    return 0;
}

