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

#include <set>

#include "config.hpp"
#include "config_parser.hpp"
#include "keypad.hpp"
#include "tap_to_wake.hpp"

class syslock {

	public:
		syslock(const std::map<std::string, std::map<std::string, std::string>>&);
		void lock();

		std::map<std::string, std::map<std::string, std::string>> config_main;
		std::vector<Gtk::Window*> windows;
		Gtk::Window* primary_window;
		Gtk::Entry entry_password;

	private:
		bool locked;
		double window_height;
		int start_height;
		bool auth;
		sigc::connection connection;
		std::map<std::string, Gtk::Window*> monitor_windows;

		#ifdef CONFIG_FILE
		config_parser *config;
		#endif

		std::string lock_cmd = "";
		std::string unlock_cmd = "";

		Gtk::Box box_lock_screen;
		Gtk::Box box_login_screen;
		Gtk::Box box_layout;
		Gtk::Box box_widgets;
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
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		Glib::Dispatcher dispatcher_auth;
		keypad *keypad_main;
		std::thread thread_auth;

		#ifdef FEATURE_TAP_TO_WAKE
		tap_to_wake *listener;
		#endif

		Glib::RefPtr<Gdk::Pixbuf> create_rounded_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &src_pixbuf, const int &size, const int &rounding_radius);

		Gtk::Window* create_main_window(GdkMonitor* monitor);
		Gtk::Window* create_secondary_window(GdkMonitor* monitor);

		void auth_start();
		void auth_end();
		void on_entry_changed();
		void handle_monitors_initial();
		void on_monitors_changed(guint position, guint removed, guint added);
		void setup_window(GtkWindow *window, GdkMonitor *monitor, const char* name);

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);

		bool update_time_date();
};

extern "C" {
	syslock *syslock_create(const std::map<std::string, std::map<std::string, std::string>>&);
	void syslock_lock(syslock* window);
}

