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
#include <glibmm/dispatcher.h>
#include <gdkmm/general.h>

#include "config.hpp"
#include "config_parser.hpp"
#include "keypad.hpp"
#include "tap_to_wake.hpp"

class syslock : public Gtk::Window {

	public:
		syslock(const config_lock &cfg);
		void lock();

		std::vector<Gtk::Window*> windows;

	private:
		config_lock config_main;
		double window_height;
		int start_height;
		bool auth;
		sigc::connection connection;

		#ifdef CONFIG_FILE
		config_parser *config;
		#endif

		std::string lock_cmd = "";
		std::string unlock_cmd = "";

		Gtk::Box box_lock_screen;
		Gtk::Box box_login_screen;
		Gtk::Box box_layout;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::Overlay overlay;

		Gtk::Label label_time;
		Gtk::Label label_date;
		std::string time_format = "%H:%M";
		std::string date_format = "%a %d %b";

		Gtk::Image image_profile;
		std::string profile_picture_path = "";
		int profile_scale = 128;
		int profile_rounding = 64;

		Gtk::Label label_username;
		Gtk::Label label_error;
		Gtk::Entry entry_password;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		Glib::Dispatcher dispatcher_auth;
		keypad *keypad_main;
		std::thread thread_auth;

		#ifdef FEATURE_TAP_TO_WAKE
		tap_to_wake *listener;
		#endif

		Glib::RefPtr<Gdk::Pixbuf> create_rounded_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &src_pixbuf, const int &size, const int &rounding_radius);

		void auth_start();
		void auth_end();
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

