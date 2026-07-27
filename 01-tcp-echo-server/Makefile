CC = gcc

# Warnings: All (turns on the most critical set of high probability bug detectors)
# -Wextra : Additional layer of stricter checks that -Wall leaves out
# -g tells gcc to embed debugging symbols inside the compiled executable. These symbols map machine instructions back to the human readable C source code, to be used while debugging (gdb or valgrind)
CFLAGS = -Wall -Wextra -g 

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

.PHONY: all clean

