CC = g++
CFLAGS = -Wall -Wextra -std=c++11

all: twmailer-client twmailer-server

twmailer-client: twmailer-client.cpp
	$(CC) $(CFLAGS) -o $@ $<

twmailer-server: twmailer-server.cpp
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f twmailer-client twmailer-server
