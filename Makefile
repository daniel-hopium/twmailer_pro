CC = g++
CFLAGS = -Wall -Wextra -std=c++11
SRC_DIR = src/
INC_DIR = include/
OBJ_DIR = obj/
BIN_DIR = bin/

all: $(BIN_DIR)twmailer-client $(BIN_DIR)twmailer-server

$(BIN_DIR)twmailer-client: $(OBJ_DIR)twmailer-client.o $(OBJ_DIR)encryption.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)twmailer-server: $(OBJ_DIR)twmailer-server.o $(OBJ_DIR)encryption.o 
	$(CC) $(CFLAGS) -o $@ $^ -I$(INC_DIR) -lldap -llber

$(OBJ_DIR)twmailer-client.o: $(SRC_DIR)twmailer-client.cpp $(INC_DIR)encryption.h
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INC_DIR)

$(OBJ_DIR)twmailer-server.o: $(SRC_DIR)twmailer-server.cpp $(INC_DIR)encryption.h $(INC_DIR)ldap_authentication.h
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INC_DIR)

$(OBJ_DIR)encryption.o: $(SRC_DIR)encryption.cpp $(INC_DIR)encryption.h
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INC_DIR)

$(OBJ_DIR)ldap_authentication.o: $(SRC_DIR)ldap_authentication.cpp $(INC_DIR)ldap_authentication.h
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INC_DIR)

clean:
	rm -f $(BIN_DIR)twmailer-client $(BIN_DIR)twmailer-server $(OBJ_DIR)*
