/*
 * Simple serial terminal
 *
 * Copyright 2014, 2015, 2016, 2017 Michal Sojka <sojkam1@fel.cvut.cz>
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

/*
 * This is a minimalist terminal program like minicom or cu. The only
 * thing it does is creating a bidirectional connection between
 * stdin/stdout and a device (e.g. serial terminal). It can also set
 * serial line baudrate and manipulate DTR/RTS modem lines.
 *
 * The -d and -r option create short pulse on DTR/RTS. The lines are
 * always raised when the device is opened and those options lower the
 * lines immediately after opening.
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <sys/ioctl.h>
#include <sys/types.h>
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
#include <lockdev.h>

#define STRINGIFY(val) #val
#define TOSTRING(val) STRINGIFY(val)
#define CHECK(cmd) ({ int ret = (cmd); if (ret == -1) { perror(#cmd " line " TOSTRING(__LINE__)); exit(1); }; ret; })
#define CHECKPTR(cmd) ({ void *ptr = (cmd); if (ptr == (void*)-1) { perror(#cmd " line " TOSTRING(__LINE__)); exit(1); }; ptr; })
#define CHECKNULL(cmd) ({ void *ptr = (cmd); if (ptr == NULL) { perror(#cmd " line " TOSTRING(__LINE__)); exit(1); }; ptr; })

#define VERBOSE(format, ...) do { if (verbose) fprintf(stderr, "sterm: " format, ##__VA_ARGS__); } while (0)

bool verbose = false;
bool exit_on_escape = true;

struct termios stdin_tio_backup;
char *dev = NULL;

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

void unlock()
{
	dev_unlock(dev, getpid());
}

void sighandler(int arg)
{
	exit(0); /* Invoke exit handlers */
}

int dtr_rts_arg(const char option, const char *optarg)
{
	int val = -1;

	if (optarg) {
		char *end;
		val = strtol(optarg, &end, 10);
		if (end == optarg) {
			/* Not a number */
			switch (optarg[0]) {
			case '+': val = +1; break;
			case '-': val = -1; break;
			default:
				fprintf(stderr, "Unknown -%c argument: %s\n", option, optarg);
				exit(1);
			}
		}
	}
	return val;
}

void exit_on_escapeseq(const char *buf, int len)
{
	static const char escseq[] = "\r~.";
	static const char *state = escseq+1;
	int i;
	for (i = 0; i < len; i++) {
		if (buf[i] == *state) {
			state++;
			if (*state == 0)
				exit(0);
		} else {
			state = escseq;
			if (buf[i] == *state)
				state++;
		}
	}
}

void usage(const char* argv0)
{
	fprintf(stderr, "Usage: %s [options] <device>\n", argv0);
	fprintf(stderr,
		"Options:\n"
		"  -c        enter command mode\n"
		"  -d[PULSE] make pulse on DTR\n"
		"  -e        ignore '~.' escape sequence\n"
		"  -n        do not switch the device to raw mode\n"
		"  -r[PULSE] make pulse on RTS\n"
		"  -s <baudrate>\n"
		"  -v        verbose mode\n"
		"\n"
		"PULSE is a number specifying the pulse. Absolute value defines the\n"
		"length of the pulse in milliseconds, sign determines the polarity of\n"
		"the pulse. Alternatively, PULSE can be either '+' or '-', which\n"
		"corresponds to +1 or -1.\n"
		);
}

void pulse(int fd, int dtr, int rts)
{
	int status, ms = 0;
	/* tio.c_cflag &= ~HUPCL; */ /* Don't lower DTR/RTS on close */

	CHECK(ioctl(fd, TIOCMGET, &status));
	if (dtr > 0) { status &= ~TIOCM_DTR; ms = +dtr; }
	if (dtr < 0) { status |=  TIOCM_DTR; ms = -dtr; }
	if (rts > 0) { status &= ~TIOCM_RTS; ms = +rts; }
	if (rts < 0) { status |=  TIOCM_RTS; ms = -rts; }
	CHECK(ioctl(fd, TIOCMSET, &status));

	usleep(ms*1000);

	if (dtr < 0) { status &= ~TIOCM_DTR; }
	if (dtr > 0) { status |=  TIOCM_DTR; }
	if (rts < 0) { status &= ~TIOCM_RTS; }
	if (rts > 0) { status |=  TIOCM_RTS; }
	CHECK(ioctl(fd, TIOCMSET, &status));
}

void handle_commands(int fd)
{
	char command[100];
	bool go = false;

	while (!go) {
		char *p1 = NULL;
		if (fgets(command, sizeof(command), stdin) == NULL) {
			if (!feof(stdin))
			    perror("Command read");
			exit(1);
		}
		if (sscanf(command, "dtr %ms", &p1) == 1)
			pulse(fd, dtr_rts_arg('d', p1), 0);
		else if (sscanf(command, "rts %ms", &p1) == 1)
			pulse(fd, 0, dtr_rts_arg('r', p1));
		else if (strcmp(command, "go\n") == 0)
			break;
		else {
			fprintf(stderr, "Unknown command: %s\n", command);
			exit(1);
		}

		free(p1);
	}
}

int main(int argc, char *argv[])
{
	int fd;
	int opt;
	speed_t speed = 0;
	int dtr = 0, rts = 0;
	struct termios tio;
	bool stdin_tty;
	bool raw = true;
	bool cmd = false;

	if ((stdin_tty = isatty(0))) {
		CHECK(tcgetattr(0, &stdin_tio_backup));
		atexit(restore_stdin_term);
	}

	while ((opt = getopt(argc, argv, "cnd::er::s:v")) != -1) {
		switch (opt) {
		case 'c': cmd = true; break;
		case 'd': dtr = dtr_rts_arg(opt, optarg); break;
		case 'e': exit_on_escape = false; break;
		case 'n': raw = false; break;
		case 'r': rts = dtr_rts_arg(opt, optarg); break;
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
			usage(argv[0]);
			exit(1);
		}
	}

	if (optind < argc)
		dev = argv[optind];

	if (!dev) {
		fprintf(stderr, "No device specified\n");
		usage(argv[0]);
		exit(1);
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGHUP, sighandler);

	pid_t pid = dev_lock(dev);
	if (pid > 0) {
		fprintf(stderr, "%s is used by PID %d\n", dev, pid);
		exit(1);
	} else if (pid < 0) {
		perror("dev_lock()");
		exit(1);
	}
	atexit(unlock);

	/* O_NONBLOCK is needed to not wait for the CDC signal. See tty_ioctl(4). */
	if ((fd = open(dev, O_RDWR|O_NOCTTY|O_NONBLOCK)) < 0) {
		perror(dev);
		exit(1);
	}
        /* Cancel the efect of O_NONBLOCK flag. */
	int n = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, n & ~O_NDELAY);

	if (isatty(fd)) {
		CHECK(ioctl(fd, TIOCEXCL, NULL));

		CHECK(tcgetattr(fd, &tio));

		cfmakeraw(&tio);

		if (speed) {
			CHECK(cfsetospeed(&tio, speed));
			CHECK(cfsetispeed(&tio, speed));
		}

		if (dtr || rts)
			pulse(fd, dtr, rts);

		 /* Disable flow control */
		tio.c_cflag &= ~(CRTSCTS);
		tio.c_iflag &= ~(IXON|IXOFF);

		CHECK(tcsetattr(fd, TCSANOW, &tio));
	} else if (speed || dtr || rts) {
		fprintf(stderr, "Cannot set speed, DTR or RTS on non-terminal %s\n", dev);
		exit(1);
	}

	VERBOSE("Connected.\r\n");

	if (cmd)
		handle_commands(fd);

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

	if (exit_on_escape)
		VERBOSE("Use '<Enter>~.' sequence to exit.\r\n");

	while (1) {
		int r1, r2;
		CHECK(poll(fds, 2, -1));
		if (fds[0].revents & POLLIN) {
			r1 = CHECK(read(0, buf, sizeof(buf)));
			if (r1 == 0) {
				VERBOSE("EOF on stdin\r\n");
				break;
			}
			if (exit_on_escape)
				exit_on_escapeseq(buf, r1);
			r2 = CHECK(write(fd, buf, r1));
			if (r1 != r2) {
				fprintf(stderr, "Not all data written to %s (%d/%d)\n", dev, r1, r2);
				exit(1);
			}
		}
		if (fds[1].revents & POLLIN) {
			r1 = CHECK(read(fd, buf, sizeof(buf)));
			if (r1 == 0) {
				VERBOSE("EOF on %s\r\n", dev);
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
