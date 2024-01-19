// encryption.h
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

class Encryption
{
public:
    // Function to encrypt a string using a simple substitution cipher
    std::string encrypt(const std::string &input);

    // Function to decrypt a string using a simple substitution cipher
    std::string decrypt(const std::string &input);

private:
    static const int shift = 3;      // Shift for letters
    static const int digitShift = 5; // Shift for digits
};

#endif // ENCRYPTION_H
