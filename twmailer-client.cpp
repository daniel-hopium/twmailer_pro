#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error opening socket");

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        error("Error connecting to the server");

    // TODO: Implement message sending logic
    std::string message = "SEND\nSender\nReceiver\nSubject\nHello, this is a test message.\n.\n";

    if (send(sockfd, message.c_str(), message.size(), 0) < 0)
        error("Error sending message to the server");

    

    close(sockfd);

    return 0;
}
