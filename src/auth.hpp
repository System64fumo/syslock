#pragma once
#include <security/pam_appl.h>
#include <wayland-client.h>
#include <gtkmm/window.h>

inline struct wl_display *display = nullptr;
inline struct wl_registry *registry = nullptr;
inline struct wl_output *output = nullptr;
inline struct ext_session_lock_manager_v1 *session_lock_manager = nullptr;
inline struct ext_session_lock_v1 *session_lock = nullptr;
inline struct ext_session_lock_surface_v1 *lock_surface = nullptr;
inline struct wl_surface *wl_surface = nullptr;

int pam_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr);
bool authenticate(char *user, const char *password);
void lock_session(Gtk::Window &window);
void unlock_session();
