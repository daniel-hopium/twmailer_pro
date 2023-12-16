#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <dirent.h>

///////////////////////////////////////////////////////////////////////////////

#define BUF 1024

///////////////////////////////////////////////////////////////////////////////

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;
std::string mailSpoolDir;

///////////////////////////////////////////////////////////////////////////////

void *clientCommunication(void *data);
void signalHandler(int sig);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>" << std::endl;
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    mailSpoolDir = argv[2];

    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int reuseValue = 1;

    ////////////////////////////////////////////////////////////////////////////
    // SIGNAL HANDLER
    // SIGINT (Interrup: ctrl+c)
    // https://man7.org/linux/man-pages/man2/signal.2.html
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CREATE A SOCKET
    // https://man7.org/linux/man-pages/man2/socket.2.html
    // https://man7.org/linux/man-pages/man7/ip.7.html
    // https://man7.org/linux/man-pages/man7/tcp.7.html
    // IPv4, TCP (connection oriented), IP (same as client)
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error"); // errno set by socket()
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // SET SOCKET OPTIONS
    // https://man7.org/linux/man-pages/man2/setsockopt.2.html
    // https://man7.org/linux/man-pages/man7/socket.7.html
    // socket, level, optname, optvalue, optlen
    if (setsockopt(create_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &reuseValue,
                   sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reuseAddr");
        return EXIT_FAILURE;
    }

    if (setsockopt(create_socket,
                   SOL_SOCKET,
                   SO_REUSEPORT,
                   &reuseValue,
                   sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reusePort");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // INIT ADDRESS
    // Attention: network byte order => big endian
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    ////////////////////////////////////////////////////////////////////////////
    // ASSIGN AN ADDRESS WITH PORT TO SOCKET
    if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind error");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // ALLOW CONNECTION ESTABLISHING
    // Socket, Backlog (= count of waiting connections allowed)
    if (listen(create_socket, 5) == -1)
    {
        perror("listen error");
        return EXIT_FAILURE;
    }

    // Create the mail-spool directory if it doesn't exist
    if (mkdir(mailSpoolDir.c_str(), 0777) == -1 && errno != EEXIST)
    {
        perror("Error creating mail-spool directory");
        return EXIT_FAILURE;
    }

    while (!abortRequested)
    {
        /////////////////////////////////////////////////////////////////////////
        // ignore errors here... because only information message
        // https://linux.die.net/man/3/printf
        printf("Waiting for connections...\n");

        /////////////////////////////////////////////////////////////////////////
        // ACCEPTS CONNECTION SETUP
        // blocking, might have an accept-error on ctrl+c
        addrlen = sizeof(struct sockaddr_in);
        if ((new_socket = accept(create_socket,
                                 (struct sockaddr *)&cliaddress,
                                 &addrlen)) == -1)
        {
            if (abortRequested)
            {
                perror("accept error after aborted");
            }
            else
            {
                perror("accept error");
            }
            break;
        }

        /////////////////////////////////////////////////////////////////////////
        // START CLIENT
        // ignore printf error handling
        printf("Client connected from %s:%d...\n",
               inet_ntoa(cliaddress.sin_addr),
               ntohs(cliaddress.sin_port));
        clientCommunication(&new_socket); // returnValue can be ignored
        new_socket = -1;
    }

    // frees the descriptor
    if (create_socket != -1)
    {
        if (shutdown(create_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown create_socket");
        }
        if (close(create_socket) == -1)
        {
            perror("close create_socket");
        }
        create_socket = -1;
    }

    return EXIT_SUCCESS;
}

void *clientCommunication(void *data)
{
    char buffer[BUF];
    int size;
    int *current_socket = (int *)data;

    // SEND welcome message
    const char welcomeMessage[] = "Welcome to myserver!\r\nPlease enter your commands...\r\n";
    if (send(*current_socket, welcomeMessage, strlen(welcomeMessage), 0) == -1)
    {
        perror("send failed");
        return NULL;
    }

    do
    {
        // RECEIVE
        size = recv(*current_socket, buffer, BUF - 1, 0);
        if (size == -1)
        {
            if (abortRequested)
            {
                perror("recv error after aborted");
            }
            else
            {
                perror("recv error");
            }
            break;
        }

        if (size == 0)
        {
            printf("Client closed remote socket\n"); // ignore error
            break;
        }

        // Remove newline characters from the received message
        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
        {
            size -= 2;
        }
        else if (buffer[size - 1] == '\n')
        {
            --size;
        }

        buffer[size] = '\0';
        printf("Message received: \n%s\n", buffer); // ignore error

        // Extract the command from the first line
        std::istringstream commandStream(buffer);
        std::string command;
        std::getline(commandStream, command);  // Consume the first line

        // Inside the "SEND" command block
        if (command == "SEND")
        {
            // Extract sender, receiver, subject, and message
            std::string sender, receiver, subject, message;

            // Receive sender
            std::getline(commandStream, sender);

            // Receive receiver
            std::getline(commandStream, receiver);

            // Receive subject
            std::getline(commandStream, subject);

            // Receive the message until a line contains "."
            message = "";
            std::string line;
            while (std::getline(commandStream, line) && line != ".") {
                // Append each line to the message
                message += line + "\n";
            }

            // Now, sender, receiver, subject, and message are set
            std::cout << "Sender: " << sender << std::endl;
            std::cout << "Receiver: " << receiver << std::endl;
            std::cout << "Subject: " << subject << std::endl;
            std::cout << "Message:\n" << message;

            // Save the message in the sender's directory
            std::string senderDir = mailSpoolDir + "/" + sender;
            mkdir(senderDir.c_str(), 0777);

            // Generate a unique filename for each received message using a timestamp
            std::string timestamp = std::to_string(time(nullptr));
            std::string fileName = senderDir + "/message_" + timestamp + ".txt";

            // Write the message to the file
            std::ofstream outfile(fileName);
            if (outfile.is_open())
            {
                outfile << "Sender: " << sender << "\nSubject: " << subject << "\n" << message;
                outfile.close();
            }
            else
            {
                perror("Error writing message to file");
            }

            // Respond to the client
            const char okMessage[] = "OK";
            if (send(*current_socket, okMessage, strlen(okMessage), 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            // Log the SEND operation
            std::cout << "SEND operation completed for user " << sender << std::endl;
        }

        // Inside the "LIST" command block
        else if (command == "LIST")
        {
            // Extract username
            std::string username;
            std::getline(commandStream, username);
            std::cout << "LIST request for user: " << username << std::endl;

            // Get the list of messages for the user
            std::string userDir = mailSpoolDir + "/" + username;
            DIR *dir;
            struct dirent *ent;
            int messageCount = 0;

            if ((dir = opendir(userDir.c_str())) != NULL)
            {
                // Count the number of messages
                while ((ent = readdir(dir)) != NULL)
                {
                    if (ent->d_type == DT_REG)
                    {
                        ++messageCount;
                    }
                }
                closedir(dir);
            }
            else
            {
                perror("Error opening user directory");
            }

            // Respond to the client with the message count and subjects
            std::ostringstream responseStream;
            responseStream << messageCount << "\n";

            if (messageCount > 0)
            {
                // Get the list of message subjects
                dir = opendir(userDir.c_str());
                if (dir != NULL)
                {
                    while ((ent = readdir(dir)) != NULL)
                    {
                        if (ent->d_type == DT_REG)
                        {
                            // Read the subject from the second line of the message file
                            std::ifstream messageFile(userDir + "/" + ent->d_name);
                            std::string subject;
                            std::getline(messageFile, subject); // Read the first line (empty line)
                            std::getline(messageFile, subject); // Read the second line (subject)
                            messageFile.close();

                            responseStream << subject << "\n";
                        }
                    }
                    closedir(dir);
                }
                else
                {
                    perror("Error opening user directory");
                }
            }

            const std::string response = responseStream.str();
            if (send(*current_socket, response.c_str(), response.length(), 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            // Log the LIST operation
            std::cout << "LIST operation completed for user " << username << std::endl;
        }
        else if (command == "READ")
        {
            // Extract username and message number
            std::string username, messageNumberStr;
            std::getline(commandStream, username);
            std::getline(commandStream, messageNumberStr);

            // Convert message number to integer
            int messageNumber = std::stoi(messageNumberStr);

            // Get the user's directory
            std::string userDir = mailSpoolDir + "/" + username;

            // Log the request
            std::cout << "READ request for user: " << username << ", message number: " << messageNumber << std::endl;

            // Check if the user directory exists
            DIR *dir = opendir(userDir.c_str());
            if (dir == NULL)
            {
                // User directory does not exist
                const char errorMessage[] = "ERR\n";
                if (send(*current_socket, errorMessage, strlen(errorMessage), 0) == -1)
                {
                    perror("send answer failed");
                }
                std::cerr << "READ error: User directory does not exist for user " << username << std::endl;
                closedir(dir);
                return NULL;
            }
            closedir(dir);

            // Get the list of message files
            std::vector<std::string> messageFiles;
            dir = opendir(userDir.c_str());
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL)
            {
                if (ent->d_type == DT_REG)
                {
                    messageFiles.push_back(ent->d_name);
                }
            }
            closedir(dir);

            // Check if the requested message number is valid
            if (messageNumber <= 0 || messageNumber > static_cast<int>(messageFiles.size()))
            {
                // Invalid message number
                const char errorMessage[] = "ERR\n";
                if (send(*current_socket, errorMessage, strlen(errorMessage), 0) == -1)
                {
                    perror("send answer failed");
                }
                std::cerr << "READ error: Invalid message number for user " << username << std::endl;
                return NULL;
            }

            // Get the message file corresponding to the requested message number
            std::string messageFile = userDir + "/" + messageFiles[messageNumber - 1];

            // Read the content of the message file
            std::ifstream messageFileStream(messageFile);
            if (!messageFileStream.is_open())
            {
                // Error opening the message file
                const char errorMessage[] = "ERR\n";
                if (send(*current_socket, errorMessage, strlen(errorMessage), 0) == -1)
                {
                    perror("send answer failed");
                }
                std::cerr << "READ error: Could not open message file for user " << username << std::endl;
                return NULL;
            }

            // Read the entire content of the message file
            std::ostringstream messageContentStream;
            messageContentStream << "OK\n";
            messageContentStream << messageFileStream.rdbuf();
            messageFileStream.close();

            // Send the complete message content to the client without extra newlines
            const std::string messageContent = messageContentStream.str();
            if (send(*current_socket, messageContent.c_str(), messageContent.length(), 0) == -1)
            {
                perror("send answer failed");
            }

            std::cout << "READ request successful for user " << username << ", message number " << messageNumber << std::endl;
        }



        else
        {
            // Respond to the client with a generic message
            const char okMessage[] = "TEST";
            if (send(*current_socket, okMessage, strlen(okMessage), 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }
        }
        // Add other command processing logic (LIST, READ, DEL, QUIT) here

    } while (strcmp(buffer, "QUIT") != 0 && !abortRequested);

    // Close the socket
    if (*current_socket != -1)
    {
        if (shutdown(*current_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown new_socket");
        }
        if (close(*current_socket) == -1)
        {
            perror("close new_socket");
        }
        *current_socket = -1;
    }

    return NULL;
}

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        printf("abort Requested... "); // ignore error
        abortRequested = 1;
        /////////////////////////////////////////////////////////////////////////
        // With shutdown() one can initiate normal TCP close sequence ignoring
        // the reference count.
        // https://beej.us/guide/bgnet/html/#close-and-shutdownget-outta-my-face
        // https://linux.die.net/man/3/shutdown
        if (new_socket != -1)
        {
            if (shutdown(new_socket, SHUT_RDWR) == -1)
            {
                perror("shutdown new_socket");
            }
            if (close(new_socket) == -1)
            {
                perror("close new_socket");
            }
            new_socket = -1;
        }

        if (create_socket != -1)
        {
            if (shutdown(create_socket, SHUT_RDWR) == -1)
            {
                perror("shutdown create_socket");
            }
            if (close(create_socket) == -1)
            {
                perror("close create_socket");
            }
            create_socket = -1;
        }
    }
    else
    {
        exit(sig);
    }
}
