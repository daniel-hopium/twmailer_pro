#include "blacklist.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>


void Blacklist::writeAttemptToBlacklist(std::string username, std::string ipAddress)
{

  std::string  fileName = blacklistDir + ipAddress + ".txt";

  // Write the message to the file in append mode
  std::ofstream blacklistFile(fileName, std::ios::app);
  if (blacklistFile.is_open())
  {
    // Get the current timestamp
    time_t currentTime = time(nullptr);
    std::string timestamp = std::to_string(currentTime);

    // Append the message to the file
    blacklistFile << timestamp << " " << username << "\n";
    blacklistFile.close();
  }
  else
  {
    perror("Error writing message to file");
  }
}

int Blacklist::getLoginAttempts(std::string ipAddress)
{
  // Generate the filename for the blacklist file
  std::string fileName = blacklistDir + ipAddress + ".txt";

  // Check if file exists
  std::ifstream file(fileName);
  if (!file.good())
  {
    return 0;
  }

  // Open the blacklist file
  std::ifstream blacklistFile(fileName);
  if (!blacklistFile.is_open())
  {
    std::cerr << "Error opening blacklist file for IP: " << ipAddress << std::endl;
    return -1;
  }

  // Count the number of lines (login attempts)
  int loginAttempts = 0;
  std::string line;
  while (std::getline(blacklistFile, line))
  {
    loginAttempts++;
  }

  return loginAttempts;
}

bool Blacklist::hasTooManyAttempts(std::string ipAddress)
{
  if (getLoginAttempts(ipAddress) == 3)
  {
    blacklistIp(ipAddress);
    return true;
  }
  return false;
}

bool Blacklist::isBlacklisted(std::string ipAddress)
{
  if (getLoginAttempts(ipAddress) != 4)
  {
    return false;
  }

  // Generate a unique filename for each received message using a timestamp
  std::string fileName = blacklistDir + ipAddress + ".txt";

  std::string lastEntry = extractLastEntry(fileName);

  // Extract last entry for comparison
  std::istringstream iss(lastEntry);
  std::string lastEntryTimeStr;
  iss >> lastEntryTimeStr;
  time_t lastEntryTime = std::stoll(lastEntryTimeStr);

  time_t currentTime = time(nullptr);

  if (currentTime - lastEntryTime > 60)
  {
    clearBlacklist(ipAddress);
    return false;
  }
  return true;
}

void Blacklist::blacklistIp(std::string ipAddress)
{
  // Generate a unique filename for each received message using a timestamp
  std::string fileName = blacklistDir + ipAddress + ".txt";

  // Write the message to the file in append mode
  std::ofstream blacklistFile(fileName, std::ios::app);
  if (blacklistFile.is_open())
  {
    // Get the current timestamp
    time_t currentTime = time(nullptr);
    std::string timestamp = std::to_string(currentTime);

    // Append the message to the file
    blacklistFile << timestamp << " "
                  << "BLACKLISTED"
                  << "\n";
    blacklistFile.close();
  }
  else
  {
    perror("Error writing message to file");
  }
}

void Blacklist::clearBlacklist(std::string ipAddress)
{
  std::string fileName = blacklistDir + ipAddress + ".txt";

  // Use remove to delete the file
  if (remove(fileName.c_str()) == 0)
  {
    std::cout << "Blacklist for IP " << ipAddress << " cleared successfully." << std::endl;
  }
  else
  {
    perror("Error clearing blacklist");
  }
}
void Blacklist::printLoginAttempts(std::string ipAddress)
{
  int loginAttempts = getLoginAttempts(ipAddress);
  std::cout << "Login attempts for IP " + ipAddress + ": " << loginAttempts << std::endl;
}

std::string Blacklist::extractLastEntry(const std::string &filePath)
{
  std::ifstream file(filePath);

  if (!file.is_open())
  {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return "";
  }

  std::string lastEntry;

  // Read the file line by line
  std::string line;
  while (std::getline(file, line))
  {
    lastEntry = line; // Update last entry for each line read
  }

  file.close();
  return lastEntry;
}
