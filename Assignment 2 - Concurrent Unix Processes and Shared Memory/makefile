SHELL = /bin/sh

CC = gcc
CFLAGS = -g
ALL_CFLAGS = -I. $(CFLAGS)
TARGET1 = oss
TARGET2 = prime
OBJS1 = Control.o 
OBJS2 = ChildProcess.o 
MATH_FLAG = -lm

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS2) $(MATH_FLAG)

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<

clean:
	rm -f *.o *.dat $(TARGET1) $(TARGET2)
	find . -maxdepth 1 -type f -executable -exec rm {} +
