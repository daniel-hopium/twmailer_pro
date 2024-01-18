#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>
#include <iostream>

int main()
{
  ////////////////////////////////////////////////////////////////////////////
  // LDAP config
  // anonymous bind with user and pw empty
  const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
  const int ldapVersion = LDAP_VERSION3;

  // read username (bash: export ldapuser=<yourUsername>)
  char ldapBindUser[256];
  char rawLdapUser[128];

  const char *rawLdapUserEnv = getenv("ldapuser");
  if (rawLdapUserEnv == NULL)
  {
    printf("(user not found... set to empty string)\n");
    strcpy(ldapBindUser, "");
  }
  else
  {
    sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUserEnv);
    printf("user based on environment variable ldapuser set to: %s\n", ldapBindUser);
  }

  // read password (bash: export ldappw=<yourPW>)
  char ldapBindPassword[256];

  const char *ldapBindPasswordEnv = getenv("ldappw");
  if (ldapBindPasswordEnv == NULL)
  {
    strcpy(ldapBindPassword, "");
    printf("(pw not found... set to empty string)\n");
  }
  else
  {
    strcpy(ldapBindPassword, ldapBindPasswordEnv);
    printf("pw taken over from environment variable ldappw\n");
  }

  // general
  int rc = 0; // return code

  ////////////////////////////////////////////////////////////////////////////
  // setup LDAP connection
  // https://linux.die.net/man/3/ldap_initialize
  LDAP *ldapHandle;
  rc = ldap_initialize(&ldapHandle, ldapUri);
  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr, "ldap_init failed\n");
    return EXIT_FAILURE;
  }
  printf("connected to LDAP server %s\n", ldapUri);

  ////////////////////////////////////////////////////////////////////////////
  // set version options
  // https://linux.die.net/man/3/ldap_set_option
  rc = ldap_set_option(
      ldapHandle,
      LDAP_OPT_PROTOCOL_VERSION, // OPTION
      &ldapVersion);             // IN-Value
  if (rc != LDAP_OPT_SUCCESS)
  {
    // https://www.openldap.org/software/man.cgi?query=ldap_err2string&sektion=3&apropos=0&manpath=OpenLDAP+2.4-Release
    fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    return EXIT_FAILURE;
  }

  ////////////////////////////////////////////////////////////////////////////
  // start connection secure (initialize TLS)
  // https://linux.die.net/man/3/ldap_start_tls_s
  rc = ldap_start_tls_s(
      ldapHandle,
      NULL,
      NULL);
  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
    return EXIT_FAILURE;
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
    return EXIT_FAILURE;
  }

  printf("Authentication successful.\n");

  ////////////////////////////////////////////////////////////////////////////
  // unbind and close connection
  ldap_unbind_ext_s(ldapHandle, NULL, NULL);

  return EXIT_SUCCESS;
}
