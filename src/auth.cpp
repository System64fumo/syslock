#include "auth.hpp"
#include "ext-session-lock-v1.h"

#include <cstring>
#include <gdk/gdk.h>
#include <gdk/wayland/gdkwayland.h>

int pam_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr) {
	struct pam_response *response = (struct pam_response *)malloc(num_msg * sizeof(struct pam_response));
	if (response == NULL) {
		return PAM_CONV_ERR;
	}

	for (int i = 0; i < num_msg; ++i) {
		if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
			const char *password = (const char *)appdata_ptr;
			response[i].resp = strdup(password);
			response[i].resp_retcode = 0;
		} else {
			free(response);
			return PAM_CONV_ERR;
		}
	}

	*resp = response;
	return PAM_SUCCESS;
}

bool authenticate(char *user, const char *password) {
	struct pam_conv conv = { pam_conversation, (void *)password };
	pam_handle_t *pamh = NULL;

	int retval = pam_start("login", user, &conv, &pamh);
	retval = pam_authenticate(pamh, 0);

	pam_end(pamh, retval);
	return (retval == PAM_SUCCESS);
}

static void registry_handler(void *data, struct wl_registry *registry,
							 uint32_t id, const char *interface, uint32_t version) {
	if (strcmp(interface, ext_session_lock_manager_v1_interface.name) == 0) {
		session_lock_manager = (struct ext_session_lock_manager_v1 *)
			wl_registry_bind(registry, id, &ext_session_lock_manager_v1_interface, 1);
		std::cout << "Bound to ext_session_lock_manager_v1, id: "
				  << id << ", version: " << version << std::endl;
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		output = (struct wl_output *) wl_registry_bind(registry, id, &wl_output_interface, 1);
		std::cout << "Bound to wl_output, id: " << id << ", version: " << version << std::endl;
	}
}

static const struct wl_registry_listener registry_listener = {
	registry_handler,
	nullptr
};

static void session_lock_done(void *data, struct ext_session_lock_v1 *lock) {
	std::cout << "Session locked" << std::endl;
}

static void lock_surface_configure(void *data, struct ext_session_lock_surface_v1 *lock_surface, uint32_t width, uint32_t height, uint32_t serial) {
	std::cout << "Lock surface configured: width=" << width \
			  << ", height=" << height << "serial=" << serial << std::endl;
	ext_session_lock_surface_v1_ack_configure(lock_surface, serial);
}

static const struct ext_session_lock_v1_listener session_lock_listener = {
	session_lock_done
};

static const struct ext_session_lock_surface_v1_listener lock_surface_listener = {
	lock_surface_configure
};

void lock_session(Gtk::Window &window) {
	display = wl_display_connect(nullptr);

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, nullptr);
	wl_display_roundtrip(display);

	session_lock = ext_session_lock_manager_v1_lock(session_lock_manager);

	ext_session_lock_v1_add_listener(session_lock, &session_lock_listener, nullptr);

	auto gdk_surface = window.get_surface()->gobj();
	wl_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);

	std::cout << "Creating lock surface with session_lock: " << session_lock
			  << ", wl_surface: " << wl_surface << ", output: " << output << std::endl;

	// Sanity check
	if (session_lock == nullptr) {
		std::cout << "session_lock is null" << std::endl;
	}
	if (wl_surface == nullptr) {
		std::cout << "wl_surface is null" << std::endl;
	}
	if (output == nullptr) {
		std::cout << "wl_output is null" << std::endl;
	}

	// TODO: Fix this
	lock_surface = ext_session_lock_v1_get_lock_surface(session_lock, wl_surface, output);
	ext_session_lock_surface_v1_add_listener(lock_surface, &lock_surface_listener, nullptr);
	wl_display_dispatch(display);
}

void unlock_session() {
	ext_session_lock_v1_unlock_and_destroy(session_lock);
	wl_display_roundtrip(display);
}
