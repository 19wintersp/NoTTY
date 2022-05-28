#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "run.h"
#include "process.h"

int run(char* argv[]) {
	Child child = launch(argv);

	char buf[256];

	for (;;) {
		struct pollfd fds[2] = {
			{ child.pty, POLLIN },
			{ stdin, POLLIN }
		};

		if (poll(fds, 2, -1) < 0) {
			kill(child.pid, SIGTERM);

			perror("poll() failed");
			return EXIT_FAILURE;
		}

		if (fds[0].revents & POLLIN) {
			size_t r = read(buf, sizeof(buf), child.pty);
			size_t w = write(buf, r, stdout);

			if (w != r) perror("write() failed");
		}

		if (fds[1].revents & POLLIN) {
			size_t r = read(buf, sizeof(buf), stdin);
			size_t w = write(buf, r, child.pty);

			if (w != r) perror("write() failed");
		}

		int status;
		if (waitpid(child.pid, &status, NULL) < 0) {
			perror("waitpid() failed");
		} else {
			if (WIFEXITED(status)) return WEXITSTATUS(status);
			if (WIFSIGNALED(status)) return WTERMSIG(status) | 0x80;
			if (WIFSTOPPED(status)) return WSTOPSIG(status) | 0x80;

			return EXIT_FAILURE;
		}
	}
}
