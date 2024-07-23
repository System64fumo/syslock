#include "tap_to_wake.hpp"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <cstring>
#include <sys/select.h>

// TODO: This looks bad.. Maybe clean it up?
tap_to_wake::tap_to_wake() {
	int fd = open(device_path.c_str(), O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		std::cerr << "Failed to open device: " << device_path << std::endl;
		return;
	}

	struct libevdev *dev = nullptr;
	int rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		std::cerr << "Failed to init libevdev (" << strerror(-rc) << ")" << std::endl;
		return;
	}

	if (verbose) {
		std::cout << "Input device name: " << libevdev_get_name(dev) << std::endl;
		std::cout << "Input device ID: bus " << libevdev_get_id_bustype(dev)
				<< " vendor " << libevdev_get_id_vendor(dev)
				<< " product " << libevdev_get_id_product(dev) << std::endl;
	}

	while (true) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		int select_result = select(fd + 1, &fds, nullptr, nullptr, nullptr);
		if (select_result > 0 && FD_ISSET(fd, &fds)) {
			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

			if (rc == 0) {
				// Do these change on other touchscreens?
				if (ev.type == 3 && ev.code == 57) {
					if (verbose) {
						std::cout << "Event: time " << ev.time.tv_sec << "." << ev.time.tv_usec
								<< ", type " << ev.type
								<< ", code " << ev.code
								<< ", value " << ev.value << std::endl;
					}

					// Tap hold
					if (ev.value != -1) {
						start_timestamp = ev.time.tv_sec * 1000000;
						start_timestamp += ev.time.tv_usec;
					}
					// Tap release
					else {
						long stop_timestamp = ev.time.tv_sec * 1000000;
						stop_timestamp += ev.time.tv_usec;
						if (stop_timestamp - start_timestamp < 500000)
							std::cout << "Screen tapped within 500ms" << std::endl;
						// TODO: Run a user defined command here
					}
				}

			} else if (rc != -EAGAIN) {
				std::cerr << "Failed to read next event: " << strerror(-rc) << std::endl;
				break;
			}
		} else if (select_result < 0) {
			std::cerr << "select() failed: " << strerror(errno) << std::endl;
			break;
		}
	}

	libevdev_free(dev);
	close(fd);
}
