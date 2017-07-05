# Development specific settings

include android-config.mak

####### x86 and ARM devices support
#ARM is currently very slow
#Possible Values=
#QEMU_TARGET_LIST = aarch64-softmmu

#QEMU_TARGET_LIST = ppc64-softmmu
#QEMU_TARGET_LIST = ppcemb-softmmu
#QEMU_TARGET_LIST = s390x-softmmu
#QEMU_TARGET_LIST = sh4-softmmu
#QEMU_TARGET_LIST = sh4eb-softmmu
#QEMU_TARGET_LIST = sparc64-softmmu
#QEMU_TARGET_LIST = xtensa-softmmu
#QEMU_TARGET_LIST = xtensaeb-softmmu
#QEMU_TARGET_LIST = microblaze-softmmu
#QEMU_TARGET_LIST = microblazeel-softmmu
#QEMU_TARGET_LIST = mips64-softmmu
#QEMU_TARGET_LIST = mips64el-softmmu
#QEMU_TARGET_LIST = mipsel-softmmu
#QEMU_TARGET_LIST = lm32-softmmu
#QEMU_TARGET_LIST = alpha-softmmu
#QEMU_TARGET_LIST = cris-softmmu
#QEMU_TARGET_LIST = i386-softmmu
#QEMU_TARGET_LIST = m68k-softmmu
#QEMU_TARGET_LIST = mips-softmmu

#QEMU_TARGET_LIST = ppc-softmmu
#QEMU_TARGET_LIST = x86_64-softmmu
QEMU_TARGET_LIST = arm-softmmu
#QEMU_TARGET_LIST = sparc-softmmu

#QEMU_TARGET_LIST = ppc-softmmu,x86_64-softmmu,arm-softmmu,sparc-softmmu

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
# Currently hanging QEMU anyway too high latency so we disable for now

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
#INCLUDE_ENC += -I$(LIMBO_JNI_ROOT_INC)/png -I$(LIMBO_JNI_ROOT_INC)/jpeg

#SMART CARD
SMARTCARD =	--disable-smartcard

#FDT
#FDT =	--disable-fdt
FDT =	--enable-fdt
FDT_INC = -I$(LIMBO_JNI_ROOT_INC)/qemu/dtc/libfdt

#Disable nptl
#NPTL += --disable-nptl 

#For 2.3.0
#Misc
MISC = --disable-tools --disable-libusb --disable-libnfs --disable-tpm 
MISC +=  --disable-qom-cast-debug
MISC += --disable-libnfs --disable-libiscsi --disable-docs
MISC += --disable-rdma --disable-brlapi --disable-curl --disable-uuid
MISC += --disable-vde --disable-netmap --disable-cap-ng --disable-zlib-test
MISC += --disable-attr --disable-guest-agent --disable-pie
MISC += --disable-rbd --disable-xfsctl  --disable-lzo  --disable-snappy 
MISC += --disable-seccomp --disable-bzip2 --disable-glusterfs 
MISC += --disable-vte --disable-libssh2 --disable-vhdx
MISC += --disable-opengl
MISC += --disable-blobs
MISC += --disable-werror
MISC += --disable-gnutls
MISC += --disable-nettle

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
PIXMAN = --with-system-pixman

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
	-I$(LIMBO_JNI_ROOT_INC)/limbo/include \
	-I$(LIMBO_JNI_ROOT_INC)/glib/glib \
	-I$(LIMBO_JNI_ROOT_INC)/glib \
	-I$(LIMBO_JNI_ROOT_INC)/glib/gmodule \
	-I$(LIMBO_JNI_ROOT_INC)/glib/io \
	-I$(LIMBO_JNI_ROOT_INC)/glib/android \
	-I$(LIMBO_JNI_ROOT_INC)/pixman \
	-I$(LIMBO_JNI_ROOT_INC)/scsi \
	-I$(LIMBO_JNI_ROOT_INC)/png \
	-I$(LIMBO_JNI_ROOT_INC)/jpeg \
	-I$(LIMBO_JNI_ROOT_INC) \
	-I$(LIMBO_JNI_ROOT_INC)/SDL/include  \
	-I$(LIMBO_JNI_ROOT_INC)/compat  \
	-I$(LIMBO_JNI_ROOT_INC)/spice-protocol  \
	-I$(LIMBO_JNI_ROOT_INC)/spice/server  \
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

