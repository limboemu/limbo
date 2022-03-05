Limbo Emulator (QEMU) for Android

================================================================================

1. What is Limbo?

Limbo is a QEMU-based emulator for Android. It currently supports PC emulation 
for Intel x86 architecture.
For more information, instructions, guides, known issues, and downloads visit:
https://github.com/limboemu/limbo

===============================================================================

2. Requirements:

    Android SDK
    Android NDK: r14b/gcc or r23b/clang
    Android Studio (3.1.1 preferred)
    Android device with Android OS 8.0 (Oreo) and above
    Linux Desktop pc (Ubuntu preferred)
    Make sure you have the following packages installed, if not run:
    sudo apt-get install make autoconf automake git python binutils 
    sudo apt-get install libtool-bin pkg-config flex bison gettext texinfo rsync
    For development you can use your own editors Geany is highly
	recommended for editing the native code

===============================================================================

3. Known Issues:
    https://github.com/limboemu/limbo/issues

===============================================================================

4. Update Configuration

    If you build the supported QEMU version then the only thing you need to do is edit file:
    limbo-android-lib/src/main/jni/android-limbo-config.mak
    In all other cases you have to look at the config files under:
    limbo-android-lib/src/main/jni/android-config/
    
    For more information see the Building section.
    	

5. Get and Compile external libraries

    #Make sure you're under the jni directory
    cd ./limbo-android-lib/src/main/jni
    
    # Now download the source code for the external libraries and unzip them under the jni directory
    #Note: if some of these file links don't download with wget use your browser to download them

    ##### Get QEMU
    # download link:  http://download.qemu-project.org/qemu-x.x.x.tar.xz 
    # Current versions supported by limbo: 5.1.0 and 2.9.1
    # example for version 5.1.0:
    wget http://download.qemu-project.org/qemu-5.1.0.tar.xz -P /tmp/
    tar -xJf /tmp/qemu-5.1.0.tar.xz
    mv qemu-5.1.0 qemu
    # For QEMU version 2.9.1:
    wget http://download.qemu-project.org/qemu-2.9.1.tar.xz -P /tmp/
    tar -xJf /tmp/qemu-2.9.1.tar.xz
    mv qemu-2.9.1 qemu

    ##### GET glib
    wget https://ftp.gnome.org/pub/GNOME/sources/glib/2.56/glib-2.56.1.tar.xz -P /tmp/
    tar -xJf /tmp/glib-2.56.1.tar.xz
    mv glib-2.56.1 glib

    ##### GET libffi
    wget https://sourceware.org/pub/libffi/libffi-3.3.tar.gz -P /tmp/
    tar -xzf /tmp/libffi-3.3.tar.gz
    mv libffi-3.3 libffi

    ##### GET pixman
    wget https://www.cairographics.org/releases/pixman-0.40.0.tar.gz -P /tmp/
    tar -xzf /tmp/pixman-0.40.0.tar.gz
    mv pixman-0.40.0 pixman

    ##### GET SDL2
    wget https://www.libsdl.org/release/SDL2-2.0.8.tar.gz -P /tmp/
    tar -xzf /tmp/SDL2-2.0.8.tar.gz
    mv SDL2-2.0.8 SDL2

    Now you should have this directory structure:
    jni/
        android-config/
        compat/
        glib/
        libffi/
        limbo/
        patches/
        pixman/
        qemu/
        SDL2/

===============================================================================

6. Apply patches

    ### Apply patch for QEMU:
    # example for 5.1.0:
    cd ./limbo-android-lib/src/main/jni/qemu/
    patch -p1 < ../patches/qemu-5.1.0.patch
    # for 2.9.1:
    patch -p1 < ../patches/qemu-2.9.1.patch

    ### Apply glib patch for Limbo:
    cd ./limbo-android-lib/src/main/jni/glib/
    patch -p1 < ../patches/glib-2.56.1.patch

    ### Apply SDL2 patch for Limbo:
    cd ./limbo-android-lib/src/main/jni/SDL2/
    patch -p1 < ../patches/sdl2-2.0.8.patch

    ### Other QEMU versions:
    # If you want to redistribute Limbo build with other QEMU versions, create your own patch like this:
    cd /limbo-android-lib/src/main/jni/qemu/
    diff -ru --no-dereference /tmp/qemu-x.x.x . | grep -v '^Only in' > ../patches/qemu-x.x.x.patch
    Don't forget to create your android-qemu-config-x.x.x.mak file
      and include it in android-qemu-config.mak

===============================================================================

7. Build

    a. To build the native part of the app make sure you're under the jni directory:
        cd limbo-android-lib/src/main/jni
        
    Make sure you update file android-limbo-config.mak with the NDK folder to your installation folder
    keep in mind the last NDK version  that supported gcc was r14b. The alternative is to use clang 
    with ndk version r23. For example: 
    NDK_ROOT = /home/dev/tools/ndk/android-ndk-r14b

    The rest of the configuration is for specifying the right QEMU version: USE_QEMU_VERSION
    the host architecture (android device): BUILD_HOST
    and the emulator guest architecture: BUILD_GUEST
    
    b. From Android Studio import BOTH the Android library limbo-android-lib AND the module for the guest
        architecture you need (x86,arm,ppc,sparc) ie limbo-android-x86.
        
    c. Build the native libraries:
        # Make sure you're still under the jni directory:
        cd limbo-android-lib/src/main/jni

        # Instead of editing android-limbo-config.mak you can also supply the variables on the command line.
        # For example:
            export BUILD_HOST=<EABI>
            export BUILD_GUEST=<GUEST_ARCH>
            export NDK_DEBUG=<ENABLE_DEBUG>

        where:
            EABI is the Android device type (host arch): armeabi-v7a, arm64-v8a, x86, x86_64
            GUEST_ARCH is the Emulator type: x86_64-softmmu,aarch64-softmmu,sparc64-softmmu,ppc64-softmmu
            ENABLE_DEBUG is 1 (optional)

        # To start the build type on your terminal:
            make limbo
            
        Notes:
        If you want to remove ALL previously compiled native objects and libraries:
        make clean

        If you're building apk for multiple host architectures you need to do in between builds:
        make distclean

        If you're building apk for multiple guest architectures you can specify them with commas:
        BUILD_GUEST=x86_64-softmmu,aarch64-softmmu

        Examples:
        1) To build Limbo x86 Emulator for ARM64 phones type:
            export BUILD_HOST=arm64-v8a
            export BUILD_GUEST=x86_64-softmmu
            make limbo

        2) To build Limbo x86 Emulator for ARM phones type:
            export BUILD_HOST=armeabi-v7a
            export BUILD_GUEST=x86_64-softmmu
            make limbo

        3) To build Limbo ARM Emulator for ARM64 phones type:
            export BUILD_HOST=arm64-v8a
            export BUILD_GUEST=aarch64-softmmu
            make limbo

        4) To build Limbo x86 Emulator for Intel x86 32bit phones/tablets/PCs type:
            export BUILD_HOST=x86
            export BUILD_GUEST=x86_64-softmmu
            make limbo

        5) To build Limbo x86 Emulator for Intel x86 64bit PCs type:
            export BUILD_HOST=x86_64
            export BUILD_GUEST=x86_64-softmmu
            make limbo

        6) To build Limbo ARM Emulator for ARM64 phones for debugging type:
            export NDK_DEBUG=1
            export BUILD_HOST=arm64-v8a
            export BUILD_GUEST=aarch64-softmmu
            make limbo

        7) To build multiple Limbo emulators for ARM phones type:
            export BUILD_HOST=arm64-v8a
            export BUILD_GUEST=x86_64-softmmu,aarch64-softmmu
            make limbo

		8) To build limbo for older devices with gcc set the following options:
			export NDK_ROOT=/home/dev/tools/ndk/android-ndk-r14b
			export USE_GCC=true
			export BUILD_HOST=armeabi-v7a
			export BUILD_GUEST=x86_64-softmmu
			export USE_QEMU_VERSION=2.9.1
			export USE_AAUDIO=false

		For more options see android-limbo-config.mak
		
       You should now have the following libraries in these 2 folders:

       limbo-android-lib/src/main/jniLibs/<EABI>/
         libcompat-iconv.so
         libcompat-intl.so
         libcompat-limbo.so
         libcompat-SDL2-ext.so
         libglib-2.0.so
         liblimbo.so
         libpixman-1.so
         libSDL2.so

       limbo-android-<GUEST_ARCH>/src/main/jniLibs/<EABI>/
         libqemu-system-xxx.so

        Note:
            When you build the apk in Android Studio it will contain the libraries from both
            folders so you don't need to copy files manually.

    d. Build the Android apk for the corresponding guest using Android Studio.
        Make sure the *.so libraries are zipped in the final .apk
        
    e. To build from the command line instead of Android Studio:
        export ANDROID_SDK_ROOT=~/Android/Sdk
    	gradle wrapper
    	./gradlew assembleRelease

    f. If you want to build the debugging version:
        Set variables in Config.java:
          debug = true;
        Modify android-config/android-limbo-config.mak and point to a configuration
          with no optimization:
        USE_OPTIMIZATION=false

        Follow the steps to build the native libraries as described above
        export NDK_DEBUG=1

        Important:
           From Android studio click Build> Rebuild Project and Run > Debug

===============================================================================

8. Debugging
        To debug the native code for a particular guest:

        # for x86 guest and ARM64 phone:
        export BUILD_HOST=arm64-v8a
        export BUILD_GUEST=x86_64-softmmu
        make ndk-gdb PKG_NAME=com.limbo.emu.main

        # for other guests use respectively:
        make ndk-gdb PKG_NAME=com.limbo.emu.main.arm
		make ndk-gdb PKG_NAME=com.limbo.emu.main.sparc
		make ndk-gdb PKG_NAME=com.limbo.emu.main.ppc

        Note: you might want to disable signal handling in gdb:
        handle all nostop noprint

        To catch a SIGSEGV:
        handle 11 stop print

        Useful commands:
        To set breakpoints:       b <file.c>:<code_line>
        To step statement:        s
        To go to next statement:  n
        To continue:              c
        To halt(interrupt):       ctrl-c
        To see stack (backtrace): bt

===============================================================================

9. Development Notes

    a. Codes Changes for Android compatibility are in patch files marked with __ANDROID__

    b. Similarly for LIMBO functionality code changes are tagged with __LIMBO__

    c. Important Configuration files (You'll need to update this with the NDK path before compiling):
        limbo-android-lib/src/main/jni/android-config/android-limbo-config.mak

    d. Advanced QEMU config files:
        limbo-android-lib/src/main/jni/android-config/android-qemu-config.mak

    d. Advanced Device Configuration files:
        limbo-android-lib/src/main/jni/android-config/android-config/*.mak

    e. Important Makefiles:
        limbo-android-lib/src/main/jni/Makefile
        limbo-android-lib/src/main/jni/Android.mk
        limbo-android-lib/src/main/jni/Application.mk
        limbo-android-lib/src/main/jni/android-limbo-build.mak
        limbo-android-lib/src/main/jni/android-qemu-build.mak

    f. Frontend UI options configuration see: Config.java
    
    g. When using git apply the following setting to your config file if you want to use 
        return new lines for windows:
    [core]
        eol = true
        autocrlf = input
        fileMode = false
        
===============================================================================
10. Run

    a. Installing a full Qwerty keyboard for Android like Hacker's keyboard 
        from the Google Android store. Make sure you use Transparent theme 
        and Direct Draw found under Theme settings.
    b. Start the Limbo app and choose CPU, Memory (~8-64MB),etc..
    c. Choose a bootable disk image(s) for CDRom, Floppy, and a HDD image
    d. Start the virtual machine.
    e. For more instructions and guides visit: 
        https://github.com/limboemu/limbo
    f. Have fun!

===============================================================================
10. Changelog
See limbo-android-lib/src/main/assets/CHANGELOG for release notes

===============================================================================
11. License

Limbo PC Emulator is released under GPL v2 License.
All icons under /res are from Gnome Project (GPL v2 License)
See file COPYING under root directory
and LICENSE under limbo-android-lib/src/main/assets

Other source included are released under their own license please view Licenses under each subdirectory
