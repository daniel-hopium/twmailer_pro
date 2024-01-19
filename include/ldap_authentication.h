#ifndef LDAP_AUTHENTICATION_H
#define LDAP_AUTHENTICATION_H

#include <string>

class LDAPAuthentication
{
public:
  std::string authenticateUser( std::string rawLdapUser,  std::string rawLdapPassword);
};

#endif // LDAP_AUTHENTICATION_H
