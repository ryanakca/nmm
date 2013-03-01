CFLAGS:=-Wall -Wextra -pedantic -Werror=format-security -fstack-protector-all $(CFLAGS)

all: nmm

nmm: nmm.c
	$(CC) $(CFLAGS) -lcurses -o $@ $<

clean:
	rm nmm

.PHONY: clean nmm
