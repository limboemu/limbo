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
package com.max2idea.android.limbo.machine;

import android.annotation.SuppressLint;
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
import java.util.Observable;
import java.util.Observer;

/**
 * DAO implementation for storing our machines into an SQLite database
 */
public class MachineOpenHelper extends SQLiteOpenHelper implements IMachineDatabase, Observer {
    private static final String TAG = "MachineOpenHelper";

    private static final int DATABASE_VERSION = 15;
    private static final String DATABASE_NAME = "LIMBO";
    private static final String MACHINE_TABLE_NAME = "machines";

    private static final String MACHINE_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS " + MACHINE_TABLE_NAME + " ("
            + MachineProperty.MACHINE_NAME.name() + " TEXT , " + MachineProperty.SNAPSHOT_NAME.name() + " TEXT , " + MachineProperty.CPU.name() + " TEXT, " + MachineProperty.ARCH.name() + " TEXT, " + MachineProperty.MEMORY.name()
            + " TEXT, " + MachineProperty.FDA.name() + " TEXT, " + MachineProperty.FDB.name() + " TEXT, " + MachineProperty.CDROM.name() + " TEXT, " + MachineProperty.HDA.name() + " TEXT, " + MachineProperty.HDB.name() + " TEXT, "
            + MachineProperty.HDC.name() + " TEXT, " + MachineProperty.HDD.name() + " TEXT, " + MachineProperty.BOOT_CONFIG.name() + " TEXT, " + MachineProperty.NETCONFIG.name() + " TEXT, " + MachineProperty.NICCONFIG.name()
            + " TEXT, " + MachineProperty.VGA.name() + " TEXT, " + MachineProperty.SOUNDCARD.name() + " TEXT, " + MachineProperty.HDCONFIG.name() + " TEXT, " + MachineProperty.DISABLE_ACPI.name()
            + " INTEGER, " + MachineProperty.DISABLE_HPET.name() + " INTEGER, " + MachineProperty.ENABLE_USBMOUSE.name() + " INTEGER, " + MachineProperty.STATUS.name() + " TEXT, "
            + MachineProperty.LAST_UPDATED.name() + " DATE, " + MachineProperty.KERNEL.name() + " INTEGER, " + MachineProperty.INITRD.name() + " TEXT, " + MachineProperty.APPEND.name() + " TEXT, " + MachineProperty.CPUNUM.name()
            + " INTEGER, " + MachineProperty.MACHINETYPE.name() + " TEXT, " + MachineProperty.DISABLE_FD_BOOT_CHK.name() + " INTEGER, " + MachineProperty.SD.name() + " TEXT, " + MachineProperty.PAUSED.name()
            + " INTEGER, " + MachineProperty.SHARED_FOLDER.name() + " TEXT, " + MachineProperty.SHARED_FOLDER_MODE.name() + " INTEGER, " + MachineProperty.EXTRA_PARAMS.name() + " TEXT, "
            + MachineProperty.HOSTFWD.name() + " TEXT, " + MachineProperty.GUESTFWD.name() + " TEXT, " + MachineProperty.UI.name() + " TEXT, " + MachineProperty.DISABLE_TSC.name() + " INTEGER, "
            + MachineProperty.MOUSE.name() + " TEXT, " + MachineProperty.KEYBOARD.name() + " TEXT, " + MachineProperty.ENABLE_MTTCG.name() + " INTEGER, " + MachineProperty.ENABLE_KVM.name() + " INTEGER "
            + ");";

    private static MachineOpenHelper sInstance;
    private SQLiteDatabase db;

    private MachineOpenHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        getDB();
    }

    static MachineOpenHelper getInstance() {
        return sInstance;
    }

    public static synchronized void initialize(Context context) {
        if (sInstance == null) {
            sInstance = new MachineOpenHelper(context.getApplicationContext());
            sInstance.setWriteAheadLoggingEnabled(true);
        }
    }

    private synchronized void getDB() {
        if (db == null)
            db = getWritableDatabase();
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(MACHINE_TABLE_CREATE);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        Log.w("machineOpenHelper", "Upgrading database from version " + oldVersion + " to " + newVersion);
        if (newVersion >= 3 && oldVersion <= 2) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.KERNEL + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.INITRD + " TEXT;");
        }

        if (newVersion >= 4 && oldVersion <= 3) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.CPUNUM + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.MACHINETYPE + " TEXT;");
        }

        if (newVersion >= 5 && oldVersion <= 4) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.HDC + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.HDD + " TEXT;");
        }

        if (newVersion >= 6 && oldVersion <= 5) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.APPEND + " TEXT;");
        }

        if (newVersion >= 7 && oldVersion <= 6) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.DISABLE_FD_BOOT_CHK + " INTEGER;");
        }

        if (newVersion >= 8 && oldVersion <= 7) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.ARCH + " TEXT;");
        }

        if (newVersion >= 9 && oldVersion <= 8) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.SD + " TEXT;");
        }

        if (newVersion >= 10 && oldVersion <= 9) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.PAUSED + " INTEGER;");
        }

        if (newVersion >= 11 && oldVersion <= 10) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.SHARED_FOLDER + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.SHARED_FOLDER_MODE + " INTEGER;");
        }

        if (newVersion >= 12 && oldVersion <= 11) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.EXTRA_PARAMS + " TEXT;");
        }

        if (newVersion >= 13 && oldVersion <= 12) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.HOSTFWD + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.GUESTFWD + " TEXT;");
        }

        if (newVersion >= 14 && oldVersion <= 13) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.UI + " TEXT;");
        }

        if (newVersion >= 15 && oldVersion <= 14) {
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.DISABLE_TSC + " INTEGER;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.MOUSE + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.KEYBOARD + " TEXT;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.ENABLE_MTTCG + " INTEGER;");
            db.execSQL("ALTER TABLE " + MACHINE_TABLE_NAME + " ADD COLUMN " + MachineProperty.ENABLE_KVM + " INTEGER;");
        }
    }

    public synchronized int insertMachine(Machine machine) {
        int seqnum = -1;
        SQLiteDatabase db = getWritableDatabase();

        Log.v("DB", "insert machine: " + machine.getName());
        ContentValues stateValues = new ContentValues();
        stateValues.put(MachineProperty.MACHINE_NAME.name(), machine.getName()); //Legacy
        stateValues.put(MachineProperty.CPU.name(), machine.getCpu());
        stateValues.put(MachineProperty.CPUNUM.name(), machine.getCpuNum());
        stateValues.put(MachineProperty.MEMORY.name(), machine.getMemory());
        stateValues.put(MachineProperty.HDA.name(), machine.getHdaImagePath());
        stateValues.put(MachineProperty.HDB.name(), machine.getHdbImagePath());
        stateValues.put(MachineProperty.HDC.name(), machine.getHdcImagePath());
        stateValues.put(MachineProperty.HDD.name(), machine.getHddImagePath());
        stateValues.put(MachineProperty.CDROM.name(), machine.getCdImagePath());
        stateValues.put(MachineProperty.FDA.name(), machine.getFdaImagePath());
        stateValues.put(MachineProperty.FDB.name(), machine.getFdbImagePath());
        stateValues.put(MachineProperty.SHARED_FOLDER.name(), machine.getSharedFolderPath());
        stateValues.put(MachineProperty.SHARED_FOLDER_MODE.name(), machine.getShared_folder_mode());
        stateValues.put(MachineProperty.BOOT_CONFIG.name(), machine.getBootDevice());
        stateValues.put(MachineProperty.NETCONFIG.name(), machine.getNetwork());
        stateValues.put(MachineProperty.NICCONFIG.name(), machine.getNetworkCard());
        stateValues.put(MachineProperty.VGA.name(), machine.getVga());
        stateValues.put(MachineProperty.DISABLE_ACPI.name(), machine.getDisableAcpi());
        stateValues.put(MachineProperty.DISABLE_HPET.name(), machine.getDisableHPET());
        stateValues.put(MachineProperty.DISABLE_TSC.name(), machine.getDisableTSC());
        stateValues.put(MachineProperty.DISABLE_FD_BOOT_CHK.name(), machine.getDisableFdBootChk());
        stateValues.put(MachineProperty.SOUNDCARD.name(), machine.getSoundCard());
        stateValues.put(MachineProperty.KERNEL.name(), machine.getKernel());
        stateValues.put(MachineProperty.INITRD.name(), machine.getInitRd());
        stateValues.put(MachineProperty.APPEND.name(), machine.getAppend());
        stateValues.put(MachineProperty.MACHINETYPE.name(), machine.getMachineType());
        stateValues.put(MachineProperty.ARCH.name(), machine.getArch());
        stateValues.put(MachineProperty.EXTRA_PARAMS.name(), machine.getExtraParams());
        stateValues.put(MachineProperty.HOSTFWD.name(), machine.getHostFwd());
        stateValues.put(MachineProperty.GUESTFWD.name(), machine.getGuestFwd());
        stateValues.put(MachineProperty.UI.name(), machine.getEnableVNC() == 1 ? "VNC" : "SDL");
        stateValues.put(MachineProperty.MOUSE.name(), machine.getMouse());
        stateValues.put(MachineProperty.KEYBOARD.name(), machine.getKeyboard());
        stateValues.put(MachineProperty.ENABLE_MTTCG.name(), machine.getEnableMTTCG());
        stateValues.put(MachineProperty.ENABLE_KVM.name(), machine.getEnableKVM());

        @SuppressLint("SimpleDateFormat")
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

        Date date = new Date();
        stateValues.put(MachineProperty.LAST_UPDATED.name(), dateFormat.format(date));
        stateValues.put(MachineProperty.STATUS.name(), Config.STATUS_CREATED);

        try {
            seqnum = (int) db.insertOrThrow(MACHINE_TABLE_NAME, null, stateValues);
            Log.v(TAG, "Inserted Machine: " + machine.getName() + " : " + seqnum);
        } catch (Exception e) {
            // catch code
            Log.e(TAG, "Error while Insert machine: " + e.getMessage());
            e.printStackTrace();
        }
        return seqnum;
    }

    public void updateMachineFieldAsync(final Machine machine, final MachineProperty property,
                                 final String value) {
        Thread t = new Thread(new Runnable() {
            public void run() {
                updateMachineField(machine, property, value);
            }
        });
        t.start();
    }

    public void updateMachineField(Machine machine, MachineProperty property, String value) {
        if (machine == null)
            return;
        ContentValues stateValues = new ContentValues();
        stateValues.put(property.name(), value);
        try {
            db.beginTransaction();
            db.update(MACHINE_TABLE_NAME, stateValues,
                    MachineProperty.MACHINE_NAME.name() + "=\"" + machine.getName() + "\" ",
                    null);
            db.setTransactionSuccessful();
        } catch (Exception e) {
            Log.e(TAG, "Error while Updating value: " + e.getMessage());
            if (Config.debug)
                e.printStackTrace();
        } finally {
            db.endTransaction();
        }
    }

    public Machine getMachine(String machine) {
        String qry = "select "
                + MachineProperty.MACHINE_NAME + " , " + MachineProperty.CPU + " , " + MachineProperty.MEMORY + " , " + MachineProperty.CDROM + " , " + MachineProperty.FDA
                + " , " + MachineProperty.FDB + " , " + MachineProperty.HDA + " , " + MachineProperty.HDB + " , " + MachineProperty.HDC + " , " + MachineProperty.HDD + " , "
                + MachineProperty.NETCONFIG + " , " + MachineProperty.NICCONFIG + " , " + MachineProperty.VGA + " , " + MachineProperty.SOUNDCARD + " , "
                + MachineProperty.HDCONFIG + " , " + MachineProperty.DISABLE_ACPI + " , " + MachineProperty.DISABLE_HPET + " , "
                + MachineProperty.ENABLE_USBMOUSE + " , " + MachineProperty.SNAPSHOT_NAME + " , " + MachineProperty.BOOT_CONFIG + " , " + MachineProperty.KERNEL
                + " , " + MachineProperty.INITRD + " , " + MachineProperty.APPEND + " , " + MachineProperty.CPUNUM + " , " + MachineProperty.MACHINETYPE + " , "
                + MachineProperty.DISABLE_FD_BOOT_CHK + " , " + MachineProperty.ARCH + " , " + MachineProperty.PAUSED + " , " + MachineProperty.SD + " , "
                + MachineProperty.SHARED_FOLDER + " , " + MachineProperty.SHARED_FOLDER_MODE + " , " + MachineProperty.EXTRA_PARAMS + " , "
                + MachineProperty.HOSTFWD + " , " + MachineProperty.GUESTFWD + " , " + MachineProperty.UI + ", " + MachineProperty.DISABLE_TSC + ", "
                + MachineProperty.MOUSE + ", " + MachineProperty.KEYBOARD + ", " + MachineProperty.ENABLE_MTTCG + ", " + MachineProperty.ENABLE_KVM
                + " from " + MACHINE_TABLE_NAME
                + " where " + MachineProperty.STATUS + " in ( " + Config.STATUS_CREATED + " , " + Config.STATUS_PAUSED + " "
                + " ) " + " and " + MachineProperty.MACHINE_NAME + "=\"" + machine + "\"" + ";";

        Machine myMachine = null;

        Cursor cur = db.rawQuery(qry, null);

        cur.moveToFirst();
        if (!cur.isAfterLast()) {
            String machinename = cur.getString(0);
            myMachine = new Machine(machinename, false);

            myMachine.setCpu(cur.getString(1));
            myMachine.setMemory(cur.getInt(2));
            myMachine.setCdImagePath(cur.getString(3));
            if (myMachine.getCdImagePath() != null)
                myMachine.setEnableCDROM(true);

            myMachine.setFdaImagePath(cur.getString(4));
            if (myMachine.getFdaImagePath() != null)
                myMachine.setEnableFDA(true);
            myMachine.setFdbImagePath(cur.getString(5));
            if (myMachine.getFdbImagePath() != null)
                myMachine.setEnableFDB(true);

            myMachine.setHdaImagePath(cur.getString(6));
            myMachine.setHdbImagePath(cur.getString(7));
            myMachine.setHdcImagePath(cur.getString(8));
            myMachine.setHddImagePath(cur.getString(9));

            myMachine.setNetwork(cur.getString(10));
            myMachine.setNetworkCard(cur.getString(11));
            myMachine.setVga(cur.getString(12));
            myMachine.setSoundCard(cur.getString(13));
            myMachine.setDisableACPI(cur.getInt(15));
            myMachine.setDisableHPET(cur.getInt(16));
            myMachine.setBootDevice(cur.getString(19));
            myMachine.setKernel(cur.getString(20));
            myMachine.setInitRd(cur.getString(21));
            myMachine.setAppend(cur.getString(22));
            myMachine.setCpuNum(cur.getInt(23));
            myMachine.setMachineType(cur.getString(24));
            myMachine.setDisableFdBootChk(cur.getInt(25));
            myMachine.setArch(cur.getString(26));
            myMachine.setPaused(cur.getInt(27));

            myMachine.setSdImagePath(cur.getString(28));
            if (myMachine.getSdImagePath() != null)
                myMachine.setEnableSD(true);

            myMachine.setSharedFolderPath(cur.getString(29));
            myMachine.setShared_folder_mode(1); //hard drives are always Read/Write
            myMachine.setExtraParams(cur.getString(31));
            myMachine.setHostFwd(cur.getString(32));
            myMachine.setGuestFwd(cur.getString(33));
            myMachine.setEnableVNC(cur.getString(34).equals("VNC") ? 1 : 0);
            myMachine.setDisableTSC(cur.getInt(35));
            myMachine.setMouse(cur.getString(36));
            myMachine.setKeyboard(cur.getString(37));
            myMachine.setEnableMTTCG(cur.getInt(38));
            myMachine.setEnableKVM(cur.getInt(39));
        }
        cur.close();

        return myMachine;
    }

    public ArrayList<String> getMachineNames() {
        String qry = "select " + MachineProperty.MACHINE_NAME + " " + " from " + MACHINE_TABLE_NAME
                + " where " + MachineProperty.STATUS + " in ( " + Config.STATUS_CREATED + " , "
                + Config.STATUS_PAUSED + " " + " ) order by 1; ";

        ArrayList<String> arrStr = new ArrayList<>();
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

    public boolean deleteMachine(Machine machine) {
        int rowsAffected = 0;
        try {
            rowsAffected = db.delete(MACHINE_TABLE_NAME, MachineProperty.MACHINE_NAME + "=\"" + machine.getName() + "\"", null);
        } catch (Exception e) {
            Log.e(TAG, "Error while deleting VM: " + e.getMessage());
            if (Config.debug)
                e.printStackTrace();
        }
        return rowsAffected>0;
    }

    String exportMachines() {
        String qry = "select "
                + MachineProperty.MACHINE_NAME + " , " + MachineProperty.CPU + " , " + MachineProperty.MEMORY + " , " + MachineProperty.CDROM + " , " + MachineProperty.FDA
                + " , " + MachineProperty.FDB + " , " + MachineProperty.HDA + " , " + MachineProperty.HDB + " , " + MachineProperty.HDC + " , " + MachineProperty.HDD + " , "
                + MachineProperty.NETCONFIG + " , " + MachineProperty.NICCONFIG + " , " + MachineProperty.VGA + " , " + MachineProperty.SOUNDCARD + " , "
                + MachineProperty.HDCONFIG + " , " + MachineProperty.DISABLE_ACPI + " , " + MachineProperty.DISABLE_HPET + " , "
                + MachineProperty.ENABLE_USBMOUSE + " , " + MachineProperty.SNAPSHOT_NAME + " , " + MachineProperty.BOOT_CONFIG + " , " + MachineProperty.KERNEL
                + " , " + MachineProperty.INITRD + " , " + MachineProperty.APPEND + " , " + MachineProperty.CPUNUM + " , " + MachineProperty.MACHINETYPE + " , "
                + MachineProperty.DISABLE_FD_BOOT_CHK + " , " + MachineProperty.ARCH + " , " + MachineProperty.PAUSED + " , " + MachineProperty.SD + " , "
                + MachineProperty.SHARED_FOLDER + " , " + MachineProperty.SHARED_FOLDER_MODE + " , " + MachineProperty.EXTRA_PARAMS + " , "
                + MachineProperty.HOSTFWD + " , " + MachineProperty.GUESTFWD + " , " + MachineProperty.UI + ", " + MachineProperty.DISABLE_TSC + ", "
                + MachineProperty.MOUSE + ", " + MachineProperty.KEYBOARD + ", " + MachineProperty.ENABLE_MTTCG + ", " + MachineProperty.ENABLE_KVM
                // Table
                + " from " + MACHINE_TABLE_NAME + " order by 1; ";

        StringBuilder arrStr = new StringBuilder();
        Cursor cur = db.rawQuery(qry, null);

        cur.moveToFirst();
        StringBuilder headerline = new StringBuilder();
        for (int i = 0; i < cur.getColumnCount(); i++) {
            headerline.append("\"").append(cur.getColumnName(i)).append("\"");
            if (i < cur.getColumnCount() - 1) {
                headerline.append(",");
            }
        }
        arrStr.append(headerline).append("\n");
        while (!cur.isAfterLast()) {
            StringBuilder line = new StringBuilder();
            for (int i = 0; i < cur.getColumnCount(); i++) {
                line.append("\"").append(cur.getString(i)).append("\"");
                if (i < cur.getColumnCount() - 1) {
                    line.append(",");
                }
            }
            arrStr.append(line).append("\n");
            cur.moveToNext();
        }
        cur.close();

        return arrStr.toString();
    }

    @Override
    public void update(Observable observable, Object o) {
        Log.v(TAG, "Observable updated param: " + o);
        Object[] params = (Object[]) o;
        MachineProperty property = (MachineProperty) params[0];
        Object value = params[1];
        switch(property) {
            case UI:
                updateMachineField((Machine) observable, property, ((int) params[1]) == 1?"VNC":"SDL");
                return;
            case OTHER:
                return;
        }
        if(value instanceof Integer)
            updateMachineField((Machine) observable, property, ((int) params[1])+"");
        else
            updateMachineField((Machine) observable, property, (String) params[1]);

    }
}
