#include "chat_server.c"
#include "daemon.c"
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int port = 2000;
	if (argc > 1) {
		port = atoi(argv[1]);
	}
	start_daemon();
	server_init(port);
	syslog(LOG_INFO, "Sever initialize");
	server_listen();
	return 0;
}
