# Development specific settings

include android-config.mak

#### QEMU advance options
#use coroutine
#ucontext is deprecated and also not avail in Bionic
# gthread is not working right AFAIK
# possible values: gthread, ucontext, sigaltstack, windows
COROUTINE=sigaltstack
#COROUTINE=gthread

#COROUTINE_POOL=--disable-coroutine-pool 
COROUTINE_POOL = --enable-coroutine-pool 

#Enable Internal profiler
#CONFIG_PROFILER = --enable-gprof

ifeq ($(USE_SDL),true)
	#ENABLE SDL
	SDL = --enable-sdl 
	#SDL += --with-sdlabi=1.2
	SDL += --with-sdlabi=2.0
else 
	# DISABLE
	SDL = --disable-sdl
endif

# Set SDL Software rendering (issue with pause/resume)
#SDL_RENDERING = -D__LIMBO_SDL_FORCE_SOFTWARE_RENDERING__

# Or SDL Hardware Acceleration (faster though needs whole screen redraw)
SDL_RENDERING = -D__LIMBO_SDL_FORCE_HARDWARE_RENDERING__ 

#ENABLE SOUND VIA SDL 
# Note: Most guests can play wav files but it's still choppy

ifeq ($(USE_SDL_AUDIO),true)
	AUDIO += --audio-drv-list=sdl
else 
	# DISABLE
	AUDIO += --audio-drv-list=
endif

# NOT USED
#AUDIO += --audio-card-list= --audio-drv-list=
#--enable-mixemu

#USB redir
#USB_REDIR = --enable-usb-redir
USB_REDIR = --disable-usb-redir

#USB Lib
#USB_LIB = --enable-libusb
USB_LIB = --disable-libusb

#BLUETOOTH
BLUETOOTH = --disable-bluez

#NETWORK
#NET = --enable-slirp 

#DISPLAY
DISPLAY = --disable-curses --disable-cocoa --disable-gtk

#VNC 
VNC +=  --enable-vnc
#VNC += --enable-vnc-jpeg --enable-vnc-png
VNC += --disable-vnc-jpeg --disable-vnc-png
VNC += --disable-vnc-sasl


#VNC THREAD (DONT USE FOR 2.3.0+)
#VNC_THREAD += --enable-vnc-thread
#VNC_THREAD += --disable-vnc-thread


# NEEDS ABOVE ENCODING
#INCLUDE_ENC += -I$(LIMBO_JNI_ROOT)/png -I$(LIMBO_JNI_ROOT)/jpeg

#SMART CARD
SMARTCARD =	--disable-smartcard

#FDT is needed for system emulation
#FDT =	--disable-fdt
FDT =	--enable-fdt
FDT_INC = -I$(LIMBO_JNI_ROOT)/qemu/dtc/libfdt

#Disable nptl
#NPTL += --disable-nptl 

#For 2.3.0
#Misc
MISC = --disable-tools --disable-libnfs --disable-tpm
MISC +=  --disable-qom-cast-debug
MISC += --disable-libnfs --disable-libiscsi --disable-docs
MISC += --disable-rdma --disable-brlapi --disable-curl
MISC += --disable-vde --disable-netmap --disable-cap-ng --disable-zlib-test
MISC += --disable-attr --disable-guest-agent --disable-pie
MISC += --disable-rbd --disable-xfsctl  --disable-lzo  --disable-snappy 
MISC += --disable-seccomp --disable-bzip2 --disable-glusterfs 
MISC += --disable-vte --disable-libssh2
MISC += --disable-opengl
MISC += --disable-blobs
MISC += --disable-werror
MISC += --disable-gnutls
MISC += --disable-nettle
MISC += --disable-capstone

#Trying nop doesn't work
#MISC += --enable-trace-backends=nop

#NUMA
NUMA = --disable-numa

#VHOST
VHOST = --disable-vhost-net --disable-vhost-scsi

#VIRT
VIRT = --disable-virtfs

#AIO (Not supported yet)
LINUX_AIO = --disable-linux-aio

#For 2.3.0
#PIXMAN
#PIXMAN = --with-system-pixman

#Enable debugging for QEMU
DEBUG =
#ifeq ($(NDK_DEBUG), 1)
#	DEBUG = --enable-debug
#else
#	DEBUG = --disable-debug-tcg --disable-debug-info  --disable-sparse
#endif

#KVM
ifeq ($(USE_KVM),true)
	#ENABLE KVM
	KVM = --enable-kvm
else 
	# DISABLE
	KVM = --disable-kvm
endif

#XEN
XEN = --disable-xen --disable-xen-pci-passthrough

#SPICE
SPICE = --disable-spice
#SPICE = --enable-spice 

#TCI
#TCI = --enable-tcg-interpreter

WARNING_FLAGS = -Wno-redundant-decls -Wno-unused-variable \
	-Wno-maybe-uninitialized -Wno-unused-function \
	-Wunused-but-set-variable
	
ifeq ($(APP_ABI), armeabi)
    QEMU_HOST_CPU = arm
else ifeq ($(APP_ABI), armeabi-v7a)
    QEMU_HOST_CPU = arm
else ifeq ($(APP_ABI), armeabi-v7a-hard)
    QEMU_HOST_CPU = arm
else ifeq ($(APP_ABI), arm64-v8a)
	QEMU_HOST_CPU = aarch64
else ifeq ($(APP_ABI), x86)
    QEMU_HOST_CPU = i686
else ifeq ($(APP_ABI), x86_64)
    QEMU_HOST_CPU = x86_64
endif

config:
	echo TOOLCHAIN DIR: $(TOOLCHAIN_DIR)
	echo NDK ROOT: $(NDK_ROOT) 
	echo NDK PLATFORM: $(NDK_PLATFORM) 
	echo USR INCLUDE: $(NDK_INCLUDE)
	cd ./qemu	; \
	./configure \
	--target-list=$(QEMU_TARGET_LIST) \
	--cpu=$(QEMU_HOST_CPU) \
	$(PIXMAN) \
	$(FDT) \
	$(VNC) \
	$(VNC_THREAD) \
	$(SMARTCARD) \
	$(NPTL) \
	$(KVM) \
	$(SPICE) \
	$(XEN) \
	$(NUMA) \
	$(TCI) \
	$(LINUX_AIO) \
	$(VIRT) \
	$(VHOST) \
	$(DISPLAY) \
	$(USB_REDIR) \
	$(USB_LIB) \
	$(BLUETOOTH) \
	$(NET) \
	$(SDL) \
	$(AUDIO) \
	$(COROUTINE_POOL) \
	$(MISC) \
	--cross-prefix=$(TOOLCHAIN_PREFIX) \
	--extra-cflags=\
	"\
	$(SYSTEM_INCLUDE) \
	-I$(LIMBO_JNI_ROOT)/limbo/include \
	-I$(LIMBO_JNI_ROOT)/glib/glib \
	-I$(LIMBO_JNI_ROOT)/glib \
	-I$(LIMBO_JNI_ROOT)/glib/gmodule \
	-I$(LIMBO_JNI_ROOT)/glib/io \
	-I$(LIMBO_JNI_ROOT)/glib/android \
	-I$(LIMBO_JNI_ROOT)/pixman \
	-I$(LIMBO_JNI_ROOT)/scsi \
	-I$(LIMBO_JNI_ROOT)/png \
	-I$(LIMBO_JNI_ROOT)/jpeg \
	-I$(LIMBO_JNI_ROOT) \
	-I$(LIMBO_JNI_ROOT)/SDL/include  \
	-I$(LIMBO_JNI_ROOT)/compat  \
	-I$(LIMBO_JNI_ROOT)/spice-protocol  \
	-I$(LIMBO_JNI_ROOT)/spice/server  \
	$(FDT_INC) \
	$(INCLUDE_ENC) \
	$(LIMBO_DISABLE_TSC) \
	$(SDL_RENDERING) \
	$(ENV_EXTRA) \
	$(ARCH_CFLAGS) \
	$(WARNING_FLAGS) \
	" \
	--with-coroutine=$(COROUTINE) \
	$(DEBUG) \
	$(CONFIG_PROFILER)

