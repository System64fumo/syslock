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
#include <thread>

#include "config.hpp"

#ifdef FEATURE_TAP_TO_WAKE
#include "tap_to_wake.hpp"
#endif
#ifdef FEATURE_KEYPAD
#include "keypad.hpp"
#endif

class syslock_window : public Gtk::Window {
	public:
		syslock_window(std::map<std::string, std::map<std::string, std::string>>* config);

		Glib::Dispatcher dispatcher_auth;

	private:
		// Configs
		std::map<std::string, std::map<std::string, std::string>>* config;
		std::string time_format = "%H:%M";
		std::string date_format = "%a %d %b";
		std::string profile_picture_path = "";
		int profile_scale = 128;
		int profile_rounding = 64;

		// Internal variables
		double window_height;
		int start_height;
		bool authenticated;

		Gtk::Overlay overlay;
		Gtk::Box box_lock_screen;
		Gtk::Box box_login_screen;

		Gtk::Box box_layout;
		Gtk::Box box_widgets;
		Gtk::ScrolledWindow scrolled_window;

		Gtk::Label label_time;
		Gtk::Label label_date;

		Gtk::Image image_profile;
		Gtk::Label label_username;
		Gtk::Label label_error;

		Gtk::Entry entry_password;

		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		sigc::connection connection;
		std::thread thread_auth;

		// Features
		#ifdef FEATURE_TAP_TO_WAKE
		tap_to_wake *listener;
		#endif

		#ifdef FEATURE_KEYPAD
		keypad *keypad_main;
		#endif

		void reset();
		bool update_time_date();
		void auth_start();
		void on_entry_changed();

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);
};