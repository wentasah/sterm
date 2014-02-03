/*
 * Simple terminal
 *
 * This is a minimalist terminal program like minicom or cu. The only
 * thing it does is creating a bidirectional connection between
 * stdin/stdout and a device (e.g. serial terminal). It can also set
 * serial line baudrate and manipulate DTR/RTS modem lines.
 *
 * Copyright 2014 Michal Sojka <sojkam1@fel.cvut.cz>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#define _BSD_SOURCE
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#define STRINGIFY(val) #val
#define TOSTRING(val) STRINGIFY(val)
#define CHECK(cmd) ({ int ret = (cmd); if (ret == -1) { perror(#cmd " line " TOSTRING(__LINE__)); exit(1); }; ret; })
#define CHECKPTR(cmd) ({ void *ptr = (cmd); if (ptr == (void*)-1) { perror(#cmd " line " TOSTRING(__LINE__)); exit(1); }; ptr; })

#define VERBOSE(format, ...) do { if (verbose) fprintf(stderr, format, ##__VA_ARGS__); } while (0)

bool verbose = false;

char template[] = "/var/lock/TMPXXXXXX";
char lockfile[100];
struct termios stdin_tio_backup;

void rm_file(int status, void *arg)
{
	char *fn = arg;
	if (fn[0])
		unlink(fn);
	fn[0] = 0;
}

void restore_stdin_term()
{
	tcsetattr(0, TCSANOW, &stdin_tio_backup);
}

void sighandler(int arg)
{
	exit(0);
}


int main(int argc, char *argv[])
{
	int fd;
	char *dev = NULL;
	int opt;
	speed_t speed = 0;
	int ret;
	int dtr = 0, rts = 0;
	struct termios tio;
	bool stdin_tty;
	bool raw = true;

	if ((stdin_tty = isatty(0))) {
		CHECK(tcgetattr(0, &stdin_tio_backup));
		atexit(restore_stdin_term);
	}

	while ((opt = getopt(argc, argv, "ndrs:v")) != -1) {
		switch (opt) {
		case 'd':
			dtr = 1;
			break;
		case 'n':
			raw = false;
		case 'r':
			rts = 1;
			break;
		case 's': {
			int s = atoi(optarg);
			switch (s) {
#define S(s) case s: speed = B##s; break;
				S(0);
				S(50);
				S(75);
				S(110);
				S(134);
				S(150);
				S(200);
				S(300);
				S(600);
				S(1200);
				S(1800);
				S(2400);
				S(4800);
				S(9600);
				S(19200);
				S(38400);
				S(57600);
				S(115200);
				S(230400);
#undef S
			default:
				fprintf(stderr, "Unknown baud rate %d\n", s);
				exit(1);
			}
			break;
		}
		case 'v':
			verbose = true;
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-s baudrate] [-v] <device>\n", argv[0]);
			exit(1);
		}
	}

	if (optind < argc)
		dev = argv[optind];

	if (!dev) {
		fprintf(stderr, "No device specified\n");
		exit(1);
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGHUP, sighandler);

	if (strncmp(dev, "/dev/", 5) == 0 &&
	    strrchr(dev, '/') == dev + 4 &&
	    dev[5] != 0)
	{ /* Create lock file (to be inter-operable with other programs) */
		/* This is racy, but what we can do - see also comments in uucp / cu */
		int tmp = CHECK(mkstemp(template));
		on_exit(rm_file, template);
	        char pid[20];
		snprintf(pid, sizeof(pid), "%u", getpid());
		CHECK(write(tmp, pid, strlen(pid)));
		close(tmp);
		snprintf(lockfile, sizeof(lockfile), "/var/lock/LCK..%s", dev + 5);
		if (link(template, lockfile) == -1) {
			perror(lockfile);
			exit(1);
		}
		rm_file(0, template);
		on_exit(rm_file, lockfile);
	}

	if ((fd = open(dev, O_RDWR)) < 0) {
		perror(dev);
		exit(1);
	}

	if (isatty(fd)) {
		CHECK(ioctl(fd, TIOCEXCL, NULL));

		CHECK(tcgetattr(fd, &tio));

		cfmakeraw(&tio);

		if (speed) {
			CHECK(cfsetospeed(&tio, speed));
			CHECK(cfsetispeed(&tio, speed));
		}

		if (dtr || rts) {
			int status;
			tio.c_cflag &= ~HUPCL;

			CHECK(ioctl(fd, TIOCMGET, &status));
			if (dtr == +1) status &= ~TIOCM_DTR;
			if (dtr == -1) status |=  TIOCM_DTR;
			if (rts == +1) status &= ~TIOCM_RTS;
			if (rts == -1) status |=  TIOCM_RTS;
			CHECK(ioctl(fd, TIOCMSET, &status));
		}

		CHECK(tcsetattr(fd, TCSANOW, &tio));
	} else if (speed || dtr || rts) {
		fprintf(stderr, "Cannot set speed, DTR or RTS on non-terminal %s\n", dev);
		exit(1);
	}

	struct pollfd fds[2] = {
		{ .fd = 0,  .events = POLLIN },
		{ .fd = fd, .events = POLLIN },
	};
	char buf[4096];

	if (stdin_tty) {
		tio = stdin_tio_backup;
		if (raw)
			cfmakeraw(&tio);
		CHECK(tcsetattr(0, TCSANOW, &tio));
	}

	VERBOSE("Connected.\n");
	while (1) {
		int r1, r2;
		ret = CHECK(poll(fds, 2, -1));
		if (fds[0].revents & POLLIN) {
			r1 = CHECK(read(0, buf, sizeof(buf)));
			if (r1 == 0) {
				VERBOSE("EOF on stdin\n");
				break;
			}
			r2 = CHECK(write(fd, buf, r1));
			if (r1 != r2) {
				fprintf(stderr, "Not all data written to %s (%d/%d)\n", dev, r1, r2);
				exit(1);
			}
		}
		if (fds[1].revents & POLLIN) {
			r1 = CHECK(read(fd, buf, sizeof(buf)));
			if (r1 == 0) {
				VERBOSE("EOF on %s\n", dev);
				break;
			}
			r2 = CHECK(write(1, buf, r1));
			if (r1 != r2) {
				fprintf(stderr, "Not all data written to stdout (%d/%d)\n", r1, r2);
				exit(1);
			}
		}
	}
	return 0;
}
