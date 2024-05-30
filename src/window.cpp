#include "main.hpp"
#include "window.hpp"
#include "pam.hpp"

#include <gtk4-layer-shell.h>
#include <iostream>
#include <filesystem>
#include <pwd.h>

syslock::syslock() {
	// TODO: Add a way to disable this for debugging/customization
	if (true) {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
		gtk_layer_set_namespace(gobj(), "syslock");
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);

		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	}

	// Initialize
	set_default_size(640, 480);
	set_hide_on_close(true);
	show();
	// TODO: Add multimonitor support

	// TODO: Clean this whole mess up
	// And add a way to enable/disable specific features (PFP, Username, Ect)
	set_child(box_layout);
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	box_layout.set_valign(Gtk::Align::CENTER);


	std::string home_dir = getenv("HOME");
	std::string profile_picture = home_dir + "/.face";

	if (std::filesystem::exists(profile_picture)) {
		box_layout.append(image_profile);
		auto pixbuf = Gdk::Pixbuf::create_from_file(profile_picture);
		pixbuf = pixbuf->scale_simple(profile_scale, profile_scale, Gdk::InterpType::BILINEAR);
		// TODO: Add a way to enable/disable rounding the profile picture
		pixbuf = create_circular_pixbuf(pixbuf);
		image_profile.set_size_request(profile_scale, profile_scale);
		image_profile.set(pixbuf);
		image_profile.set_halign(Gtk::Align::CENTER);
	}

	box_layout.append(label_username);
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	label_username.set_text((Glib::ustring)pw->pw_gecos);

	box_layout.append(entry_password);
	entry_password.set_size_request(250, 30);
	entry_password.set_halign(Gtk::Align::CENTER);
	entry_password.set_visibility(false);
	entry_password.signal_activate().connect(sigc::mem_fun(*this, &syslock::on_entry));
	entry_password.grab_focus();

	// Load custom css
	std::string css_path = home_dir + "/.config/sys64/lock.css";

	if (!std::filesystem::exists(css_path)) return;

	auto css = Gtk::CssProvider::create();
	css->load_from_path(css_path);
	auto style_context = get_style_context();
	style_context->add_provider_for_display(property_display(), css, GTK_STYLE_PROVIDER_PRIORITY_USER);
}

// TODO: Make this non blocking
void syslock::on_entry() {
	char *user = getenv("USER");
	std::string password = entry_password.get_buffer()->get_text().raw();
	bool auth = authenticate(user, password.c_str());

	if (auth) {
		std::cout << "Authentication successful" << std::endl;
		app->quit();
	}
	else {
		// TODO: Display how many times the user can retry the password
		std::cerr << "Authentication failed" << std::endl;
		entry_password.set_text("");
	}
}
