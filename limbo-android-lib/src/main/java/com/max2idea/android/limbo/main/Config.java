/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
package com.max2idea.android.limbo.main;

import android.androidVNC.COLORMODEL;
import android.androidVNC.VncCanvasActivity;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Environment;
import android.widget.ImageView.ScaleType;

import com.max2idea.android.limbo.utils.LinksManager;

import java.util.Hashtable;
import java.util.LinkedHashMap;

/**
 *
 * @author dev
 */
public class Config {

    // Constants
    public static final int UI_VNC = 0;
    public static final int UI_SDL = 1;
    public static final int UI_SPICE = 2;
    public static final int SDL_MOUSE_LEFT = 1;
    public static final int SDL_MOUSE_MIDDLE = 2;
    public static final int SDL_MOUSE_RIGHT = 3;
    public static final int SETTINGS_RETURN_CODE = 1000;
    public static final int SETTINGS_REQUEST_CODE = 1001;
    public static final int FILEMAN_RETURN_CODE = 1002;

    public static final int VNC_REQUEST_CODE = 1004;
    public static final int VNC_RESULT_CODE = 1005;
    public static final int VNC_RESET_RESULT_CODE = 1006;
    public static final int SDL_REQUEST_CODE = 1007;
    public static final int SDL_RESULT_CODE = 1008;
    public static final int SDL_QUIT_RESULT_CODE = 1009;

    public static final int OPEN_IMAGE_FILE_REQUEST_CODE = 2001;
    public static final int OPEN_IMAGE_FILE_ASF_REQUEST_CODE = 2002;

    public static final int OPEN_IMAGE_DIR_REQUEST_CODE = 2003;
    public static final int OPEN_IMAGE_DIR_ASF_REQUEST_CODE = 2004;

    public static final int OPEN_SHARED_DIR_REQUEST_CODE = 2005;
    public static final int OPEN_SHARED_DIR_ASF_REQUEST_CODE = 2006;

    public static final int OPEN_EXPORT_DIR_REQUEST_CODE = 2007;
    public static final int OPEN_EXPORT_DIR_ASF_REQUEST_CODE = 2008;

    public static final int OPEN_IMPORT_FILE_REQUEST_CODE = 2009;
    public static final int OPEN_IMPORT_FILE_ASF_REQUEST_CODE = 2010;

    public static final int OPEN_LOG_FILE_DIR_REQUEST_CODE = 2011;
    public static final int OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE = 2012;

    public static final int STATUS_NULL = -1;
    public static final int STATUS_CREATED = 1000;
    public static final int STATUS_PAUSED = 1001;
    public static final String ACTION_START = "com.max2idea.android.limbo.action.STARTVM";

    // GUI Options
    public static final boolean enable_SDL = true;
    public static boolean enable_SDL_sound = true;
    public static final boolean enable_SPICE = false;

    public static final int MAX_CPU_NUM = 8;

    //Do not update these directly, see inherited project java files
    public static boolean enable_X86; //Enable if you build QEMU with x86 softmmu
    public static boolean enable_X86_64; //Enable if you build QEMU with x86_64 softmmu
    public static boolean enable_ARM; //Enable if you build QEMU with Arm softmmu
    public static boolean enable_ARM64; //Enable if you build QEMU with Arm64 softmmu
    public static boolean enable_MIPS; //Enable if you build QEMU with Mips softmmu
    public static boolean enable_PPC; //Enable if you build QEMU with PPC softmmu
    public static boolean enable_PPC64; //Enable if you build QEMU with PPC64 softmmu
    public static boolean enable_m68k ;
    public static boolean enable_sparc ;
    public static boolean enable_sparc64;

    //Enable if you build with KVM support, needes android-21 platform
    public static boolean enable_KVM = false;
    
    public static final boolean enable_qemu_fullScreen = true;

    // App config
    public static final String APP_NAME = "Limbo Emulator";
    public static String storagedir = null;

    //Some OSes don't like emulated multi cores for QEMU 2.9.1 you can disable here
    /// thought there is also the Disable TSC feature so you don't have to do it here
    public static boolean enableSMPOnlyOnKVM = false;

    //set to false if you need to debug native library loading
    public static boolean loadNativeLibsEarly = false;

    //XXX: QEMU 3.1.0 needs the libraries to be loaded from the main thread
    public static boolean loadNativeLibsMainThread = true;

    public static String wakeLockTag = "limbo:wakelock";
    public static String wifiLockTag = "limbo:wifilock";

    //this will be populated later
    public static String cacheDir = null;

    //we disable mouse modes for now
    public static boolean disableMouseModes = true;

    //double tap an hold is still buggy so we keep using the old-way trackpad
    public static boolean enableDragOnLongPress = true;

    //we need to define the configuration for the VNC client since we replaced some deprecated
    //  functions
    public static Bitmap.Config bitmapConfig = Bitmap.Config.RGB_565;

    //XXX set scaling to linear it's a tad slower but it's worth it
    public static int SDLHintScale=1;
    public static boolean viewLogInternally = true;


    //XXX some archs don't support floppy or sd card
    public static boolean enableEmulatedFloppy = true;
    public static boolean enableEmulatedSDCard;
    public static String destLogFilename = "limbolog.txt";

    public static final String getCacheDir(){
        return cacheDir.toString();
    }
    public static final String getBasefileDir() {
        return getCacheDir() + "/limbo/";
    }

    public static String getTmpFolder() {
        return getBasefileDir() + "var/tmp"; // Do not modify
    }
    public static String machineFolder = "machines/";
    public static String getMachineDir(){
        return getBasefileDir() + machineFolder;
    }
    public static String logFilePath = null;

    //TODO: future enhancement
    public static boolean enableOpenSL;

    //hd cache is not quite applicable for Android
    public static final boolean enableHDCache = false;


    public static final String defaultDNSServer = "8.8.8.8";
    public static final String defaultUI = "VNC";
    public static String state_filename = "vm.state";

    //QMP
    public static String QMPServer = "127.0.0.1";
    public static int QMPPort = 4444;

    public static int MAX_DISPLAY_REFRESH_RATE = 100; //Hz
    
    // App Config
    public static final String downloadLink = "https://github.com/limboemu/limbo/wiki/Downloads";
    public static final String guidesLink = "https://github.com/limboemu/limbo/wiki/Guides";
    public static final String kvmLink = "https://github.com/limboemu/limbo/wiki/KVM";
    public static final String faqLink = "https://github.com/limboemu/limbo/wiki/FAQ";
    public static final String toolsLink = "https://github.com/limboemu/limbo/wiki/Tools";
    public static final String downloadUpdateLink = "https://raw.githubusercontent.com/limboemu/limbo/master/VERSION";

    // VNC Defaults
    public static String defaultVNCHost = "127.0.0.1";
    public static final String defaultVNCUsername = "limbo";
    public static final String defaultVNCPasswd = "";

    //It seems that for new veersion of qemu it expectes a relative number
    //  so we stop using absolute port numbers
    public static final int defaultVNCPort = 1;

    public static final String defaultVNCColorMode = COLORMODEL.C24bit.nameString();
    public static final ScaleType defaultFullscreenScaleMode = ScaleType.FIT_CENTER;
    public static final ScaleType defaultScaleModeCenter = ScaleType.CENTER;
    public static final String defaultInputMode = VncCanvasActivity.TOUCH_ZOOM_MODE;
    
    //Keyboard Layout
    public static String defaultKeyboardLayout = "en-us";


    //a little nicer ui
    public static boolean collapseSections = true;

    public static boolean enableFlashMemoryImages = true;
    public static boolean enableToggleKeyboard = false;

    //override this at the app level it dependes on the host arch
    public static boolean enableMTTCG = false;




    public static final boolean enableKeyboardLayoutOption = true;
    public static final boolean enableMouseOption = true;
    
    // Features
    protected static final boolean enableSaveVMmonitor = true; // we use the
                                                                // Monitor
                                                                // console to
                                                                // save vms

    // Debug
    public static final boolean debug = false;
    public static boolean debugQmp = false;

    //remove in production
    public static boolean debugStrictMode = false;

    public static LinkedHashMap<String, LinksManager.LinkInfo> osImages = new LinkedHashMap<String, LinksManager.LinkInfo>();

    public static boolean processMouseHistoricalEvents = false;

    public static String getLocalQMPSocketPath() {
        return Config.getCacheDir()+"/qmpsocket";
    }

    public static String getLocalVNCSocketPath() {
        return Config.getCacheDir()+"/vncsocket";
    }

    public static enum MouseMode {
        Trackpad, External
    }
    public static MouseMode mouseMode = MouseMode.Trackpad;

    //specify hd interface, alternative we don't need it right now
    public static boolean enable_hd_if = false;
    public static String hd_if_type = "ide";

    //Change to true in prod if you want to be notified by default for new versions
    public static boolean defaultCheckNewVersion = true;
}
