# ControllerEmu 

A WIP Linux only keyboard -> controller emulator 

# Usage 

1. Download the latest binary from the Actions tab(requires a github account) 
2. Run it with `./controller-emu <keyboard event file>`, where the event file is one inside `/dev/input/`, i use `/dev/input/by-path/pci-0000:00:14.0-usb-0:4:1.0-event-kbd`   

Also note that you will have to be in the input group or run this program as root/sudo  

# Default key mappings  
The letter keys (ABXY) are equivalent  

DPAD UP=HOME  
DPAD DOWN=END  
DPAD LEFT=DEL  
DPAD RIGHT=PAGE DOWN  

LEFT THUMBSTICK UP=I  
LEFT THUMBSTICK DOWN=K  
LEFT THUMBSTICK LEFT=J  
LEFT THUMBSTICK RIGHT=L  
LEFT THUMBSTICK CLICK=G  

LEFT TRIGGER=Q  
RIGHT TRIGGER=E  

LEFT BUMPER=T  
RIGHT BUMPER=U  

RIGHT THUMBSTICK UP=NUMPAD 8  
RIGHT THUMBSTICK DOWN=NUMPAD 5  
RIGHT THUMBSTICK LEFT=NUMPAD 4  
RIGHT THUMBSTICK RIGHT=NUMPAD 6  
RIGHT THUMBSTICK CLICK=H  

# Compilation 
I use gcc but other compilers probably work  
`gcc -Wall Main.c -lm -linih -o controller-emu`  
To compile without config support, removing the libinih requirement, use:  
`gcc -Wall Main.c -lm -DNO_CONFIG -o controller-emu`

# Configuration  
To use a config file, either set the CONTROLLEREMU_CONFIG environment variable to the path to it, or pass the --find-config argument and place the config inside $XDG_CONFIG_HOME/controller-emu.cfg or $HOME/.config/controller-emu.cfg where they are searched in that order