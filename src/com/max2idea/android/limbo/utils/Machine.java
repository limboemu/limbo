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
package com.max2idea.android.limbo.utils;



import android.app.Activity;
import android.util.Log;

import com.max2idea.android.limbo.main.Config;

public class Machine {
	public static Activity activity;
    
    public static String TAG = "Machine";
    public String machinename;
   
    public  String cd_iso_path;
    
    //hdd
    public  String hda_img_path;
    public  String hdb_img_path;
    public  String hdc_img_path;
    public  String hdd_img_path;
    
    public  String fda_img_path;
    public  String fdb_img_path;
    public  String sd_img_path;
    
    public  String cpu;
    public  String arch;
    
    public  String kernel;
    public  String initrd;
    public  String append;
    
    //Default Settings
    public int memory = 128;
    public String bootdevice = "c";
    //net
    public String net_cfg = "None";
    public int nic_num = 1;
    public String vga_type = "std";
    public String hd_cache = "default";
    public String nic_driver = "ne2k_pci";
    public String lib = "liblimbo.so";
    public String lib_path = "libqemu-system-i386.so";
    public String soundcard = "None";
	public int restart = 0;
	public int disableacpi = 0;
	public int disablehpet = 0;
	public int disablefdbootchk = 0;
	
	public int bluetoothmouse = 0;
	
        public int enableqmp = 1;
        public int enablevnc = 1;
	public int status = Config.STATUS_NULL;

	public String snapshot_name = "";

	public int cpuNum =1;

	public String machine_type ;

	public int paused;

	public int enablespice;

	public String shared_folder;
	public int shared_folder_mode;

	   public Machine(String machinename) {
		   this.machinename = machinename;
	   }


    
    public void insertMachineDB() {
        MachineOpenHelper db = new MachineOpenHelper(activity);
        int rows = db.insertMachine(this);
        Log.v(TAG, "Attempting insert to DB after rows = " + rows);
        db.close();

    }
   

    public void updateStatus(int status) {

        String msg = "";
        if (status == Config.STATUS_CREATED) {
            msg = "Machine created";
        } else if (status == Config.STATUS_PAUSED) {
            msg = "Machine Paused";
            MachineOpenHelper db = new MachineOpenHelper(activity);
            db.updateStatus(this, status);
            db.close();
            return;
        }
        this.insertMachineDB();        
        this.status = status;
        MachineOpenHelper db = new MachineOpenHelper(activity);
        db.updateStatus(this, status);
        db.close();
    }
}
