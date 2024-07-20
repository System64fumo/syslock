#pragma once
#include <gtkmm/overlay.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/gesturedrag.h>
#include <gdkmm/general.h>

#include "config.hpp"
#include "keypad.hpp"

class syslock : public Gtk::Window {

	public:
		syslock(const config_lock &cfg);
		void lock();

		std::vector<Gtk::Window*> windows;

	private:
		config_lock config_main;
		double window_height;
		int start_height;
		sigc::connection connection;

		Gtk::Box box_lock_screen;
		Gtk::Box box_login_screen;
		Gtk::Box box_layout;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::Overlay overlay;

		Gtk::Label label_clock;
		Gtk::Label label_date;

		Gtk::Image image_profile;
		Gtk::Label label_username;
		Gtk::Label label_error;
		Gtk::Entry entry_password;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		keypad *keypad_main;

		Glib::RefPtr<Gdk::Pixbuf> create_circular_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src_pixbuf, int size);

		void on_entry();
		void on_entry_changed();
		void setup_window(GtkWindow *window, GdkMonitor *monitor, const char* name);
		void show_windows();

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);

		bool update_time_date();
};

extern "C" {
	syslock *syslock_create(const config_lock &cfg);
	void syslock_lock(syslock* window);
}

