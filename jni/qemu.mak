all:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) all V=1

clean:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) clean V=1
