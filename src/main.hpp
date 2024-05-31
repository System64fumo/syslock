#pragma once

#include "window.hpp"

Glib::RefPtr<Gdk::Pixbuf> create_circular_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src_pixbuf, int size);
inline Glib::RefPtr<Gtk::Application> app;
inline syslock* win;
