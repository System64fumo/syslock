#include <iostream>
#include <security/pam_appl.h>
#include <cstring>

int pam_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr);
bool authenticate(char *user, const char *password);
