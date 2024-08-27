#include "keypad.hpp"

#include <gtkmm/button.h>

keypad::keypad(Gtk::Entry &entry, const std::function<void()> &enter_func) : Gtk::FlowBox() {
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

		button->set_can_focus(false);
		button->signal_clicked().connect([&entry, i]() {
			entry.set_text(entry.get_text() + std::to_string(i));
			entry.set_position(-1);
		});
	}

	Gtk::FlowBoxChild child_enter;
	Gtk::Button *button_enter = Gtk::manage(new Gtk::Button());
	button_enter->set_image_from_icon_name("am-dialog-ok-symbolic");
	append(child_enter);
	child_enter.set_size_request(100, 100);
	child_enter.set_child(*button_enter);

	button_enter->set_can_focus(false);
	button_enter->signal_clicked().connect([&entry, enter_func]() {
		enter_func();
	});

	Gtk::FlowBoxChild child_zero;
	Gtk::Button *button_zero = Gtk::manage(new Gtk::Button("0"));
	append(child_zero);
	child_zero.set_size_request(100, 100);
	child_zero.set_child(*button_zero);

	button_zero->set_can_focus(false);
	button_zero->signal_clicked().connect([&entry]() {
		entry.set_text(entry.get_text() + "0");
		entry.set_position(-1);
	});

	Gtk::FlowBoxChild child_delete;
	Gtk::Button *button_delete = Gtk::manage(new Gtk::Button());
	button_delete->set_image_from_icon_name("edit-clear-symbolic");
	append(child_delete);
	child_delete.set_size_request(100, 100);
	child_delete.set_child(*button_delete);

	button_delete->set_can_focus(false);
	button_delete->signal_clicked().connect([&entry]() {
		if (entry.get_text().length() > 0)
			entry.set_text(entry.get_text().substr(1));
		entry.set_position(-1);
	});
}
