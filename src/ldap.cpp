#include <iostream>
#include <cstring>
#include <ldap.h>

#define LDAP_HOST "ldap.technikum-wien.at"
#define LDAP_PORT 389
#define LDAP_SEARCH_BASE "dc=technikum-wien,dc=at"

// Function to initialize LDAP connection
LDAP *initializeLDAP()
{
  LDAP *ldap;
  int ldapResult = ldap_initialize(&ldap, ("ldap://" LDAP_HOST ":389"));

  if (ldapResult != LDAP_SUCCESS)
  {
    std::cerr << "LDAP initialization failed. Error: " << ldap_err2string(ldapResult) << std::endl;
    return nullptr; // Return nullptr on failure
  }

  return ldap;
}

// Function to authenticate user using LDAP
bool authenticateUser(LDAP *ldap, const std::string &username, const std::string &password)
{
  int ldapResult;

  // Set up SASL credentials
  struct berval cred;
  cred.bv_val = const_cast<char *>(password.c_str());
  cred.bv_len = password.length();

  // Perform SASL bind synchronously
  ldapResult = ldap_sasl_bind_s(ldap,
                                ("uid=" + username + ",ou=people," + LDAP_SEARCH_BASE).c_str(),
                                LDAP_SASL_SIMPLE,
                                &cred,
                                nullptr, nullptr, nullptr);

  if (ldapResult == LDAP_SUCCESS)
  {
    std::cout << "Authentication successful." << std::endl;
    return true;
  }
  else
  {
    std::cerr << "Authentication failed. Error: " << ldap_err2string(ldapResult) << std::endl;
    return false;
  }
}

int main()
{
  // Initialize LDAP
  LDAP *ldap = initializeLDAP();

  if (ldap == nullptr)
  {
    // Error message already printed in initializeLDAP
    return 1;
  }

  

  // Example: Authenticate user with hidden password
  std::string username;
  std::cout << "Enter username: ";
  std::cin >> username;

  std::string password;
  std::cout << "Enter password: ";
  std::cin >> password;


  std::cout << "wait ";
  if (authenticateUser(ldap, username, password))
  {
    // Perform other operations after successful authentication
    std::cout << "Successfully logged in." << std::endl;
  }
  else
  {
    std::cout << "Login failed." << std::endl;
  }

  // Close LDAP connection
  ldap_unbind_ext_s(ldap, nullptr, nullptr);

  return 0;
}
