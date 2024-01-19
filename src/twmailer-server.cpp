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
#include <dirent.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include "encryption.h"
#include <filesystem>
#include "ldap_authentication.h"
#include "blacklist.h"
#include "twmailer-server.h"

#include <ldap.h>

#define BUF 1024

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;
std::string mailDir;
Blacklist blacklist;

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>" << std::endl;
        return EXIT_FAILURE;
    }
    Server server;


    int port = atoi(argv[1]);
    mailDir = argv[2];
    // Give blacklist class the maildirec
    blacklist.mailDir = mailDir;

    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int reuseValue = 1;

    // SIGNAL HANDLER
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    // CREATE A SOCKET
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    // SET SOCKET OPTIONS
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

    // INIT ADDRESS
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // ASSIGN AN ADDRESS WITH PORT TO SOCKET
    if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind error");
        return EXIT_FAILURE;
    }

    // ALLOW CONNECTION ESTABLISHING
    if (listen(create_socket, 5) == -1)
    {
        perror("listen error");
        return EXIT_FAILURE;
    }

    // Create the mail-spool directory if it doesn't exist
    if (mkdir(mailDir.c_str(), 0777) == -1 && errno != EEXIST)
    {
        perror("Error creating mail-spool directory");
        return EXIT_FAILURE;
    }

    // Create the blacklist directory if it doesn't exist
    std::string blacklistDir = mailDir + "/" + "blacklist";
    if (mkdir(blacklistDir.c_str(), 0777) == -1 && errno != EEXIST)
    {
        perror("Error creating mail-spool directory");
        return EXIT_FAILURE;
    }

    while (!abortRequested)
    {
        printf("Waiting for connections...\n");

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

        printf("Client connected from %s:%d...\n",
               inet_ntoa(cliaddress.sin_addr),
               ntohs(cliaddress.sin_port));
        server.clientCommunication(&new_socket, inet_ntoa(cliaddress.sin_addr)); // returnValue can be ignored
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

    return EXIT_SUCCESS;
}

void* Server::clientCommunication(void *data, std::string ipAddress)
{
    char buffer[BUF];
    int size;
    int *current_socket = (int *)data;
    // Username to see if user is logged in
    std::string username = "";

    // SEND welcome message
    const char welcomeMessage[] = "Welcome to the twmailer!\r\nPlease enter your commands...\r\n";
    sendMessage(*current_socket, welcomeMessage);

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
            printf("Client closed remote socket\n");
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
        printf("[INFO] Message received: \n%s\n", buffer);

        // Extract the command from the first line
        std::istringstream bufferStream(buffer);
        std::string command;
        std::getline(bufferStream, command); // Consume the first line

        // Call the appropriate handler based on the command
        if (command == "LOGIN")
        {
            username = handleLogin(*current_socket, bufferStream, ipAddress);
        }
        else if (command == "SEND")
        {
            handleSend(*current_socket, bufferStream, username);
        }
        else if (command == "LIST")
        {
            handleList(*current_socket, username);
        }
        else if (command == "READ")
        {
            handleRead(*current_socket, bufferStream, username);
        }
        else if (command == "DEL")
        {
            handleDelete(*current_socket, bufferStream, username);
        }
        else if (command == "QUIT")
        {
            abortRequested = true;
        }
        else
        {
            // Respond to the client with a generic message
            const char Message[] = "ERR";
            sendMessage(*current_socket, Message);
        }

    } while (!abortRequested);

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
        printf("abort Requested... ");
        abortRequested = 1;

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

std::string Server::handleLogin(int current_socket, std::istringstream &bufferStream, std::string ipAddress)
{
    if (blacklist.isBlacklisted(ipAddress))
    {
        // Respond to the client
        const char errorMessage[] = "ERR - blacklisted - too many attempts\n";
        std::cout << "Warning: Blacklisted user tried to login:  " << ipAddress << std::endl;
        sendMessage(current_socket, errorMessage);
        return "";
    }

    // Extract username and password
    std::string username, password;

    // Receive sender
    std::getline(bufferStream, username);

    // Receive receiver
    std::getline(bufferStream, password);

    Encryption encryption;
    LDAPAuthentication LDAPAuthentication;
    std::string returnString = LDAPAuthentication.authenticateUser(username, encryption.decrypt(password));

    if (username == returnString)
    {
        std::cout << "LOGIN operation successfully completed for user " << username << std::endl;
        // Respond to the client
        const char okMessage[] = "OK";
        sendMessage(current_socket, okMessage);
        return username;
    }

    std::cout << "LOGIN operation unsuccessful for user " << username << ": " << returnString << std::endl;

    blacklist.writeAttemptToBlacklist(username, ipAddress);
    std::cout << blacklist.getLoginAttempts(ipAddress) << std::endl;
    if (blacklist.hasTooManyAttempts(ipAddress))
    {
        // Respond to the client
        const char errorMessage[] = "ERR - blacklisted - too many attempts\n";
        sendMessage(current_socket, errorMessage);
        std::cout << "Too many attempts for user " << username << " -> blacklisted for 60 seconds" << std::endl;
    }
    else
    {
        // Respond to the client
        const char errorMessage[] = "ERR\n";
        sendMessage(current_socket, errorMessage);
        // Log the LOGIN operation
    }

    return "";
}

void Server::handleSend(int current_socket, std::istringstream &bufferStream, std::string username)
{
    if (!isAuthorized(username, current_socket))
        return;

    // Extract sender, receiver, subject, and message
    std::string sender, receiver, subject, message;

    // Get sender
    sender = username;

    // Receive receiver
    std::getline(bufferStream, receiver);

    // Receive subject
    std::getline(bufferStream, subject);

    // Receive the message until a line contains "."
    message = "";
    std::string line;
    while (std::getline(bufferStream, line) && line != ".")
    {
        // Append each line to the message
        message += line + "\n";
    }

    // Save the message in the receiver's directory
    std::string receiverDir = mailDir + "/" + receiver;
    mkdir(receiverDir.c_str(), 0777);

    // Generate a unique filename for each received message using a timestamp
    std::string timestamp = std::to_string(time(nullptr));
    std::string fileName = receiverDir + "/message_" + timestamp + ".txt";

    // Write the message to the file
    std::ofstream outfile(fileName);
    if (outfile.is_open())
    {
        outfile << "Sender: " << sender << "\nSubject: " << subject << "\nMessage: " << message;
        outfile.close();
    }
    else
    {
        perror("Error writing message to file");
    }

    // Respond to the client
    const char okMessage[] = "OK";
    sendMessage(current_socket, okMessage);

    // Log the SEND operation
    std::cout << "SEND operation completed for user " << sender << std::endl;
}

void Server::handleList(int current_socket, std::string username)
{
    if (!isAuthorized(username, current_socket))
        return;

    // Extract username
    // std::string username;
    // std::getline(bufferStream, username);
    std::cout << "LIST request for user: " << username << std::endl;

    // Get the list of messages for the user
    std::string userDir = mailDir + "/" + username;
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
    if (send(current_socket, response.c_str(), response.length(), 0) == -1)
    {
        perror("send answer failed");
        return;
    }

    // Log the LIST operation
    std::cout << "LIST operation completed for user " << username << std::endl;
}

void Server::handleRead(int current_socket, std::istringstream &bufferStream, std::string username)
{
    if (!isAuthorized(username, current_socket))
        return;

    // Extract message number
    std::string messageNumberStr;
    std::getline(bufferStream, messageNumberStr);

    // Convert message number to integer
    int messageNumber = std::stoi(messageNumberStr);

    // Get the user's directory
    std::string userDir = mailDir + "/" + username;

    // Log the request
    std::cout << "READ request for user: " << username << ", message number: " << messageNumber << std::endl;

    // Check if the user directory exists
    DIR *dir = opendir(userDir.c_str());
    if (dir == NULL)
    {
        // User directory does not exist
        const char errorMessage[] = "ERR\n";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
        }
        std::cerr << "READ error: User directory does not exist for user " << username << std::endl;
        closedir(dir);
        return;
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
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
        }
        std::cerr << "READ error: Invalid message number for user " << username << std::endl;
        return;
    }

    // Get the message file corresponding to the requested message number
    std::string messageFile = userDir + "/" + messageFiles[messageNumber - 1];

    // Read the content of the message file
    std::ifstream messageFileStream(messageFile);
    if (!messageFileStream.is_open())
    {
        // Error opening the message file
        const char errorMessage[] = "ERR\n";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
        }
        std::cerr << "READ error: Could not open message file for user " << username << std::endl;
        return;
    }

    // Read the entire content of the message file
    std::ostringstream messageContentStream;
    messageContentStream << "OK\n";
    messageContentStream << messageFileStream.rdbuf();
    messageFileStream.close();

    // Send the complete message content to the client without extra newlines
    const std::string messageContent = messageContentStream.str();
    if (send(current_socket, messageContent.c_str(), messageContent.length(), 0) == -1)
    {
        perror("send answer failed");
    }

    std::cout << "READ request successful for user " << username << ", message number " << messageNumber << std::endl;
}

void Server::handleDelete(int current_socket, std::istringstream &bufferStream, std::string username)
{
    if (!isAuthorized(username, current_socket))
        return;

    // Extract message number
    std::string messageNumberStr;
    std::getline(bufferStream, messageNumberStr);

    // Convert message number to integer
    int messageNumber = std::stoi(messageNumberStr);

    // Get the user's directory
    std::string userDir = mailDir + "/" + username;

    // Check if the user directory exists
    DIR *dir = opendir(userDir.c_str());
    if (dir == NULL)
    {
        // User directory does not exist
        const char errorMessage[] = "ERR\nUser directory does not exist.";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
            return;
        }
        closedir(dir);
        return;
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
        const char errorMessage[] = "ERR\nInvalid message number.";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
            return;
        }
        return;
    }

    // Get the message file corresponding to the requested message number
    std::string messageFile = userDir + "/" + messageFiles[messageNumber - 1];

    // Delete the message file
    if (remove(messageFile.c_str()) != 0)
    {
        // Error deleting the message file
        const char errorMessage[] = "ERR\nError deleting message file.";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
            return;
        }
    }
    else
    {
        // Successfully deleted the message file
        const char okMessage[] = "OK";
        if (send(current_socket, okMessage, strlen(okMessage), 0) == -1)
        {
            perror("send answer failed");
            return;
        }
    }
}

void Server::sendMessage(int current_socket, const std::string &message)
{
    if (send(current_socket, message.c_str(), message.length(), 0) == -1)
    {
        perror("send answer failed");
    }
}


bool Server::isAuthorized(std::string username, int current_socket)
{

    if ((username.size() == 0))
    {
        const char errorMessage[] = "ERR\n";
        if (send(current_socket, errorMessage, strlen(errorMessage), 0) == -1)
        {
            perror("send answer failed");
        }
        std::cerr << "Unauthorized" << std::endl;
        return false;
    }
    return true;
}

