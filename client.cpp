#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF 1024

using namespace std;

class TCPClient {
public:
    TCPClient() : create_socket(-1), isQuit(false) {}

    ~TCPClient() {
        closeConnection();
    }

    void run(const char* serverIp, int serverPort) {
        if (setupConnection(serverIp, serverPort) == -1) {
            cerr << "Failed to establish a connection.\n";
            return;
        }

        cout << "Connection with server (" << serverIp << ") established.\n";

        receiveData();

        do {
            cout << ">> ";
            if (cin.getline(buffer, BUF)) {
                processInput();
                sendData();
                receiveFeedback();
            }
        } while (!isQuit);
    }

private:
    int create_socket;
    char buffer[BUF];
    bool isQuit;

    void closeConnection() {
        if (create_socket != -1) {
            shutdown(create_socket, SHUT_RDWR);
            close(create_socket);
            create_socket = -1;
        }
    }

    int setupConnection(const char* serverIp, int serverPort) {
        create_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (create_socket == -1) {
            perror("Socket error");
            return -1;
        }

        sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(serverPort);
        inet_aton(serverIp, &address.sin_addr);

        if (connect(create_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
            perror("Connect error - no server available");
            closeConnection();
            return -1;
        }

        return 0;
    }

    void receiveData() {
        int size = recv(create_socket, buffer, BUF - 1, 0);
        if (size == -1) {
            perror("recv error");
        } else if (size == 0) {
            cout << "Server closed remote socket.\n";
        } else {
            buffer[size] = '\0';
            cout << buffer;
        }
    }

    void processInput() {
        int size = strlen(buffer);
        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
            size -= 2;
            buffer[size] = 0;
        } else if (buffer[size - 1] == '\n') {
            --size;
            buffer[size] = 0;
        }
        isQuit = strcmp(buffer, "quit") == 0;
    }

    void sendData() {
        if (send(create_socket, buffer, strlen(buffer), 0) == -1) {
            perror("send error");
            closeConnection();
            isQuit = true;
        }
    }

    void receiveFeedback() {
        int size = recv(create_socket, buffer, BUF - 1, 0);
        if (size == -1) {
            perror("recv error");
            isQuit = true;
        } else if (size == 0) {
            cout << "Server closed remote socket.\n";
            isQuit = true;
        } else {
            buffer[size] = '\0';
            cout << "<< " << buffer << '\n';
            if (strcmp("OK", buffer) != 0) {
                cerr << "<< Server error occurred, abort.\n";
                isQuit = true;
            }
        }
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    const char* serverIp = argv[1];
    int serverPort = std::atoi(argv[2]);

    TCPClient client;
    client.run(serverIp, serverPort);

    return 0;
}
