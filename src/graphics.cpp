#include "window.hpp"

#include <gdkmm/general.h>

Glib::RefPtr<Gdk::Pixbuf> syslock::create_circular_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src_pixbuf, int size) {
	int width = src_pixbuf->get_width();
	int height = src_pixbuf->get_height();

	auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, size, size);
	auto cr = Cairo::Context::create(surface);

	cr->set_source_rgba(0, 0, 0, 0);
	cr->set_operator(Cairo::Context::Operator::SOURCE);
	cr->paint();

	cr->arc(size / 2.0, size / 2.0, size / 2.0, 0, 2 * M_PI);
	cr->clip();

	double scale = std::min(static_cast<double>(size) / width, static_cast<double>(size) / height);
	cr->scale(scale, scale);

	Gdk::Cairo::set_source_pixbuf(cr, src_pixbuf, (size / scale - width) / 2.0, (size / scale - height) / 2.0);
	cr->paint();

	return Gdk::Pixbuf::create(surface, 0, 0, size, size);
}
