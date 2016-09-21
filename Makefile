CFLAGS = -O2 -Wall
LDLIBS = -llockdev

PREFIX ?= /usr/local

all: sterm

clean:
	rm -f sterm

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 sterm $(DESTDIR)$(PREFIX)/bin
ifneq ($(NO_MAN),1)
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 sterm.man $(DESTDIR)$(PREFIX)/share/man/man1/sterm.1
	gzip -f $(DESTDIR)$(PREFIX)/share/man/man1/sterm.1
endif
