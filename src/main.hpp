#pragma once

#include "window.hpp"

Glib::RefPtr<Gdk::Pixbuf> create_circular_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src_pixbuf);
inline Glib::RefPtr<Gtk::Application> app;
inline int profile_scale = 128;
inline syslock* win;
