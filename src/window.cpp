#include "main.hpp"
#include "window.hpp"
#include "css.hpp"
#include "config.hpp"
#include "keypad.hpp"
#include "auth.hpp"

#include <gtk4-layer-shell.h>
#include <iostream>
#include <filesystem>
#include <pwd.h>
#include <glibmm/main.h>
#include <ctime>

syslock::syslock() {
	// Initialize
	windows.push_back(this);
	set_default_size(640, 480);
	set_hide_on_close(true);
	show_windows();

	// Set up drag gestures
	gesture_drag = Gtk::GestureDrag::create();
	gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &syslock::on_drag_start));
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &syslock::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &syslock::on_drag_stop));
	add_controller(gesture_drag);

	set_child(overlay);
	overlay.set_child(box_lock_screen);
	box_lock_screen.get_style_context()->add_class("lock_screen");
	box_lock_screen.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_lock_screen.append(box_lock_screen);

	// TODO: Add config to set custom time formats
	box_lock_screen.append(label_clock);
	label_clock.get_style_context()->add_class("clock");
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &syslock::update_time), 1000);
	update_time();

	// TODO: Add an on entry change event to show the login screen.
	// TODO: Add a timeout event to hide the login screen. (Also clean the password box)
	overlay.add_overlay(box_layout);
	box_layout.append(scrolled_window);
	box_layout.get_style_context()->add_class("login_screen");
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_layout.set_opacity(0);

	scrolled_window.set_kinetic_scrolling(false);
	scrolled_window.set_child(box_login_screen);
	scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);
	scrolled_window.set_valign(Gtk::Align::END);

	box_login_screen.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_login_screen.set_valign(Gtk::Align::CENTER);
	box_login_screen.set_vexpand(true);

	// TODO: Clean this whole mess up
	// And add a way to enable/disable specific features (PFP, Username, Ect)
	std::string home_dir = getenv("HOME");
	if (profile_scale > 0) {
		std::string profile_picture = home_dir + "/.face";
	
		if (std::filesystem::exists(profile_picture)) {
			box_login_screen.append(image_profile);
			image_profile.get_style_context()->add_class("image_profile");
			auto pixbuf = Gdk::Pixbuf::create_from_file(profile_picture);
			pixbuf = pixbuf->scale_simple(profile_scale, profile_scale, Gdk::InterpType::BILINEAR);
			// TODO: Add a way to enable/disable rounding the profile picture
			pixbuf = create_circular_pixbuf(pixbuf, profile_scale);
			image_profile.set_size_request(profile_scale, profile_scale);
			image_profile.set(pixbuf);
			image_profile.set_halign(Gtk::Align::CENTER);
		}
	}

	box_login_screen.append(label_username);
	label_username.get_style_context()->add_class("label_username");
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	label_username.set_text((Glib::ustring)pw->pw_gecos);

	box_login_screen.append(entry_password);
	entry_password.get_style_context()->add_class("entry_password");
	entry_password.set_size_request(250, 30);
	entry_password.set_halign(Gtk::Align::CENTER);
	entry_password.set_visibility(false);
	entry_password.signal_activate().connect(sigc::mem_fun(*this, &syslock::on_entry));
	entry_password.signal_changed().connect(sigc::mem_fun(*this, &syslock::on_entry_changed));
	entry_password.grab_focus();
	entry_password.signal_changed().connect([&]() {
		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
		scrolled_window.set_size_request(-1, get_height());
		connection.disconnect();
		connection = Glib::signal_timeout().connect([&]() {lock();return false;}, 10 * 1000);
	});

	// TODO: add remaining tries left
	box_login_screen.append(label_error);
	label_error.get_style_context()->add_class("label_error");
	label_error.set_text("Incorrect password");
	label_error.hide();

	// Load custom css
	std::string css_path = home_dir + "/.config/sys64/lock/style.css";
	css_loader css(css_path, this);

	// TODO: Figure out why ext session lock causes hyprland to red screen
	//lock_session(*this);

	// Keypad
	if (keypad_enabled) {
		keypad keypad_main = keypad(entry_password, std::bind(&syslock::on_entry, this));
		box_login_screen.append(keypad_main);
	}
}

// TODO: Make this non blocking
void syslock::on_entry() {
	label_error.hide();
	char *user = getenv("USER");
	std::string password = entry_password.get_buffer()->get_text().raw();
	bool auth = authenticate(user, password.c_str());

	if (auth) {
		std::cout << "Authentication successful" << std::endl;
		//unlock_session();

		for (std::vector<Gtk::Window*>::iterator it = windows.begin(); it != windows.end(); ++it) {
			Gtk::Window* window = *it;
			window->hide();
		}

		entry_password.set_text("");
	}
	else {
		// TODO: Display how many times the user can retry the password
		std::cerr << "Authentication failed" << std::endl;
		entry_password.set_text("");
		label_error.show();
	}
}

void syslock::on_entry_changed() {
	// Trigger a password check automatically
	if ((int)entry_password.get_text().length() == pw_length) {
		on_entry();
	}
}

void syslock::setup_window(GtkWindow *window, GdkMonitor *monitor, const char* name) {
	gtk_layer_init_for_window(window);
	gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_namespace(gobj(), name);
	
	gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, true);
	gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, true);
	gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, true);
	gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	gtk_layer_set_monitor(window, monitor);
}

void syslock::show_windows() {
	GdkDisplay *display = gdk_display_get_default();
	GListModel *monitors = gdk_display_get_monitors(display);

	int monitorCount = g_list_model_get_n_items(monitors);

	if (main_monitor < 0)
		main_monitor = 0;
	else if (main_monitor >= monitorCount)
		main_monitor = monitorCount - 1;

	// Set up layer shell
	if (!debug) {
		setup_window(gobj(), GDK_MONITOR(g_list_model_get_item(monitors, main_monitor)), "syslock");
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

		// TODO: (VERY CRITICAL!!!)
		// Add a way to detect when a monitor is connected/disconnected
		for (int i = 0; i < monitorCount; ++i) {
			// Ignore primary monitor
			if (i == main_monitor)
				continue;
	
			GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_item(monitors, i));
	
			// Create empty windows
			Gtk::Window *window = new Gtk::Window();
			app->add_window(*window);
			windows.push_back(window);
			setup_window(window->gobj(), monitor, "syslock-empty-window");
			window->show();
		}
	}
	show();
}

// TODO: Make the login screen background static,
// Have the lock interface go up when pulling up instead.
void syslock::on_drag_start(const double &x, const double &y) {
	connection.disconnect();
	if (!gesture_drag->get_current_event()->get_pointer_emulated()) {
		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
		scrolled_window.set_size_request(-1, get_height());
		gesture_drag->reset();
		return;
	}

	window_height = get_height();
	start_height = scrolled_window.get_height();
	scrolled_window.set_valign(Gtk::Align::END);
	if (start_height > 300)
		scrolled_window.set_size_request(-1, window_height);
}

void syslock::on_drag_update(const double &x, const double &y) {
	if (start_height < 100)
		scrolled_window.set_size_request(-1, -y);
	else
		scrolled_window.set_size_request(-1, start_height - y);
	box_layout.set_opacity(scrolled_window.get_height() / window_height);
}

void syslock::on_drag_stop(const double &x, const double &y) {
	connection = Glib::signal_timeout().connect([&]() {lock();return false;}, 10 * 1000);
	if (!gesture_drag->get_current_event()->get_pointer_emulated())
		return;

	if (scrolled_window.get_height() > 300) {
		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
	}
	else {
		scrolled_window.set_valign(Gtk::Align::END);
		box_layout.set_opacity(0);
	}
	scrolled_window.set_size_request(-1, -1);
}


bool syslock::update_time() {
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);

	char label_buffer[32];
	std::strftime(label_buffer, sizeof(label_buffer), "%H:%M", local_time);
	label_clock.set_text(label_buffer);
	return true;
}

void syslock::lock() {
	scrolled_window.set_valign(Gtk::Align::END);
	box_layout.set_opacity(0);
	scrolled_window.set_size_request(-1, -1);
}
