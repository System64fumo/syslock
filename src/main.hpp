#pragma once
#include "window.hpp"

config config_main;
syslock *win;

typedef syslock* (*syslock_create_func)(const config &cfg);
typedef void (*syslock_lock_func)(syslock*);

syslock_create_func syslock_create_ptr;
syslock_lock_func syslock_lock_ptr;
