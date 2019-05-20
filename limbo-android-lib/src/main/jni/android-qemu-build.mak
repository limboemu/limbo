#### Makefile for building libqemu-system-*.so
#### DO NOT MODIFY
#### this is called from qemu/Makefile.target

include ../../android-config/android-qemu-config.mak

ifeq ($(USE_SDL),true)
sdllibs=../../../obj/local/$(APP_ABI)/libSDL2.so
endif

glibs=-lglib-2.0

musllib=-lcompat-musl
pixmanlib=-lpixman-1
compatlib=-lcompat-limbo
fdtlib=../../qemu/dtc/libfdt/libfdt.a


LIBQEMU_PROG=../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).a
LIBQEMU=../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).so

### Need this only if we enable png/jpeg encoding for VNC
#pnglib=-lpng
#jpeglib=-ljpeg
### Spice library not used for now
#spicelib=-lspice
#openssllib=-lssl
#cryptolib=-lcrypto

# we create a temp static library and relink some functions
#  to our compatibility library to enable things like opening
#  files for Android Storage Framework (Lollipop+ devices)
# TODO: create a rule to RELINK each .o and .a file separately
QEMU_UTIL_LIB=../libqemuutil.a

RELINK_PARAMS=--redefine-sym open=android_open \
              	--redefine-sym fopen=android_fopen \
              	--redefine-sym stat=android_stat \
              	--redefine-sym mkstemp=android_mkstemp

RELINK_PARAMS_2=--redefine-sym __open_2=android_open

RELINK_QEMUPROG=$(OBJ_COPY) $(RELINK_PARAMS) $(LIBQEMU_PROG)
RELINK_QEMUPROG_2=	$(OBJ_COPY) $(RELINK_PARAMS_2) $(LIBQEMU_PROG)

RELINK_QEMUUTIL=$(OBJ_COPY) $(RELINK_PARAMS) $(QEMU_UTIL_LIB)
RELINK_QEMUUTIL_2=$(OBJ_COPY) $(RELINK_PARAMS_2) $(QEMU_UTIL_LIB)


## newer versions of QEMU don't generate a stub lib
ifeq ($(USE_QEMUSTAB),true)
	QEMU_UTIL_STUB=../libqemustub.a
    RELINK_QEMUSTUB=$(OBJ_COPY) $(RELINK_PARAMS) $(QEMU_UTIL_STUB)
    RELINK_QEMUSTUB_2=$(OBJ_COPY) $(RELINK_PARAMS_2) $(QEMU_UTIL_STUB)
endif

## newer versions of QEMU link slirp as a lib
ifeq ($(USE_SLIRP_LIB),true)
	slirplib=../../qemu/slirp/libslirp.a
	RELINK_LIBSLIRP=$(OBJ_COPY) $(RELINK_PARAMS) $(slirplib)
	RELINK_LIBSLIRP_2=$(OBJ_COPY) $(RELINK_PARAMS_2) $(slirplib)
endif

qemu-static: $(all-obj-y) $(COMMON_LDADDS)
	$(AR)  rcs  \
	$(LIBQEMU_PROG) \
	$(sort $(all-obj-y)) $(filter-out %.a, $(COMMON_LDADDS))
	$(RELINK_QEMUPROG)
	$(RELINK_QEMUPROG_2)
	$(RELINK_QEMUSTUB)
	$(RELINK_QEMUSTUB_2)
	$(RELINK_QEMUUTIL)
	$(RELINK_QEMUUTIL_2)
	$(RELINK_LIBSLIRP)
	$(RELINK_LIBSLIRP_2)

# Create our dynamic lib for use with Android
$(QEMU_PROG): $(all-obj-y) $(COMMON_LDADDS) qemu-static
	$(LNK)     \
	-shared $(SYS_ROOT) \
	-Wl,--no-undefined \
	-Wl,-z,noexecstack  \
	-Wl,-z,relro -Wl,-z,now \
	-Wl,-soname,$(QEMU_PROG) \
	-Wl,--no-warn-mismatch \
	-Wl,--whole-archive \
	$(LIBQEMU_PROG) \
	-Wl,--no-whole-archive \
	$(QEMU_UTIL_LIB) \
	$(QEMU_UTIL_STUB) \
	-L../../../obj/local/$(APP_ABI) \
	$(compatlib) \
	$(fdtlib) \
	$(slirplib) \
	$(glibs) \
	$(musllib) \
	$(pixmanlib) \
	$(sdllibs) \
	$(sdlaudiolibs) \
	$(pnglib) \
	$(jpeglib) \
	$(spicelib) \
	-Wl,--no-whole-archive \
	$(STL_LIB) \
	$(ARCH_CLANG_FLAGS) \
	$(ARCH_LD_FLAGS) \
	-lc -lgcc -lm -ldl -lz -llog -latomic \
	$(INCLUDE_SYMS) \
	-o $(LIBQEMU)
