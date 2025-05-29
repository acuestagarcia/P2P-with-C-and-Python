CC = gcc
CFLAGS = -Wall -g
OBJS = server.o socketFunctions.o
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

socketFunctions.o: socketFunctions.c
	$(CC) $(CFLAGS) -c socketFunctions.c

clean:
	rm -f $(OBJS) $(TARGET)