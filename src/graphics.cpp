#include "window.hpp"

#include <gdkmm/general.h>

Glib::RefPtr<Gdk::Pixbuf> syslock::create_rounded_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &src_pixbuf, const int &size, const int &rounding_radius) {
	// Limit to 50% rounding otherwise funky stuff happens
	int rounding = std::clamp(rounding_radius, 0, size / 2);

	int width = src_pixbuf->get_width();
	int height = src_pixbuf->get_height();

	auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, size, size);
	auto cr = Cairo::Context::create(surface);

	cr->begin_new_path();
	cr->arc(rounding, rounding, rounding, M_PI, 3.0 * M_PI / 2.0);
	cr->arc(width - rounding, rounding, rounding, 3.0 * M_PI / 2.0, 0.0);
	cr->arc(width - rounding, height - rounding, rounding, 0.0, M_PI / 2.0);
	cr->arc(rounding, height - rounding, rounding, M_PI / 2.0, M_PI);
	cr->close_path();
	cr->clip();

	Gdk::Cairo::set_source_pixbuf(cr, src_pixbuf, 0, 0);
	cr->paint();

	return Gdk::Pixbuf::create(surface, 0, 0, size, size);
}
