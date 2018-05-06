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
import android.os.Environment;
import android.widget.ImageView.ScaleType;

import java.util.Hashtable;

/**
 *
 * @author dev
 */
public class Config {

	// Constants
	public static final int MIN_AIO_THREADS = 1;
	public static final int MAX_AIO_THREADS = 64;
	public static final int UI_VNC = 0;
	public static final int UI_SDL = 1;
	public static final int UI_SPICE = 2;
	public static final int SDL_MOUSE_LEFT = 1;
	public static final int SDL_MOUSE_MIDDLE = 2;
	public static final int SDL_MOUSE_RIGHT = 3;
	public static final int SETTINGS_RETURN_CODE = 1000;
	public static final int SETTINGS_REQUEST_CODE = 1001;
	public static final int FILEMAN_RETURN_CODE = 1002;
	public static final int FILEMAN_REQUEST_CODE = 1003;
	public static final int VNC_REQUEST_CODE = 1004;
	public static final int VNC_RESULT_CODE = 1005;
	public static final int VNC_RESET_RESULT_CODE = 1006;
	public static final int SDL_REQUEST_CODE = 1007;
	public static final int SDL_RESULT_CODE = 1008;
	public static final int SDL_QUIT_RESULT_CODE = 1009;
	public static final int REQUEST_SDCARD_CODE = 1010;
	public static final int REQUEST_SDCARD_DIR_CODE = 1011;
	public static final int STATUS_NULL = -1;
	public static final int STATUS_CREATED = 1000;
	public static final int STATUS_PAUSED = 1001;
	public static final int VM_CREATED = 1002;
	public static final int VM_STARTED = 1003;
	public static final int VM_STOPPED = 1004;
	public static final int VM_NOTRUNNING = 1005;
	public static final int VM_RESTARTED = 1006;
	public static final int VM_PAUSED = 1007;
	public static final int VM_RESUMED = 1008;
	public static final int VM_SAVED = 1009;
	public static final int IMG_CREATED = 1010;
	public static final int VNC_PASSWORD = 1011;
	public static final int SNAPSHOT_CREATED = 1012;
	public static final int UIUTILS_SHOWALERT_HTML = 1013;
	public static final int VM_NO_QCOW2 = 1014;
	public static final int STATUS_CHANGED = 1015;
	public static final int UIUTILS_SHOWALERT_LICENSE = 1016;
	public static final int VM_NO_KERNEL = 1017;
	public static final int VM_NO_INITRD = 1018;
	public static final int VM_EXPORT = 1019;
	public static final int VM_IMPORT = 1020;
	public static final int UI_RESET = 1021;
	public static final int VM_ARM_NOMACHINE = 1022;
	public static final String ACTION_START = "com.max2idea.android.limbo.action.STARTVM";
	public static final String SEND_VNC_DATA = "com.max2idea.android.limbo.action.SEND_VNC_DATA";

	// GUI Options
	public static final boolean enable_SDL = true;
	public static final boolean enable_SDL_sound = true;
	public static final boolean enable_SPICE = false;


    //Backend libs
    public static final boolean enable_iconv= false; //not needed for now

    //Do not update these directly, see inherited project java files
	public static boolean enable_X86 = false; //Enable if you build QEMU with x86 softmmu
	public static boolean enable_X86_64 = false; //Enable if you build QEMU with x86_64 softmmu
	public static boolean enable_ARM = false; //Enable if you build QEMU with Arm softmmu
	public static boolean enable_MIPS = false; //Enable if you build QEMU with Mips softmmu
	public static boolean enable_PPC = false; //Enable if you build QEMU with PPC softmmu
    public static boolean enable_PPC64 = false; //Enable if you build QEMU with PPC64 softmmu
	public static boolean enable_m68k = false;
	public static boolean enable_sparc = false;
	
	//Enable if you build with KVM support, needes android-21 platform
	public static final boolean enable_KVM = true;
	
	public static final boolean enable_qemu_fullScreen = true;
	public static boolean enable_trackpad_relative_position = true; //We should also support "-usbdevice tablet" that needs absolute positions
	public static boolean enableSDLAlwaysFullscreen = true;

	// App config
	public static final String APP_NAME = "Limbo PC Emulator (QEMU)";
	public static final String storagedir = Environment.getExternalStorageDirectory().toString();
	public static final String getCacheDir(Context context){
		return context.getCacheDir().toString();
	}
    public static final String getBasefileDir(Context context) {
    	return getCacheDir(context) + "/limbo/";
	}
	public static final String DBFile = storagedir + "/limbo/machines.csv";

	public static String getSharedFolder (Context context) {
		return storagedir + "/limbo/shared";
	}
	public static String getTmpFolder(Context context) {
		return getBasefileDir(context) + "var/tmp"; // Do not modify
	}
	public static String machineFolder = "machines/";
	public static String getMachineDir(Context context){
		return getBasefileDir(context) + machineFolder;
	}
	public static String logFilePath = storagedir + "/limbo/limbolog.txt";


	public static boolean enableExternalSD = true; // set to true for Lollipop+ devices
	public static boolean enableOpenSL; //future enhancement
	

	public static final boolean enableHDCache = false;
	public static final String defaultDNSServer = "8.8.8.8";
	public static final String defaultUI = "VNC";
	public static String state_filename = "vm.state";

	//QMP
	public static String QMPServer = "localhost"; 
	public static int QMPPort = 4444;
	//For debugging purposes, make sure you're on a private network
	public static boolean enable_QMP_External = false;

    public static int MAX_DISPLAY_REFRESH_RATE = 100; //Hz
	
	// App Config
	public static final String downloadLink = "http://limboemulator.weebly.com/downloads";
	public static final String downloadUpdateLink = "https://raw.githubusercontent.com/limboemu/limbo/master/VERSION";

	// VNC Defaults
	public static final String defaultVNCHost = "localhost";
	public static final String defaultVNCUsername = "limbo";
	public static final String defaultVNCPasswd = "";
	public static final int defaultVNCPort = 5901;
	public static final String defaultVNCColorMode = COLORMODEL.C24bit.nameString();
	public static final ScaleType defaultFullscreenScaleMode = ScaleType.FIT_CENTER;
	public static final ScaleType defaultScaleModeCenter = ScaleType.CENTER;
	public static final String defaultInputMode = VncCanvasActivity.TOUCH_ZOOM_MODE;
	
	//Keyboard Layout
	public static String defaultKeyboardLayout = "en-us";
	public static boolean collapseSections = false;
	public static boolean enableFlashMemoryImages = false;
	public static boolean enableToggleKeyboard = false;


	public static boolean enableMTTCG = false;
	public static String isosImagesURL = "http://limboemulator.weebly.com/guides";
	
	

	public static final boolean enableKeyboardLayoutOption = true;
    public static final boolean enableMouseOption = true;
	
	// Features
	protected static final boolean enableSaveVMmonitor = true; // we use the
																// Monitor
																// console to
																// save vms

	// Debug
	public static final boolean debug = false;

    // VNC
	public static final String VNC_BYTE = "VNC_BYTE";
	public static final String VNC_DATA_TYPE = "VNC_DATA_TYPE";
	public static final String VNC_BYTES = "VNC_BYTES";
	public static final String VNC_OFFSET = "VNC_OFFSET";
	public static final String VNC_COUNT = "VNC_COUNT";
	public static final int VNC_SEND_BYTE = 1;
	public static final int VNC_SEND_BYTES = 2;
	public static final int VNC_SEND_BYTES_OFFSET = 3;
	
	public static Hashtable<String, String> osImages = new Hashtable<String, String>();
    public static boolean processMouseHistoricalEvents = false;
    public static enum MouseMode {
        Trackpad, External
    }
    public static MouseMode mouseMode = MouseMode.Trackpad;
}
