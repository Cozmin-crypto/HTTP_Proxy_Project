CC		= gcc 
CFLAGS	= -std=gnu11 -Wall -Wextra -g -Isrc
SRC		= $(wildcard src/*.c src/*/*.c)

proxy: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o proxy


clean:
	rm -f proxy

	.PHONY: clean