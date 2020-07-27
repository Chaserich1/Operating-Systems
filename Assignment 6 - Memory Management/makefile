SHELL = /bin/sh

CC = gcc
CFLAGS = -g
ALL_CFLAGS = -I. -Wall -pthread $(CFLAGS)
MATHFLAG = -lm
TARGET1 = oss
TARGET2 = user
OBJS1 = oss.o 
OBJS2 = user.o
 

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS1) $(MATHFLAG)

$(TARGET2): $(OBJS2)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS2)

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<

clean:
	rm -f *.o *.dat $(TARGET1) $(TARGET2)
	find . -maxdepth 1 -type f -executable -exec rm {} +
