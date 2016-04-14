.PHONY: createdirs ndk qemu limbo

include android-config.mak
	
ndkbuild=cd $(NDK_PROJECT_PATH) && ndk-build V=1 NDK_DEBUG=$(NDK_DEBUG) LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) -j $(BUILD_THREADS)
ndkclean=cd $(NDK_PROJECT_PATH) && ndk-build clean V=1 NDK_DEBUG=$(NDK_DEBUG) LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT)
ndkcleanobj=rm -rf $(NDK_PROJECT_PATH)/obj/

CREATE_OBJDIR=mkdir -p $(NDK_PROJECT_PATH)/obj/local/$(APP_ABI)/
CREATE_LIBSDIR=mkdir -p $(NDK_PROJECT_PATH)/libs/$(APP_ABI_DIR)/
DEL_LIBSDIR=rm -rf $(NDK_PROJECT_PATH)/libs/armeabi \
	../libs/armeabi-v7a \
	../libs/x86 ../libs/mips \   
INSTALL_LIBS=cp -f $(NDK_PROJECT_PATH)/obj/local/$(APP_ABI)/*.so \
	../libs/$(APP_ABI_DIR)/
#RENAME_TUNCTL=mv -f $(NDK_PROJECT_PATH)/libs/$(APP_ABI)/tunctl \
	$(NDK_PROJECT_PATH)/libs/$(APP_ABI)/libtunctl.so
#RENAME_PARPROUTED=mv -f $(NDK_PROJECT_PATH)/libs/$(APP_ABI)/parprouted \
	$(NDK_PROJECT_PATH)/libs/$(APP_ABI)/libparprouted.so

ifneq ($(NDK_DEBUG),1)
	STRIP_LIBS=$(STRIP) --strip-unneeded  $(subst /cygdrive/c,c:,$(NDK_PROJECT_PATH))/libs/$(APP_ABI_DIR)/*.so
endif
	
#debug is default
# use "make release" for production version
all: config-debug debug
	
	
createdirs:
	$(CREATE_OBJDIR)
	$(DEL_LIBSDIR)
	$(CREATE_LIBSDIR)

ndk:
	$(ndkbuild)

ndkclean:
	$(ndkclean)
	$(ndkcleanobj)
	
qemu: 
	$(MAKE) -f $(LIMBO_JNI_ROOT)/qemu.mak all V=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) -j $(BUILD_THREADS)
	
#limbo: ndk config qemu
#	$(MAKE) -f $(LIMBO_JNI_ROOT)/limbo.mak all V=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT)

config-debug:
	$(MAKE) createdirs LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(MAKE) -f $(LIMBO_JNI_ROOT)/android-qemu-config.mak NDK_DEBUG=1 V=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT)
	
config:
	$(MAKE) createdirs LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(MAKE) -f $(LIMBO_JNI_ROOT)/android-qemu-config.mak V=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT)

debug:
	$(MAKE) ndk NDK_DEBUG=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(MAKE) qemu NDK_DEBUG=1 LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(INSTALL_LIBS)
	$(RENAME_TUNCTL)
	$(RENAME_PARPROUTED)
	
release: config
	$(MAKE) ndk LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(MAKE) qemu LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT) V=1
	$(INSTALL_LIBS)
	$(RENAME_TUNCTL)
	$(RENAME_PARPROUTED)
	$(STRIP_LIBS)

distclean: 
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) distclean LIMBO_JNI_ROOT=$(LIMBO_JNI_ROOT)

clean: distclean ndkclean
	-find . -name "*.d" -exec rm -rf {} \;
	-find . -name "*.a" -exec rm -rf {} \;
	-find . -name "*.o" -exec rm -rf {} \;
	-find . -name "*.so" -exec rm -rf {} \;
	