CC = gcc
LD = $(CC)
CFLAGS = -Wall -Wextra -std=gnu99 #-Werror
VPATH = src


default: server client

server: server.o game.o network.o
client: client.o game.o network.o
server.o: game.o network.o server.c server.h
client.o: game.o network.o client.c client.h

game.o: game.c game.h
network.o: network.c network.h


.PHONY: clean clean-submit
clean:
	rm -f *.o
	find . -maxdepth 1 -type f -executable -delete
clean-submit: clean
	rm -rf .git .gitignore docs
