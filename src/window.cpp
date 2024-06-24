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

syslock::syslock() {
	// Initialize
	windows.push_back(this);
	set_default_size(640, 480);
	set_hide_on_close(true);
	show_windows();

	// TODO: Clean this whole mess up
	// And add a way to enable/disable specific features (PFP, Username, Ect)
	set_child(box_layout);
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_layout.set_valign(Gtk::Align::CENTER);

	std::string home_dir = getenv("HOME");
	if (profile_scale > 0) {
		std::string profile_picture = home_dir + "/.face";
	
		if (std::filesystem::exists(profile_picture)) {
			box_layout.append(image_profile);
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

	box_layout.append(label_username);
	label_username.get_style_context()->add_class("label_username");
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	label_username.set_text((Glib::ustring)pw->pw_gecos);

	box_layout.append(entry_password);
	entry_password.get_style_context()->add_class("entry_password");
	entry_password.set_size_request(250, 30);
	entry_password.set_halign(Gtk::Align::CENTER);
	entry_password.set_visibility(false);
	entry_password.signal_activate().connect(sigc::mem_fun(*this, &syslock::on_entry));
	entry_password.grab_focus();

	// TODO: add remaining tries left
	box_layout.append(label_error);
	label_error.get_style_context()->add_class("label_error");
	label_error.set_text("Incorrect password");
	label_error.hide();

	// Load custom css
	std::string css_path = home_dir + "/.config/sys64/lock.css";
	css_loader css(css_path, this);

	// TODO: Figure out why ext session lock causes hyprland to red screen
	//lock_session(*this);

	// Keypad
	// TODO: Enable this at a later date
	//keypad keypad_main = keypad(entry_password);
	//box_layout.append(keypad_main);
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
