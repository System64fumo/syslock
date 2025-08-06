#include "main.hpp"
#include "syslock.hpp"
#include "config.hpp"
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
	#ifdef CONFIG_FILE
	std::string config_path;
	std::map<std::string, std::map<std::string, std::string>> config;
	std::map<std::string, std::map<std::string, std::string>> config_usr;

	bool cfg_sys = std::filesystem::exists("/usr/share/sys64/lock/config.conf");
	bool cfg_sys_local = std::filesystem::exists("/usr/local/share/sys64/lock/config.conf");
	bool cfg_usr = std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/lock/config.conf");

	// Load default config
	if (cfg_sys)
		config_path = "/usr/share/sys64/lock/config.conf";
	else if (cfg_sys_local)
		config_path = "/usr/local/share/sys64/lock/config.conf";
	else
		std::fprintf(stderr, "No default config found, Things will get funky!\n");

	config = config_parser(config_path).data;

	// Load user config
	if (cfg_usr)
		config_path = std::string(getenv("HOME")) + "/.config/sys64/lock/config.conf";
	else
		std::fprintf(stderr, "No user config found\n");

	config_usr = config_parser(config_path).data;

	// Merge configs
	for (const auto& [key, nested_map] : config_usr)
		for (const auto& [inner_key, inner_value] : nested_map)
			config[key][inner_key] = inner_value;

	// Sanity check
	if (!(cfg_sys || cfg_sys_local || cfg_usr)) {
		std::fprintf(stderr, "No config available, Something ain't right here.");
		return 1;
	}
	#endif

	// Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "skl:dm:dedvh")) {
			case 's':
				config["main"]["start-unlocked"] = "true";
				continue;

			case 'k':
				config["main"]["keypad"] = "true";
				continue;

			case 'l':
				config["main"]["password-length"] = optarg;
				continue;

			case 'm':
				config["main"]["main-monitor"] = optarg;
				continue;

			case 'e':
				config["main"]["experimental"] = "true";
				continue;

			case 'd':
				config["main"]["debug"] = "true";
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
	win = syslock_create_ptr(config);

	signal(SIGUSR1, handle_signal);

	return app->run();
}
