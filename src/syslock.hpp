#pragma once
#include "window.hpp"
#include "config.hpp"
#include "config_parser.hpp"

#include <gtk4-session-lock.h>

class syslock {
	public:
		syslock(const std::map<std::string, std::map<std::string, std::string>>&);
		GtkSessionLockInstance* lock_instance;

		void lock();
		void unlock();
		std::vector<Gtk::Window*> windows;

	private:
		bool locked;
		std::string lock_cmd = "";
		std::string unlock_cmd = "";
		std::map<std::string, std::map<std::string, std::string>> config_main;

		void on_monitors_changed(guint position, guint removed, guint added);
		void setup_window(GtkWindow *window, GdkMonitor *monitor, const char *name);
};

extern "C" {
	syslock *syslock_create(const std::map<std::string, std::map<std::string, std::string>>&);
	void syslock_lock(syslock* window);
}

