#include "encryption.h"
#include <iostream>
#include <string>

using namespace std;

std::string Encryption::encrypt(const std::string &input)
{
  std::string result = input;

  for (char &ch : result)
  {
    // Encrypt letters (both lowercase and uppercase)
    if (isalpha(ch))
    {
      char base = islower(ch) ? 'a' : 'A';
      ch = (ch - base + shift) % 26 + base;
    }
    // Encrypt digits
    else if (isdigit(ch))
    {
      ch = ((ch - '0') + digitShift) % 10 + '0';
    }
    // For other characters, leave them unchanged
  }

  return result;
}

std::string Encryption::decrypt(const std::string &input)
{
  std::string result = input;

  for (char &ch : result)
  {
    // Decrypt letters (both lowercase and uppercase)
    if (isalpha(ch))
    {
      char base = islower(ch) ? 'a' : 'A';
      ch = (ch - base - shift + 26) % 26 + base;
    }
    // Decrypt digits
    else if (isdigit(ch))
    {
      ch = ((ch - '0') - digitShift + 10) % 10 + '0';
    }
    // For other characters, leave them unchanged
  }

  return result;
}
