SHELL = /bin/sh

CC = gcc
CFLAGS = -g
ALL_CFLAGS = -I. -pthread $(CFLAGS)
MATHFLAG = -lm
TARGET1 = master
TARGET2 = bin_adder
OBJS1 = Main.o 
OBJS2 = ChildProcess.o
LOGFILE = adder_log 

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS1) $(MATHFLAG)

$(TARGET2): $(OBJS2)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJS2)

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<

clean:
	rm -f *.o *.dat $(TARGET1) $(TARGET2) $(LOGFILE)
	find . -maxdepth 1 -type f -executable -exec rm {} +
