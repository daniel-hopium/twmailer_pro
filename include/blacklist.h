#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <string>

class Blacklist
{
public:
  std::string mailDir;

  int getLoginAttempts(std::string ipAddress);
  void writeAttemptToBlacklist(std::string username, std::string ipAddress);
  void blacklistIp(std::string ipAddress);
  void clearBlacklist(std::string ipAddress);
  bool isBlacklisted(std::string ipAddress);
  bool hasTooManyAttempts(std::string ipAddress);
  std::string extractLastEntry(const std::string &filePath);
};

#endif // BLACKLIST_H
