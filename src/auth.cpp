#include "auth.hpp"
#include <cstdlib>
#include <string.h>

int pam_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr) {
	struct pam_response *response = (struct pam_response *)malloc(num_msg * sizeof(struct pam_response));
	if (response == NULL)
		return PAM_CONV_ERR;

	for (int i = 0; i < num_msg; ++i) {
		if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
			const char *password = (const char *)appdata_ptr;
			response[i].resp = strdup(password);
			response[i].resp_retcode = 0;
		}
		else {
			free(response);
			return PAM_CONV_ERR;
		}
	}

	*resp = response;
	return PAM_SUCCESS;
}

bool authenticate(char *user, const char *password) {
	struct pam_conv conv = {
		pam_conversation,
		(void *)password
	};
	pam_handle_t *pamh = NULL;
	int retval = PAM_ERROR_MSG;
	retval = pam_start("login", user, &conv, &pamh);
	retval = pam_authenticate(pamh, 0);
	pam_end(pamh, retval);
	return (retval == PAM_SUCCESS);
}