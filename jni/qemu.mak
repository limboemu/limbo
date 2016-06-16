all:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) all V=$(V) -j $(BUILD_THREADS)

clean:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) clean V=$(V)
