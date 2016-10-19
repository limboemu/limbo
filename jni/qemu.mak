all:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) all -j $(BUILD_THREADS) V=$(V)

clean:
	cd $(LIMBO_JNI_ROOT)/qemu && $(MAKE) clean V=$(V)
