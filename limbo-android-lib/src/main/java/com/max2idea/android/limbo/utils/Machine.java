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

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

public class Machine {

    public static final String EMPTY = "empty";
    public static String TAG = "Machine";

    public String machinename;

    public String snapshot_name = "";

    public String ui = "VNC";
    public String keyboard="en-us";
    public String mouse = null;
    public int enablespice;
    public int enablevnc = 1;

    public String arch;
    public String machine_type;
    public String cpu = "Default";
    public int cpuNum = 1;
    public int memory = 128;
    public int enableMTTCG;
    public int enableKVM;
    public int disableacpi = 0;
    public int disablehpet = 0;
    public int disablefdbootchk = 0;
    public int disabletsc = 1; //disabling TSC by default

    // Storage
    public String hda_img_path;
    public String hdb_img_path;
    public String hdc_img_path;
    public String hdd_img_path;
    public String hd_cache = "default";
    public String shared_folder;
    public int shared_folder_mode;

    //Removable devices
    public boolean enableCDROM;
    public boolean enableFDA;
    public boolean enableFDB;
    public boolean enableSD;
    public String cd_iso_path;
    public String fda_img_path;
    public String fdb_img_path;
    public String sd_img_path;

    // Default Settings
    public String bootdevice = "Default";
    public String kernel;
    public String initrd;
    public String append;

    // net
    public String net_cfg = "None";
    public String nic_card = "ne2k_pci";
    public String guestfwd;
    public String hostfwd;

    //display
    public String vga_type = "std";

    //sound
    public String soundcard = "None";

    //extra qemu params
    public String extra_params;

    public int status = Config.STATUS_NULL;
    public String lib = "liblimbo.so";
    public String lib_path = "libqemu-system-i386.so";
    public int enableqmp = 1;
    public int restart = 0;
    public int paused;

    public Machine(String machinename) {
        this.machinename = machinename;
    }


    public static void promptPausedVM(final Activity activity) {

        new AlertDialog.Builder(activity).setCancelable(false).setTitle("Paused").setMessage("VM is now Paused tap OK to exit")
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        Log.i(TAG, "VM Paused, Shutting Down");
                        if (LimboActivity.vmexecutor != null) {
                            LimboActivity.vmexecutor.stopvm(0);
                        } else if (activity.getParent() != null) {
                            activity.getParent().finish();
                        } else {
                            activity.finish();
                        }
                    }
                }).show();
    }


    public static void onRestartVM(final Context context) {
        Thread t = new Thread(new Runnable() {
            public void run() {
                if (LimboActivity.vmexecutor != null) {
                    Log.v(TAG, "Restarting the VM...");
                    LimboActivity.vmexecutor.stopvm(1);

                    LimboActivity.vmStarted = true;
                    UIUtils.toastShort(context, "VM Reset");

                } else {

                    UIUtils.toastShort(context, "VM Not Running");
                }
            }
        });
        t.start();
    }

    public static void pausedErrorVM(Activity activity, String errStr) {

        errStr = errStr != null ? errStr : "Could not pause VM. View log for details";

        new AlertDialog.Builder(activity).setTitle("Error").setMessage(errStr)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        Thread t = new Thread(new Runnable() {
                            public void run() {
                                String command = QmpClient.cont();
                                String msg = QmpClient.sendCommand(command);
                            }
                        });
                        t.start();
                    }
                }).show();
    }

    public static void stopVM(final Activity activity) {

        new AlertDialog.Builder(activity).setTitle("Shutdown VM")
                .setMessage("To avoid any corrupt data make sure you "
                        + "have already shutdown the Operating system from within the VM. Continue?")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (LimboActivity.vmexecutor != null) {
                            LimboActivity.vmexecutor.stopvm(0);
                        } else if (activity.getParent() != null) {
                            activity.getParent().finish();
                        } else {
                            activity.finish();
                        }
                    }
                }).setNegativeButton("No", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }

    public static LimboActivity.VMStatus checkSaveVMStatus(final Activity activity) {
        String pause_state = "";
        if (LimboActivity.vmexecutor != null) {

            String command = QmpClient.query_migrate();
            String res = QmpClient.sendCommand(command);

            if (res != null && !res.equals("")) {
                //Log.d(TAG, "Migrate status: " + res);
                try {
                    JSONObject resObj = new JSONObject(res);
                    String resInfo = resObj.getString("return");
                    JSONObject resInfoObj = new JSONObject(resInfo);
                    pause_state = resInfoObj.getString("status");
                } catch (JSONException e) {
                    if (Config.debug)
                        Log.e(TAG,e.getMessage());
                        //e.printStackTrace();
                }
                if (pause_state != null && pause_state.toUpperCase().equals("FAILED")) {
                    Log.e(TAG, "Error: " + res);
                }
            }
        }

        if (pause_state.toUpperCase().equals("ACTIVE")) {
            return LimboActivity.VMStatus.Saving;
        } else if (pause_state.toUpperCase().equals("COMPLETED")) {
            LimboActivity.vmexecutor.paused = 1;
            LimboActivity.saveStateVMDB();

            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    promptPausedVM(activity);
                }
            }, 1000);
            return LimboActivity.VMStatus.Completed;

        } else if (pause_state.toUpperCase().equals("FAILED")) {
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    pausedErrorVM(activity, null);
                }
            }, 100);
            return LimboActivity.VMStatus.Failed;
        }
        return LimboActivity.VMStatus.Unknown;
    }

    public static boolean isHostX86_64() {
    	if(Build.SUPPORTED_64_BIT_ABIS != null)
		{
			for(int i=0; i< Build.SUPPORTED_64_BIT_ABIS.length ; i++)
				if(Build.SUPPORTED_64_BIT_ABIS[i].equals("x86_64"))
					return true;
		}
		return false;
	}

    public static boolean isHostX86() {
		if(Build.SUPPORTED_32_BIT_ABIS != null)
		{
			for(int i=0; i< Build.SUPPORTED_32_BIT_ABIS.length ; i++)
				if(Build.SUPPORTED_32_BIT_ABIS[i].equals("x86"))
					return true;
		}
		return false;
	}

    public static boolean isHostArm() {
		if(Build.SUPPORTED_32_BIT_ABIS != null)
		{
			for(int i=0; i< Build.SUPPORTED_32_BIT_ABIS.length ; i++)
				if(Build.SUPPORTED_32_BIT_ABIS[i].equals("armeabi-v7a"))
					return true;
		}
		return false;
	}

    public static boolean isHostArmv8() {
		if(Build.SUPPORTED_64_BIT_ABIS != null)
		{
			for(int i=0; i< Build.SUPPORTED_64_BIT_ABIS.length ; i++)
				if(Build.SUPPORTED_64_BIT_ABIS[i].equals("arm64-v8a"))
					return true;
		}
		return false;
	}

    public static boolean isHost64Bit() {
        return Build.SUPPORTED_64_BIT_ABIS!=null && Build.SUPPORTED_64_BIT_ABIS.length > 0 ;
    }


    public static void resetVM(final Activity activity) {

        new AlertDialog.Builder(activity).setTitle("Reset VM")
                .setMessage("To avoid any corrupt data make sure you "
                        + "have already shutdown the Operating system from within the VM. Continue?")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        new Thread(new Runnable() {
                            public void run() {
                                Log.v(TAG, "VM is reset");
                                Machine.onRestartVM(activity);
                            }
                        }).start();

                    }
                }).setNegativeButton("No", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }

    public void insertMachineDB(Context context) {
        int rows = MachineOpenHelper.getInstance(context).insertMachine(this);
        Log.v(TAG, "Attempting insert to DB after rows = " + rows);
    }

    public boolean hasRemovableDevices() {

        return enableCDROM || enableFDA || enableFDB || enableSD;
    }
}
