# Syslock
Syslock is a simple lockscreen for wayland written in gtkmm4<br>

> [!CAUTION]
> This is heavily work in progress,<br>
> The session lock protocol is not yet implemented and other features are missing.<br>
> As long as this warning is still present the program is considered unsafe for daily use.<br>

# Configuration
syslock can be configured in 2 ways<br>
1: By changing config.hpp and recompiling (Suckless style)<br>
2: Using launch arguments<br>
```
arguments:
  -p	Set profile picture size
  -m	Set primary monitor
  -d	Enable debug mode
  -v	Prints version info
```
