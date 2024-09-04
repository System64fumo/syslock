#pragma once
#include <security/pam_appl.h>
#include <wayland-client.h>
#include <gtkmm/window.h>
#include <gdk/wayland/gdkwayland.h>
#include "ext-session-lock-v1.h"

inline GdkSurface* gdk_surface;
inline wl_display* wayland_display;
inline wl_surface* wayland_surface;
inline wl_output* output;
inline ext_session_lock_v1* session_lock;
inline ext_session_lock_manager_v1* session_lock_manager;

int pam_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr);
bool authenticate(char *user, const char *password);
void lock_session(Gtk::Window *window);
void unlock_session();
