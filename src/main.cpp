#include "main.hpp"
#include "window.hpp"

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
	// TODO: Add config system
	// TODO: Add session lock protocol
	app = Gtk::Application::create("funky.sys64.syslock");
	app->hold();
	win = new syslock();

	return app->run();
}
