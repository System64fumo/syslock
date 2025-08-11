#include "tap_to_wake.hpp"

#include <fcntl.h>
#include <sys/select.h>

tap_to_wake::tap_to_wake(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
	config_main = cfg;
	device_path = config_main["tap-to-wake"]["device-path"];
	timeout = std::stoi(config_main["tap-to-wake"]["timeout"]);
	tap_cmd = config_main["events"]["on-tap-cmd"];
	running = false;
}

tap_to_wake::~tap_to_wake() {
	stop_listener();
	libevdev_free(dev);
	close(fd);
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

	thread_tap_listener = std::jthread([this](std::stop_token stoken) {
		while (!stoken.stop_requested()) {
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			FD_SET(pipefd[0], &fds);
			int max_fd = std::max(fd, pipefd[0]);
			int select_result = select(max_fd + 1, &fds, nullptr, nullptr, nullptr);

			if (!FD_ISSET(fd, &fds))
				continue;

			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

			if (rc != 0)
				continue;

			if (ev.type == 3 && ev.code == 57) {
				if (ev.value != -1) {
					fingers_held++;
					start_timestamp = ev.time.tv_sec * 1000000;
					start_timestamp += ev.time.tv_usec;
				}
				else {
					fingers_held--;
					long stop_timestamp = ev.time.tv_sec * 1000000;
					stop_timestamp += ev.time.tv_usec;
					if (stop_timestamp - start_timestamp < timeout * 1000 && fingers_held == 0) {
						if (tap_cmd != "") {
							std::thread([this]() {
								system(tap_cmd.c_str());
							}).detach();
						}
					}
				}
			}
		else if (select_result < 0)
			break;
		}

		close(pipefd[0]);
		close(pipefd[1]);
	});

	thread_tap_listener.detach();
}

void tap_to_wake::stop_listener() {
	thread_tap_listener.request_stop();
	running = false;
	write(pipefd[1], "x", 1);
}
