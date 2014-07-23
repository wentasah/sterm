CFLAGS = -O2 -Wall
LDFLAGS = -llockdev

all: sterm

clean:
	rm -f sterm

install: all
	install -d $(DESTDIR)/usr/bin
	install -m 755 sterm $(DESTDIR)/usr/bin
