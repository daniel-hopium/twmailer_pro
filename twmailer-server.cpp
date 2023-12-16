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
        std::cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>\n";
        exit(1);
    }

    int server_port = atoi(argv[1]);
    char *mail_spool_directory = argv[2];

    // TODO: Implement the server logic here

    return 0;
}
