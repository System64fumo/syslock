# Syslock
Syslock is a simple lockscreen for wayland written in gtkmm4<br>
![preview](https://github.com/System64fumo/syslock/blob/main/preview.gif "preview")

[![Packaging status](https://repology.org/badge/vertical-allrepos/syslock.svg)](https://repology.org/project/syslock/versions)

# Configuration
syslock can be configured in 2 ways<br>
1: By changing config.hpp and recompiling (Suckless style)<br>
2: Using a config file (~/.config/sys64/lock/config.conf)<br>
3: Using launch arguments<br>
```
arguments:
  -s	Start unlocked
  -k	Enable the keypad
  -l	Set password length (For automatic unlocks)
  -m	Set primary monitor
  -d	Enable debug mode
  -v	Prints version info
```

# Signals
You can send a signal to show the window/s again.<br>
``pkill -10 syslock``<br>

# Theming
syslock uses your gtk4 theme by default, However it can be also load custom css,<br>
Just copy the included style.css file to ~/.config/sys64/lock/style.css<br>

# Session lock
This lockscreen utilizes the [session lock protocol](https://wayland.app/protocols/ext-session-lock-v1).<br>
Special thanks to [wmww's gtk4-layer-shell](https://github.com/wmww/gtk4-layer-shell) for making this process painless.<br>