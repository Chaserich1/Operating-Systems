SHELL = /bin/sh

CC = gcc
CFLAGS = -g
ALL_CFLAGS = -I. $(CFLAGS)
TARGET = bt
OBJS = main.o SearchFileSystem.o DirectoryCheck.o CommandOptions.o Queue.o

$(TARGET): $(OBJS)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)
	find . -maxdepth 1 -type f -executable -exec rm {} +
