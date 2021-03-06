CC = gcc
LD = $(CC)
CFLAGS = -Wall -Wextra -std=gnu99 -Werror
VPATH = src


default: server client

server: server.o game.o protocol.o
client: client.o game.o protocol.o
server.o: game.o protocol.o server.c server.h
client.o: game.o protocol.o client.c client.h

game.o: game.c game.h
protocol.o: protocol.c protocol.h


.PHONY: clean clean-submit loc
clean:
	rm -f *.o
clean-submit: clean
	find . -maxdepth 1 -type f -executable -delete
	rm -rf .git .gitignore docs
loc:
	find . -name '*.c' -print0 -o -name '*.h' -print0 | wc -l --files0-from=- | tail -n 1
