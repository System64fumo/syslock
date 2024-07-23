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

		// TODO: Add option to set custom device paths
		std::string device_path = "/dev/input/by-path/platform-a90000.i2c-event";
		long start_timestamp = 0;
		bool verbose = false;
};
