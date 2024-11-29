#pragma once
#include "config.hpp"
#ifdef FEATURE_TAP_TO_WAKE

#include <string>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <thread>
#include <map>

class tap_to_wake {
	public:
		tap_to_wake(const std::map<std::string, std::map<std::string, std::string>>&);
		~tap_to_wake();

		bool running;
		void start_listener();
		void stop_listener();

	private:
		std::map<std::string, std::map<std::string, std::string>> config_main;
		int pipefd[2];
		int fd;
		int rc;
		struct libevdev* dev = nullptr;
		std::jthread thread_tap_listener;
		long start_timestamp = 0;
		int fingers_held = 0;

		// Configs
		std::string device_path = "/dev/input/by-path/SET-ME-UP";
		int timeout = 500;
		std::string tap_cmd = "";
};

#endif
