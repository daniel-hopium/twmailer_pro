// server.h

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <sstream>
#include "blacklist.h" 

class Server
{
public:
  Server(Blacklist &bl, const std::string &dir) : mailDir(dir), blacklist(bl) {}
  void *clientCommunication(void *data, std::string ipAddress);

private:
  int create_socket;
  int new_socket;
  bool abortRequested;
  std::string mailDir;
  Blacklist &blacklist;

  // Helper function to send messages to the client
  void sendMessage(int current_socket, const std::string &message);

  // Command handlers
  std::string handleLogin(int current_socket, std::istringstream &bufferStream, std::string ipAddress);
  void handleSend(int current_socket, std::istringstream &bufferStream, std::string username);
  void handleList(int current_socket, std::string username);
  void handleRead(int current_socket, std::istringstream &bufferStream, std::string username);
  void handleDelete(int current_socket, std::istringstream &bufferStream, std::string username);

  bool isAuthorized(std::string username, int current_socket);
};
void signalHandler(int sig);

#endif // SERVER_H
