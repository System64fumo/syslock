# Syslock
Syslock is a simple lockscreen for wayland written in gtkmm4<br>
![preview](https://github.com/System64fumo/syslock/blob/main/preview.gif "preview")

> [!CAUTION]
> This program does not use the ext session lock protocol. *Yet*<br>
> Additional info at the bottom.<br>

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
  -e	Enable experimental session lock
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
Currently the session lock protocol is implemented but disabled by default due to instability and generally being broken.<br>
Though if you are curious and wish to give it a try you can enable it by using the -e launch flag.<br>

Things that will happen when the experimental session lock option is enabled:<br>
* syslock will crash upon succesfully authenticating.<br>
* The primary window gets mirrored to all displays.<br>
* Touch inputs will not work.<br>

Help with this would be greatly appreciated.
