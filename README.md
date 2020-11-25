# Simple serial terminal

This is a minimalist terminal program like *minicom* or *cu*. The only
thing it does is creating a bidirectional connection between
stdin/stdout and a terminal device (e.g. serial line). It can also set
serial line baudrate, manipulate DTR/RTS modem lines, send break and
throttle transmission speed.

The `-d` and `-r` option create short pulse on DTR/RTS. Linux always
raises these lines when the device is opened and those options lower
the lines immediately after opening.

For more details see the [man page](./sterm.man).

## Installation

    make
	make install
