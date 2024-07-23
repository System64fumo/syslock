#pragma once
#include <string>

class tap_to_wake {
	public:
		tap_to_wake();

	private:
		// TODO: Add option to set custom device paths
		std::string device_path = "/dev/input/by-path/platform-a90000.i2c-event";
		long start_timestamp = 0;
		bool verbose = false;
};
