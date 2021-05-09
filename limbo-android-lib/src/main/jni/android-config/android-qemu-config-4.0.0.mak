
#### QEMU version-specific options

# QEMU version 4.x is not using a stab lib
USE_QEMUSTAB ?= false

# For QEMU 4.0.0 uses slirp as a static lib so set to true
USE_SLIRP_LIB ?= true

# For QEMU 4.0.0 set the explicit sdlabi to false
USE_SDL_ABI ?= false

# For QEMU 2.11.0 and above (3.x, 4.x) disable these features
MISC += --disable-capstone
MISC += --disable-malloc-trim

#BLUETOOTH
BLUETOOTH = --disable-bluez


SSH2 += --disable-libssh2