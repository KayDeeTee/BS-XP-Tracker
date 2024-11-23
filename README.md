# BS-XP-Tracker
Linux XP and XP/h Tracker for brighter shores
Uses memory reading to match a pattern to find the exp struct, this will probably break immediately upon updates.
It may also not actually work on your device, you will have to find a pattern yourself.

## Libraries
dear imgui https://github.com/ocornut/imgui
implot https://github.com/epezent/implot

## Building
Should just require running compile.sh, it'll cmake and run the program for you

## Other OSes
It'd work on Windows if someone wants to write memory reading stuff for it.

## Todo
Save data after closing so you can track over multiple sessions.