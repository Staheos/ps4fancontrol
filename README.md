# ps4fancontrol

Just a simple program to change the fan speed from linux (PS4).
The fork was created to use it easier in terminal.

## Fork

Forked from https://github.com/Ps3itaTeam/ps4fancontrol  
Modified by [Staheos](https://github.com/Staheos)

## Build
~~Download, build, install the XForms Toolkit: http://xforms-toolkit.org/~~

Type 
```
make
```
That's all

## Usage
You need run ps4fancontrol with root privileges
```
sudo ./ps4fancontrol 69
```
~~Select your favorite threshold temperature, save and exit.~~
~~The selected temperature will be saved in a config file and loaded when ps4fancontrol starts.~~
Run program in terminal and type your favorite threshold temperature as an argument.
If you want load automatically ps4fancontrol at boot of your distro just put
```
ps4fancontrol <default temp>
```
in a unit configuration file: https://wiki.archlinux.org/index.php/Systemd#Writing_unit_files or use crontab or similar..

Enjoy

## Kudos
Thanks to Zer0xFF for finding the right icc cmd to change the threshold temperature
and to shuffle2 for the patch to expose the icc to usermode.
