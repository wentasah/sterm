CFLAGS = -O2 -Wall -g
LDLIBS = -llockdev

PREFIX ?= /usr/local
INSTALL ?= install

all: sterm

clean:
	rm -f sterm

install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -s -m 755 sterm $(DESTDIR)$(PREFIX)/bin
ifneq ($(NO_MAN),1)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/man/man1
	$(INSTALL) -m 644 sterm.man $(DESTDIR)$(PREFIX)/share/man/man1/sterm.1
	gzip -f $(DESTDIR)$(PREFIX)/share/man/man1/sterm.1
endif
ifneq ($(NO_COMP),1)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/bash-completion/completions/
	$(INSTALL) -m 644 completion.bash $(DESTDIR)$(PREFIX)/share/bash-completion/completions/sterm
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/zsh/functions/Completion/Unix/
	$(INSTALL) -m 644 completion.zsh $(DESTDIR)$(PREFIX)/share/zsh/functions/Completion/Unix/_sterm
endif
