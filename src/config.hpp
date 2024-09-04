#pragma once

// Build time configuration				Description
#define CONFIG_RUNTIME					// Allow the use of runtime arguments
#define CONFIG_FILE						// Allow the use of a config file
#define FEATURE_TAP_TO_WAKE				// Include tap to wake feature

// Default config
struct config_lock {
	bool start_unlocked = false;
	bool keypad_enabled = false;
	int pw_length = -1;
	int main_monitor = 0;
	bool experimental = false;
	bool debug = false;
};
