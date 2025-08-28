#include "syslock.hpp"

#include <gtkmm.h>
#include <gtkmm/cssprovider.h>
#include <gtk4-layer-shell.h>
#include <filesystem>
#include <glibmm/main.h>

static void on_monitor(GtkSessionLockInstance* lock, GdkMonitor* monitor, void* data) {
	//std::printf("On Monitor\n");
	syslock* syslock_instance = (syslock*)data;
	syslock_instance->monitor_changed(monitor);
}

/*static void on_locked(GtkSessionLockInstance *lock, void *data) {
	std::printf("Locked\n");
	syslock* syslock_instance = (syslock*)data;
}

static void on_failed(GtkSessionLockInstance *lock, void *data) {
	std::printf("Failed!!!\n");
	syslock* syslock_instance = (syslock*)data;
}

static void on_unlocked(GtkSessionLockInstance *lock, void *data) {
	std::printf("Unlocked\n");
	syslock* syslock_instance = (syslock*)data;
}*/

syslock::syslock(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
	config_main = cfg;

	if (config_main["main"]["debug"] != "true") {
		lock_instance = gtk_session_lock_instance_new();
		/*g_signal_connect(lock_instance, "locked", G_CALLBACK(on_locked), this);
		g_signal_connect(lock_instance, "failed", G_CALLBACK(on_unlocked), this);
		g_signal_connect(lock_instance, "unlocked", G_CALLBACK(on_unlocked), this);*/
		g_signal_connect(lock_instance, "monitor", G_CALLBACK(on_monitor), this);
	}

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

	#ifdef FEATURE_TAP_TO_WAKE
	listener = new tap_to_wake(cfg);
	#endif

	if (config_main["main"]["start-unlocked"] != "true")
		lock();
}

void syslock::on_monitors_changed(guint position, guint removed, guint added) {
	// Get current monitors
	auto display = Gdk::Display::get_default();
	auto monitor_list = display->get_monitors();
	guint n_monitors = monitor_list->get_n_items();

	for (guint i = 0; i < n_monitors; ++i) {
		GObject* raw = static_cast<GObject*>(g_list_model_get_item(monitor_list->gobj(), i));

	}
}

void syslock::monitor_changed(GdkMonitor *monitor) {
	const char* connector = NULL;
	if (monitor != NULL)
		connector = gdk_monitor_get_connector(monitor);

	const char *main_monitor = config_main["main"]["main-monitor"].c_str();

	if (connector == NULL || !strcmp(connector, main_monitor)) {
		syslock_window* window = new syslock_window(&config_main);
		window->dispatcher_auth.connect(sigc::mem_fun(*this, &syslock::unlock));
		setup_window(window->gobj(), monitor, "syslock");
		window->show();
	}
	else {
		if (config_main["main"]["debug"] == "true")
			return;

		Gtk::Window* window = Gtk::make_managed<Gtk::Window>();
		setup_window(window->gobj(), monitor, "syslock-empty-window");
		window->show();
	}
}

void syslock::setup_window(GtkWindow* window, GdkMonitor* monitor, const char* name) {
	gtk_widget_set_name(GTK_WIDGET(window), name);

	if (config_main["main"]["debug"] == "true")
		return;

	gtk_session_lock_instance_assign_window_to_monitor(lock_instance, window, monitor);
}

void syslock::lock() {
	#ifdef FEATURE_TAP_TO_WAKE
	if (!listener->running)
		listener->start_listener();
	#endif

	if (locked)
		return;
	locked = true;

	Glib::signal_idle().connect_once([&]() {
		if (config_main["main"]["debug"] != "true")
			gtk_session_lock_instance_lock(lock_instance);
		else
			monitor_changed(NULL);

		if (lock_cmd != "") {
			std::thread([this]() {
				system(lock_cmd.c_str());
			}).detach();
		}
	});
}

void syslock::unlock() {
	if (config_main["main"]["debug"] != "true")
		gtk_session_lock_instance_unlock(lock_instance);

	locked = false;
	if (unlock_cmd != "") {
		std::thread([this]() {
			system(unlock_cmd.c_str());
		}).detach();
	}

	#ifdef FEATURE_TAP_TO_WAKE
	if (listener->running)
		listener->stop_listener();
	#endif
}

extern "C" {
	syslock *syslock_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new syslock(cfg);
	}

	void syslock_lock(syslock *window) {
		window->lock();
	}
}
