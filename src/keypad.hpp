#include <gtkmm/flowbox.h>
#include <gtkmm/entry.h>

class keypad : public Gtk::FlowBox {
	public:
		keypad(Gtk::Entry &entry);
};
