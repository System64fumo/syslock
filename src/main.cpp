#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <iostream>
#include <dlfcn.h>

void load_libsyslock() {
	void* handle = dlopen("libsyslock.so", RTLD_LAZY);
	if (!handle) {
		std::cerr << "Cannot open library: " << dlerror() << '\n';
		exit(1);
	}

	syslock_create_ptr = (syslock_create_func)dlsym(handle, "syslock_create");
	syslock_lock_ptr = (syslock_lock_func)dlsym(handle, "syslock_lock");

	if (!syslock_create_ptr || !syslock_lock_ptr) {
		std::cerr << "Cannot load symbols: " << dlerror() << '\n';
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

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "p:dkl:dm:ddvh")) {
			case 'p':
				config_main.profile_scale = std::stoi(optarg);
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

			case 'd':
				config_main.debug = true;
				continue;

			case 'v':
				std::cout << "Commit: " << GIT_COMMIT_MESSAGE << std::endl;
				std::cout << "Date: " << GIT_COMMIT_DATE << std::endl;
				return 0;

			case 'h':
			default :
				std::cout << "usage:" << std::endl;
				std::cout << "  syslock [argument...]:\n" << std::endl;
				std::cout << "arguments:" << std::endl;
				std::cout << "  -p	Set profile picture size" << std::endl;
				std::cout << "  -k	Enable the keypad" << std::endl;
				std::cout << "  -l	Set password length" << std::endl;
				std::cout << "  -m	Set primary monitor" << std::endl;
				std::cout << "  -d	Enable debug mode" << std::endl;
				std::cout << "  -v	Prints version info" << std::endl;
				std::cout << "  -h	Show this help message" << std::endl;
				return 0;

			case -1:
				break;
			}

			break;
	}

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.syslock");
	app->hold();

	load_libsyslock();
	win = syslock_create_ptr(config_main);

	signal(SIGUSR1, handle_signal);

	return app->run();
}
