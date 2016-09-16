CFLAGS = -O2 -Wall
LDFLAGS = -llockdev

all: sterm

%: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f sterm

install: all
	install -d $(DESTDIR)/usr/bin
	install -m 755 sterm $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/local/man/man1
	install -m 644 sterm.man $(DESTDIR)/usr/local/man/man1/sterm.1
	gzip -f $(DESTDIR)/usr/local/man/man1/sterm.1
