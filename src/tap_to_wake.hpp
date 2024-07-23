#pragma once
#include <string>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <thread>

class tap_to_wake {
	public:
		tap_to_wake();
		~tap_to_wake();

		void start_listener();
		void stop_listener();

	private:
		int pipefd[2];
		int fd;
		int rc;
		struct libevdev *dev = nullptr;
		std::jthread thread_tap_listener;
		long start_timestamp = 0;

		// Configs
		std::string device_path = "/dev/input/by-path/SET-ME-UP";
		bool verbose = false;
		int timeout = 500;
};
