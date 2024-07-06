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

class syslock : public Gtk::Window {

	public:
		syslock();

	private:
		double window_height;
		int start_height;

		Gtk::Box box_lock_screen;
		Gtk::Box box_login_screen;
		Gtk::ScrolledWindow box_layout;
		Gtk::Overlay overlay;

		Gtk::Label label_clock;

		Gtk::Image image_profile;
		Gtk::Label label_username;
		Gtk::Label label_error;
		Gtk::Entry entry_password;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;

		void on_entry();
		void on_entry_changed();
		void setup_window(GtkWindow *window, GdkMonitor *monitor, const char* name);
		void show_windows();

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);

		bool update_time();
};

