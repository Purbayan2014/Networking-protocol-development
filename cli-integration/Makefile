CC = gcc
CFLAGS = -Wall
LDFLAGS = -lcli
SOURCE = cli-manager.c
OUTPUT = cli-manager

res:
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)
	
clean:
	rm -f cli-manager  cli-manager.o

.PHONY: clean
