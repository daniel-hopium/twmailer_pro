CC = g++
CFLAGS = -Wall -Wextra -std=c++11

all: client server

client: client.cpp
	$(CC) $(CFLAGS) -o $@ $<

server: server.cpp
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f client server
