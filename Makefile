CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic `sdl2-config --cflags`
LDFLAGS=`sdl2-config --libs` -lSDL2_image

default:
	$(CC) main.c $(CFLAGS) $(LDFLAGS) -o grem
