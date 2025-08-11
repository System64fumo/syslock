#include "window.hpp"
#include "auth.hpp"
#include <filesystem>
#include <pwd.h>
#include <ctime>

Glib::RefPtr<Gdk::Pixbuf> create_rounded_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &src_pixbuf, const int &size, const int &rounding_radius) {
	// Limit to 50% rounding otherwise funky stuff happens
	int rounding = std::clamp(rounding_radius, 0, size / 2);

	int width = src_pixbuf->get_width();
	int height = src_pixbuf->get_height();

	auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, size, size);
	auto cr = Cairo::Context::create(surface);

	cr->begin_new_path();
	cr->arc(rounding, rounding, rounding, M_PI, 3.0 * M_PI / 2.0);
	cr->arc(width - rounding, rounding, rounding, 3.0 * M_PI / 2.0, 0.0);
	cr->arc(width - rounding, height - rounding, rounding, 0.0, M_PI / 2.0);
	cr->arc(rounding, height - rounding, rounding, M_PI / 2.0, M_PI);
	cr->close_path();
	cr->clip();

	Gdk::Cairo::set_source_pixbuf(cr, src_pixbuf, 0, 0);
	cr->paint();

	return Gdk::Pixbuf::create(surface, 0, 0, size, size);
}

syslock_window::syslock_window(std::map<std::string, std::map<std::string, std::string>>* config) : config(config) {
	profile_picture_path = (*config)["profile"]["image-path"];
	profile_scale = std::stoi((*config)["profile"]["scale"]);
	profile_rounding = std::stoi((*config)["profile"]["rounding"]);
	time_format = (*config)["clock"]["time-format"];
	date_format = (*config)["clock"]["date-format"];

	set_default_size(640, 480);
	set_hide_on_close(true);

	set_child(overlay);
	overlay.set_child(box_lock_screen);
	overlay.add_overlay(box_overlay);
	box_overlay.get_style_context()->add_class("box_overlay");
	overlay.add_overlay(box_layout);

	gesture_drag = Gtk::GestureDrag::create();
	gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &syslock_window::on_drag_start));
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &syslock_window::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &syslock_window::on_drag_stop));
	overlay.add_controller(gesture_drag);

	box_lock_screen.get_style_context()->add_class("lock_screen");
	box_lock_screen.property_orientation().set_value(Gtk::Orientation::VERTICAL);

	// TODO: Add a config option to select what appears on the lockscreen
	box_lock_screen.append(label_time);
	label_time.get_style_context()->add_class("time");
	box_lock_screen.append(label_date);
	label_date.get_style_context()->add_class("date");
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &syslock_window::update_time_date), 1000);
	update_time_date();

	box_layout.append(scrolled_window);
	box_layout.get_style_context()->add_class("login_screen");
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_layout.set_opacity(0);

	scrolled_window.set_kinetic_scrolling(false);
	scrolled_window.set_child(box_login_screen);
	scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);
	scrolled_window.set_valign(Gtk::Align::END);

	box_login_screen.property_orientation().set_value(Gtk::Orientation::HORIZONTAL);
	box_login_screen.set_valign(Gtk::Align::CENTER);
	box_login_screen.set_vexpand(true);
	box_login_screen.append(box_widgets);

	box_widgets.set_orientation(Gtk::Orientation::VERTICAL);
	box_widgets.set_valign(Gtk::Align::CENTER);
	box_widgets.set_hexpand(true);


	// And add a way to enable/disable specific features (PFP, Username, Ect)
	const std::string& home_dir = getenv("HOME");
	if (profile_picture_path == "")
		profile_picture_path = home_dir + "/.face";

	if (std::filesystem::exists(profile_picture_path) && profile_scale > 0) {
		box_widgets.append(image_profile);
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

	box_widgets.append(label_username);
	label_username.get_style_context()->add_class("label_username");
	const uid_t& uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	label_username.set_text((Glib::ustring)pw->pw_gecos);


	box_widgets.append(entry_password);
	entry_password.get_style_context()->add_class("entry_password");
	entry_password.set_size_request(250, 30);
	entry_password.set_halign(Gtk::Align::CENTER);
	entry_password.set_visibility(false);
	entry_password.set_input_purpose(Gtk::InputPurpose::PASSWORD);
	entry_password.signal_activate().connect(sigc::mem_fun(*this, &syslock_window::auth_start));
	entry_password.signal_changed().connect(sigc::mem_fun(*this, &syslock_window::on_entry_changed));
	entry_password.grab_focus();
	entry_password.signal_changed().connect([&]() {
		if (entry_password.get_text() == "")
			return;

		scrolled_window.set_valign(Gtk::Align::FILL);
		box_layout.set_opacity(1);
		scrolled_window.set_size_request(-1, get_height());
		connection.disconnect();
		connection = Glib::signal_timeout().connect([&]() {
			reset();
			return false;
		}, 10 * 1000);
	});

	box_widgets.append(label_error);
	label_error.get_style_context()->add_class("label_error");
	label_error.set_text("Incorrect password");
	label_error.hide();

	//
	// Features
	//

	// Keypad
	#ifdef FEATURE_KEYPAD
	if ((*config)["main"]["keypad"] == "true") {
		keypad_main = Gtk::make_managed<keypad>(entry_password, sigc::mem_fun(*this, &syslock_window::auth_start));
		box_login_screen.append(*keypad_main);
		keypad_main->set_hexpand(true);
		entry_password.set_input_purpose(Gtk::InputPurpose::PIN);
	}
	#endif
}

void syslock_window::on_drag_start(const double &x, const double &y) {
	window_height = get_height();

	if (window_height > get_width())
		box_login_screen.set_orientation(Gtk::Orientation::VERTICAL);
	else
		box_login_screen.set_orientation(Gtk::Orientation::HORIZONTAL);

	connection.disconnect();

	// Block gesture inputs from the keypad
	if ((*config)["main"]["keypad"] == "true") {
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

	start_height = scrolled_window.get_height();
	scrolled_window.set_valign(Gtk::Align::END);
	if (start_height > 300) {
		scrolled_window.set_size_request(-1, get_height());
	}
}

void syslock_window::on_drag_update(const double &x, const double &y) {
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

void syslock_window::on_drag_stop(const double &x, const double &y) {
	connection = Glib::signal_timeout().connect([&]() {
		reset();
		return false;
	}, 10 * 1000);

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

void syslock_window::reset() {
	scrolled_window.set_valign(Gtk::Align::END);
	box_layout.set_opacity(0);
	scrolled_window.set_size_request(-1, -1);
	entry_password.set_text("");
}

bool syslock_window::update_time_date() {
	const std::time_t& now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);

	char time_buffer[32];
	std::strftime(time_buffer, sizeof(time_buffer), time_format.c_str(), local_time);
	label_time.set_text(time_buffer);

	char date_buffer[32];
	std::strftime(date_buffer, sizeof(date_buffer), date_format.c_str(), local_time);
	label_date.set_text(date_buffer);
	return true;
}

void syslock_window::auth_start() {
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
		authenticated = authenticate(user, password.c_str());
		if (authenticated) {
			dispatcher_auth.emit();
			label_error.hide();
		}
		Glib::MainContext::get_default()->invoke([&]() {
			// TODO: Display how many times the user can retry the password
			entry_password.set_text("");
			if (!authenticated)
				label_error.show();
			else
				reset();
				
			entry_password.set_sensitive(true);
			return false;
		});
	});
	thread_auth.detach();
}

void syslock_window::on_entry_changed() {
	// Trigger a password check automatically
	const unsigned long password_length = entry_password.get_text().length();
	const unsigned long max_length = std::stoul((*config)["main"]["password-length"]);

	if (password_length >= max_length)
		auth_start();
}