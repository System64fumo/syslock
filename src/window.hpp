#pragma once

#include <gtkmm.h>

class syslock : public Gtk::Window {

	public:
		syslock();

	private:
		Gtk::Box box_layout;
		Gtk::Image image_profile;
		Gtk::Label label_username;
		Gtk::Entry entry_password;

		void on_entry();
		void setup_window(GtkWindow *window, GdkMonitor *monitor, const char* name);
		void show_windows();
};

