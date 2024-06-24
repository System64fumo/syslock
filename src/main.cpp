#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "git_info.hpp"

#include <iostream>

void handle_signal(int signum) {
	if (signum == 10) {
		for (std::vector<Gtk::Window*>::iterator it = windows.begin(); it != windows.end(); ++it) {
			Gtk::Window* window = *it;
			window->show();
		}
	}
}

int main(int argc, char* argv[]) {

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "p:dkl:dm:ddvh")) {
			case 'p':
				profile_scale = std::stoi(optarg);
				continue;

			case 'k':
				keypad_enabled = true;
				continue;

			case 'l':
				pw_length = std::stoi(optarg);
				continue;

			case 'm':
				main_monitor = std::stoi(optarg);
				continue;

			case 'd':
				debug = true;
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

	app = Gtk::Application::create("funky.sys64.syslock");
	app->hold();
	win = new syslock();

	signal(SIGUSR1, handle_signal);

	return app->run();
}
