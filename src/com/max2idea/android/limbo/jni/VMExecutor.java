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
package com.max2idea.android.limbo.jni;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.HashMap;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboService;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;

import android.app.Notification;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.Toast;

public class VMExecutor {

	private static Notification mNotification;
	public int paused;
	public String cd_iso_path;
	// HDD
	private String hda_img_path;
	private String hdb_img_path;
	private String hdc_img_path;
	private String hdd_img_path;

	public String fda_img_path;
	public String fdb_img_path;
	public String sd_img_path;
	private String cpu;

	private String kernel;
	private String initrd;

	private final String TAG = "VMExecutor";

	public int aiomaxthreads = 1;
	// Default Settings
	private int memory = 128;
	private int cpuNum = 128;
	private String bootdevice = null;
	// net
	private String net_cfg = "None";
	private int nic_num = 1;
	private String vga_type = "std";
	private String hd_cache = "default";
	public String sound_card;
	private String nic_driver = null;
	private String liblimbo = "limbo";
	private String libqemu = null;
	private int restart = 0;
	private String snapshot_name = "limbo";
	public String save_state_name = null;
	public int fd_save_state;
	private int disableacpi = 0;
	private int disablehpet = 0;
	private int disablefdbootchk = 0;
	private int usbmouse = 0; // for -usb -usbdevice tablet - fixes mouse
								// positioning
	private int enableqmp;
	public int enablevnc;
	public String vnc_passwd = null;
	public int vnc_allow_external = 0;
	private String qemu_dev;
	private String qemu_dev_value;
	public String base_dir = Config.basefiledir;
	public String dns_addr;
	private int width;
	private int height;
	private String arch = "x86";
	public String append = "";
	public boolean busy = false;
	private String machine_type;
	public boolean libLoaded = false;
	private static Context context;
	public String name;
	private String save_dir;
	public int enablespice = 0;

	/**
	 */
	public VMExecutor(Machine machine, Context context) {

		name = machine.machinename;
		save_dir = Config.machinedir + name;
		save_state_name = save_dir + "/" + Config.state_filename;
		this.context = context;
		this.memory = machine.memory;
		this.cpuNum = machine.cpuNum;
		this.vga_type = machine.vga_type;
		this.hd_cache = machine.hd_cache;
		this.snapshot_name = machine.snapshot_name;
		this.disableacpi = machine.disableacpi;
		this.disablehpet = machine.disablehpet;
		this.disablefdbootchk = machine.disablefdbootchk;
		this.enableqmp = machine.enableqmp;
		this.enablevnc = machine.enablevnc;
		this.enablevnc = machine.enablespice;
		if (!machine.soundcard.equals("none"))
			this.sound_card = machine.soundcard;
		this.kernel = machine.kernel;
		this.initrd = machine.initrd;

		this.cpu = machine.cpu;

		if (machine.arch.endsWith("ARM")) {
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-arm.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "arm";
			this.machine_type = machine.machine_type.split(" ")[0];
			this.disablehpet = 0;
			this.disableacpi = 0;
			this.disablefdbootchk = 0;
		} else if (machine.arch.endsWith("x64")) {
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-x86_64.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "x86_64";
			this.machine_type = "pc";
		} else if (machine.arch.endsWith("x86")) {
			this.cpu = machine.cpu;
			// x86_64 can run 32bit as well as no need for the extra lib
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-x86_64.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "x86";
			this.machine_type = "pc";
		}

		if (machine.cd_iso_path == null || machine.cd_iso_path.equals("None")) {
			this.cd_iso_path = null;
		} else {
			this.cd_iso_path = machine.cd_iso_path;
		}
		if (machine.hda_img_path == null || machine.hda_img_path.equals("None")) {
			this.hda_img_path = null;
		} else {

			this.hda_img_path = machine.hda_img_path;
		}

		if (machine.hdb_img_path == null || machine.hdb_img_path.equals("None")) {
			this.hdb_img_path = null;
		} else {

			this.hdb_img_path = machine.hdb_img_path;
		}

		// Check if CD is set
		if (machine.hdc_img_path != null || machine.hdc_img_path == null || machine.hdc_img_path.equals("None")) {
			this.hdc_img_path = null;
		} else {

			this.hdc_img_path = machine.hdc_img_path;
		}

		if (machine.hdd_img_path == null || machine.hdd_img_path.equals("None")) {
			this.hdd_img_path = null;
		} else {
			this.hdd_img_path = machine.hdd_img_path;
		}

		if (machine.fda_img_path == null || machine.fda_img_path.equals("None")) {
			this.fda_img_path = null;
		} else {

			this.fda_img_path = machine.fda_img_path;
		}

		if (machine.fdb_img_path == null || machine.fdb_img_path.equals("None")) {
			this.fdb_img_path = null;
		} else {

			this.fdb_img_path = machine.fdb_img_path;
		}

		if (machine.sd_img_path == null || machine.sd_img_path.equals("None")) {
			this.sd_img_path = null;
		} else {

			this.sd_img_path = machine.sd_img_path;
		}

		if (this.arch.equals("arm")) {
			this.bootdevice = null;
		} else if (machine.bootdevice.equals("Default")) {
			this.bootdevice = null;
		} else if (machine.bootdevice.equals("CD Rom")) {
			this.bootdevice = "d";
		} else if (machine.bootdevice.equals("Floppy")) {
			this.bootdevice = "a";
		} else if (machine.bootdevice.equals("Hard Disk")) {
			this.bootdevice = "c";
		}

		if (machine.net_cfg == null || machine.net_cfg.equals("None")) {
			this.net_cfg = "none";
			this.nic_driver = null;
		} else if (machine.net_cfg.equals("User")) {
			this.net_cfg = "user";
			this.nic_driver = machine.nic_driver;
		} else if (machine.net_cfg.equals("TAP")) {
			this.net_cfg = "tap";
			this.nic_driver = machine.nic_driver;
		}

		if (machine.soundcard != null && machine.soundcard.equals("None")) {
			this.sound_card = null;
		}

		this.prepPaths();
	}

	public void loadNativeLibs() {

		// Load the C library
		// FIXME: use standard load without the hardcoded path
		// loadNativeLib(this., FileUtils.getDataDir() + "/lib");

		if (!Config.debug) {
			if (arch.equals("x86") || arch.equals("x86_64")) {
				System.loadLibrary("qemu-system-x86_64");
			} else if (arch.equals("arm")) {
				System.loadLibrary("qemu-system-arm");
			}
		}
		libLoaded = true;

	}

	// Load the shared lib
	private void loadNativeLib(String lib, String destDir) {
		if (true) {
			String libLocation = destDir + "/" + lib;
			try {
				System.load(libLocation);
			} catch (Exception ex) {
				Log.e("JNIExample", "failed to load native library: " + ex);
			}
		}

	}

	public void print() {
		Log.v(TAG, "CPU: " + this.cpu);
		Log.v(TAG, "MEM: " + this.memory);
		Log.v(TAG, "HDA: " + this.hda_img_path);
		Log.v(TAG, "HDB: " + this.hdb_img_path);
		Log.v(TAG, "HDC: " + this.hdc_img_path);
		Log.v(TAG, "HDD: " + this.hdd_img_path);
		Log.v(TAG, "CD: " + this.cd_iso_path);
		Log.v(TAG, "FDA: " + this.fda_img_path);
		Log.v(TAG, "FDB: " + this.fdb_img_path);
		Log.v(TAG, "Boot Device: " + this.bootdevice);
		Log.v(TAG, "NET: " + this.net_cfg);
		Log.v(TAG, "NIC: " + this.nic_driver);
	}

	/**
	 * JNI interface for converting PCM file to a WAV file
	 */
	public native String start();

	/**
	 * JNI interface for converting PCM file to a WAV file
	 */
	protected native String stop();

	public native String save();

	protected native String vncchangepassword();

	protected native void dnschangeaddr();

	protected native void scale();

	protected native String getsavestate();

	protected native String getpausestate();
	
	public native String pausevm(String uri);

	protected native void resize();

	protected native void togglefullscreen();

	protected native String getstate();

	protected native String changedev();

	protected native String ejectdev();

	public String startvm(Context context) {
		LimboService.executor = this;
		Intent i = new Intent(Config.ACTION_START, null, context, LimboService.class);
		Bundle b = new Bundle();
		// b.putString("machine_type", this.machine_type);
		i.putExtras(b);
		context.startService(i);
		Log.v(TAG, "startVMService");
		return "startVMService";

		// Log.v(TAG, "Starting the VM");
		// loadNativeLibs();
		// return this.start();

	}

	public String stopvm(int restart) {
		Log.v(TAG, "Stopping the VM");
		this.restart = restart;
		return this.stop();
	}

	public String savevm(String statename) {
		// Set to delete previous snapshots after vm resumed
		Log.v(TAG, "Save the VM");
		this.snapshot_name = statename;
		return this.save();
	}

	public String resumevm() {
		// Set to delete previous snapshots after vm resumed
		Log.v(TAG, "Resume the VM");
		return this.start();
	}

	public String change_vnc_password() {
		return this.vncchangepassword();
	}

	public void change_dns_addr() {
		this.dnschangeaddr();
	}

	public String get_save_state() {
		if (this.libLoaded)
			return this.getsavestate();
		return "";
	}

	public String get_pause_state() {
		if (this.libLoaded)
			return this.getpausestate();
		return "";
	}

	public String get_state() {
		return this.getstate();
	}

	public String change_dev(String dev, String image_path) {
		this.busy = true;
		this.qemu_dev = dev;
		this.qemu_dev_value = image_path;
		if (qemu_dev_value == null || qemu_dev_value.trim().equals("")) {
			Log.v("Limbo", "Ejecting Dev: " + dev);
			this.busy = false;
			return this.ejectdev();
		} else {
			Log.v("Limbo", "Changing Dev: " + dev + " to: " + image_path);
			this.busy = false;
			return this.changedev();
		}

	}

	public void resizeScreen() {
		// TODO Auto-generated method stub
		this.resize();

	}

	public void toggleFullScreen() {
		// TODO Auto-generated method stub
		this.togglefullscreen();

	}

	public void screenScale(int width, int height) {
		// TODO Auto-generated method stub
		this.width = width;
		this.height = height;

		this.scale();

	}

	public int get_fd(String path) {
		int fd = LimboActivity.get_fd(path);
		return fd;

	}

	public int close_fd(int fd) {
		int res = LimboActivity.close_fd(fd);
		return res;

	}

	public void prepPaths() {
		File destDir = new File(save_dir);
		if (!destDir.exists()) {
			destDir.mkdir();
		}

		// Protect the paths from qemu thinking they contain a protocol in the
		// string
		if (this.kernel != null && this.kernel.startsWith("content:")) {
			this.kernel = ("/" + this.kernel).replace(":", "");
		}

		if (this.initrd != null && this.initrd.startsWith("content:")) {
			this.initrd = ("/" + this.initrd).replace(":", "");
		}

		// Non Removable Disks
		if (this.hda_img_path != null) {
			if (this.hda_img_path.startsWith("content:"))
				this.hda_img_path = ("/" + this.hda_img_path).replace(":", "");
			if (hda_img_path.equals("")) {
				hda_img_path = null;
			}
		}

		if (this.hdb_img_path != null) {
			if (this.hdb_img_path.startsWith("content:"))
				this.hdb_img_path = ("/" + this.hdb_img_path).replace(":", "");
			if (hdb_img_path.equals("")) {
				hdb_img_path = null;
			}
		}

		if (this.hdc_img_path != null) {

			if (this.hdc_img_path.startsWith("content:"))
				this.hdc_img_path = ("/" + this.hdc_img_path).replace(":", "");
			if (hdc_img_path.equals("")) {
				hdc_img_path = null;
			}

		}

		if (this.hdd_img_path != null) {
			if (this.hdd_img_path.startsWith("content:"))
				this.hdd_img_path = ("/" + this.hdd_img_path).replace(":", "");
			if (hdd_img_path.equals("")) {
				hdd_img_path = null;
			}
		}

		// Removable disks
		if (this.cd_iso_path != null && this.cd_iso_path.startsWith("content:")) {
			this.cd_iso_path = ("/" + this.cd_iso_path).replace(":", "");
		}
		if (this.fda_img_path != null && this.fda_img_path.startsWith("content:")) {
			this.fda_img_path = ("/" + this.fda_img_path).replace(":", "");
		}
		if (this.fdb_img_path != null && this.fdb_img_path.startsWith("content:")) {
			this.fdb_img_path = ("/" + this.fdb_img_path).replace(":", "");
		}
		if (this.sd_img_path != null && this.sd_img_path.startsWith("content:")) {
			this.sd_img_path = ("/" + this.sd_img_path).replace(":", "");
		}

	}

}
