#pragma once

// Build time configuration				Description
#define RUNTIME_CONFIG					// Allow the use of runtime arguments
#define CONFIG_FILE						// Allow the use of a config file

// Default config
struct config_lock {
	bool start_unlocked = false;
	int profile_scale = 128;
	bool keypad_enabled = false;
	int pw_length = -1;
	int main_monitor = 0;
	bool debug = false;
};
