#include "main.hpp"
#include "window.hpp"
#include "config.hpp"

#include <iostream>
#include <getopt.h>

int main(int argc, char* argv[]) {
	// TODO: Add session lock protocol

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "p:dm:ddh")) {
			case 'p':
				profile_scale = std::stoi(optarg);
				continue;

			case 'm':
				main_monitor = std::stoi(optarg);
				continue;

			case 'd':
				debug = true;
				continue;

			case 'h':
			default :
				std::cout << "usage:" << std::endl;
				std::cout << "  syslock [argument...]:\n" << std::endl;
				std::cout << "arguments:" << std::endl;
				std::cout << "  -p	Set profile picture size" << std::endl;
				std::cout << "  -m	Set primary monitor" << std::endl;
				std::cout << "  -d	Enable debug mode" << std::endl;
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

	return app->run();
}
