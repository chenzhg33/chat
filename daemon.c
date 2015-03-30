#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <fcntl.h>
void start_daemon() {
	int i, fd;
	struct rlimit rl;
	pid_t pid;
	for (i = 0; i < rl.rlim_max; ++i) {
		close(i);
	}
	openlog("chat_server", LOG_PID, 0);
	if ((pid = fork()) < 0) {
		syslog(LOG_ERR, "fork error");
		return;
	} else if (pid != 0) {
		exit(0);
	}
	if (setsid() < 0) {
		syslog(LOG_ERR, "setsid error");
		return;
	}
	if ((pid = fork()) < 0) {
		syslog(LOG_ERR, "fork error");
	} else if (pid != 0) {
		exit(0);
	}
	umask(0);
	signal(SIGCHLD, SIG_IGN);
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	syslog(LOG_INFO, "start daemon");
}
