#### Makefile for building libqemu-system-*.so
#### DO NOT MODIFY
#### this is called from qemu/Makefile.target

ifeq ($(USE_SDL),true)
sdllibs=../../../obj/local/$(APP_ABI)/libSDL2.so
endif

glibs=-lglib-2.0

# Not needed right now
musllib=-lcompat-musl
pixmanlib=-lpixman-1
compatlib=-lcompat-limbo
fdtlib=../../qemu/dtc/libfdt/libfdt.a

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
QEMU_UTIL_STUB=../libqemustub.a

RELINK_QEMUPROG=$(OBJ_COPY) --redefine-sym open=android_open \
	--redefine-sym fopen=android_fopen \
	--redefine-sym stat=android_stat \
	--redefine-sym mkstemp=android_mkstemp \
	../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).a

RELINK_QEMUSTUB=$(OBJ_COPY) --redefine-sym open=android_open \
    --redefine-sym fopen=android_fopen \
    --redefine-sym stat=android_stat \
    --redefine-sym mkstemp=android_mkstemp \
    ../libqemustub.a

RELINK_QEMUUTIL=$(OBJ_COPY) --redefine-sym open=android_open \
                    --redefine-sym fopen=android_fopen \
                    --redefine-sym stat=android_stat \
                    --redefine-sym mkstemp=android_mkstemp \
                    ../libqemuutil.a

qemu-static: $(all-obj-y) $(COMMON_LDADDS)
	$(AR)  rcs  \
	../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).a \
	$(sort $(all-obj-y)) $(filter-out %.a, $(COMMON_LDADDS))
	$(RELINK_QEMUPROG)
	$(RELINK_QEMUSTUB)
	$(RELINK_QEMUUTIL)

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
	../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).a \
	-Wl,--no-whole-archive \
	$(QEMU_UTIL_LIB) \
	$(QEMU_UTIL_STUB) \
	-L../../../obj/local/$(APP_ABI) \
	$(compatlib) \
	$(fdtlib) \
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
	-lc -lgcc -lm -ldl -lz -llog \
	$(INCLUDE_SYMS) \
	-o ../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).so
