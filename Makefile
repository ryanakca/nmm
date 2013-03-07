CFLAGS:=-Wall -Wextra -pedantic -Werror=format-security -fstack-protector-all $(CFLAGS)
CFLAGS:=-g $(CFLAGS)

PREFIX=/usr/local
MANPATH=$(PREFIX)/man
MAKEWHATIS=/usr/libexec/makewhatis

all: nmm tmm twmm

nmm: nmm.c
	$(CC) $(CFLAGS) -lcurses -o $@ $<

tmm twmm:
	ln -s nmm $@

tmm.6 twmm.6:
	ln -s nmm.6 $@

install: nmm tmm twmm installman
	-mkdir -p $(PREFIX)/games
	install -m 555 nmm $(PREFIX)/games/
	install -m 555 tmm $(PREFIX)/games/
	install -m 555 twmm $(PREFIX)/games/

installman: nmm.6 tmm.6 twmm.6
	install -m 444 nmm.6 $(MANPATH)/man6/
	install -m 444 tmm.6 $(MANPATH)/man6/tmm.6
	install -m 444 twmm.6 $(MANPATH)/man6/twmm.6
	-$(MAKEWHATIS) -d $(MANPATH) $(MANPATH)/man6/nmm.6 \
		$(MANPATH)/man6/tmm.6 \
		$(MANPATH)/man6/twmm.6

uninstall:
	-rm -f  $(PREFIX)/games/tmm \
		$(PREFIX)/games/nmm \
		$(PREFIX)/games/twmm
	-$(MAKEWHATIS) -u $(MANPATH) $(MANPATH)/man6/nmm.6 \
		$(MANPATH)/man6/tmm.6 \
		$(MANPATH)/man6/twmm.6
	-rm -f  $(MANPATH)/man6/nmm.6 \
		$(MANPATH)/man6/tmm.6 \
		$(MANPATH)/man6/twmm.6

clean:
	rm nmm

.PHONY: clean install installman
