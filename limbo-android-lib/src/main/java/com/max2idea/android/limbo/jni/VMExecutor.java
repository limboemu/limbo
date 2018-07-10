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
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboSDLActivity;
import com.max2idea.android.limbo.main.LimboService;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;
import android.app.Notification;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;

import org.json.JSONException;
import org.json.JSONObject;
import org.libsdl.app.SDLActivity;

public class VMExecutor {

	private static Notification mNotification;
	private static Context context;
	private static final String TAG = "VMExecutor";
	public int paused;
	public String cd_iso_path;
	public String fda_img_path;
	public String fdb_img_path;
	public String sd_img_path;
	public int aiomaxthreads = 1;
	public String sound_card;
	public String snapshot_name = "limbo";
	public String save_state_name = null;
	public int fd_save_state;
								// positioning
	public int enableqmp;
	public int enablekvm;
	public int enablevnc;
	public String vnc_passwd = null;
	public int vnc_allow_external = 0;
	public int enable_mttcg = Config.enableMTTCG?1:0;
	public String base_dir ;
	public String dns_addr;
	public String append = "";
	public boolean busy = false;
	public boolean libLoaded = false;
	public String name;
	public int enablespice = 0;
	public String keyboard_layout = Config.defaultKeyboardLayout;
    public int mouse_conf = 0;
	public String shared_folder_path;
	public int shared_folder_readonly = 1;
	String [] params =null;
	// HDD
	private String hda_img_path;
	private String hdb_img_path;
	private String hdc_img_path;
	private String hdd_img_path;
	private String cpu;
	private String kernel;
	private String initrd;
	// Default Settings
	private int memory = 128;
	private int cpuNum = 128;
	private String bootdevice = null;
	// net
	private String net_cfg = "None";
	private String hostfwd = null;
	private String guestfwd = null;
	private int nic_num = 1;
	private String vga_type = "std";
	private String hd_cache = "default";
	private String nic_driver = null;
	private String liblimbo = "limbo";
	private String libqemu = null;
	private String libglib = null;
	private int restart = 0;
	private int disableacpi = 0;
	private int disablehpet = 0;
	private int disablefdbootchk = 0;
	private int usbmouse = 0; // for -usb -usbdevice tablet - fixes mouse
	private int width;
	private int height;
	private String arch = "x86";
	private String machine_type;
	private String save_dir;
	private String qmp_server;
	private int qmp_port;
	private String extra_params;

	public int current_fd= 0;

	/**
	 * @throws Exception
	 */
	public VMExecutor(Machine machine, Context context) throws Exception {

		name = machine.machinename;
		base_dir = Config.getBasefileDir(context);
		save_dir = Config.getMachineDir(context) + name;
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
		this.qmp_server = Config.QMPServer;
		this.qmp_port = Config.QMPPort;
		this.enablevnc = machine.enablevnc;
		this.enablevnc = machine.enablespice;
		if (Config.enable_SDL_sound)
			if (!machine.soundcard.equals("none"))
				this.sound_card = machine.soundcard;
		this.kernel = machine.kernel;
		this.initrd = machine.initrd;

		this.cpu = machine.cpu;
		this.libglib = FileUtils.getDataDir() + "/lib/libglib-2.0.so";

		if (machine.arch.endsWith("ARM") || machine.arch.endsWith("ARM64")) {
            this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-aarch64.so";
            File libFile = new File(libqemu);
            if (!libFile.exists()) {
                this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-arm.so";
                libFile = new File(libqemu);
            }
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
			if (machine.machine_type == null)
				this.machine_type = "pc";
			else
				this.machine_type = machine.machine_type;
		} else if (machine.arch.endsWith("x86")) {
			this.cpu = machine.cpu;
            this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-x86_64.so";
			File libFile = new File(libqemu);
			if (!libFile.exists()) {
                this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-i386.so";
				libFile = new File(libqemu);
//				if (!libFile.exists()) {
//					throw new Exception ("Could not find QEMU library: " + libFile.getAbsolutePath());
//				}
			}
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "x86";
			if (machine.machine_type == null)
				this.machine_type = "pc";
			else
				this.machine_type = machine.machine_type;
		} else if (machine.arch.endsWith("MIPS")) {
			this.cpu = machine.cpu;
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-mips.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "mips";
			if (machine.machine_type == null)
				this.machine_type = "malta";
			else
				this.machine_type = machine.machine_type;
		} else if (machine.arch.endsWith("PPC")) {
			this.cpu = machine.cpu;
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-ppc.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "ppc";
			if (machine.machine_type == null || machine.machine_type.equals("Default"))
				this.machine_type = null;
			else
				this.machine_type = machine.machine_type;
		} else if (machine.arch.endsWith("PPC64")) {
            this.cpu = machine.cpu;
            this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-ppc64.so";
            this.cpu = machine.cpu.split(" ")[0];
            this.arch = "ppc64";
            if (machine.machine_type == null || machine.machine_type.equals("Default"))
                this.machine_type = null;
            else
                this.machine_type = machine.machine_type;
        } else if (machine.arch.endsWith("m68k")) {
			this.cpu = machine.cpu;
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-m68k.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "m68k";
			if (machine.machine_type == null)
				this.machine_type = "Default";
			else
				this.machine_type = machine.machine_type;
		} else if (machine.arch.endsWith("SPARC")) {
			this.cpu = machine.cpu;
			this.libqemu = FileUtils.getDataDir() + "/lib/libqemu-system-sparc.so";
			this.cpu = machine.cpu.split(" ")[0];
			this.arch = "SPARC";
			if (machine.machine_type == null || machine.machine_type.equals("Default"))
				this.machine_type = null;
			else
				this.machine_type = machine.machine_type;

			//override vga
			this.vga_type ="cg3";
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

		if (Config.enableFlashMemoryImages) {
			if (machine.sd_img_path == null || machine.sd_img_path.equals("None")) {
				this.sd_img_path = null;
			} else {
				this.sd_img_path = machine.sd_img_path;
			}
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
			this.guestfwd = machine.guestfwd;
			if (machine.hostfwd != null && !machine.hostfwd.equals(""))
				this.hostfwd = machine.hostfwd;
			else
				this.hostfwd = null;
		} else if (machine.net_cfg.equals("TAP")) {
			this.net_cfg = "tap";
			this.nic_driver = machine.nic_driver;
		}

		if (machine.soundcard != null && machine.soundcard.equals("None")) {
			this.sound_card = null;
		}

		// Shared folder
		if (machine.shared_folder != null && !machine.shared_folder.trim().equals("")) {
			shared_folder_path = machine.shared_folder;
			if (machine.shared_folder_mode == 0)
				shared_folder_readonly = 1;
			else if (machine.shared_folder_mode == 1)
				shared_folder_readonly = 0;
		} else
			shared_folder_path = null;

		// Extra Params
		if (machine.extra_params != null) {
			this.extra_params = machine.extra_params;
		}
		this.prepPaths();
	}

	public void loadNativeLibs() {
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

	public void print(String [] params) {
		Log.d(TAG, "Params:");
		for(int i=0;i<params.length; i++){
			Log.d(TAG, i +": " + params[i]);
		}

	}
	
	public String startvm() {

		String res = null;
		try {
			prepareParams();
		}catch (Exception ex) {
			UIUtils.toastLong(context, ex.getMessage());
			return res;
		}



//		Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
//			@Override
//			public void uncaughtException(Thread thread, Throwable e) {
//				e.printStackTrace();
//				Log.e(TAG, "Limbo Uncaught Exception: " + e.toString());
//			}
//		});

		//set the exit code
		FileUtils.setExitCode(context, 2);

		try {
			res = start(Config.storagedir, this.base_dir, this.libqemu, params, this.paused, this.save_state_name);
		} catch (Exception ex) {
			ex.printStackTrace();
			Log.e(TAG, "Limbo Exception: " + ex.toString());
		}
		return res;
	}
	
	public void prepareParams() throws FileNotFoundException {

		params = null;
		ArrayList<String> paramsList  = new ArrayList<String>();	
		
		paramsList.add(libqemu);

		if (this.kernel != null && !this.kernel.equals("")) {
			paramsList.add("-kernel");
			paramsList.add(this.kernel);
		}

		if (initrd != null && !initrd.equals("")) {
			paramsList.add("-initrd");
			paramsList.add(initrd);
		}

		if (append != null && !append.equals("")) {
			paramsList.add("-append");
			paramsList.add(append);
		}

		if (this.cpu != null && !cpu.equals("Default")) {
			paramsList.add("-cpu");
			paramsList.add(cpu);
		}

		paramsList.add("-m");
		paramsList.add(this.memory+"");

		paramsList.add("-L");
		paramsList.add(base_dir);

		addDrives(paramsList);

		if (vga_type != null) {
		    if(vga_type.equals("Default")) {
		        //do nothing
            }else if(vga_type.equals("virtio-gpu-pci")){
                paramsList.add("-device");
                paramsList.add(vga_type);
            }else if (vga_type.equals("nographic")){
                paramsList.add("-nographic");
            }else {
                paramsList.add("-vga");
                paramsList.add(vga_type);
            }
		}

		if (this.bootdevice != null) {
			paramsList.add("-boot");
			paramsList.add(bootdevice);
		}

		if (this.net_cfg != null) {
			paramsList.add("-net");
			if (net_cfg.equals("user")) {
				String netParams = net_cfg;
				if (hostfwd != null) {
					netParams+= ",";
					netParams+= hostfwd; // example forward ssh: "hostfwd=tcp:127.0.0.1:2222-:22"
				}
				if (guestfwd != null) {
					netParams+=  ",";
					netParams+= guestfwd;
				}
				paramsList.add(netParams);	
			} else if (net_cfg.equals("tap")) {
				paramsList.add("tap,vlan=0,ifname=tap0,script=no");
			} else if (net_cfg.equals("none")) {
				paramsList.add("none");
			} else {
				//Unknown interface
				paramsList.add("none");
			}
		}

		if (nic_driver != null) {
			paramsList.add("-net");
			if (net_cfg.equals("user")) {
				String nicParams = "nic,model="+nic_driver;
				paramsList.add(nicParams);
			} else if (net_cfg.equals("tap")) {
				String nicParams = "nic,vlan=0,model=" + nic_driver;
				paramsList.add(nicParams);
			}
		}

		if (sound_card != null && !sound_card.equals("None") ) {
			paramsList.add("-soundhw");
			paramsList.add(sound_card);
		}

		//XXX: Snapshots not working currently, use migrate/incoming instead
		if (snapshot_name != null && !snapshot_name.equals("")) {
			paramsList.add("-loadvm");
			paramsList.add(snapshot_name);
		}

		if (this.usbmouse!=0) {
			paramsList.add("-usb");
			paramsList.add("-usbdevice");
			paramsList.add("tablet");
		}
		if (disableacpi!=0) {
			paramsList.add("-no-acpi"); //disable ACPI
		}
		if (disablehpet!=0) {
			paramsList.add("-no-hpet"); //        disable HPET
		}

		if (disablefdbootchk!=0) {
			paramsList.add("-no-fd-bootchk"); //        disable FD Boot Check
		}

		if (enableqmp!=0) {

			paramsList.add("-qmp");
			String qmpParams = "tcp:";
			if(qmp_server!=null && !Config.enable_QMP_External)
				qmpParams+=qmp_server;

			qmpParams+=(":"+this.qmp_port);
			qmpParams+=",server,nowait";
			paramsList.add(qmpParams);
		}

		//XXX: Extra options
		//    argv.add("-D");
		//    argv.add("/sdcard/limbo/log.txt");
		//    argv.add("-win2k-hack");     //use it when installing Windows 2000 to avoid a disk full bug
		//    argv.add("--trace");
		//    argv.add("events=/sdcard/limbo/tmp/events");
		//    argv.add("--trace");
		//    argv.add("file=/sdcard/limbo/tmp/trace");
		//    argv.add("-nographic"); //DO NOT USE //      disable graphical output and redirect serial I/Os to console



		if (enablekvm!=0) {
			paramsList.add("-enable-kvm");
		} else if (this.enable_mttcg!=0) {
            //XXX: not working right now, we should only do this for 64bit hosts
            // #ifdef __LP64__
			paramsList.add("-accel");
			String tcgParams = "tcg";
			if(cpuNum > 1)
				tcgParams+=",thread=multi";
			else
				tcgParams+=",thread=single";
            paramsList.add(tcgParams);
            //#endif
		}

		if (enablevnc!=0) {
			Log.v(TAG,"Enable VNC server");
			paramsList.add("-vnc");
			if (vnc_allow_external!=0) {
				paramsList.add(":1,password");
				//TODO: Allow connections from External
				//this is still not secure we prompt the user from the UI
				// Use with x509 auth and TLS for encryption
			} else
				paramsList.add("localhost:1"); // Allow only connections from localhost without password

			//Allow monitor console only for VNC,
			// SDL for android doesn't support more
			// than 1 window
			paramsList.add("-monitor");
			paramsList.add("vc");

		} else if (enablespice!=0) {
			//Not working right now
			Log.v(TAG,"Enable SPICE server");
			paramsList.add("-spice");
			String spiceParams = "port=5902";

			if (vnc_allow_external!=0 && vnc_passwd != null) {
				spiceParams+= ",password=";
				spiceParams+= vnc_passwd;
			} else
				spiceParams+=",addr=localhost"; // Allow only connections from localhost without password

				spiceParams+=",disable-ticketing";
			//argv.add("-chardev");
			//argv.add("spicevm");
		} else {
			//SDL needs explicit keyboard layout
			Log.v(TAG,"Disabling VNC server, using SDL instead");
			if (keyboard_layout == null) {
				paramsList.add("-k");
				paramsList.add("en-us");
			}

			//XXX: monitor, serial, and parallel display crashes cause SDL doesn't support more than 1 window
			paramsList.add("-monitor");
			paramsList.add("none");

			paramsList.add("-serial");
			paramsList.add("none");

			paramsList.add("-parallel");
			paramsList.add("none");
		}

		if (keyboard_layout != null) {
			paramsList.add("-k");
			paramsList.add(keyboard_layout);
		}

        if (mouse_conf > 0) {
            paramsList.add("-usb");
            paramsList.add("-device");
            if(mouse_conf == 1)
                paramsList.add("usb-mouse");
            if(mouse_conf == 2)
                paramsList.add("usb-tablet");
        }

        //XXX: SMP is not working correctly for some guest OSes
        //so we enable multi core only under KVM
        // anyway regular emulation is not gaining any benefit unless mttcg is enabled but that
        // doesn't work for x86 guests yet
        if(enablekvm == 1 || !Config.enableSMPOnlyOnKVM) {
            paramsList.add("-smp");
            paramsList.add(this.cpuNum + "");
        }

		if (machine_type != null) {
			paramsList.add("-M");
			paramsList.add(machine_type);
		}
//		LOGV("Setting tb memory");
//		argv.add("-tb-size");
//		argv.add("32M"); //Don't increase it crashes

		paramsList.add("-realtime");
		paramsList.add("mlock=off");

		paramsList.add("-rtc");
		paramsList.add("base=localtime");

		//XXX: deprecated
		//This should already be set by the rtc option above, though it shouldn't hurt if we add again
		//paramsList.add("-localtime");

		paramsList.add("-nodefaults");


		//XXX: Usb redir not working under User mode
		//Redirect ports (SSH)
		//	argv.add("-redir");
		//	argv.add("5555::22");

		//TODO:Extra options
		if(extra_params!=null && !extra_params.trim().equals("")) {
			String[] paramsTmp = extra_params.split(" ");
			paramsList.addAll(Arrays.asList(paramsTmp));
		}

		//TODO: resume incoming option seems not to accept the fd we get from java
		//  for now we do this at the c side
		if(paused == 1 && this.save_state_name != null && !save_state_name.equals("")) {
			int fd_tmp = FileUtils.get_fd(context, save_state_name);
			if (fd_tmp < 0) {
				Log.e(TAG,"Error while getting fd for: " + save_state_name);
			} else {
				//Log.i(TAG, "Got new fd "+fd_tmp + " for: " +save_state_name);
				paramsList.add("-incoming");
				paramsList.add("fd:"+fd_tmp);
			}
		}

		params = (String[]) paramsList.toArray(new String[paramsList.size()]);

		print(params);

	}

	public void addDrives(ArrayList<String> paramsList) {
		if (hda_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=0";
			if(Config.enable_hd_if) {
				param +=",if=";
				param += Config.hd_if_type;
			}
			param +=",media=disk";
			if (!hda_img_path .equals("")) {
				param += ",file="+hda_img_path ;
			}
			paramsList.add(param);
		}

		if (hdb_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=1";
			if(Config.enable_hd_if) {
				param +=",if=";
				param += Config.hd_if_type;
			}
			param +=",media=disk";
			if (!hdb_img_path .equals("")) {
				param += ",file="+hdb_img_path ;
			}
			paramsList.add(param);
		}

		if (hdc_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=2";
			if(Config.enable_hd_if) {
				param +=",if=";
				param += Config.hd_if_type;
			}
			param +=",media=disk";
			if (!hdc_img_path .equals("")) {
				param += ",file="+hdc_img_path ;
			}
			paramsList.add(param);
		}

		if (hdd_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=3";
			if(Config.enable_hd_if) {
				param +=",if=";
				param += Config.hd_if_type;
			}
			param +=",media=disk";
			if (!hdd_img_path .equals("")) {
				param += ",file="+hdd_img_path ;
			}
			paramsList.add(param);
		}

		if (cd_iso_path!= null) {
			paramsList.add("-drive"); //empty
			String param = "index=2";
			if(Config.enable_hd_if) {
				param +=",if=";
				param += Config.hd_if_type;
			}
			param +=",media=cdrom";
			if (!cd_iso_path.equals("")) {
				param += ",file="+cd_iso_path;
			}
			paramsList.add(param);
		}

		if (fda_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=0,if=floppy";
			if (!fda_img_path.equals("")) {
				param += ",file="+fda_img_path;
			}
			paramsList.add(param);
		}

		if (fdb_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=1,if=floppy";
			if (!fdb_img_path.equals("")) {
				param += ",file="+fdb_img_path;
			}
			paramsList.add(param);
		}

		if (sd_img_path != null) {
			paramsList.add("-drive"); //empty
			String param = "index=0,if=sd";
			if (!sd_img_path.equals("")) {
				param += ",file="+sd_img_path;
			}
			paramsList.add(param);
		}

		if (shared_folder_path != null) { //We use hdd to mount any virtual fat drives
			paramsList.add("-drive"); //empty
			String driveParams = "index=3";
			driveParams += ",media=disk";
			if(Config.enable_hd_if) {
				driveParams +=",if=";
				driveParams += Config.hd_if_type;
			}
			driveParams+=",format=raw";
			driveParams+=",file=fat:";
			driveParams+="rw:"; //Always Read/Write
			driveParams+=shared_folder_path;
			paramsList.add(driveParams);
		}

	}

	//JNI Methods
	public native String start(String storage_dir, String base_dir, String lib_path, Object [] params, int paused, String save_state_name);
	public native String stop(int restart);
    public native void setsdlrefreshrate(int value);
    public native void setvncrefreshrate(int value);
    public native int getsdlrefreshrate();
    public native int getvncrefreshrate();
	private native int onmouse(int button, int action, int relative, float x, float y);
	private native int setrelativemousemode(int relativemousemode);


	protected void vncchangepassword(String vnc_passwd){
		String res = QmpClient.sendCommand(QmpClient.changevncpasswd(vnc_passwd));
		String desc = null;
		if(res!=null && !res.equals("")) {
			try {
				JSONObject resObj = new JSONObject(res);
				if(resObj != null && !resObj.equals("")) {
					String resInfo = resObj.getString("error");
					if(resInfo!=null && !resInfo.equals("")) {
						JSONObject resInfoObj = new JSONObject(resInfo);
						desc = resInfoObj.getString("desc");
						UIUtils.toastLong(context, "Could not set VNC Password: " + desc);
						Log.e(TAG, desc);
					}
				}


			} catch (JSONException e) {
				e.printStackTrace();
			}
		}
	}

	protected String changedev(String dev, String dev_value){
		QmpClient.sendCommand(QmpClient.changedev(dev, dev_value));
		return "Changed device: " + dev + " to " + dev_value;
	}
	protected String ejectdev(String dev) {
		QmpClient.sendCommand(QmpClient.ejectdev(dev));
		return "Ejected device: " + dev ;
	}

	public String startvm(Context context, int ui) {
		LimboService.executor = this;
		Intent i = new Intent(Config.ACTION_START, null, context, LimboService.class);
		Bundle b = new Bundle();
		// b.putString("machine_type", this.machine_type);
        b.putInt("ui", ui);
		i.putExtras(b);
		context.startService(i);
		Log.v(TAG, "start VM service");
		return "startVMService";

	}

	public void stopvm(final int restart) {
		Log.v(TAG, "Stopping the VM");
		this.restart = restart;

		new Thread(new Runnable() {
			@Override
			public void run() {
				doStopVM(restart);
			}
		}).start();
	}

	public void doStopVM(final int restart) {

				if(restart==0) {
					LimboService.stopService();

					//XXX: Wait till service goes down
					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}

				if(restart != 0){
					QmpClient.sendCommand(QmpClient.reset());
				} else {
					//XXX: Qmp command only halts the VM but doesn't exit
					//  so we use force close
					//QmpClient.sendCommand(QmpClient.powerDown());
					stop(restart);
				}
	}

	public String savevm(String statename) {
		// Set to delete previous snapshots after vm resumed
		Log.v(TAG, "Save Snapshot");
		this.snapshot_name = statename;

		String res = null;
		//TODO:
		//res = QmpClient.sendCommand(QmpClient.saveSnapshot());
		return res;
	}

	public String resumevm() {
		// Set to delete previous snapshots after vm resumed
		Log.v(TAG, "Resume the VM");
        String res = startvm();
        Log.d(TAG, res);
        return res;
	}

	public void change_vnc_password() {
		Thread thread = new Thread(new Runnable() {
			public void run() {
				vncchangepassword(vnc_passwd);
			}
		});
		thread.start();
	}

	public String get_state() {
		String res = QmpClient.sendCommand(QmpClient.getState());
		String state = "";
		if(res!=null && !res.equals("")) {
			try {
				JSONObject resObj = new JSONObject(res);
				String resInfo = resObj.getString("return");
				JSONObject resInfoObj = new JSONObject(resInfo);
				state = resInfoObj.getString("status");
			} catch (JSONException e) {
				e.printStackTrace();
			}
		}
		return state;
	}

	public void change_dev(final String dev, final String image_path) {

        Thread thread = new Thread(new Runnable() {
            public void run() {
                if (image_path == null || image_path.trim().equals("")) {
                    VMExecutor.this.busy = true;
                    String res = VMExecutor.this.ejectdev(dev);
                    UIUtils.toastShort(context, res);
                    VMExecutor.this.busy = false;
                } else if (FileUtils.fileValid(context, image_path)){
                    VMExecutor.this.busy = true;
                    String res = VMExecutor.this.changedev(dev, image_path);
                    UIUtils.toastShort(context, res);
                    VMExecutor.this.busy = false;
                } else {
                    UIUtils.toastShort(context, "File does not exist");
                }
            }
        });
        thread.setPriority(Thread.MIN_PRIORITY);
        thread.start();


	}

	public int get_fd(String path) {
		int fd = FileUtils.get_fd(context, path);
		return fd;

	}

	public int close_fd(int fd) {
		int res = FileUtils.close_fd(fd);
		return res;

	}

	public void prepPaths() {
		File destDir = new File(save_dir);
		if (!destDir.exists()) {
			destDir.mkdirs();
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

	public static void onVMResolutionChanged(int width, int height) {

		if(LimboSDLActivity.mIsSurfaceReady)
			LimboSDLActivity.onVMResolutionChanged(width,height);
	}

	public int setRelativeMouseMode(int relative) {
		return setrelativemousemode(relative);
	}
	public int onLimboMouse(int button, int action, int relative, float x, float y) {
		//XXX: Make sure that mouse motion is not triggering crashes in SDL while resizing
		if(!LimboSDLActivity.mIsSurfaceReady || LimboSDLActivity.isResizing) {
//				Log.w(TAG, "onLimboMouse: Ignoring mouse event surface not ready");
				return -1;
		}

		//XXX: Check boundaries, perhaps not necessary since SDL is also doing the same thing
		if(relative==1
				|| (x>=0 && x<=LimboSDLActivity.vm_width && y>=0 && y<= LimboSDLActivity.vm_height)
				|| (action == MotionEvent.ACTION_SCROLL)) {
//			Log.d(TAG, "onLimboMouse: B: " + button + ", A: " + action + ", R: " + relative + ", X: " + x + ", Y: " + y);
			return onmouse(button, action, relative, x, y);
		}
		return -1;
	}
}

