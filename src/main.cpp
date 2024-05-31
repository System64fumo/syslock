#include "main.hpp"
#include "window.hpp"
#include "config.hpp"

#include <iostream>
#include <getopt.h>

// TODO: Move this to a different file
Glib::RefPtr<Gdk::Pixbuf> create_circular_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src_pixbuf) {
	int width = src_pixbuf->get_width();
	int height = src_pixbuf->get_height();

	auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, profile_scale, profile_scale);
	auto cr = Cairo::Context::create(surface);

	cr->set_source_rgba(0, 0, 0, 0);
	cr->set_operator(Cairo::Context::Operator::SOURCE);
	cr->paint();

	cr->arc(profile_scale / 2.0, profile_scale / 2.0, profile_scale / 2.0, 0, 2 * M_PI);
	cr->clip();

	double scale = std::min(static_cast<double>(profile_scale) / width, static_cast<double>(profile_scale) / height);
	cr->scale(scale, scale);

	Gdk::Cairo::set_source_pixbuf(cr, src_pixbuf, (profile_scale / scale - width) / 2.0, (profile_scale / scale - height) / 2.0);
	cr->paint();

	return Gdk::Pixbuf::create(surface, 0, 0, profile_scale, profile_scale);
}

int main(int argc, char* argv[]) {
	// TODO: Add session lock protocol

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "m:ddh")) {
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
