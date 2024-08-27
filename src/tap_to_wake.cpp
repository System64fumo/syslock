#include "tap_to_wake.hpp"
#include "config_parser.hpp"

#include <fcntl.h>
#include <sys/select.h>

tap_to_wake::tap_to_wake() {
	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/lock/config.conf");

	std::string cfg_device_path = config.get_value("tap-to-wake", "device-path");
	if (cfg_device_path != "empty")
		device_path = cfg_device_path;

	std::string cfg_verbose = config.get_value("tap-to-wake", "verbose");
	if (cfg_verbose != "empty")
		verbose = (cfg_verbose == "true");

	std::string cfg_timeout = config.get_value("tap-to-wake", "timeout");
	if (cfg_timeout != "empty")
		timeout = std::stoi(cfg_timeout);

	std::string cfg_tap_cmd = config.get_value("events", "on-tap-cmd");
	if (cfg_tap_cmd != "empty")
		tap_cmd = cfg_tap_cmd;
	#endif
}

tap_to_wake::~tap_to_wake() {
	stop_listener();
}

void tap_to_wake::start_listener() {
	// Set up the device ya dingus
	if (device_path == "/dev/input/by-path/SET-ME-UP")
		return;

	// Prevent starting another thread if already running
	if (running)
		return;

	running = true;

	// Setup
	fd = open(device_path.c_str(), O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return;

	rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		std::fprintf(stderr, "Failed to set up tap to wake\n");
		return;
	}

	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

	thread_tap_listener = std::jthread([this](std::stop_token stoken){
		while (!stoken.stop_requested()) {
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			FD_SET(pipefd[0], &fds);
			int max_fd = std::max(fd, pipefd[0]);

			int select_result = select(max_fd + 1, &fds, nullptr, nullptr, nullptr);
			if (!(select_result > 0))
				break;

			// Stop requested
			if (FD_ISSET(pipefd[0], &fds))
				break;

			if (FD_ISSET(fd, &fds)) {
				struct input_event ev;
				rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

				if (rc != 0)
					break;

				if (ev.type == 3 && ev.code == 57) {
					if (ev.value != -1) {
						start_timestamp = ev.time.tv_sec * 1000000;
						start_timestamp += ev.time.tv_usec;
					}
					else {
						long stop_timestamp = ev.time.tv_sec * 1000000;
						stop_timestamp += ev.time.tv_usec;
						if (!(stop_timestamp - start_timestamp < timeout * 1000))
							break;

						if (tap_cmd == "")
							break;

						pid_t pid = fork();
						if (pid == 0)
							system(tap_cmd.c_str());
					}
				}
			}
		}
		close(pipefd[0]);
		close(pipefd[1]);
	});

	thread_tap_listener.detach();
}

void tap_to_wake::stop_listener() {
	thread_tap_listener.request_stop();
	write(pipefd[1], "x", 1);
	libevdev_free(dev);
	close(fd);
	running = false;
}
