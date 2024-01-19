// client.h

#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "encryption.h"

#define BUF 1024

class Client
{
public:
    Client();

    ~Client();

    void run(const char *serverIp, int serverPort);

private:
    int create_socket;
    char buffer[BUF];
    bool isQuit;
    bool isValidCommand;

    // Server interaction
    void closeConnection();
    int setupConnection(const char *serverIp, int serverPort);
    void receiveData();
    void processInput();
    void sendData();
    void receiveFeedback();

    // Command handlers
    void handleLogin();
    void handleSend();
    void handleList();
    void handleRead();
    void handleDelete();
};

#endif // CLIENT_H
