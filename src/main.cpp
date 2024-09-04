#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <filesystem>
#include <dlfcn.h>

void load_libsyslock() {
	void* handle = dlopen("libsyslock.so", RTLD_LAZY);
	if (!handle) {
		std::fprintf(stderr, "Cannot open library: %s\n", dlerror());
		exit(1);
	}

	syslock_create_ptr = (syslock_create_func)dlsym(handle, "syslock_create");
	syslock_lock_ptr = (syslock_lock_func)dlsym(handle, "syslock_lock");

	if (!syslock_create_ptr || !syslock_lock_ptr) {
		std::fprintf(stderr, "Cannot load symbols: %s\n", dlerror());
		dlclose(handle);
		exit(1);
	}
}

void handle_signal(int signum) {
	if (signum == 10) {
		syslock_lock_ptr(win);
	}
}

int main(int argc, char* argv[]) {
	// Load the config
	#ifdef CONFIG_FILE
	std::string config_path;
	if (std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/lock/config.conf"))
		config_path = std::string(getenv("HOME")) + "/.config/sys64/lock/config.conf";
	else if (std::filesystem::exists("/usr/share/sys64/lock/config.conf"))
		config_path = "/usr/share/sys64/lock/config.conf";
	else
		config_path = "/usr/local/share/sys64/lock/config.conf";

	config_parser config(config_path);

	if (config.available) {
		std::string cfg_start_unlocked = config.get_value("main", "start-unlocked");
		if (cfg_start_unlocked != "empty")
			config_main.start_unlocked = (cfg_start_unlocked == "true");

		std::string cfg_keypad = config.get_value("main", "keypad");
		if (cfg_keypad != "empty")
			config_main.keypad_enabled = (cfg_keypad == "true");

		std::string cfg_pw_length = config.get_value("main", "password-length");
		if (cfg_pw_length != "empty")
			config_main.pw_length = std::stoi(cfg_pw_length);

		std::string cfg_main_monitor = config.get_value("main", "main-monitor");
		if (cfg_main_monitor != "empty")
			config_main.main_monitor = std::stoi(cfg_main_monitor);
	}

	// Debug doesn't need a config entry
	#endif

	// Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "skl:dm:dedvh")) {
			case 's':
				config_main.start_unlocked=true;
				continue;

			case 'k':
				config_main.keypad_enabled = true;
				continue;

			case 'l':
				config_main.pw_length = std::stoi(optarg);
				continue;

			case 'm':
				config_main.main_monitor = std::stoi(optarg);
				continue;

			case 'e':
				config_main.experimental = true;
				continue;

			case 'd':
				config_main.debug = true;
				continue;

			case 'v':
				std::printf("Commit: %s\n", GIT_COMMIT_MESSAGE);
				std::printf("Date: %s\n", GIT_COMMIT_DATE);
				return 0;

			case 'h':
			default :
				std::printf("usage:\n");
				std::printf("  syslock [argument...]:\n\n");
				std::printf("arguments:\n");
				std::printf("  -s	Start unlocked\n");
				std::printf("  -k	Enable the keypad\n");
				std::printf("  -l	Set password length\n");
				std::printf("  -m	Set primary monitor\n");
				std::printf("  -e	Enable experimental session lock\n");
				std::printf("  -d	Enable debug mode\n");
				std::printf("  -v	Prints version info\n");
				std::printf("  -h	Show this help message\n");
				return 0;

			case -1:
				break;
			}

			break;
	}
	#endif

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.syslock");
	app->hold();

	load_libsyslock();
	win = syslock_create_ptr(config_main);

	signal(SIGUSR1, handle_signal);

	return app->run();
}
