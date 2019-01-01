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

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import com.max2idea.android.limbo.main.Config;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

/**
 * 
 * @author Dev
 */
public class MachineOpenHelper extends SQLiteOpenHelper {

	private static final int DATABASE_VERSION = 15;
	private static final String DATABASE_NAME = "LIMBO";
	private static final String MACHINE_TABLE_NAME = "machines";

	// Columns
	public static final String MACHINE_NAME = "MACHINE_NAME";
	public static final String SNAPSHOT_NAME = "SNAPSHOT_NAME";
	public static final String ARCH = "ARCH";
	public static final String CPU = "CPU";
	public static final String CPUNUM = "CPUNUM";
	public static final String MEMORY = "MEMORY";
	public static final String KERNEL = "KERNEL";
	public static final String INITRD = "INITRD";
	public static final String APPEND = "APPEND";
	public static final String MACHINE_TYPE = "MACHINETYPE";
	public static final String CDROM = "CDROM";
	public static final String FDA = "FDA";
	public static final String FDB = "FDB";
	public static final String HDA = "HDA";
	public static final String HDB = "HDB";
	public static final String HDC = "HDC";
	public static final String HDD = "HDD";
	public static final String SD = "SD";
	public static final String SHARED_FOLDER = "SHARED_FOLDER";
	public static final String SHARED_FOLDER_MODE = "SHARED_FOLDER_MODE";
	public static final String BOOT_CONFIG = "BOOT_CONFIG";
	public static final String NET_CONFIG = "NETCONFIG";
	public static final String HOSTFWD = "HOSTFWD";
	public static final String GUESTFWD = "GUESTFWD";
	public static final String NIC_CONFIG = "NICCONFIG";
	public static final String VGA = "VGA";
	public static final String SOUNDCARD_CONFIG = "SOUNDCARD";
	public static final String HDCACHE_CONFIG = "HDCONFIG";
	public static final String DISABLE_ACPI = "DISABLE_ACPI";
	public static final String DISABLE_HPET = "DISABLE_HPET";
    public static final String DISABLE_TSC = "DISABLE_TSC";
	public static final String DISABLE_FD_BOOT_CHK = "DISABLE_FD_BOOT_CHK";
	public static final String ENABLE_USBMOUSE = "ENABLE_USBMOUSE";
	public static final String STATUS = "STATUS";
	public static final String LASTUPDATED = "LAST_UPDATED";
	public static final String PAUSED = "PAUSED";
	public static final String EXTRA_PARAMS = "EXTRA_PARAMS";
	public static final String UI = "UI";
    public static final String MOUSE = "MOUSE";
    public static final String KEYBOARD = "KEYBOARD";
    public static final String ENABLE_MTTCG = "ENABLE_MTTCG";
    public static final String ENABLE_KVM = "ENABLE_KVM";

	// Create DDL
	private static final String MACHINE_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS " + MACHINE_TABLE_NAME + " ("
			+ MACHINE_NAME + " TEXT , " + SNAPSHOT_NAME + " TEXT , " + CPU + " TEXT, " + ARCH + " TEXT, " + MEMORY
			+ " TEXT, " + FDA + " TEXT, " + FDB + " TEXT, " + CDROM + " TEXT, " + HDA + " TEXT, " + HDB + " TEXT, "
			+ HDC + " TEXT, " + HDD + " TEXT, " + BOOT_CONFIG + " TEXT, " + NET_CONFIG + " TEXT, " + NIC_CONFIG
			+ " TEXT, " + VGA + " TEXT, " + SOUNDCARD_CONFIG + " TEXT, " + HDCACHE_CONFIG + " TEXT, " + DISABLE_ACPI
			+ " INTEGER, " + DISABLE_HPET + " INTEGER, " + ENABLE_USBMOUSE + " INTEGER, " + STATUS + " TEXT, "
			+ LASTUPDATED + " DATE, " + KERNEL + " INTEGER, " + INITRD + " TEXT, " + APPEND + " TEXT, " + CPUNUM
			+ " INTEGER, " + MACHINE_TYPE + " TEXT, " + DISABLE_FD_BOOT_CHK + " INTEGER, " + SD + " TEXT, " + PAUSED
			+ " INTEGER, " + SHARED_FOLDER + " TEXT, " + SHARED_FOLDER_MODE + " INTEGER, " + EXTRA_PARAMS + " TEXT, "
			+ HOSTFWD + " TEXT, " + GUESTFWD + " TEXT, " + UI + " TEXT, " + DISABLE_TSC + " INTEGER, "
            + MOUSE + " TEXT, " + KEYBOARD + " TEXT, " + ENABLE_MTTCG + " INTEGER, " + ENABLE_KVM  + " INTEGER "
            + ");";

	private String TAG = "MachineOpenHelper";

    private static MachineOpenHelper sInstance;
	private SQLiteDatabase db;

	
	public MachineOpenHelper(Context context) {
		super(context, DATABASE_NAME, null, DATABASE_VERSION);
		getDB();
	}
	
	private synchronized void getDB() {
		
		if (db == null)
			db = getWritableDatabase();
	}


	public static synchronized MachineOpenHelper getInstance(Context context) {

        if (sInstance == null) {
            sInstance = new MachineOpenHelper(context.getApplicationContext());
            sInstance.setWriteAheadLoggingEnabled(true);
        }
        return sInstance;
    }
	
	@Override
	public void onCreate(SQLiteDatabase db) {
		db.execSQL(MACHINE_TABLE_CREATE);

	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

		Log.w("machineOpenHelper", "Upgrading database from version " + oldVersion + " to " + newVersion);

		if (newVersion >= 3 && oldVersion <= 2) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + KERNEL + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + INITRD + " TEXT;");

		}

		if (newVersion >= 4 && oldVersion <= 3) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + CPUNUM + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MACHINE_TYPE + " TEXT;");
		}

		if (newVersion >= 5 && oldVersion <= 4) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + HDC + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + HDD + " TEXT;");
		}

		if (newVersion >= 6 && oldVersion <= 5) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + APPEND + " TEXT;");
		}

		if (newVersion >= 7 && oldVersion <= 6) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + DISABLE_FD_BOOT_CHK + " INTEGER;");
		}

		if (newVersion >= 8 && oldVersion <= 7) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + ARCH + " TEXT;");
		}

		if (newVersion >= 9 && oldVersion <= 8) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + SD + " TEXT;");
		}

		if (newVersion >= 10 && oldVersion <= 9) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + PAUSED + " INTEGER;");
		}

		if (newVersion >= 11 && oldVersion <= 10) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + SHARED_FOLDER + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + SHARED_FOLDER_MODE + " INTEGER;");
		}

		if (newVersion >= 12 && oldVersion <= 11) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + EXTRA_PARAMS + " TEXT;");
		}

		if (newVersion >= 13 && oldVersion <= 12) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + HOSTFWD + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + GUESTFWD + " TEXT;");
		}

		if (newVersion >= 14 && oldVersion <= 13) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + UI + " TEXT;");
		}

        if (newVersion >= 15 && oldVersion <= 14) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + DISABLE_TSC + " INTEGER;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MOUSE + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + KEYBOARD + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + ENABLE_MTTCG + " INTEGER;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + ENABLE_KVM + " INTEGER;");
        }
	}

	public synchronized int insertMachine(Machine machine) {
		int seqnum = -1;
		SQLiteDatabase db = getWritableDatabase();

		 Log.v("DB", "insert machine: " + machine.machinename);
		ContentValues stateValues = new ContentValues();
		stateValues.put(MACHINE_NAME, machine.machinename);
		stateValues.put(SNAPSHOT_NAME, machine.snapshot_name);
		stateValues.put(CPU, machine.cpu);
		stateValues.put(CPUNUM, machine.cpuNum);
		stateValues.put(MEMORY, machine.memory);
		stateValues.put(HDA, machine.hda_img_path);
		stateValues.put(HDB, machine.hdb_img_path);
		stateValues.put(HDC, machine.hdc_img_path);
		stateValues.put(HDD, machine.hdd_img_path);
		stateValues.put(CDROM, machine.cd_iso_path);
		stateValues.put(FDA, machine.fda_img_path);
		stateValues.put(FDB, machine.fdb_img_path);
		stateValues.put(SHARED_FOLDER, machine.shared_folder);
		stateValues.put(SHARED_FOLDER_MODE, machine.shared_folder_mode);
		stateValues.put(BOOT_CONFIG, machine.bootdevice);
		stateValues.put(NET_CONFIG, machine.net_cfg);
		stateValues.put(NIC_CONFIG, machine.nic_card);
		stateValues.put(VGA, machine.vga_type);
		stateValues.put(HDCACHE_CONFIG, machine.hd_cache);
		stateValues.put(DISABLE_ACPI, machine.disableacpi);
		stateValues.put(DISABLE_HPET, machine.disablehpet);
        stateValues.put(DISABLE_TSC, machine.disabletsc);
		stateValues.put(DISABLE_FD_BOOT_CHK, machine.disablefdbootchk);
		stateValues.put(SOUNDCARD_CONFIG, machine.soundcard);
		stateValues.put(KERNEL, machine.kernel);
		stateValues.put(INITRD, machine.initrd);
		stateValues.put(APPEND, machine.append);
		stateValues.put(MACHINE_TYPE, machine.machine_type);
		stateValues.put(ARCH, machine.arch);
		stateValues.put(EXTRA_PARAMS, machine.extra_params);
		stateValues.put(HOSTFWD, machine.hostfwd);
		stateValues.put(GUESTFWD, machine.guestfwd);
		stateValues.put(UI, machine.ui);
        stateValues.put(MOUSE, machine.mouse);
        stateValues.put(KEYBOARD, machine.keyboard);
        stateValues.put(ENABLE_MTTCG, machine.enableMTTCG);
        stateValues.put(ENABLE_KVM, machine.enableKVM);

		SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date date = new Date();
		stateValues.put(LASTUPDATED, dateFormat.format(date));
		stateValues.put(STATUS, Config.STATUS_CREATED);

		try {
			seqnum = (int) db.insertOrThrow(MACHINE_TABLE_NAME, null, stateValues);
            Log.v(TAG, "Inserted Machine: " + machine.machinename + " : " + seqnum);
        } catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while Insert machine: " + e.getMessage());
            if(Config.debug)
                e.printStackTrace();
        }
		return seqnum;
	}

	public int deleteMachines() {
		int rowsAffected = 0;
		SQLiteDatabase db = getReadableDatabase();
		// Insert arrFiles in
		try {
			db.delete(MACHINE_TABLE_NAME, STATUS + "=\"" + Config.STATUS_CREATED + "\"" + "or " + STATUS
					+ "=\"" + Config.STATUS_PAUSED + "\"", null);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while deleting Machines: " + e.getMessage());
            if(Config.debug)
                e.printStackTrace();
		}
		

		return rowsAffected;
	}

	public void update(final Machine machine, final String colname, final String value) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				updateDB(machine, colname, value);
			}
		});
		t.start();
	}

	public int updateDB(Machine machine, String colname, String value) {
			if (machine == null)
			return -1;
		int rows = -1;

		// Log.v("DB", "Updating Machine: " + myMachine.machinename
		// + " column: " + colname + " = " + value);
		ContentValues stateValues = new ContentValues();
		stateValues.put(colname, value);

		try {
			db.beginTransaction();
			rows = (int) db.update(MACHINE_TABLE_NAME, stateValues,
					MACHINE_NAME + "=\"" + machine.machinename + "\"" + " and " + SNAPSHOT_NAME + "=\"\"",
					null);
			db.setTransactionSuccessful();
		} catch (Exception e) {
			Log.e(TAG, "Error while Updating value: " + e.getMessage());
			if(Config.debug)
			    e.printStackTrace();
		} finally {
			db.endTransaction();
		}
		
		return rows;
	}

	public Machine getMachine(String machine, String snapshot) {
		String qry = "select "
				+ MACHINE_NAME + " , " + CPU + " , " + MEMORY + " , " + CDROM + " , " + FDA
				+ " , " + FDB + " , " + HDA + " , " + HDB + " , " + HDC + " , " + HDD + " , "
				+ NET_CONFIG + " , " + NIC_CONFIG + " , " + VGA + " , " + SOUNDCARD_CONFIG + " , "
				+ HDCACHE_CONFIG + " , " + DISABLE_ACPI + " , " + DISABLE_HPET + " , "
				+ ENABLE_USBMOUSE + " , " + SNAPSHOT_NAME + " , " + BOOT_CONFIG + " , " + KERNEL
				+ " , " + INITRD + " , " + APPEND + " , " + CPUNUM + " , " + MACHINE_TYPE + " , "
				+ DISABLE_FD_BOOT_CHK + " , " + ARCH + " , " + PAUSED + " , " + SD + " , "
				+ SHARED_FOLDER + " , " + SHARED_FOLDER_MODE + " , " + EXTRA_PARAMS + " , "
				+ HOSTFWD + " , " + GUESTFWD + " , " + UI + ", " + DISABLE_TSC + ", "
                + MOUSE + ", " + KEYBOARD + ", " + ENABLE_MTTCG + ", " + ENABLE_KVM
				+ " from " + MACHINE_TABLE_NAME
				+ " where " + STATUS + " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " "
				+ " ) " + " and " + MACHINE_NAME + "=\"" + machine + "\"" + " and " + SNAPSHOT_NAME + "=\""
				+ snapshot + "\"" + ";";

		Machine myMachine = null;

		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (!cur.isAfterLast()) {

		    String machinename = cur.getString(0);
            myMachine = new Machine(machinename);

            myMachine.cpu  = cur.getString(1);
            myMachine.memory = cur.getInt(2);
            myMachine.cd_iso_path = cur.getString(3);
            if (myMachine.cd_iso_path != null)
                myMachine.enableCDROM = true;

            myMachine.fda_img_path = cur.getString(4);
            if (myMachine.fda_img_path != null)
                myMachine.enableFDA = true;
            myMachine.fdb_img_path = cur.getString(5);
            if (myMachine.fdb_img_path  != null)
                myMachine.enableFDB = true;

            myMachine.hda_img_path = cur.getString(6);
            myMachine.hdb_img_path = cur.getString(7);
            myMachine.hdc_img_path = cur.getString(8);
            myMachine.hdd_img_path = cur.getString(9);

			myMachine.net_cfg = cur.getString(10);
			myMachine.nic_card = cur.getString(11);
            myMachine.vga_type= cur.getString(12);
            myMachine.soundcard = cur.getString(13);
            myMachine.hd_cache = cur.getString(14);
            myMachine.disableacpi = cur.getInt(15);
            myMachine.disablehpet = cur.getInt(16);
			int usbmouse = cur.getInt(17);
            myMachine.snapshot_name = cur.getString(18);
            myMachine.bootdevice = cur.getString(19);
            myMachine.kernel = cur.getString(20);
            myMachine.initrd = cur.getString(21);
            myMachine.append = cur.getString(22);
            myMachine.cpuNum = cur.getInt(23);
            myMachine.machine_type = cur.getString(24);
            myMachine.disablefdbootchk = cur.getInt(25);
            myMachine.arch = cur.getString(26);
            myMachine.paused = cur.getInt(27);

            myMachine.sd_img_path = cur.getString(28);
            if (myMachine.sd_img_path != null)
                myMachine.enableSD = true;

            myMachine.shared_folder= cur.getString(29);
            myMachine.shared_folder_mode = 1; //hard drives are always Read/Write
            myMachine.extra_params = cur.getString(31);
            myMachine.hostfwd = cur.getString(32);
            myMachine.guestfwd  = cur.getString(33);
            myMachine.ui = cur.getString(34);

            myMachine.disabletsc = cur.getInt(35);

            myMachine.mouse = cur.getString(36);
            myMachine.keyboard = cur.getString(37);
            myMachine.enableMTTCG = cur.getInt(38);
            myMachine.enableKVM = cur.getInt(39);


			break;
		}
		cur.close();
		
		return myMachine;
	}

	public ArrayList<String> getMachines() {
		String qry = "select " + MACHINE_NAME + " " + " from " + MACHINE_TABLE_NAME + " where " + STATUS
				+ " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " " + " ) " + " and "
				+ SNAPSHOT_NAME + " = " + "\"\" " + ";";

		ArrayList<String> arrStr = new ArrayList<String>();
		
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (!cur.isAfterLast()) {
			String machinename = cur.getString(0);
			cur.moveToNext();
			arrStr.add(machinename);
		}
		cur.close();
		
		return arrStr;
	}



	public int deleteMachineDB(Machine machine) {
		int rowsAffected = 0;
		try {
			db.delete(MACHINE_TABLE_NAME, MACHINE_NAME + "=\"" + machine.machinename + "\"" + " and "
					+ SNAPSHOT_NAME + "=\"" + machine.snapshot_name + "\"", null);
		} catch (Exception e) {
			Log.e(TAG, "Error while deleting VM: " + e.getMessage());
            if(Config.debug)
                e.printStackTrace();
        }
		return rowsAffected;
	}

	public ArrayList<String> getSnapshots(Machine machine) {
		String qry = "select " + SNAPSHOT_NAME + " " + " from " + MACHINE_TABLE_NAME + " where " + STATUS
				+ " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " ) " + " and " + MACHINE_NAME
				+ " = " + "\"" + machine.machinename + "\" " + " and " + SNAPSHOT_NAME + " != " + "\"\" "
				+ ";";

		ArrayList<String> arrStr = new ArrayList<String>();
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (!cur.isAfterLast()) {
			String snapshotname = cur.getString(0);

			cur.moveToNext();
			arrStr.add(snapshotname);
		}
		cur.close();
		
		return arrStr;
	}

	public void insertMachines(String machines) {
		

	}

	public String exportMachines() {
		String qry = "select " 
				//Columns
				+ MACHINE_NAME + " , " + CPU + " , " + MEMORY + " , " + CDROM + " , " + FDA
				+ " , " + FDB + " , " + HDA + " , " + HDB + " , " + HDC + " , " + HDD + " , "
				+ NET_CONFIG + " , " + NIC_CONFIG + " , " + VGA + " , " + SOUNDCARD_CONFIG + " , "
				+ HDCACHE_CONFIG + " , " + DISABLE_ACPI + " , " + DISABLE_HPET + " , "
				+ ENABLE_USBMOUSE + " , " + SNAPSHOT_NAME + " , " + BOOT_CONFIG + " , " + KERNEL
				+ " , " + INITRD + " , " + APPEND + " , " + CPUNUM + " , " + MACHINE_TYPE + " , "
				+ DISABLE_FD_BOOT_CHK + " , " + ARCH + " , " + PAUSED + " , " + SD + " , "
				+ SHARED_FOLDER + " , " + SHARED_FOLDER_MODE + " , " + EXTRA_PARAMS + " , "
				+ HOSTFWD + " , " + GUESTFWD + " , " + UI + ", " + DISABLE_TSC + ", "
                + MOUSE + ", " + KEYBOARD + ", " + ENABLE_MTTCG + ", " + ENABLE_KVM
				// Table
				+ " from " + MACHINE_TABLE_NAME + "; ";

		String arrStr = "";
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		String headerline = "";
		for (int i = 0; i < cur.getColumnCount(); i++) {
			headerline += ("\"" + cur.getColumnName(i) + "\"");
			if (i < cur.getColumnCount() - 1) {
				headerline += ",";
			}
		}
		arrStr += (headerline + "\n");
		while (!cur.isAfterLast()) {
			String line = "";
			for (int i = 0; i < cur.getColumnCount(); i++) {
				line += ("\"" + cur.getString(i) + "\"");
				if (i < cur.getColumnCount() - 1) {
					line += ",";
				}

			}
			arrStr += (line + "\n");
			cur.moveToNext();
		}
		cur.close();
		
		return arrStr;
	}

}
