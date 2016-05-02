all:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) all V=$(V)

clean:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) clean V=$(V)
