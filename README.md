# Syslock
Syslock is a simple lockscreen for wayland written in gtkmm4<br>

> [!CAUTION]
> While ext session lock has been implemented,<br>
> It is currently disabled as it does not behave as intended.<br>
> I don't know why or how it breaks, Feel free to make a commit if you do.<br>
> As long as this warning is still present the program is considered unsafe for serious use.<br>

# Configuration
syslock can be configured in 2 ways<br>
1: By changing config.hpp and recompiling (Suckless style)<br>
2: Using a config file (~/.config/sys64/lock/config.conf)<br>
3: Using launch arguments<br>
```
arguments:
  -s	Start unlocked
  -p	Set profile picture size
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
