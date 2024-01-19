#include "ldap_authentication.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>
#include <iostream>

std::string LDAPAuthentication::authenticateUser( std::string rawLdapUser,  std::string rawLdapPassword)
{

  ////////////////////////////////////////////////////////////////////////////
  // LDAP config
  // anonymous bind with user and pw empty
  const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
  const int ldapVersion = LDAP_VERSION3;

  char ldapBindUser[256];
  char ldapBindPassword[256];
  strcpy(ldapBindPassword, rawLdapPassword.c_str());

  const char *rawLdapUserCStr = rawLdapUser.c_str();

  sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUserCStr);
  printf("user based on environment variable ldapuser set to: %s\n", ldapBindUser);

  // general
  int rc = 0; // return code

  ////////////////////////////////////////////////////////////////////////////
  // setup LDAP connection
  LDAP *ldapHandle;
  rc = ldap_initialize(&ldapHandle, ldapUri);
  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr, "ldap_init failed\n");
    return "Authentication failed";
  }
  printf("connected to LDAP server %s\n", ldapUri);

  ////////////////////////////////////////////////////////////////////////////
  // set version options
  rc = ldap_set_option(
      ldapHandle,
      LDAP_OPT_PROTOCOL_VERSION, // OPTION
      &ldapVersion);             // IN-Value
  if (rc != LDAP_OPT_SUCCESS)
  {
    fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    return "Authentication failed";
  }

  ////////////////////////////////////////////////////////////////////////////
  // start connection secure (initialize TLS)
  rc = ldap_start_tls_s(
      ldapHandle,
      NULL,
      NULL);
  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    return "Authentication failed";
  }

  ////////////////////////////////////////////////////////////////////////////
  // bind credentials for authentication
  BerValue bindCredentials;
  bindCredentials.bv_val = (char *)ldapBindPassword;
  bindCredentials.bv_len = strlen(ldapBindPassword);
  BerValue *servercredp; // server's credentials
  rc = ldap_sasl_bind_s(
      ldapHandle,
      ldapBindUser,
      LDAP_SASL_SIMPLE,
      &bindCredentials,
      NULL,
      NULL,
      &servercredp);
  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));

    if (rc == LDAP_INVALID_CREDENTIALS)
    {
      std::cout << "Invalid credentials";
    }
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    return "Authentication failed";
  }

  printf("Authentication successful.\n");

  ////////////////////////////////////////////////////////////////////////////
  // unbind and close connection
  ldap_unbind_ext_s(ldapHandle, NULL, NULL);

  return ldapBindUser;
}
