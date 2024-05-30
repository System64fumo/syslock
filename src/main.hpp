#include <gtkmm.h>

Glib::RefPtr<Gtk::Application> app;

class syslock : public Gtk::Window {

	public:
		syslock();

	private:
		Gtk::Box box_layout;
		Gtk::Image image_profile;
		Gtk::Label label_username;
		Gtk::Entry entry_password;

		void on_entry();
};
