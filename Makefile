CC = gcc
LD = $(CC)
CFLAGS = -Wall -Wextra -Werror -std=gnu99
VPATH = src


game.o: game.c game.h
network.o: network.c network.h


.PHONY: clean clean-submit
clean:
	rm -f *.o
	find . -maxdepth 1 -type f -executable -delete
clean-submit: clean
	rm -rf .git .gitignore docs
