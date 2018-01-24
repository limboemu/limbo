#### Makefile for building libqemu-system-*.so
#### DO NOT MODIFY
#### this is called from qemu/Makefile.target

ifeq ($(USE_SDL),true)
sdllibs=../../../obj/local/$(APP_ABI)/libSDL2.so \
	../../../obj/local/$(APP_ABI)/libSDL2_image.so
endif

ifeq ($(USE_SDL_AUDIO),true)
	sdlaudiolibs=../../../obj/local/$(APP_ABI)/libSDL2_mixer.so
endif

glibs=../../../obj/local/$(APP_ABI)/libglib-2.0.so \
	../../../obj/local/$(APP_ABI)/libgthread-2.0.so \
	../../../obj/local/$(APP_ABI)/libgmodule-2.0.so \
	../../../obj/local/$(APP_ABI)/libgobject-2.0.so

# Not needed right now
#iconvlib=../../../obj/local/$(APP_ABI)/libiconv.so

pixmanlib=../../../obj/local/$(APP_ABI)/libpixman.so

compatlib=../../../obj/local/$(APP_ABI)/liblimbocompat.a

fdtlib=../../qemu/dtc/libfdt/libfdt.a

### Need this only if we enable png/jpeg encoding for VNC
#pnglib=../../../obj/local/$(APP_ABI)/libpng.a
#jpeglib=../../../obj/local/$(APP_ABI)/libjpeg.a

### Spice library not used for now
#spicelib=../../../obj/local/$(APP_ABI)/libspice.so
#openssllib=../../../obj/local/$(APP_ABI)/libssl.so
#cryptolib=../../../obj/local/$(APP_ABI)/libcrypto.so

$(QEMU_PROG): $(all-obj-y) ../libqemuutil.a ../libqemustub.a
	$(LNK)     \
	-shared $(SYS_ROOT) \
	-Wl,--no-undefined \
	-Wl,-z,noexecstack  \
	-Wl,-z,relro -Wl,-z,now \
	-Wl,-soname,$(QEMU_PROG) \
	-Wl,--no-warn-mismatch \
	$(sort $(all-obj-y)) \
	$(COMMON_LDADDS) \
	$(compatlib) \
	$(fdtlib) \
	$(glibs) \
	$(iconvlib) \
	$(pixmanlib) \
	$(sdllibs) \
	$(sdlaudiolibs) \
	$(pnglib) \
	$(jpeglib) \
	$(spicelib) \
	$(STL_LIB) \
	$(ARCH_LD_FLAGS) \
	-lc -lgcc -dl -lz -llog \
	$(INCLUDE_FUNCS) \
	-o ../../../obj/local/$(APP_ABI)/lib$(QEMU_PROG).so
