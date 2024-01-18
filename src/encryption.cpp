#include "encryption.h"
#include <iostream>
#include <string>

using namespace std;

std::string Encryption::encrypt(const std::string &input, const std::string &key)
{
  string encrypted = input;
  for (size_t i = 0; i < input.size(); ++i)
  {
    encrypted[i] ^= key[i % key.size()];
  }
  return encrypted;
}

std::string Encryption::decrypt(const std::string &input, const std::string &key)
{
  return encrypt(input, key); // XOR encryption and decryption are the same operation
}
