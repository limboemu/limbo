#### QEMU 2.9.1 version-specific options

# Set this to true for 2.9 and prior versions
USE_QEMUSTAB ?= true

# uses slirp as a dyn lib so set to false
USE_SLIRP_LIB ?= false

# set the explicit sdlabi
USE_SDL_ABI ?= true

# we need to specify our own pixman library
# For 2.9.x and prior pixman is included in QEMU so we request this explicitly
PIXMAN = --with-system-pixman

#BLUETOOTH
BLUETOOTH = --disable-bluez


SSH2 += --disable-libssh2