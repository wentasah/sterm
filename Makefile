CFLAGS = -O2 -Wall

all: sterm

install: all
	install -d $(DESTDIR)/usr/bin
	install -m 755 sterm $(DESTDIR)/usr/bin
