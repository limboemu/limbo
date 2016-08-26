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
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import com.max2idea.android.limbo.main.Config;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;

/**
 * 
 * @author Dev
 */
public class MachineOpenHelper extends SQLiteOpenHelper {

	private static final int DATABASE_VERSION = 12;
	private static final String DATABASE_NAME = "LIMBO";
	private static final String MACHINE_TABLE_NAME = "machines";
	// COlumns
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
	public static final String NIC_CONFIG = "NICCONFIG";
	public static final String VGA = "VGA";
	public static final String SOUNDCARD_CONFIG = "SOUNDCARD";
	public static final String HDCACHE_CONFIG = "HDCONFIG";
	public static final String DISABLE_ACPI = "DISABLE_ACPI";
	public static final String DISABLE_HPET = "DISABLE_HPET";
	public static final String DISABLE_FD_BOOT_CHK = "DISABLE_FD_BOOT_CHK";
	public static final String ENABLE_USBMOUSE = "ENABLE_USBMOUSE";
	public static final String STATUS = "STATUS";
	public static final String LASTUPDATED = "LAST_UPDATED";
	public static final String PAUSED = "PAUSED";
	public static final String EXTRA_PARAMS = "EXTRA_PARAMS";
	
	// Create DDL
	private static final String MACHINE_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS " + MACHINE_TABLE_NAME + " ("
			+ MACHINE_NAME + " TEXT , " + SNAPSHOT_NAME + " TEXT , " + CPU + " TEXT, " + ARCH + " TEXT, " + MEMORY
			+ " TEXT, " + FDA + " TEXT, " + FDB + " TEXT, " + CDROM + " TEXT, " + HDA + " TEXT, " + HDB + " TEXT, "
			+ HDC + " TEXT, " + HDD + " TEXT, " + BOOT_CONFIG + " TEXT, " + NET_CONFIG + " TEXT, " + NIC_CONFIG
			+ " TEXT, " + VGA + " TEXT, " + SOUNDCARD_CONFIG + " TEXT, " + HDCACHE_CONFIG + " TEXT, " + DISABLE_ACPI
			+ " INTEGER, " + DISABLE_HPET + " INTEGER, " + ENABLE_USBMOUSE + " INTEGER, " + STATUS + " TEXT, "
			+ LASTUPDATED + " DATE, " + KERNEL + " INTEGER, " + INITRD + " TEXT, " + APPEND + " TEXT, " + CPUNUM
			+ " INTEGER, " + MACHINE_TYPE + " TEXT, " + DISABLE_FD_BOOT_CHK + " INTEGER, " + SD + " TEXT, " + PAUSED
			+ " INTEGER, " 
			+ SHARED_FOLDER + " TEXT, "
			+ SHARED_FOLDER_MODE + " INTEGER, "
			+ EXTRA_PARAMS + " TEXT "
			+ ");";

	private final Activity activity;
	private String TAG = "MachineOpenHelper";

	public MachineOpenHelper(Activity activity) {
		super((Context) activity, DATABASE_NAME, null, DATABASE_VERSION);
		this.activity = activity;
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		db.execSQL(MACHINE_TABLE_CREATE);

	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		Log.w("machineOpenHelper", "Upgrading database from version " + oldVersion + " to " + newVersion);
		if (newVersion >= 3 && oldVersion <= 2) {
			Log.w("machineOpenHelper", "Upgrading database from version " + oldVersion + " to " + newVersion);
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.KERNEL + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.INITRD + " TEXT;");

		}
		if (newVersion >= 4 && oldVersion <= 3) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.CPUNUM + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.MACHINE_TYPE + " TEXT;");

		}
		if (newVersion >= 5 && oldVersion <= 4) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.HDC + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.HDD + " TEXT;");

		}
		if (newVersion >= 6 && oldVersion <= 5) {
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.APPEND + " TEXT;");

		}
		if (newVersion >= 7 && oldVersion <= 6) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.DISABLE_FD_BOOT_CHK + " INTEGER;");

		}
		if (newVersion >= 8 && oldVersion <= 7) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.ARCH + " TEXT;");

		}
		if (newVersion >= 9 && oldVersion <= 8) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.SD + " TEXT;");

		}
		if (newVersion >= 10 && oldVersion <= 9) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.PAUSED + " INTEGER;");

		}
		if (newVersion >= 11 && oldVersion <= 10) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.SHARED_FOLDER + " TEXT;");
			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.SHARED_FOLDER_MODE + " INTEGER;");

		}
		if (newVersion >= 12 && oldVersion <= 11) {

			db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + this.EXTRA_PARAMS + " TEXT;");

		}
	}

	public synchronized int insertMachine(Machine myMachine) {
		int seqnum = -1;
		SQLiteDatabase db = this.getWritableDatabase();
		// Insert filename in db
		// Log.v("DB", "insert urlPath: " + filename);
		ContentValues stateValues = new ContentValues();
		stateValues.put(MACHINE_NAME, myMachine.machinename);
		stateValues.put(SNAPSHOT_NAME, myMachine.snapshot_name);
		stateValues.put(this.CPU, myMachine.cpu);
		stateValues.put(this.CPUNUM, myMachine.cpuNum);
		stateValues.put(this.MEMORY, myMachine.memory);
		stateValues.put(this.HDA, myMachine.hda_img_path);
		stateValues.put(this.HDB, myMachine.hdb_img_path);
		stateValues.put(this.HDC, myMachine.hdc_img_path);
		stateValues.put(this.HDD, myMachine.hdd_img_path);
		stateValues.put(this.CDROM, myMachine.cd_iso_path);
		stateValues.put(this.FDA, myMachine.fda_img_path);
		stateValues.put(this.FDB, myMachine.fdb_img_path);
		stateValues.put(this.SHARED_FOLDER, myMachine.shared_folder);
		stateValues.put(this.SHARED_FOLDER_MODE, myMachine.shared_folder_mode);
		stateValues.put(this.BOOT_CONFIG, myMachine.bootdevice);
		stateValues.put(this.NET_CONFIG, myMachine.net_cfg);
		stateValues.put(this.NIC_CONFIG, myMachine.nic_driver);
		stateValues.put(this.VGA, myMachine.vga_type);
		stateValues.put(this.HDCACHE_CONFIG, myMachine.hd_cache);
		stateValues.put(this.DISABLE_ACPI, myMachine.disableacpi);
		stateValues.put(this.DISABLE_HPET, myMachine.disablehpet);
		stateValues.put(this.DISABLE_FD_BOOT_CHK, myMachine.disablefdbootchk);
		stateValues.put(this.ENABLE_USBMOUSE, myMachine.bluetoothmouse);
		stateValues.put(this.SOUNDCARD_CONFIG, myMachine.soundcard);
		stateValues.put(this.KERNEL, myMachine.kernel);
		stateValues.put(this.INITRD, myMachine.initrd);
		stateValues.put(this.APPEND, myMachine.append);
		stateValues.put(this.MACHINE_TYPE, myMachine.machine_type);
		stateValues.put(this.ARCH, myMachine.arch);
		stateValues.put(this.EXTRA_PARAMS, myMachine.extra_params);

		SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date date = new Date();
		stateValues.put(this.LASTUPDATED, dateFormat.format(date));
		stateValues.put(this.STATUS, Config.STATUS_CREATED);

		try {
			seqnum = (int) db.insertOrThrow(MACHINE_TABLE_NAME, null, stateValues);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while Insert machine: " + e.getMessage());
		}
		Log.v(TAG, "Inserting Machine: " + myMachine.machinename + " : " + seqnum);
		db.close();
		return seqnum;
	}

	public int deleteMachines() {
		int rowsAffected = 0;
		SQLiteDatabase db = this.getReadableDatabase();
		// Insert arrFiles in
		try {
			db.delete(this.MACHINE_TABLE_NAME, this.STATUS + "=\"" + Config.STATUS_CREATED + "\"" + "or " + this.STATUS
					+ "=\"" + Config.STATUS_PAUSED + "\"", null);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while deleting Machines..............: " + e.getMessage());
		}
		db.close();

		return rowsAffected;
	}

	public synchronized int updateStatus(Machine myMachine, int status) {
		int seqnum = -1;
		SQLiteDatabase db = this.getReadableDatabase();
		// Insert filename in db
		// Log.v("DB seq: " + seqnum, "Updating status............... " +
		// status);
		ContentValues stateValues = new ContentValues();
		stateValues.put(this.STATUS, status);
		SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date date = new Date();
		stateValues.put(this.LASTUPDATED, dateFormat.format(date));
		try {
			seqnum = (int) db.update(this.MACHINE_TABLE_NAME, stateValues,
					this.MACHINE_NAME + "=\"" + myMachine.machinename + "\"", null);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while updating Machine status...............: " + e.getMessage());
		}
		db.close();
		return seqnum;
	}

	public int update(Machine myMachine, String colname, String value) {
		if (myMachine == null)
			return -1;
		int rows = -1;
		SQLiteDatabase db = this.getWritableDatabase();

		//Log.v("DB", "Update Machine: " + myMachine.machinename + " column: " + colname + "=" + value);
		ContentValues stateValues = new ContentValues();
		stateValues.put(colname, value);

		try {
			db.beginTransaction();
			rows = (int) db.update(this.MACHINE_TABLE_NAME, stateValues,
					this.MACHINE_NAME + "=\"" + myMachine.machinename + "\"" + " and " + this.SNAPSHOT_NAME + "=\"\"",
					null);
			db.setTransactionSuccessful();
			// Log.v("DB", "ret = " + rows);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while Updating value: " + e.getMessage());
		} finally {
			db.endTransaction();
		}
		db.close();
		return rows;
	}

	public Machine getMachine(String machine, String snapshot) {
		String qry = "select " + this.MACHINE_NAME + " , " + this.CPU + " , " + this.MEMORY + " , " + this.CDROM + " , "
				+ this.FDA + " , " + this.FDB + " , " + this.HDA + " , " + this.HDB + " , " + this.HDC + " , "
				+ this.HDD + " , " + this.NET_CONFIG + " , " + this.NIC_CONFIG + " , " + this.VGA + " , "
				+ this.SOUNDCARD_CONFIG + " , " + this.HDCACHE_CONFIG + " , " + this.DISABLE_ACPI + " , "
				+ this.DISABLE_HPET + " , " + this.ENABLE_USBMOUSE + " , " + this.SNAPSHOT_NAME + " , "
				+ this.BOOT_CONFIG + " , " + this.KERNEL + " , " + this.INITRD + " , " + this.APPEND + " , "
				+ this.CPUNUM + " , " + this.MACHINE_TYPE + " , " + this.DISABLE_FD_BOOT_CHK + " , " + this.ARCH + " , "
				+ this.PAUSED + " , " + this.SD + " , " + this.SHARED_FOLDER + " , " + this.SHARED_FOLDER_MODE + " , "
				+ this.EXTRA_PARAMS
				+ " from " + this.MACHINE_TABLE_NAME + " where " + this.STATUS + " in ( "
				+ Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " " + " ) " + " and " + this.MACHINE_NAME + "=\""
				+ machine + "\"" + " and " + this.SNAPSHOT_NAME + "=\"" + snapshot + "\"" + ";";

		Machine myMachine = null;
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (cur.isAfterLast() == false) {
			String machinename = cur.getString(0);
			String cpu = cur.getString(1);
			int mem = (int) cur.getInt(2);
			String cdrom = cur.getString(3);
			String fda = cur.getString(4);
			String fdb = cur.getString(5);
			String hda = cur.getString(6);
			String hdb = cur.getString(7);
			String hdc = cur.getString(8);
			String hdd = cur.getString(9);
			String net = cur.getString(10);
			String nic = cur.getString(11);
			String vga = cur.getString(12);
			String snd = cur.getString(13);
			String hdcache = cur.getString(14);
			int acpi = (int) cur.getInt(15);
			int hpet = (int) cur.getInt(16);
			int usbmouse = (int) cur.getInt(17);
			String snapshotStr = cur.getString(18);
			String bootdev = cur.getString(19);
			String kernel = cur.getString(20);
			String initrd = cur.getString(21);
			String append = cur.getString(22);
			int cpuNum = (int) cur.getInt(23);
			String machineType = cur.getString(24);
			int fdbootchk = (int) cur.getInt(25);
			String arch = cur.getString(26);
			int paused = cur.getInt(27);
			String sd = cur.getString(28);
			String sharedFolder = cur.getString(29);
			int sharedFolderMode = cur.getInt(30);
			String extraParams = cur.getString(31);

			// Log.v("DB", "Got Machine: " + machinename);
			// Log.v("DB", "Got cpu: " + cpu);
			myMachine = new Machine(machinename);
			myMachine.cpu = cpu;
			myMachine.memory = mem;
			myMachine.cd_iso_path = cdrom;

			// HDD
			myMachine.hda_img_path = hda;
			myMachine.hdb_img_path = hdb;
			myMachine.hdc_img_path = hdc;
			myMachine.hdd_img_path = hdd;

			// Shared Folder
			myMachine.shared_folder = sharedFolder;
			myMachine.shared_folder_mode = sharedFolderMode;
			
			// FDA
			myMachine.fda_img_path = fda;
			myMachine.fdb_img_path = fdb;
			
			//SD
			myMachine.sd_img_path = sd;
			
			myMachine.vga_type = vga;
			myMachine.soundcard = snd;
			myMachine.net_cfg = net;
			myMachine.nic_driver = nic;
			myMachine.disableacpi = acpi;
			myMachine.disablehpet = hpet;
			myMachine.disablefdbootchk = fdbootchk;
			myMachine.bluetoothmouse = usbmouse;
			myMachine.hd_cache = hdcache;
			myMachine.snapshot_name = snapshotStr;
			myMachine.bootdevice = bootdev;
			myMachine.kernel = kernel;
			myMachine.initrd = initrd;
			myMachine.append = append;
			myMachine.cpuNum = cpuNum;
			myMachine.machine_type = machineType;
			myMachine.arch = arch;

			myMachine.paused = paused;
			
			myMachine.extra_params = extraParams;

			break;
		}
		cur.close();
		db.close();
		return myMachine;
	}

	public ArrayList<String> getMachines() {
		String qry = "select " + this.MACHINE_NAME + " " + " from " + this.MACHINE_TABLE_NAME + " where " + this.STATUS
				+ " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " " + " ) " + " and "
				+ this.SNAPSHOT_NAME + " = " + "\"\" " + ";";

		ArrayList<String> arrStr = new ArrayList<String>();
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (cur.isAfterLast() == false) {
			String urlPath = cur.getString(0);

			cur.moveToNext();
			arrStr.add(urlPath);
		}
		cur.close();
		db.close();
		return arrStr;
	}

	public int deleteMachine(Machine b) {
		int rowsAffected = 0;
		SQLiteDatabase db = this.getReadableDatabase();
		// Insert arrFiles in
		try {
			db.delete(this.MACHINE_TABLE_NAME, this.MACHINE_NAME + "=\"" + b.machinename + "\"" + " and "
					+ this.SNAPSHOT_NAME + "=\"" + b.snapshot_name + "\"", null);
		} catch (Exception e) {
			// catch code
			Log.e(TAG, "Error while deleting VM: " + e.getMessage());

		}
		db.close();

		return rowsAffected;
	}

	public ArrayList<String> getSnapshots(Machine currMachine) {
		String qry = "select " + this.SNAPSHOT_NAME + " " + " from " + this.MACHINE_TABLE_NAME + " where " + this.STATUS
				+ " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " ) " + " and " + this.MACHINE_NAME
				+ " = " + "\"" + currMachine.machinename + "\" " + " and " + this.SNAPSHOT_NAME + " != " + "\"\" "
				+ ";";

		ArrayList<String> arrStr = new ArrayList<String>();
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		while (cur.isAfterLast() == false) {
			String urlPath = cur.getString(0);

			cur.moveToNext();
			arrStr.add(urlPath);
		}
		cur.close();
		db.close();
		return arrStr;
	}

	public void insertMachines(String machines) {
		// TODO Auto-generated method stub

	}

	public String exportMachines() {
		String qry = "select " + this.MACHINE_NAME + " , " + this.CPU + " , " + this.MEMORY + " , " + this.CDROM + " , "
				+ this.FDA + " , " + this.FDB + " , " + this.HDA + " , " + this.HDB + " , " + this.HDC + " , "
				+ this.HDD + " , " + this.NET_CONFIG + " , " + this.NIC_CONFIG + " , " + this.VGA + " , "
				+ this.SOUNDCARD_CONFIG + " , " + this.HDCACHE_CONFIG + " , " + this.DISABLE_ACPI + " , "
				+ this.DISABLE_HPET + " , " + this.ENABLE_USBMOUSE + " , " + this.SNAPSHOT_NAME + " , "
				+ this.BOOT_CONFIG + " , " + this.KERNEL + " , " + this.INITRD + " , " + this.APPEND + " , "
				+ this.CPUNUM + " , " + this.MACHINE_TYPE + " , " + this.DISABLE_FD_BOOT_CHK + " , " + this.ARCH
				+ " from " + this.MACHINE_TABLE_NAME + "; ";

		String arrStr = "";
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cur = db.rawQuery(qry, null);

		cur.moveToFirst();
		String urlPath1 = "";
		for (int i = 0; i < cur.getColumnCount(); i++) {
			urlPath1 += ("\"" + cur.getColumnName(i) + "\"");
			if (i < cur.getColumnCount() - 1) {
				urlPath1 += ",";
			}
		}
		arrStr += (urlPath1 + "\n");
		while (cur.isAfterLast() == false) {
			String urlPath = "";
			for (int i = 0; i < cur.getColumnCount(); i++) {
				urlPath += ("\"" + cur.getString(i) + "\"");
				if (i < cur.getColumnCount() - 1) {
					urlPath += ",";
				}

			}
			arrStr += (urlPath + "\n");
			cur.moveToNext();
		}
		cur.close();
		db.close();
		return arrStr;
	}

}
