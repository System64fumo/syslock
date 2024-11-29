#include "window.hpp"
#include "css.hpp"
#include "auth.hpp"

#include <gtk4-layer-shell.h>
#include <filesystem>
#include <pwd.h>
#include <glibmm/main.h>
#include <ctime>

syslock::syslock(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
	config_main = cfg;

	// Initialize
	set_name("syslock");
	windows.push_back(this);
	set_default_size(640, 480);
	set_hide_on_close(true);

	profile_picture_path = config_main["profile"]["image-path"];
	profile_scale = std::stoi(config_main["profile"]["scale"]);
	profile_rounding = std::stoi(config_main["profile"]["rounding"]);
	time_format = config_main["clock"]["time-format"];
	date_format = config_main["clock"]["date-format"];
	lock_cmd = config_main["events"]["on-lock-cmd"];
	unlock_cmd = config_main["events"]["on-unlock-cmd"];

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

	// TODO: Add a config option to select what appears on the lockscreen
	box_lock_screen.append(label_time);
	label_time.get_style_context()->add_class("time");

	box_lock_screen.append(label_date);
	label_date.get_style_context()->add_class("date");

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &syslock::update_time_date), 1000);
	update_time_date();

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

	// And add a way to enable/disable specific features (PFP, Username, Ect)

	std::string home_dir = getenv("HOME");
	if (profile_picture_path == "")
		profile_picture_path = home_dir + "/.face";

	if (std::filesystem::exists(profile_picture_path) && profile_scale > 0) {
		box_login_screen.append(image_profile);
		image_profile.get_style_context()->add_class("image_profile");
		auto pixbuf = Gdk::Pixbuf::create_from_file(profile_picture_path);
		pixbuf = pixbuf->scale_simple(profile_scale, profile_scale, Gdk::InterpType::BILINEAR);

		// Round the image
		if (profile_rounding != 0)
			pixbuf = create_rounded_pixbuf(pixbuf, profile_scale, profile_rounding);

		image_profile.set_size_request(profile_scale, profile_scale);
		image_profile.set(pixbuf);
		image_profile.set_halign(Gtk::Align::CENTER);
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
	entry_password.set_input_purpose(Gtk::InputPurpose::PASSWORD);
	entry_password.signal_activate().connect(sigc::mem_fun(*this, &syslock::auth_start));
	entry_password.signal_changed().connect(sigc::mem_fun(*this, &syslock::on_entry_changed));
	entry_password.grab_focus();
	entry_password.signal_changed().connect([&]() {
		if (entry_password.get_text() == "")
			return;

		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
		scrolled_window.set_size_request(-1, get_height());
		connection.disconnect();
		connection = Glib::signal_timeout().connect([&]() {
			lock();
			return false;
		}, 10 * 1000);
	});

	dispatcher_auth.connect(sigc::mem_fun(*this, &syslock::auth_end));

	// TODO: add remaining tries left
	box_login_screen.append(label_error);
	label_error.get_style_context()->add_class("label_error");
	label_error.set_text("Incorrect password");
	label_error.hide();

	// Keypad
	if (config_main["main"]["keypad"] == "true") {
		keypad_main = Gtk::make_managed<keypad>(entry_password, sigc::mem_fun(*this, &syslock::auth_start));
		box_login_screen.append(*keypad_main);
		entry_password.set_input_purpose(Gtk::InputPurpose::PIN);
	}

	// Tap to wake
	#ifdef FEATURE_TAP_TO_WAKE
	listener = new tap_to_wake(config_main);
	#endif

	// Load custom css
	std::string style_path;
	if (std::filesystem::exists(home_dir + "/.config/sys64/lock/style.css"))
		style_path = home_dir + "/.config/sys64/lock/style.css";
	else if (std::filesystem::exists("/usr/share/sys64/lock/style.css"))
		style_path = "/usr/share/sys64/lock/style.css";
	else
		style_path = "/usr/local/share/sys64/lock/style.css";

	css_loader css(style_path, this);

	// Set classes properly (No clue why this has to be done this way, Don't question it)
	signal_map().connect([this] () {
		get_style_context()->remove_class("locked");
		Glib::signal_timeout().connect([this]() {
			get_style_context()->add_class("locked");
			return false;
		}, 100);
		// TODO: This is unstable, Sometimes it works sometimes it doesn't
		// Figure out why it's like this somehow.
		// Also for some reason this causes the window to show twice?
		if (config_main["main"]["experimental"] == "true")
			lock_session(this);
	});

	show_windows();
	lock();
}

void syslock::auth_start() {
	label_error.hide();
	std::string password = entry_password.get_buffer()->get_text().raw();

	// Remove focus
	Gtk::Button focus_dummy;
	box_layout.append(focus_dummy);
	focus_dummy.grab_focus();
	entry_password.set_sensitive(false);
	box_layout.remove(focus_dummy);

	std::thread thread_auth([&, password]() {
		char *user = getenv("USER");
		auth = authenticate(user, password.c_str());
		dispatcher_auth.emit();
	});
	thread_auth.detach();
}

void syslock::auth_end() {
	if (auth) {
		if (config_main["main"]["experimental"] == "true")
			unlock_session();

		#ifdef FEATURE_TAP_TO_WAKE
		if (listener->running)
			listener->stop_listener();
		#endif

		// Add a delay for fancy css animations
		for (std::vector<Gtk::Window*>::iterator it = windows.begin(); it != windows.end(); ++it) {
			Gtk::Window* window = *it;
			window->get_style_context()->remove_class("locked");
			window->get_style_context()->add_class("unlocked");
		}

		Glib::signal_timeout().connect([this]() {
			for (std::vector<Gtk::Window*>::iterator it = windows.begin(); it != windows.end(); ++it) {
				Gtk::Window* window = *it;
				window->get_style_context()->add_class("locked");
				window->get_style_context()->remove_class("unlocked");
				window->hide();
				locked = false;
			}
			entry_password.set_text("");
			connection.disconnect();

			if (unlock_cmd != "") {
				std::thread([this]() {
					system(unlock_cmd.c_str());
				}).detach();
			}
			return false;
		}, 250);
	}
	else {
		// TODO: Display how many times the user can retry the password
		entry_password.set_text("");
		label_error.show();
	}
	entry_password.set_sensitive(true);
}

void syslock::on_entry_changed() {
	// Trigger a password check automatically
	if ((int)entry_password.get_text().length() == std::stoi(config_main["main"]["password-length"])) {
		auth_start();
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

	int main_monitor = std::stoi(config_main["main"]["main-monitor"]);
	int monitorCount = g_list_model_get_n_items(monitors);

	if (main_monitor < 0)
		main_monitor = 0;
	else if (main_monitor >= monitorCount)
		main_monitor = monitorCount - 1;

	// Set up layer shell
	if (config_main["main"]["debug"] != "true") {
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
			Gtk::Window *window = Gtk::make_managed<Gtk::Window>();
			window->set_name("syslock-empty-window");
			window->get_style_context()->add_class("locked");
			windows.push_back(window);
			setup_window(window->gobj(), monitor, "syslock-empty-window");

			if (config_main["main"]["start-unlocked"] != "true")
				window->show();
		}
	}

	if (config_main["main"]["start-unlocked"] != "true")
		show();
}

void syslock::on_drag_start(const double &x, const double &y) {
	connection.disconnect();

	// Block gesture inputs from the keypad
	if (config_main["main"]["keypad"] == "true") {
		double keypad_x, keypad_y;
		keypad_main->translate_coordinates(box_lock_screen, 0, 0, keypad_x, keypad_y);

		if (x >= keypad_x && x <= keypad_x + keypad_main->get_width() &&
			y >= keypad_y && y <= keypad_y + keypad_main->get_height()) {
			gesture_drag->reset();
		}
	}

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
	if (start_height < 100) {
		if (scrolled_window.get_height() >= window_height)
			return;
		scrolled_window.set_size_request(-1, std::min(window_height, std::max(0.0, - y)));
	}
	else {
		if (-y > 0)
			return;
		scrolled_window.set_size_request(-1, std::min(window_height, std::max(0.0, start_height - y)));
	}

	box_layout.set_opacity(scrolled_window.get_height() / window_height);
}

void syslock::on_drag_stop(const double &x, const double &y) {
	connection = Glib::signal_timeout().connect([&]() {lock();return false;}, 10 * 1000);
	if (!gesture_drag->get_current_event()->get_pointer_emulated())
		return;

	if (scrolled_window.get_height() > 300) {
		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
		#ifdef FEATURE_TAP_TO_WAKE
		if (listener->running)
			listener->stop_listener();
		#endif
	}
	else {
		scrolled_window.set_valign(Gtk::Align::END);
		box_layout.set_opacity(0);
		#ifdef FEATURE_TAP_TO_WAKE
		if (!listener->running)
			listener->start_listener();
		#endif
	}
	scrolled_window.set_size_request(-1, -1);
}

bool syslock::update_time_date() {
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);

	char time_buffer[32];
	std::strftime(time_buffer, sizeof(time_buffer), time_format.c_str(), local_time);
	label_time.set_text(time_buffer);

	char date_buffer[32];
	std::strftime(date_buffer, sizeof(date_buffer), date_format.c_str(), local_time);
	label_date.set_text(date_buffer);
	return true;
}

void syslock::lock() {
	scrolled_window.set_valign(Gtk::Align::END);
	box_layout.set_opacity(0);
	scrolled_window.set_size_request(-1, -1);
	entry_password.set_text("");

	if (locked)
		return;

	if (lock_cmd != "") {
		std::thread([this]() {
			system(lock_cmd.c_str());
		}).detach();
	}

	#ifdef FEATURE_TAP_TO_WAKE
	if (!listener->running)
		listener->start_listener();
	#endif

	locked = true;
}

extern "C" {
	syslock *syslock_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new syslock(cfg);
	}

	void syslock_lock(syslock *window) {
		window->lock();
		for (std::vector<Gtk::Window*>::iterator it = window->windows.begin(); it != window->windows.end(); ++it) {
			Gtk::Window* window = *it;
			window->show();
		}
	}
}
