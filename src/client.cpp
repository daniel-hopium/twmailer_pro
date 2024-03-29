#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "encryption.h"
#include "client.h"

#define BUF 1024

using namespace std;


Client::Client() : create_socket(-1), isQuit(false), isValidCommand(true) {}

Client::~Client()
{
    closeConnection();
}

void Client::run(const char *serverIp, int serverPort)
{
    if (setupConnection(serverIp, serverPort) == -1)
    {
        cerr << "Failed to establish a connection.\n";
        return;
    }

    cout << "Connection with server (" << serverIp << ") established.\n";

    receiveData();

    do
    {
        cout << ">> ";
        if (cin.getline(buffer, BUF))
        {
            processInput();
            if (!isValidCommand)
            {
                isValidCommand = true;
            }
            else
            {
                sendData();
                receiveFeedback();
            }
        }
    } while (!isQuit);
}

void Client::closeConnection()
{
    if (create_socket != -1)
    {
        shutdown(create_socket, SHUT_RDWR);
        close(create_socket);
        create_socket = -1;
    }
}

int Client::setupConnection(const char *serverIp, int serverPort)
{
    create_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (create_socket == -1)
    {
        perror("Socket error");
        return -1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(serverPort);
    inet_aton(serverIp, &address.sin_addr);

    if (connect(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("Connect error - no server available");
        closeConnection();
        return -1;
    }

    return 0;
}

void Client::receiveData()
{
    int size = recv(create_socket, buffer, BUF - 1, 0);
    if (size == -1)
    {
        perror("recv error");
    }
    else if (size == 0)
    {
        cout << "Server closed remote socket.\n";
    }
    else
    {
        buffer[size] = '\0';
        cout << buffer;
    }
}

void Client::processInput()
{
    int size = strlen(buffer);
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
    {
        size -= 2;
        buffer[size] = 0;
    }
    else if (buffer[size - 1] == '\n')
    {
        --size;
        buffer[size] = 0;
    }
    isQuit = strcmp(buffer, "quit") == 0;

    if (strncmp(buffer, "LOGIN", 4) == 0)
    {
        handleLogin();
    }
    else if (strncmp(buffer, "SEND", 4) == 0)
    {
        handleSend();
    }
    else if (strncmp(buffer, "LIST", 4) == 0)
    {
        handleList();
    }
    else if (strncmp(buffer, "READ", 4) == 0)
    {
        handleRead();
    }
    else if (strncmp(buffer, "DEL", 3) == 0)
    {
        handleDelete();
    }
    else
    {
        std::cout << "Invalid command. Please try again.\n";
        isValidCommand = false;
    }
}

void Client::sendData()
{
    if (send(create_socket, buffer, strlen(buffer), 0) == -1)
    {
        perror("send error");
        closeConnection();
        isQuit = true;
    }
}

void Client::receiveFeedback()
{
    int size = recv(create_socket, buffer, BUF - 1, 0);
    if (size == -1)
    {
        perror("recv error");
        isQuit = true;
    }
    else if (size == 0)
    {
        cout << "Server closed remote socket.\n";
        isQuit = true;
    }
    else
    {
        buffer[size] = '\0';
        cout << "<< " << buffer << '\n';

        if (strncmp("Error", buffer, 5) == 0)
        {
            cerr << "<< Server error occurred: " << buffer << '\n';
            isQuit = true;
        }
    }
}

// Command handlers
void Client::handleLogin()
{
    string fullMessage = "LOGIN\n";
    bool isValid = true;

    cout << "Username (max 8 chars): ";
    while (true)
    {
        cin.getline(buffer, BUF);
        isValid = true;

        for (int i = 0; buffer[i] != '\0'; ++i)
        {
            if ( i >= 8 )
            {
                cerr << "Error: Username exceeds maximum length, is not only in all lowercase or contains special characters.\n";
                cout << "Username (max. 8 lowercase chars): ";
                isValid = false;
                break;
            }
        }
        if (isValid)
        {
            fullMessage += buffer;
            fullMessage += "\n";
            isValid = true;
            break; // Exit the loop if the subject is valid
        }
    }
    // Get and validate password using getpass
    char *password = getpass("Password: ");
    string passwordString(password); // Convert the C-style string to C++ string

    //  Encrypt password
    Encryption encryption;
    passwordString = encryption.encrypt(passwordString);
    fullMessage += passwordString;
    // Send the entire message to the server
    strncpy(buffer, fullMessage.c_str(), BUF);
}

void Client::handleSend()
{
    string fullMessage = "SEND\n";
    bool isValid = true;

    cout << "Receiver (max. 8 chars): ";
    while (true)
    {
        cin.getline(buffer, BUF);
        isValid = true;

        for (int i = 0; buffer[i] != '\0'; ++i)
        {
            if (i > 8)
            {
                cerr << "Error: Receiver exceeds maximum length or is not only in all lowercase.\n";
                cout << "Receiver (max. 8 chars): ";
                isValid = false;
                break;
            }
        }
        if (isValid)
        {
            fullMessage += buffer;
            fullMessage += "\n";
            break; // Exit the loop if the subject is valid
        }
    }

    cout << "Subject (max. 80 chars): ";
    while (true)
    {
        cin.getline(buffer, BUF);

        // Validate subject length
        if (strlen(buffer) <= 80)
        {
            fullMessage += buffer;
            fullMessage += "\n";
            break; // Exit the loop if the subject is valid
        }
        else
        {
            cerr << "Error: Subject exceeds maximum length (80 characters).\n";
            cout << "Subject (max. 80 chars): ";
        }
    }

    cout << "Message (type \".\" on a line by itself to end):\n";
    while (true)
    {
        cin.getline(buffer, BUF);
        if (strcmp(buffer, ".") == 0)
        {
            break;
        }
        fullMessage += buffer;
        fullMessage += "\n";
    }

    // Send the entire message to the server
    strncpy(buffer, fullMessage.c_str(), BUF);
}

void Client::handleList()
{
    string fullMessage = "LIST\n";

    // Send the LIST command to the server
    strncpy(buffer, fullMessage.c_str(), BUF);
}

void Client::handleRead()
{
    string fullMessage = "READ\n";

    do
    {
        cout << "Message-Number: ";
        cin.getline(buffer, BUF);

        // Check if the entered message number is valid (greater than 0)
        if (atoi(buffer) > 0)
        {
            fullMessage += buffer;
            fullMessage += "\n";
            break;
        }
        else
        {
            cerr << "Error: Message number must be greater than 0.\n";
        }
    } while (true);

    // Send the READ command to the server
    strncpy(buffer, fullMessage.c_str(), BUF);
}

void Client::handleDelete()
{
    string fullMessage = "DEL\n";

    do
    {
        cout << "Message-Number: ";
        cin.getline(buffer, BUF);

        // Check if the entered message number is valid (greater than 0)
        if (atoi(buffer) > 0)
        {
            fullMessage += buffer;
            fullMessage += "\n";
            break;
        }
        else
        {
            cerr << "Error: Message number must be greater than 0.\n";
        }
    } while (true);

    // Send the DEL command to the server
    strncpy(buffer, fullMessage.c_str(), BUF);
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    const char *serverIp = argv[1];
    int serverPort = std::atoi(argv[2]);

    Client client;
    client.run(serverIp, serverPort);

    return 0;
}
