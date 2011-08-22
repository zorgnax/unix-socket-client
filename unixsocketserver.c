#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

char *path;

void die (const char *mesg) {
    perror(mesg);
    exit(1);
}

void end () {
	unlink(path);
	exit(0);
}

int unixsocket (const char *path) {
    struct sockaddr_un myaddr = {0};
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        die("socket");
    myaddr.sun_family = AF_UNIX;
    strncpy(myaddr.sun_path, path, sizeof myaddr.sun_path - 1);
    if (bind(sock, &myaddr, sizeof myaddr) < 0)
        die("bind");
    if (listen(sock, 0) < 0)
        die("listen");
	return sock;
}

void init (const char *path, char **cmdav) {
    int peer;
    pid_t pid;
    int status;
    struct sockaddr_un peeraddr = {0};
	socklen_t peerlen = sizeof peeraddr;
	int sock = unixsocket(path);
	signal(SIGINT, end);
	while (1) {
		peer = accept(sock, &peeraddr, &peerlen);
		if (peer < 0)
			die("accept");
		printf("client connected\n");
		pid = fork();
		if (pid < 0)
			die("fork");
		else if (!pid) {
			dup2(peer, 0);
			dup2(peer, 1);
			close(peer);
			execvp(cmdav[0], cmdav);
			die("execvp");
		}
		close(peer);
		wait(&status);
	}
    close(sock);
}

int main (int ac, char **av) {
    char **cmdav;
    if (ac < 2) {
        fprintf(stderr, "usage: %s path cmdav ...\n", av[0]);
        exit(1);
    }
    path = av[1];
    cmdav = av + 2;
    printf("unix socket on %s\n", path);
    init(path, cmdav);
    return 0;
}

