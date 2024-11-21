#pragma once
#include "window.hpp"

syslock *win;

typedef syslock* (*syslock_create_func)(const std::map<std::string, std::map<std::string, std::string>>&);
typedef void (*syslock_lock_func)(syslock*);

syslock_create_func syslock_create_ptr;
syslock_lock_func syslock_lock_ptr;
