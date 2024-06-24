#include "keypad.hpp"

#include <gtkmm/button.h>
#include <iostream>

keypad::keypad(Gtk::Entry &entry) : Gtk::FlowBox() {
	set_min_children_per_line(3);
	set_max_children_per_line(3);
	set_halign(Gtk::Align::CENTER);
	set_selection_mode(Gtk::SelectionMode::NONE);

	for (int i=1; i<10; i++) {
		Gtk::FlowBoxChild child;
		Gtk::Button *button = Gtk::manage(new Gtk::Button(std::to_string(i)));
		append(child);
		child.set_size_request(100, 100);
		child.set_child(*button);

		button->signal_clicked().connect([&entry, i]() {
			entry.set_text(entry.get_text() + std::to_string(i));
		});
	}
}
