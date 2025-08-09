#include "syslock.hpp"

#include <gtkmm.h>
#include <gtkmm/cssprovider.h>
#include <gtk4-layer-shell.h>
#include <filesystem>
#include <glibmm/main.h>

static void on_locked(GtkSessionLockInstance *lock, void *data) {
	//syslock* syslock_instance = (syslock*)data;
	printf("Locked\n");
}

static void on_failed(GtkSessionLockInstance *lock, void *data) {
	//syslock* syslock_instance = (syslock*)data;
	printf("Failed!!!\n");
}

static void on_unlocked(GtkSessionLockInstance *lock, void *data) {
	//syslock* syslock_instance = (syslock*)data;
	printf("Unlocked\n");
}

syslock::syslock(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
	config_main = cfg;

	if (config_main["main"]["experimental"] == "true") {
		lock_instance = gtk_session_lock_instance_new();
		g_signal_connect(lock_instance, "locked", G_CALLBACK(on_locked), this);
		g_signal_connect(lock_instance, "failed", G_CALLBACK(on_unlocked), this);
		g_signal_connect(lock_instance, "unlocked", G_CALLBACK(on_unlocked), this);
	}

	if (config_main["main"]["start-unlocked"] != "true")
		lock();

	// Initialize
	lock_cmd = config_main["events"]["on-lock-cmd"];
	unlock_cmd = config_main["events"]["on-unlock-cmd"];

	// Load custom css
	std::string style_path;
	const std::string& home_dir = getenv("HOME");
	if (std::filesystem::exists(home_dir + "/.config/sys64/lock/style.css"))
		style_path = home_dir + "/.config/sys64/lock/style.css";
	else if (std::filesystem::exists("/usr/share/sys64/lock/style.css"))
		style_path = "/usr/share/sys64/lock/style.css";
	else
		style_path = "/usr/local/share/sys64/lock/style.css";

	if (std::filesystem::exists(style_path)) {
		auto css = Gtk::CssProvider::create();
		css->load_from_path(style_path);
		auto display = Gdk::Display::get_default();
		Gtk::StyleContext::add_provider_for_display(
			display,
			css,
			GTK_STYLE_PROVIDER_PRIORITY_USER
		);
	}

	// Monitor connect/disconnect detection
	// auto display = Gdk::Display::get_default();
	// auto monitors = display->get_monitors();
	// monitors->signal_items_changed().connect([this](guint position, guint removed, guint added) {
	// 	Glib::signal_timeout().connect([this, position, removed, added]() {
	// 		on_monitors_changed(position, removed, added);
	// 		return false;
	// 	}, 100);
	// }, true);
}

void syslock::on_monitors_changed(guint position, guint removed, guint added) {
	// Get current monitors
	auto display = Gdk::Display::get_default();
	auto monitor_list = display->get_monitors();
	guint n_monitors = monitor_list->get_n_items();
	const char *main_monitor = config_main["main"]["main-monitor"].c_str();

	for (guint i = 0; i < n_monitors; ++i) {
		GObject* raw = static_cast<GObject*>(g_list_model_get_item(monitor_list->gobj(), i));
		auto monitor = GDK_MONITOR(raw);
		const char* connector = gdk_monitor_get_connector(monitor);

		if (!strcmp(connector, main_monitor)) {
			syslock_window* window = new syslock_window(&config_main);
			window->dispatcher_auth.connect(sigc::mem_fun(*this, &syslock::unlock));
			setup_window(window->gobj(), monitor, "syslock");
			windows.push_back(window);
			window->show();
		}
		else {
			Gtk::Window* window = Gtk::make_managed<Gtk::Window>();
			setup_window(window->gobj(), monitor, "syslock-empty-window");
			windows.push_back(window);
			window->show();
		}
	}
}

void syslock::setup_window(GtkWindow* window, GdkMonitor* monitor, const char* name) {
	gtk_widget_set_name(GTK_WIDGET(window), name);

	if (config_main["main"]["debug"] == "true")
		printf("Setup new window: %s started\n", name);

	if (config_main["main"]["experimental"] == "true") {
		// TODO: Maybe this has to be called again on show?
		gtk_session_lock_instance_assign_window_to_monitor(lock_instance, window, monitor);
	}
	else {
		gtk_layer_init_for_window(window);
		gtk_layer_set_namespace(window, name);
		gtk_layer_set_monitor(window, monitor);
		gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);

		if (!strcmp(name, "syslock"))
			gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
		
		gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, true);
		gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, true);
		gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, true);
		gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	}

	if (config_main["main"]["debug"] == "true")
		printf("Setup new window: %s ended\n", name);
}

void syslock::lock() {
	if (locked)
		return;
	locked = true;

	Glib::signal_idle().connect_once([&]() {
		if (config_main["main"]["experimental"] == "true")
			gtk_session_lock_instance_lock(lock_instance);

		if (lock_cmd != "") {
			std::thread([this]() {
				system(lock_cmd.c_str());
			}).detach();
		}

		for (auto window : windows)
			window->show();

		// TODO: Undo this?
		// No clue
		if (!windows.empty())
			return;

		on_monitors_changed(0, 0, 0);
	});
}

void syslock::unlock() {
	if (config_main["main"]["experimental"] == "true")
		gtk_session_lock_instance_unlock(lock_instance);

	// TODO: Settle on something..
	if (config_main["main"]["debug"] == "true") {
		for (auto window : windows) window->destroy();
	}
	else {
		for (auto window : windows) window->hide();
	}


	locked = false;
	if (unlock_cmd != "") {
		std::thread([this]() {
			system(unlock_cmd.c_str());
		}).detach();
	}

	if (config_main["main"]["debug"] == "true")
		windows.clear();
}

extern "C" {
	syslock *syslock_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new syslock(cfg);
	}

	void syslock_lock(syslock *window) {
		window->lock();
	}
}
