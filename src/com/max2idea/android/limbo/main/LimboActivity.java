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

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.libsdl.app.SDLActivity;

import com.limbo.emu.main.R;
import com.max2idea.android.limbo.jni.VMExecutor;
import com.max2idea.android.limbo.main.Config.DebugMode;
import com.max2idea.android.limbo.utils.FavOpenHelper;
import com.max2idea.android.limbo.utils.FileInstaller;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.UIUtils;

import android.androidVNC.ConnectionBean;
import android.androidVNC.VncCanvas;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceManager;
import android.support.v4.provider.DocumentFile;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

//import com.max2idea.android.limbo.main.R;

public class LimboActivity extends Activity {

	private static Installer a;
	public static final String TAG = "LIMBO";
	public View parent;
	public TextView mOutput;
	public static boolean vmStarted = false;
	public static Activity activity = null;
	public static VMExecutor vmexecutor;
	public boolean userPressedUI = false;
	public boolean userPressedCPU = false;
	public boolean userPressedArch = false;
	public boolean userPressedMachineType = false;
	public boolean userPressedCPUNum = false;
	public boolean userPressedMachine = false;
	public boolean userPressedRAM = false;
	public boolean userPressedCDROM = false;
	private boolean machineLoaded;

	// Misc
	protected boolean userPressedHDCfg = false;
	protected boolean userPressedSndCfg = false;
	protected boolean userPressedVGACfg = false;
	protected boolean userPressedNicCfg = false;
	protected boolean userPressedNetCfg = false;
	protected boolean userPressedBootDev = false;

	// Floppy
	protected boolean userPressedFDB = false;
	protected boolean userPressedFDA = false;

	protected boolean userPressedSD = false;

	// HDD
	protected boolean userPressedHDA = false;
	protected boolean userPressedHDB = false;
	protected boolean userPressedHDC = false;
	protected boolean userPressedHDD = false;

	// Advance
	protected boolean userPressedKernel = false;
	protected boolean userPressedInitrd = false;
	protected boolean userPressedACPI = false;
	protected boolean userPressedHPET = false;
	protected boolean userPressedFDBOOTCHK = false;
	protected boolean userPressedBluetoothMouse = false;
	protected boolean userPressedSnapshot = false;
	protected boolean userPressedVNC = false;

	private boolean userPressedHDCacheCfg;
	private boolean userPressedSoundcardCfg;

	protected boolean userPressedOrientation = false;
	protected boolean userPressedKeyboard = false;

	// Static
	private static final int HELP = 0;
	private static final int QUIT = 1;
	private static final int INSTALL = 2;
	private static final int DELETE = 3;
	private static final int EXPORT = 4;
	private static final int IMPORT = 5;
	private static final int CHANGELOG = 6;
	private static final int LICENSE = 7;
	private ImageView mStatus;
	private EditText mDNS;
	private EditText mAppend;
	private boolean timeQuit = false;
	private Object lockTime = new Object();
	public static String currStatus = "READY";
	private TextView mStatusText;
	private WakeLock mWakeLock;
	private WifiLock wlock;
	private static TextWatcher appendChangeListener;
	private static TextWatcher dnsChangeListener;

	public void setUserPressed(boolean pressed) {
		userPressedMachine = pressed;
		userPressedUI = pressed;
		userPressedArch = pressed;
		userPressedMachineType = pressed;
		userPressedCPU = pressed;
		userPressedCPUNum = pressed;

		userPressedRAM = pressed;
		userPressedCDROM = pressed;
		userPressedHDCfg = pressed;
		userPressedSndCfg = pressed;
		userPressedVGACfg = pressed;
		userPressedNicCfg = pressed;
		userPressedNetCfg = pressed;
		userPressedBootDev = pressed;
		userPressedFDB = pressed;
		userPressedFDA = pressed;

		// HDD
		userPressedHDA = pressed;
		userPressedHDB = pressed;
		userPressedHDC = pressed;
		userPressedHDD = pressed;

		userPressedKernel = pressed;
		userPressedInitrd = pressed;
		userPressedACPI = pressed;
		userPressedHPET = pressed;
		this.userPressedFDBOOTCHK = pressed;
		userPressedBluetoothMouse = pressed;
		userPressedSnapshot = pressed;

		userPressedOrientation = pressed;
		userPressedKeyboard = pressed;

		if (pressed) {
			enableListeners();
		} else
			disableListeners();

	}

	// Generic Dialog box

	private void enableListeners() {
		// TODO Auto-generated method stub

		mMachine.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

				if (position == 0) {
					enableNonRemovableDeviceOptions(false);
					enableRemovableDeviceOptions(false);
					mVNCAllowExternal.setEnabled(false);

				} else if (position == 1) {
					mMachine.setSelection(0);
					promptMachineName(activity);
					mVNCAllowExternal.setEnabled(true);

				} else {
					String machine = (String) ((ArrayAdapter<?>) mMachine.getAdapter()).getItem(position);

					loadMachine(machine, "");
					populateSnapshot();
					mVNCAllowExternal.setEnabled(true);

				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mArch.setOnItemSelectedListener(new OnItemSelectedListener() {

			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				// your code here

				String arch = (String) ((ArrayAdapter<?>) mArch.getAdapter()).getItem(position);

				if (userPressedArch) {
					currMachine.arch = arch;
					int ret = machineDB.update(currMachine, MachineOpenHelper.ARCH, arch);

					if (currMachine.arch.equals("ARM")) {

						if (!machineLoaded) {
							populateMachineType("integratorcp");
							populateCPUs("arm926");
							populateNetDevices("smc91c111");
						}

					} else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

						if (!machineLoaded) {
							populateMachineType("pc");
							if (currMachine.arch.equals("x86")) {
								populateCPUs("n270");
							} else
								populateCPUs("phenom");
							populateNetDevices("ne2k_pci");
						}

					}
				}

				if (currMachine != null)
					if (currMachine.arch.equals("ARM")) {
						mACPI.setEnabled(false);
						mHPET.setEnabled(false);
						mFDBOOTCHK.setEnabled(false);

					} else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

						mACPI.setEnabled(true);
						mHPET.setEnabled(true);
						mFDBOOTCHK.setEnabled(true);

					}
				machineLoaded = false;
				userPressedArch = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here

				userPressedArch = true;
			}
		});

		mCPU.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				// your code here

				String cpu = (String) ((ArrayAdapter<?>) mCPU.getAdapter()).getItem(position);

				if (userPressedCPU) {
					currMachine.cpu = cpu;
					int ret = machineDB.update(currMachine, MachineOpenHelper.CPU, cpu);
				}
				userPressedCPU = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here
				userPressedCPU = true;
			}
		});

		mMachineType.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String machineType = (String) ((ArrayAdapter<?>) mMachineType.getAdapter()).getItem(position);
				if (userPressedMachineType) {
					currMachine.machine_type = machineType;
					int ret = machineDB.update(currMachine, MachineOpenHelper.MACHINE_TYPE, machineType);
				}
				userPressedMachineType = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
				userPressedMachineType = true;
			}
		});

		mUI.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				// your code here

				String ui = (String) ((ArrayAdapter<?>) mUI.getAdapter()).getItem(position);
				if (userPressedUI) {
					LimboSettingsManager.setLastUI(activity, ui);

				}
				if (position == 0) {
					mVNCAllowExternal.setEnabled(true);
					if (mSnapshot.getSelectedItemPosition() == 0)
						mSoundCardConfig.setEnabled(false);
				} else {
					mVNCAllowExternal.setEnabled(false);
					if (mSnapshot.getSelectedItemPosition() == 0)
						mSoundCardConfig.setEnabled(true);
				}
				userPressedUI = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
				userPressedUI = true;
			}
		});

		mCPUNum.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cpuNum = (String) ((ArrayAdapter<?>) mCPUNum.getAdapter()).getItem(position);
				if (userPressedCPUNum) {
					currMachine.cpuNum = Integer.parseInt(cpuNum);
					int ret = machineDB.update(currMachine, MachineOpenHelper.CPUNUM, cpuNum);
				}

				userPressedCPUNum = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
				userPressedCPUNum = true;
			}
		});

		mRamSize.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String ram = (String) ((ArrayAdapter<?>) mRamSize.getAdapter()).getItem(position);
				if (userPressedRAM) {
					currMachine.memory = Integer.parseInt(ram);
					int ret = machineDB.update(currMachine, MachineOpenHelper.MEMORY, ram);
				}
				userPressedRAM = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {
				userPressedRAM = true;
			}
		});

		mKernel.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String kernel = (String) ((ArrayAdapter<?>) mKernel.getAdapter()).getItem(position);
				if (userPressedKernel && position == 0) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.KERNEL, null);
					currMachine.kernel = null;
				} else if (userPressedKernel && position == 1) {
					browse("kernel");
					mKernel.setSelection(0);
				} else if (userPressedKernel && position > 1) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.KERNEL, kernel);
					currMachine.kernel = kernel;
					// TODO: If Machine is running eject and set floppy img
				}
				userPressedKernel = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mInitrd.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String initrd = (String) ((ArrayAdapter<?>) mInitrd.getAdapter()).getItem(position);
				if (userPressedInitrd && position == 0) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.INITRD, null);
					currMachine.initrd = null;
				} else if (userPressedInitrd && position == 1) {
					browse("initrd");
					mInitrd.setSelection(0);
				} else if (userPressedInitrd && position > 1) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.INITRD, initrd);
					currMachine.initrd = initrd;
					// TODO: If Machine is running eject and set floppy img
				}
				userPressedInitrd = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDA.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hda = (String) ((ArrayAdapter<?>) mHDA.getAdapter()).getItem(position);
				if (userPressedHDA && position == 0 && mHDAenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDA, "");
					currMachine.hda_img_path = "";
				} else if (userPressedHDA && (position == 0 || !mHDAenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDA, null);
					currMachine.hda_img_path = null;
				} else if (userPressedHDA && position == 1 && mHDAenable.isChecked()) {
					promptImageName(activity, "hda");

				} else if (userPressedHDA && position == 2 && mHDAenable.isChecked()) {
					browse("hda");
					mHDA.setSelection(0);
				} else if (userPressedHDA && position > 2 && mHDAenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDA, hda);
					currMachine.hda_img_path = hda;
				}

				userPressedHDA = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDB.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdb = (String) ((ArrayAdapter<?>) mHDB.getAdapter()).getItem(position);

				if (userPressedHDB && position == 0 && mHDBenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDB, "");
					currMachine.hdb_img_path = "";
				} else if (userPressedHDB && (position == 0 || !mHDBenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDB, null);
					currMachine.hdb_img_path = null;
				} else if (userPressedHDB && position == 1 && mHDBenable.isChecked()) {
					promptImageName(activity, "hdb");

				} else if (userPressedHDB && position == 2 && mHDBenable.isChecked()) {
					browse("hdb");
					mHDB.setSelection(0);
				} else if (userPressedHDB && position > 2 && mHDBenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDB, hdb);
					currMachine.hdb_img_path = hdb;
				}

				userPressedHDB = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDC.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdc = (String) ((ArrayAdapter<?>) mHDC.getAdapter()).getItem(position);
				if (userPressedHDC && position == 0 && mHDCenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDC, "");
					currMachine.hdc_img_path = "";
				} else if (userPressedHDC && (position == 0 || !mHDCenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDC, null);
					currMachine.hdc_img_path = null;
				} else if (userPressedHDC && position == 1 && mHDCenable.isChecked()) {
					promptImageName(activity, "hdc");

				} else if (userPressedHDC && position == 2 && mHDCenable.isChecked()) {
					browse("hdc");
					mHDC.setSelection(0);
				} else if (userPressedHDC && position > 2 && mHDCenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDC, hdc);
					currMachine.hdc_img_path = hdc;
				}

				userPressedHDC = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdd = (String) ((ArrayAdapter<?>) mHDD.getAdapter()).getItem(position);
				if (userPressedHDD && position == 0 && mHDDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDD, "");
					currMachine.hdd_img_path = "";
				} else if (userPressedHDD && (position == 0 || !mHDDenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDD, null);
					currMachine.hdd_img_path = null;
				} else if (userPressedHDD && position == 1 && mHDDenable.isChecked()) {
					promptImageName(activity, "hdd");
				} else if (userPressedHDD && position == 2 && mHDDenable.isChecked()) {
					browse("hdd");
					mHDD.setSelection(0);
				} else if (userPressedHDD && position > 2 && mHDDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDD, hdd);
					currMachine.hdd_img_path = hdd;
				}

				userPressedHDD = true;

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mSnapshot.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String snapshot_name = (String) ((ArrayAdapter<?>) mSnapshot.getAdapter()).getItem(position);
				if (userPressedSnapshot && position == 0) {
					currMachine.snapshot_name = "";
					userPressedSnapshot = false;
					loadMachine(currMachine.machinename, currMachine.snapshot_name);
					mStart.setImageResource(R.drawable.play);
				} else if (userPressedSnapshot && position > 0) {
					currMachine.snapshot_name = snapshot_name;
					userPressedSnapshot = false;
					loadMachine(currMachine.machinename, currMachine.snapshot_name);
					mStart.setImageResource(R.drawable.play);
					enableNonRemovableDeviceOptions(false);
					enableRemovableDeviceOptions(false);
					mSnapshot.setEnabled(true);
				} else {
					userPressedSnapshot = true;
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mCD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cd = (String) ((ArrayAdapter<?>) mCD.getAdapter()).getItem(position);
				if (userPressedCDROM && position == 0 && mCDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, "");
					currMachine.cd_iso_path = "";
				} else if (userPressedCDROM && (position == 0 || !mCDenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, null);
					currMachine.cd_iso_path = null;
				} else if (userPressedCDROM && position == 1 && mCDenable.isChecked()) {
					browse("cd");
					mCD.setSelection(0);
				} else if (userPressedCDROM && position > 1 && mCDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, cd);
					currMachine.cd_iso_path = cd;
				}
				if (userPressedCDROM && currStatus.equals("RUNNING") && position > 1 && mCDenable.isChecked()) {
					vmexecutor.change_dev("ide1-cd0", currMachine.cd_iso_path);
				} else if (mCDenable.isChecked() && userPressedCDROM && currStatus.equals("RUNNING") && position == 0) {
					vmexecutor.change_dev("ide1-cd0", null); // Eject
				}
				userPressedCDROM = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mFDA.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String fda = (String) ((ArrayAdapter<?>) mFDA.getAdapter()).getItem(position);
				if (userPressedFDA && position == 0 && mFDAenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, "");
					currMachine.fda_img_path = "";
				} else if (userPressedFDA && (position == 0 || !mFDAenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, null);
					currMachine.fda_img_path = null;
				} else if (userPressedFDA && position == 1 && mFDAenable.isChecked()) {
					browse("fda");
					mFDA.setSelection(0);
				} else if (userPressedFDA && position > 1 && mFDAenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, fda);
					currMachine.fda_img_path = fda;
					// TODO: If Machine is running eject and set floppy img
				}
				if (userPressedFDA && currStatus.equals("RUNNING") && position > 1 && mFDAenable.isChecked()) {
					vmexecutor.change_dev("floppy0", currMachine.fda_img_path);
				} else if (userPressedFDA && currStatus.equals("RUNNING") && position == 0 && mFDAenable.isChecked()) {
					vmexecutor.change_dev("floppy0", null); // Eject
				}

				userPressedFDA = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mFDB.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String fdb = (String) ((ArrayAdapter<?>) mFDB.getAdapter()).getItem(position);
				if (userPressedFDB && position == 0 && mFDBenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, "");
					currMachine.fdb_img_path = "";
				} else if (userPressedFDB && (position == 0 || !mFDBenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, null);
					currMachine.fdb_img_path = null;
				} else if (userPressedFDB && position == 1 && mFDBenable.isChecked()) {
					browse("fdb");
					mFDB.setSelection(0);
				} else if (userPressedFDB && position > 1 && mFDBenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, fdb);
					currMachine.fdb_img_path = fdb;
					// TODO: If Machine is running eject and set floppy img
				}
				if (userPressedFDB && currStatus.equals("RUNNING") && position > 1 && mFDBenable.isChecked()) {
					vmexecutor.change_dev("floppy1", currMachine.fdb_img_path);
				} else if (userPressedFDB && currStatus.equals("RUNNING") && position == 0 && mFDBenable.isChecked()) {
					vmexecutor.change_dev("floppy1", null); // Eject
				}
				userPressedFDB = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mSD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String sd = (String) ((ArrayAdapter<?>) mSD.getAdapter()).getItem(position);
				if (userPressedSD && position == 0 && mSDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.SD, "");
					currMachine.sd_img_path = "";
				}
				if (userPressedSD && (position == 0 || !mSDenable.isChecked())) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.SD, null);
					currMachine.sd_img_path = null;
				} else if (userPressedSD && position == 1 && mSDenable.isChecked()) {
					browse("sd");
					mSD.setSelection(0);
				} else if (userPressedSD && position > 1 && mSDenable.isChecked()) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.SD, sd);
					currMachine.sd_img_path = sd;
					// TODO: If Machine is running eject and set floppy img
				}
				if (userPressedSD && currStatus.equals("RUNNING") && position > 1 && mSDenable.isChecked()) {
					// vmexecutor.change_dev("floppy1",
					// currMachine.sd_img_path);
				} else if (userPressedSD && currStatus.equals("RUNNING") && position == 0 && mSDenable.isChecked()) {
					// vmexecutor.change_dev("floppy1", null); // Eject
				}
				userPressedSD = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mCDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mCD.setEnabled(isChecked);
				enableCDROM = isChecked;
				if (currMachine != null) {
					if (isChecked) {
						currMachine.cd_iso_path = "";
						int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, "");
					} else {
						currMachine.cd_iso_path = null;
						int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, null);
					}
				}

				triggerUpdateSpinner(mCD);

			}

		}

		);

		mHDAenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mHDA.setEnabled(isChecked);
				if (currMachine != null) {
					if (isChecked) {
						currMachine.hda_img_path = "";
					} else {
						currMachine.hda_img_path = null;
					}
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDA, currMachine.hda_img_path);
				}
				triggerUpdateSpinner(mHDA);
			}

		});
		mHDBenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mHDB.setEnabled(isChecked);
				if (currMachine != null) {
					if (isChecked) {
						currMachine.hdb_img_path = "";
					} else {
						currMachine.hdb_img_path = null;
					}
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDB, currMachine.hdb_img_path);
				}
				triggerUpdateSpinner(mHDB);
			}

		});
		mHDCenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mHDC.setEnabled(isChecked);
				if (currMachine != null) {
					if (isChecked) {
						currMachine.hdc_img_path = "";
					} else {
						currMachine.hdc_img_path = null;
					}
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDC, currMachine.hdc_img_path);
				}
				triggerUpdateSpinner(mHDC);
			}

		});
		mHDDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mHDD.setEnabled(isChecked);
				if (currMachine != null) {
					if (isChecked) {
						currMachine.hdd_img_path = "";
					} else {
						currMachine.hdd_img_path = null;
					}
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDD, currMachine.hdd_img_path);
				}
				triggerUpdateSpinner(mHDD);
			}

		});
		mFDAenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mFDA.setEnabled(isChecked);
				enableFDA = isChecked;
				if (currMachine != null) {
					if (isChecked) {
						currMachine.fda_img_path = "";
						int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, "");
					} else {
						currMachine.fda_img_path = null;
						int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, null);
					}
				}

				triggerUpdateSpinner(mFDA);
			}

		});
		mFDBenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mFDB.setEnabled(isChecked);
				enableFDB = isChecked;
				if (currMachine != null) {
					if (isChecked) {
						currMachine.fdb_img_path = "";
						int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, "");
					} else {
						currMachine.fdb_img_path = null;
						int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, null);
					}
				}
				triggerUpdateSpinner(mFDB);
			}

		});
		mSDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mSD.setEnabled(isChecked);
				enableSD = isChecked;
				if (currMachine != null) {
					if (isChecked) {
						currMachine.sd_img_path = "";
						int ret = machineDB.update(currMachine, MachineOpenHelper.SD, "");
					} else {
						currMachine.sd_img_path = null;
						int ret = machineDB.update(currMachine, MachineOpenHelper.SD, null);
					}
				}

				triggerUpdateSpinner(mSD);
			}

		});

		mBootDevices.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String bootDev = (String) ((ArrayAdapter<?>) mBootDevices.getAdapter()).getItem(position);
				if (userPressedBootDev) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.BOOT_CONFIG, bootDev);
					currMachine.bootdevice = bootDev;
				}
				userPressedBootDev = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		this.mNetConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String netfcg = (String) ((ArrayAdapter<?>) mNetConfig.getAdapter()).getItem(position);
				if (userPressedNetCfg) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.NET_CONFIG, netfcg);
					currMachine.net_cfg = netfcg;
				}
				if (position > 0) {
					mNetDevices.setEnabled(true);
					mDNS.setEnabled(true);
				} else {
					mNetDevices.setEnabled(false);
					mDNS.setEnabled(false);
				}

				userPressedNetCfg = true;
				ApplicationInfo pInfo = null;

				if (netfcg.equals("TAP")) {
					onTap();
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		this.mNetDevices.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

				if (position < 0 || position >= mNetDevices.getCount()) {
					userPressedNicCfg = true;
					mNetDevices.setSelection(0);
					return;
				}
				String niccfg = (String) ((ArrayAdapter<?>) mNetDevices.getAdapter()).getItem(position);
				if (userPressedNicCfg) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.NIC_CONFIG, niccfg);
					currMachine.nic_driver = niccfg;
				}

				userPressedNicCfg = true;

			}

			public void onNothingSelected(final AdapterView<?> parentView) {

			}
		});

		mVGAConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String vgacfg = (String) ((ArrayAdapter<?>) mVGAConfig.getAdapter()).getItem(position);
				if (userPressedVGACfg) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.VGA, vgacfg);
					currMachine.vga_type = vgacfg;
				}

				userPressedVGACfg = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		this.mSoundCardConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String sndcfg = (String) ((ArrayAdapter<?>) mSoundCardConfig.getAdapter()).getItem(position);
				if (userPressedSndCfg) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.SOUNDCARD_CONFIG, sndcfg);
					currMachine.soundcard = sndcfg;
				}
				userPressedSndCfg = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mHDCacheConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdcfg = (String) ((ArrayAdapter<?>) mHDCacheConfig.getAdapter()).getItem(position);
				if (userPressedHDCfg) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.HDCACHE_CONFIG, hdcfg);
					currMachine.hd_cache = hdcfg;
				}
				userPressedHDCfg = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mACPI.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				if (userPressedACPI) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.DISABLE_ACPI, ((isChecked ? 1 : 0) + ""));
					currMachine.disableacpi = (isChecked ? 1 : 0);
				}
				userPressedACPI = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHPET.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				if (userPressedHPET) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.DISABLE_HPET, ((isChecked ? 1 : 0) + ""));
					currMachine.disablehpet = (isChecked ? 1 : 0);
				}
				userPressedHPET = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mFDBOOTCHK.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				if (userPressedFDBOOTCHK) {
					int ret = machineDB.update(currMachine, MachineOpenHelper.DISABLE_FD_BOOT_CHK,
							((isChecked ? 1 : 0) + ""));
					currMachine.disablefdbootchk = (isChecked ? 1 : 0);
				}

				userPressedFDBOOTCHK = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		if (dnsChangeListener == null)
			dnsChangeListener = new TextWatcher() {

				public void afterTextChanged(Editable s) {
					LimboSettingsManager.setDNSServer(activity, mDNS.getText().toString());
				}

				public void beforeTextChanged(CharSequence s, int start, int count, int after) {
				}

				public void onTextChanged(CharSequence s, int start, int before, int count) {
				}
			};

		mDNS.addTextChangedListener(dnsChangeListener);

		if (appendChangeListener == null)
			appendChangeListener = new TextWatcher() {

				public void afterTextChanged(Editable s) {

					int ret = machineDB.update(currMachine, MachineOpenHelper.APPEND, s.toString());
				}

				public void beforeTextChanged(CharSequence s, int start, int count, int after) {

				}

				public void onTextChanged(CharSequence s, int start, int before, int count) {

				}
			};
		mAppend.addTextChangedListener(appendChangeListener);

		mVNCAllowExternal.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

				if (isChecked) {
					promptVNCAllowExternal(activity);
				} else {
					vnc_passwd = null;
					vnc_allow_external = 0;
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mPrio.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

				if (isChecked) {
					promptPrio(activity);
				} else {
					LimboSettingsManager.setPrio(activity, false);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});
		mEnableKVM.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

				if (isChecked) {
					promptKVM(activity);
				} else {
					LimboSettingsManager.setEnableKVM(activity, false);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mOrientation.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String orientationCfg = (String) ((ArrayAdapter<?>) mOrientation.getAdapter()).getItem(position);
				if (userPressedOrientation) {
					LimboSettingsManager.setOrientationSetting(activity, position);
				}

				userPressedOrientation = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mKeyboard.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String keyboardCfg = (String) ((ArrayAdapter<?>) mKeyboard.getAdapter()).getItem(position);
				if (userPressedKeyboard) {
					LimboSettingsManager.setKeyboardSetting(activity, position);
				}

				userPressedKeyboard = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});
	}

	private void disableListeners() {

		mMachine.setOnItemSelectedListener(null);

		mArch.setOnItemSelectedListener(null);
		mCPU.setOnItemSelectedListener(null);
		mMachineType.setOnItemSelectedListener(null);
		mUI.setOnItemSelectedListener(null);
		mCPUNum.setOnItemSelectedListener(null);
		mRamSize.setOnItemSelectedListener(null);
		mKernel.setOnItemSelectedListener(null);
		mInitrd.setOnItemSelectedListener(null);
		mHDA.setOnItemSelectedListener(null);
		mHDB.setOnItemSelectedListener(null);
		mHDC.setOnItemSelectedListener(null);
		mHDD.setOnItemSelectedListener(null);
		mSnapshot.setOnItemSelectedListener(null);
		mCD.setOnItemSelectedListener(null);
		mFDA.setOnItemSelectedListener(null);
		mFDB.setOnItemSelectedListener(null);
		mSD.setOnItemSelectedListener(null);
		mCDenable.setOnCheckedChangeListener(null);
		mHDAenable.setOnCheckedChangeListener(null);
		mHDBenable.setOnCheckedChangeListener(null);
		mHDCenable.setOnCheckedChangeListener(null);
		mHDDenable.setOnCheckedChangeListener(null);
		mFDAenable.setOnCheckedChangeListener(null);
		mFDBenable.setOnCheckedChangeListener(null);
		mSDenable.setOnCheckedChangeListener(null);
		mBootDevices.setOnItemSelectedListener(null);
		mNetConfig.setOnItemSelectedListener(null);
		mNetDevices.setOnItemSelectedListener(null);
		mVGAConfig.setOnItemSelectedListener(null);
		mSoundCardConfig.setOnItemSelectedListener(null);
		mHDCacheConfig.setOnItemSelectedListener(null);
		mACPI.setOnCheckedChangeListener(null);
		mHPET.setOnCheckedChangeListener(null);
		mFDBOOTCHK.setOnCheckedChangeListener(null);
		mDNS.removeTextChangedListener(dnsChangeListener);
		mAppend.removeTextChangedListener(appendChangeListener);
		mVNCAllowExternal.setOnCheckedChangeListener(null);
		mPrio.setOnCheckedChangeListener(null);
		mEnableKVM.setOnCheckedChangeListener(null);
		mOrientation.setOnItemSelectedListener(null);
		mKeyboard.setOnItemSelectedListener(null);

	}

	public static void UIAlert(String title, String body, Activity activity) {
		AlertDialog ad;
		ad = new AlertDialog.Builder(activity).create();
		ad.setTitle(title);
		ad.setMessage(body);
		ad.setButton(Dialog.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				return;
			}
		});
		ad.show();
	}

	private static String output;
	private Spinner mMachine;
	private Spinner mCPU;
	private Spinner mArch;
	private Spinner mMachineType;
	private Spinner mCPUNum;
	private Spinner mKernel;
	private Spinner mInitrd;

	// HDD
	private Spinner mHDA;
	private Spinner mHDB;
	private Spinner mHDC;
	private Spinner mHDD;
	private Spinner mCD;
	private Spinner mFDA;
	private Spinner mFDB;
	private Spinner mSD;

	private CheckBox mHDAenable;
	private CheckBox mHDBenable;
	private CheckBox mHDCenable;
	private CheckBox mHDDenable;
	private CheckBox mCDenable;
	private CheckBox mFDAenable;
	private CheckBox mFDBenable;
	private CheckBox mSDenable;

	private Spinner mRamSize;
	private Spinner mBootDevices;
	private Spinner mNetDevices;
	private Spinner mNetConfig;
	private Spinner mVGAConfig;
	private Spinner mSoundCardConfig;
	private Spinner mHDCacheConfig;
	private Spinner mUI;
	private CheckBox mACPI;
	private CheckBox mHPET;
	private CheckBox mFDBOOTCHK;
	// private CheckBox mSnapshot;
	// private CheckBox mBluetoothMouse;
	private CheckBox mVNCAllowExternal;
	private CheckBox mPrio;
	private CheckBox mEnableKVM;
	private Spinner mSnapshot;
	private Spinner mOrientation;
	private Spinner mKeyboard;
	private ImageButton mStart;
	private ImageButton mStop;
	private ImageButton mRestart;
	private ImageButton mSave;
	// private Button mResume;
	public static FavOpenHelper favDB;
	public static MachineOpenHelper machineDB;

	// ADS

	public static void quit() {
		activity.finish();
	}

	/**
	 * Called when the activity is first created.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		OShandler = this.handler;

		// if (Const.enable_fullscreen
		// || android.os.Build.VERSION.SDK_INT <
		// android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
		// requestWindowFeature(Window.FEATURE_NO_TITLE);
		// }

		// getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
		// WindowManager.LayoutParams.FLAG_FULLSCREEN);

		// Declare an instance variable for your MoPubView.

		activity = this;

		// DB init
		favDB = new FavOpenHelper(activity);
		machineDB = new MachineOpenHelper(activity);

		this.setContentView(R.layout.main);
		this.setupWidgets();

		enableRemovableDeviceOptions(false);
		enableNonRemovableDeviceOptions(false);
		mVNCAllowExternal.setEnabled(false);

		resetUserPressed();
		populateAttributes();

		execTimeListener();

		if (this.isFirstLaunch()) {
			onFirstLaunch();
		}

		System.loadLibrary("limbo");
		// Load GLIB and ICONV
		// System.loadLibrary("iconv");
		System.loadLibrary("glib-2.0");
		System.loadLibrary("gthread-2.0");
		System.loadLibrary("gobject-2.0");
		System.loadLibrary("gmodule-2.0");
		System.loadLibrary("pixman");

		if (Config.enable_SPICE) {
			System.loadLibrary("crypto");
			System.loadLibrary("ssl");
			System.loadLibrary("spice");
		}

		// //Load SDL libraries
		if (Config.enable_SDL_libs) {
			System.loadLibrary("SDL2");
			System.loadLibrary("SDL2_image");
		}
		if (Config.enable_sound_libs) {
			// System.loadLibrary("mikmod");
			System.loadLibrary("SDL2_mixer");
			// System.loadLibrary("SDL_ttf");

		}
		if (Config.enable_SDL_libs) {
			System.loadLibrary("main");
		}
		// acquireLocks();

		// For debugging purposes
		if (Config.debug) {
			if (Config.debugMode == DebugMode.X86)
				System.loadLibrary("qemu-system-i386");
			else if (Config.debugMode == DebugMode.X86_64)
				System.loadLibrary("qemu-system-x86_64");
			else if (Config.debugMode == DebugMode.ARM)
				System.loadLibrary("qemu-system-arm");
		}

		Thread tsdl = new Thread(new Runnable() {
			public void run() {
				// Check for a new Version
				try {
					checkNewVersion();
				} catch (Exception e) {
					// TODO Auto-generated catch block
					// e.printStackTrace();
				}
			}
		});
		tsdl.start();

	}

	private void checkNewVersion() throws Exception {
		// TODO Auto-generated method stub

		if (!LimboSettingsManager.getPromptUpdateVersion(activity)) {
			return;
		}

		HttpClient client = new DefaultHttpClient();
		HttpGet get = new HttpGet(Config.downloadUpdateLink);
		HttpResponse response = client.execute(get);
		HttpEntity entity = response.getEntity();

		InputStream is = entity.getContent();
		ByteArrayOutputStream bos = new ByteArrayOutputStream();

		int read = 0;
		byte[] buff = new byte[1024];
		while ((read = is.read(buff)) != -1) {
			bos.write(buff, 0, read);
		}
		byte[] streamData = bos.toByteArray();
		final String versionStr = new String(streamData);
		Double version = Double.parseDouble(versionStr);

		PackageInfo pInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(),
				PackageManager.GET_META_DATA);

		if ((int) (version * 100) > pInfo.versionCode) {
			new Handler(Looper.getMainLooper()).post(new Runnable() {
				@Override
				public void run() {
					promptNewVersion(activity, versionStr);
				}
			});
		}
	}

	public void promptNewVersion(final Activity activity, String version) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("New Version " + version);
		TextView stateView = new TextView(activity);
		stateView.setText("There is a new version available with fixes and new features. Do you want to update?");
		stateView.setId(201012014);
		stateView.setPadding(5, 5, 5, 5);
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Get New Version",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {

						// UIUtils.log("Searching...");
						Intent fileIntent = new Intent(Intent.ACTION_VIEW);
						fileIntent.setData(Uri.parse(Config.downloadLink));
						activity.startActivity(fileIntent);
					}
				});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Don't Show Again",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {

						// UIUtils.log("Searching...");
						LimboSettingsManager.setPromptUpdateVersion(activity, false);

					}
				});
		alertDialog.show();

	}

	private void resetUserPressed() {
		// TODO Auto-generated method stub
		userPressedUI = false;
		userPressedMachine = false;
		userPressedArch = false;
		userPressedMachineType = false;
		userPressedCPU = false;
		userPressedCPUNum = false;

		userPressedRAM = false;
		userPressedCDROM = false;
		userPressedHDCfg = false;
		userPressedSndCfg = false;
		userPressedVGACfg = false;
		userPressedNicCfg = false;
		userPressedNetCfg = false;
		userPressedBootDev = false;
		userPressedFDB = false;
		userPressedFDA = false;

		// HDD
		userPressedHDA = false;
		userPressedHDB = false;
		userPressedHDC = false;
		userPressedHDD = false;

		userPressedKernel = false;
		userPressedInitrd = false;
		userPressedACPI = false;
		userPressedHPET = false;
		this.userPressedFDBOOTCHK = false;
		userPressedBluetoothMouse = false;
		userPressedSnapshot = false;
		userPressedVNC = false;
		userPressedOrientation = false;
	}

	private void populateAttributes() {
		// TODO Auto-generated method stub
		this.populateMachines();
		this.populateArch();
		this.populateMachineType(null);
		this.populateCPUs(null);
		this.populateCPUNum();
		this.populateRAM();
		this.populateKernel();
		this.populateInitrd();
		this.populateHD("hda");
		this.populateHD("hdb");
		this.populateHD("hdc");
		this.populateHD("hdd");
		this.populateCDRom("cd");
		this.populateFloppy("fda");
		this.populateFloppy("fdb");
		this.populateSDCard("sd");
		this.populateBootDevices();
		this.populateNet();
		this.populateNetDevices(null);
		this.populateVGA();
		this.populateSoundcardConfig();
		this.populateHDCacheConfig();
		this.populateSnapshot();
		this.populateUI();
		this.populateOrientation();
		this.populateKeyboardLayout();
	}

	public void onFirstLaunch() {
		onLicense();
	}

	static protected boolean isFirstLaunch() {
		PackageInfo pInfo = null;

		try {
			pInfo = activity.getPackageManager().getPackageInfo(activity.getClass().getPackage().getName(),
					PackageManager.GET_META_DATA);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		boolean firstTime = prefs.getBoolean("firstTime" + pInfo.versionName, true);
		// UIUtils.log("Getting First time: " + firstTime);
		return firstTime;
	}

	static protected void setFirstLaunch() {
		PackageInfo pInfo = null;

		try {
			pInfo = activity.getPackageManager().getPackageInfo(activity.getClass().getPackage().getName(),
					PackageManager.GET_META_DATA);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("firstTime" + pInfo.versionName, false);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}

	static private void install() {
		progDialog = ProgressDialog.show(activity, "Please Wait", "Installing Files...", true);
		a = new Installer();
		a.execute();
	}

	private static class Installer extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {
			// Get files from last dir
			onInstall();

			if (progDialog.isShowing()) {
				progDialog.dismiss();
			}

			return null;
		}

		@Override
		protected void onPostExecute(Void test) {

		}
	}

	public Handler handler = new Handler() {
		@Override
		public synchronized void handleMessage(Message msg) {
			Bundle b = msg.getData();
			Integer messageType = (Integer) b.get("message_type");

			if (messageType != null && messageType == Config.VM_PAUSED) {
				Toast.makeText(activity, "VM Paused", Toast.LENGTH_LONG).show();

			}
			if (messageType != null && messageType == Config.VM_RESUMED) {
				Toast.makeText(activity, "VM Resuming, Please Wait", Toast.LENGTH_LONG).show();
				vmStarted = true;
			}
			if (messageType != null && messageType == Config.VM_STARTED) {
				if (!vmStarted) {
					Toast.makeText(activity, "VM Started\nPause the VM instead so you won't have to boot again!",
							Toast.LENGTH_LONG).show();
				} else {
					Toast.makeText(activity, "Connecting to VM Display", Toast.LENGTH_LONG).show();
				}
				enableNonRemovableDeviceOptions(false);
				// mStart.setText("Resume");
				mStart.setImageResource(R.drawable.play);
				vmStarted = true;
			}
			if (messageType != null && messageType == Config.VM_STOPPED) {
				Toast.makeText(activity, "VM Shutdown", Toast.LENGTH_LONG).show();
				// mStart.setText("Start");
				mStart.setImageResource(R.drawable.play);
				vmStarted = false;
			}
			if (messageType != null && messageType == Config.VM_RESTARTED) {
				Toast.makeText(activity, "VM Reset", Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_SAVED) {
				Toast.makeText(activity, "VM Saved", Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_NO_QCOW2) {
				Toast.makeText(activity, "Couldn't find a QCOW2 image\nPlease attach an HDA or HDB image first!",
						Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_NO_KERNEL) {
				Toast.makeText(activity, "Couldn't find a Kernel image\nPlease attach a Kernel image first!",
						Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_NO_INITRD) {
				Toast.makeText(activity, "Couldn't find a initrd image\nPlease attach an initrd image first!",
						Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_ARM_NOMACHINE) {
				Toast.makeText(activity, "Please select an ARM machine type first!", Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_NOTRUNNING) {
				Toast.makeText(activity, "VM not Running", Toast.LENGTH_SHORT).show();
			}
			if (messageType != null && messageType == Config.VM_CREATED) {
				String machineValue = (String) b.get("machine_name");
				if (machineDB.getMachine(machineValue, "") != null) {
					Toast.makeText(activity, "VM Name \"" + machineValue + "\" exists please choose another name!",
							Toast.LENGTH_SHORT).show();
				}
				currMachine = new Machine(machineValue);
				machineDB.insertMachine(currMachine);

				// default settings
				machineDB.update(currMachine, MachineOpenHelper.ARCH, "x86");
				machineDB.update(currMachine, MachineOpenHelper.MACHINE_TYPE, "pc");
				machineDB.update(currMachine, MachineOpenHelper.CPU, "n270");
				machineDB.update(currMachine, MachineOpenHelper.MEMORY, "128");
				machineDB.update(currMachine, MachineOpenHelper.NET_CONFIG, "User");
				machineDB.update(currMachine, MachineOpenHelper.NIC_CONFIG, "ne2k_pci");

				Toast.makeText(activity, "VM Created: " + machineValue, Toast.LENGTH_SHORT).show();
				populateMachines();
				setMachine(machineValue);
				if (LimboActivity.currMachine != null && currMachine.cpu != null && currMachine.cpu.endsWith("(arm)")) {
					mMachineType.setEnabled(true); // Disabled for now
				}
				enableNonRemovableDeviceOptions(true);
				enableRemovableDeviceOptions(true);

			}
			if (messageType != null && messageType == Config.IMG_CREATED) {
				String hdValue = (String) b.get("hd");
				String imageValue = (String) b.get("image_name");
				if (progDialog != null && progDialog.isShowing()) {
					progDialog.dismiss();
				}
				Toast.makeText(activity, "Image Created: " + imageValue, Toast.LENGTH_SHORT).show();
				setDriveAttr(hdValue, Config.machinedir + currMachine.machinename + "/" + imageValue);

			}
			if (messageType != null && messageType == Config.SNAPSHOT_CREATED) {

				String imageValue = (String) b.get("snapshot_name");
				savevm(imageValue);

			}
			if (messageType != null && messageType == Config.VNC_PASSWORD) {

				String imageValue = (String) b.get("vnc_passwd");

			}
			if (messageType != null && messageType == Config.UIUTILS_SHOWALERT_LICENSE) {
				String title = (String) b.get("title");
				String body = (String) b.get("body");
				UIAlertLicense(title, body, activity);
			}
			if (messageType != null && messageType == Config.UIUTILS_SHOWALERT_HTML) {
				String title = (String) b.get("title");
				String body = (String) b.get("body");
				UIAlertHtml(title, body, activity);
			}
			if (messageType != null && messageType == Config.STATUS_CHANGED) {
				String status_changed = (String) b.get("status_changed");
				if (status_changed.equals("RUNNING")) {
					mStatus.setImageResource(R.drawable.on);
					mStatusText.setText("Running");
				} else if (status_changed.equals("READY") || status_changed.equals("STOPPED")) {
					mStatus.setImageResource(R.drawable.off);
					mStatusText.setText("Stopped");
				} else if (status_changed.equals("SAVING")) {
					mStatus.setImageResource(R.drawable.on);
					mStatusText.setText("Saving State");
				} else if (status_changed.equals("PAUSED")) {
					mStatus.setImageResource(R.drawable.on);
					mStatusText.setText("Paused");
				}
			}
			if (messageType != null && messageType == Config.VM_EXPORT) {
				if (progDialog.isShowing()) {
					progDialog.dismiss();
				}
				Toast.makeText(activity, "Machines are exported in " + Config.DBFile, Toast.LENGTH_LONG).show();
			}
			if (messageType != null && messageType == Config.VM_IMPORT) {
				if (progDialog.isShowing()) {
					progDialog.dismiss();
				}
				Toast.makeText(activity, " Machines have been imported from " + Config.DBFile, Toast.LENGTH_LONG)
						.show();

				resetUserPressed();
				populateAttributes();
			}

		}
	};

	public static void UIAlertLicense(String title, String html, final Activity activity) {

		AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle(title);
		WebView webview = new WebView(activity);
		webview.setBackgroundColor(Color.BLACK);
		webview.loadData("<font color=\"FFFFFF\">" + html + " </font>", "text/html", "UTF-8");
		alertDialog.setView(webview);

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "I Acknowledge", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				if (isFirstLaunch()) {
					install();
					onHelp();
					onChangeLog();
				}
				setFirstLaunch();
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				if (isFirstLaunch()) {
					if (activity.getParent() != null) {
						activity.getParent().finish();
					} else {
						activity.finish();
					}
				}
			}
		});
		alertDialog.show();
	}

	public static void UIAlertHtml(String title, String html, Activity activity) {

		AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle(title);
		WebView webview = new WebView(activity);
		// webview.setBackgroundColor(Color.BLACK);
		webview.loadData(html, "text/html", "UTF-8");
		alertDialog.setView(webview);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				return;
			}
		});
		alertDialog.show();
	}

	// Generic Message to update UI
	public static void sendHandlerMessage(Handler handler, int message_type, String message_var, String message_value) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", message_type);
		b.putString(message_var, message_value);
		msg1.setData(b);
		handler.sendMessage(msg1);
	}

	public static void sendHandlerMessage(Handler handler, int message_type, String[] message_var,
			String[] message_value) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", message_type);
		for (int i = 0; i < message_var.length; i++) {
			b.putString(message_var[i], message_value[i]);
		}
		msg1.setData(b);
		handler.sendMessage(msg1);
	}

	static public ProgressDialog progDialog;

	// Another Generic Messanger
	public static void sendHandlerMessage(Handler handler, int message_type) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", message_type);
		msg1.setData(b);
		handler.sendMessage(msg1);
	}

	private void onDeleteMachine() {
		if (currMachine == null) {
			Toast.makeText(this, "Select a machine first!", Toast.LENGTH_SHORT).show();
			return;
		}
		machineDB.deleteMachine(currMachine);
		this.resetUserPressed();
		this.populateAttributes();
		Toast.makeText(this, "Machine " + currMachine.machinename + " deleted", Toast.LENGTH_SHORT).show();
	}

	private void onExportMachines() {
		progDialog = ProgressDialog.show(activity, "Please Wait", "Exporting Machines...", true);
		ExportMachines exporter = new ExportMachines();
		exporter.execute();

	}

	private void onImportMachines() {
		// Warn the user that VMs with same names will be replaced
		promptImportMachines();

	}

	private void promptImportMachines() {
		// TODO Auto-generated method stub

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Import Machines");

		RelativeLayout mLayout = new RelativeLayout(this);
		mLayout.setId(12222);

		TextView imageNameView = new TextView(activity);
		imageNameView.setVisibility(View.VISIBLE);
		imageNameView.setId(201012010);
		imageNameView.setText(
				"Step 1: Place the machine.CSV file you export previously under \"limbo\" directory in your SD card.\n"
						+ "Step 2: WARNING: Any machine with the same name will be replaced!\n"
						+ "Step 3: Press \"OK\".\n");

		RelativeLayout.LayoutParams searchViewParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(imageNameView, searchViewParams);
		alertDialog.setView(mLayout);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				// For each line create a Machine
				progDialog = ProgressDialog.show(activity, "Please Wait", "Importing Machines...", true);

				ImportMachines importer = new ImportMachines();
				importer.execute();
			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {

				return;

			}
		});
		alertDialog.show();

	}

	private static void onTap() {
		ApplicationInfo pInfo = null;
		String userid = "None";
		try {
			pInfo = activity.getPackageManager().getApplicationInfo(activity.getClass().getPackage().getName(),
					PackageManager.GET_META_DATA);
			userid = pInfo.uid + "";
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		if (!(new File("/dev/net/tun")).exists()) {
			showAlertHtml("TAP - User Id: " + userid,
					"Your device doesn't support TAP, use \"User\" network mode instead ", OShandler);
			return;
		}
		FileUtils fileutils = new FileUtils();
		try {
			showAlertHtml("TAP - User Id: " + userid, fileutils.LoadFile(activity, "TAP", false), OShandler);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private static void onHelp() {
		PackageInfo pInfo = null;

		try {
			pInfo = activity.getPackageManager().getPackageInfo(activity.getClass().getPackage().getName(),
					PackageManager.GET_META_DATA);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}

		FileUtils fileutils = new FileUtils();
		try {
			showAlertHtml(Config.APP_NAME + " v" + pInfo.versionName, fileutils.LoadFile(activity, "HELP", false),
					OShandler);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private static void onMenuHelp() {
		String url = "https://github.com/limboemu/limbo";
		Intent i = new Intent(Intent.ACTION_VIEW);
		i.setData(Uri.parse(url));
		activity.startActivity(i);

	}

	private static void onChangeLog() {
		PackageInfo pInfo = null;

		try {
			pInfo = activity.getPackageManager().getPackageInfo(activity.getClass().getPackage().getName(),
					PackageManager.GET_META_DATA);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		FileUtils fileutils = new FileUtils();
		try {
			showAlertHtml("CHANCELOG", fileutils.LoadFile(activity, "CHANGELOG", false), OShandler);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private void onLicense() {
		PackageInfo pInfo = null;

		try {
			pInfo = getPackageManager().getPackageInfo(getClass().getPackage().getName(), PackageManager.GET_META_DATA);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		FileUtils fileutils = new FileUtils();
		try {
			showAlertLicense(Config.APP_NAME + " v" + pInfo.versionName, fileutils.LoadFile(activity, "LICENSE", false),
					handler);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	public static void showAlertLicense(String title, String message, Handler handler) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", Config.UIUTILS_SHOWALERT_LICENSE);
		b.putString("title", title);
		b.putString("body", message);
		msg1.setData(b);
		handler.sendMessage(msg1);
	}

	public static void showAlertHtml(String title, String message, Handler handler) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", Config.UIUTILS_SHOWALERT_HTML);
		b.putString("title", title);
		b.putString("body", message);
		msg1.setData(b);
		handler.sendMessage(msg1);
	}

	public void exit() {
		onStopButton(true);
	}

	private void enableRemovableDeviceOptions(boolean flag) {

		mCDenable.setEnabled(flag);
		mFDAenable.setEnabled(flag);
		mFDBenable.setEnabled(flag);
		mSDenable.setEnabled(flag);

		mCD.setEnabled(flag && mCDenable.isChecked());
		mFDA.setEnabled(flag && mFDAenable.isChecked());
		mFDB.setEnabled(flag && mFDBenable.isChecked());
		mSD.setEnabled(flag && mSDenable.isChecked());
	}

	private void enableNonRemovableDeviceOptions(boolean flag) {

		// Everything Except removable devices
		// this.mMachine.setEnabled(flag);
		this.mArch.setEnabled(flag);
		this.mMachineType.setEnabled(flag);
		this.mCPU.setEnabled(flag);
		this.mCPUNum.setEnabled(flag);
		this.mRamSize.setEnabled(flag);
		this.mKernel.setEnabled(flag);
		this.mInitrd.setEnabled(flag);
		this.mAppend.setEnabled(flag);
		this.mMachineType.setEnabled(flag);

		this.mBootDevices.setEnabled(flag);
		this.mNetConfig.setEnabled(flag);

		if (mNetConfig.getSelectedItemPosition() > 0)
			this.mNetDevices.setEnabled(flag);
		this.mVGAConfig.setEnabled(flag);

		if (Config.enable_sound_menu || !flag)
			this.mSoundCardConfig.setEnabled(flag);

		this.mPrio.setEnabled(flag);
		this.mEnableKVM.setEnabled(flag);

		this.mKeyboard.setEnabled(Config.enableKeyboardLayoutOption && flag);

		this.mUI.setEnabled(flag);

		if (Config.enableHDCache || !flag)
			this.mHDCacheConfig.setEnabled(flag);

		this.mACPI.setEnabled(flag);
		this.mHPET.setEnabled(flag);
		this.mFDBOOTCHK.setEnabled(flag);
		// this.mBluetoothMouse.setEnabled(b);

		this.mSnapshot.setEnabled(flag);
		this.mFDBOOTCHK.setEnabled(flag);
		this.mDNS.setEnabled(flag);

		mHDAenable.setEnabled(flag);
		mHDBenable.setEnabled(flag);
		mHDCenable.setEnabled(flag);
		mHDDenable.setEnabled(flag);

		mHDA.setEnabled(flag && mHDAenable.isChecked());
		mHDB.setEnabled(flag && mHDBenable.isChecked());
		mHDC.setEnabled(flag && mHDCenable.isChecked());
		mHDD.setEnabled(flag && mHDDenable.isChecked());

	}

	static private void onInstall() {
		FileInstaller.installFiles(activity);
	}

	public class AutoScrollView extends ScrollView {

		public AutoScrollView(Context context, AttributeSet attrs) {
			super(context, attrs);
		}

		public AutoScrollView(Context context) {
			super(context);
		}
	}

	public AutoScrollView mLyricsScroll;
	private ArrayAdapter<String> cpuAdapter;
	private ArrayAdapter<String> archAdapter;
	private ArrayAdapter<String> machineTypeAdapter;
	private ArrayAdapter<String> cpuNumAdapter;
	private ArrayAdapter<String> uiAdapter;
	private ArrayAdapter<String> machineAdapter;
	private ArrayAdapter<String> ramAdapter;
	private ArrayAdapter<String> cdromAdapter;
	private ArrayAdapter<String> vgaAdapter;
	private ArrayAdapter<String> netAdapter;
	private ArrayAdapter<String> bootDevAdapter;
	private ArrayAdapter<String> hdCacheAdapter;
	private ArrayAdapter<String> sndAdapter;
	private ArrayAdapter<String> nicCfgAdapter;
	private ArrayAdapter<String> fdaAdapter;
	private ArrayAdapter<String> fdbAdapter;
	private ArrayAdapter<String> sdAdapter;

	// HDD
	private ArrayAdapter<String> hdaAdapter;
	private ArrayAdapter<String> hdbAdapter;
	private ArrayAdapter<String> hdcAdapter;
	private ArrayAdapter<String> hddAdapter;

	private ArrayAdapter<String> kernelAdapter;
	private ArrayAdapter<String> initrdAdapter;
	private ArrayAdapter<String> snapshotAdapter;

	private ArrayAdapter<String> orientationAdapter;
	private ArrayAdapter<String> keyboardAdapter;

	// Main event function
	// Retrives values from saved preferences
	private void onStartButton() {

		if (this.mMachine.getSelectedItemPosition() == 0 || this.currMachine == null) {
			Toast.makeText(getApplicationContext(), "Select or Create a Virtual Machine first", Toast.LENGTH_LONG)
					.show();
			return;
		}
		String filenotexists = validateFiles();
		if (filenotexists != null) {
			Toast.makeText(getApplicationContext(), "Could not find file: " + filenotexists, Toast.LENGTH_LONG).show();
			return;
		}
		if (currMachine.snapshot_name != null && !currMachine.snapshot_name.toLowerCase().equals("none")
				&& !currMachine.snapshot_name.toLowerCase().equals("") && currMachine.soundcard != null
				&& !currMachine.soundcard.toLowerCase().equals("none") && mUI.getSelectedItemPosition() != 1) {
			Toast.makeText(getApplicationContext(),
					"Snapshot was saved with soundcard enabled please use SDL \"User Interface\"", Toast.LENGTH_LONG)
					.show();
			return;
		}

		if (currMachine != null && currMachine.cpu != null && currMachine.cpu.endsWith("(arm)")
				&& (currMachine.kernel == null || currMachine.kernel.equals(""))) {
			sendHandlerMessage(handler, Config.VM_NO_KERNEL);
			return;
		}

		if (currMachine != null && currMachine.machine_type != null && currMachine.cpu.endsWith("(arm)")
				&& currMachine.machine_type.equals("None")) {
			sendHandlerMessage(handler, Config.VM_ARM_NOMACHINE);
			return;
		}
		// Initrd is optional
		// if (currMachine!=null && currMachine.cpu !=null &&
		// currMachine.cpu.startsWith("arm")
		// && (currMachine.initrd == null || currMachine.initrd.equals(""))) {
		// sendHandlerMessage(handler, Const.VM_NO_INITRD);
		//
		// }

		if (vmexecutor == null)
			vmexecutor = new VMExecutor(currMachine, this);

		if (mCDenable.isChecked() && vmexecutor.cd_iso_path == null)
			vmexecutor.cd_iso_path = "";
		if (mFDAenable.isChecked() && vmexecutor.fda_img_path == null)
			vmexecutor.fda_img_path = "";
		if (mFDBenable.isChecked() && vmexecutor.fdb_img_path == null)
			vmexecutor.fdb_img_path = "";
		if (mSDenable.isChecked() && vmexecutor.sd_img_path == null)
			vmexecutor.sd_img_path = "";

		// Global settings

		// dns
		vmexecutor.dns_addr = mDNS.getText().toString();

		// kvm
		vmexecutor.enablekvm = mEnableKVM.isChecked() ? 1 : 0;

		// Keyboard layout
		vmexecutor.keyboard_layout = getLanguageCode(mKeyboard.getSelectedItemPosition());
		// Append only when kernel is set

		if (currMachine.kernel != null && !currMachine.kernel.equals(""))
			vmexecutor.append = mAppend.getText().toString();

		vmexecutor.print();
		output = "Starting VM...";
		vmexecutor.paused = currMachine.paused;
		if (vmexecutor.paused == 1)
			sendHandlerMessage(handler, Config.VM_RESUMED);
		else
			sendHandlerMessage(handler, Config.VM_STARTED);

		if (mUI.getSelectedItemPosition() == 0) { // VNC
			vmexecutor.enableqmp = 0; // We enable qemu monitor
			startVNC();
		} else if (mUI.getSelectedItemPosition() == 1) { // SDL
			// XXX: We need to enable qmp server to be able to save the state
			// We could do it via the Monitor but SDL for Android
			// doesn't support multiple Windows
			vmexecutor.enableqmp = 1;
			startSDL();
		} else if (mUI.getSelectedItemPosition() == 2) { // SPICE
			startSPICE();
		}
		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				currMachine.paused = 0;
				machineDB.update(currMachine, machineDB.PAUSED, 0 + "");
			}
		}, 5000);

	}

	private String getLanguageCode(int index) {
		// TODO: Add more languages from /assets/roms/keymaps
		switch (index) {
		case 0:
			return "en-us";
		case 1:
			return "es";
		case 2:
			return "fr";
		}
		return null;
	}

	private void startSDL() {
		// TODO Auto-generated method stub
		Thread tsdl = new Thread(new Runnable() {
			public void run() {
				startsdl();
			}
		});
		if (mPrio.isChecked())
			tsdl.setPriority(Thread.MAX_PRIORITY);
		tsdl.start();
	}

	private void startVNC() {
		// TODO Auto-generated method stub
		VncCanvas.retries = 0;
		if (!vmStarted) {

			Thread tvm = new Thread(new Runnable() {
				public void run() {
					startvm(LimboActivity.this, Config.UI_VNC);
				}
			});
			if (mPrio.isChecked())
				tvm.setPriority(Thread.MAX_PRIORITY);
			tvm.start();
		}

		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {

			@Override
			public void run() {
				startvnc();
			}

		}, 2000);

	}

	private void startSPICE() {
		// TODO Auto-generated method stub

		if (!vmStarted) {

			Thread tvm = new Thread(new Runnable() {
				public void run() {
					startvm(LimboActivity.this, Config.UI_SPICE);
				}
			});
			if (mPrio.isChecked())
				tvm.setPriority(Thread.MAX_PRIORITY);
			tvm.start();
		}

	}

	private String validateFiles() {
		// TODO Auto-generated method stub
		int fd;
		try {
			if (!fileValid(currMachine.hda_img_path))
				return currMachine.hda_img_path;
			if (!fileValid(currMachine.hdb_img_path))
				return currMachine.hdb_img_path;
			if (!fileValid(currMachine.hdc_img_path))
				return currMachine.hdc_img_path;
			if (!fileValid(currMachine.hdd_img_path))
				return currMachine.hdd_img_path;
			if (!fileValid(currMachine.fda_img_path))
				return currMachine.fda_img_path;
			if (!fileValid(currMachine.fdb_img_path))
				return currMachine.fdb_img_path;
			if (!fileValid(currMachine.sd_img_path))
				return currMachine.sd_img_path;
			if (!fileValid(currMachine.cd_iso_path))
				return currMachine.cd_iso_path;
			if (!fileValid(currMachine.kernel))
				return currMachine.kernel;
			if (!fileValid(currMachine.initrd))
				return currMachine.initrd;

		} catch (Exception ex) {
			return "Unknown";
		}
		return null;
	}

	private boolean fileValid(String path) {
		// TODO Auto-generated method stub
		if (path == null || path.equals(""))
			return true;
		if (path.startsWith("content://") || path.startsWith("/content/")) {
			int fd = get_fd(path);
			if (fd <= 0)
				return false;
		} else {
			File file = new File(path);
			return file.exists();
		}
		return true;
	}

	private void setDNSaddr() {
		Thread t = new Thread(new Runnable() {
			public void run() {
				String dns_addr = mDNS.getText().toString();
				if (dns_addr != null && !dns_addr.equals("")) {
					vmexecutor.change_dns_addr();
				}
			}
		});
		// t.setPriority(Thread.MAX_PRIORITY);
		t.start();

	}

	private void onStopButton(boolean exit) {
		stopVM(exit);
	}

	private void onRestartButton() {

		if (vmexecutor == null) {
			sendHandlerMessage(handler, Config.VM_NOTRUNNING);
			return;
		}

		new AlertDialog.Builder(this).setTitle("Reset VM")
				.setMessage("VM will be reset and you may lose data. Continue?")
				.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						if (LimboActivity.vmexecutor != null) {
							Thread t = new Thread(new Runnable() {
								public void run() {
									restartvm();
								}
							});
							t.start();
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

	private void onSaveButton() {

		// TODO: This probably has no effect
		Thread t = new Thread(new Runnable() {
			public void run() {
				promptStateName(activity);
			}
		});
		// t.setPriority(Thread.MAX_PRIORITY);
		t.start();
	}

	private void onResumeButton() {

		// TODO: This probably has no effect
		Thread t = new Thread(new Runnable() {
			public void run() {
				resumevm();
			}
		});
		// t.setPriority(Thread.MAX_PRIORITY);
		t.start();
	}

	// Setting up the UI
	public void setupWidgets() {

		this.mStatus = (ImageView) findViewById(R.id.statusVal);
		this.mStatus.setImageResource(R.drawable.off);

		this.mStatusText = (TextView) findViewById(R.id.statusStr);

		this.mDNS = (EditText) findViewById(R.id.dnsval);
		this.mDNS.setFocusableInTouchMode(true);
		this.mDNS.setFocusable(true);
		this.mDNS.setText(LimboSettingsManager.getDNSServer(activity));

		this.mAppend = (EditText) findViewById(R.id.appendval);
		this.mAppend.setFocusableInTouchMode(true);
		this.mAppend.setFocusable(true);
		// this.mAppend.setEnabled(false);
		// this.mAppend.setText(LimboSettingsManager.getAppend(activity));

		this.mMachine = (Spinner) findViewById(R.id.machineval);

		this.mCPU = (Spinner) findViewById(R.id.cpuval);
		this.mArch = (Spinner) findViewById(R.id.archtypeval);
		this.mMachineType = (Spinner) findViewById(R.id.machinetypeval);
		// this.mMachineType.setEnabled(false);

		View machineType = (View) findViewById(R.id.machinetypel);
		if (Config.enable_ARM)
			machineType.setVisibility(View.VISIBLE);
		else
			machineType.setVisibility(View.GONE);

		this.mCPUNum = (Spinner) findViewById(R.id.cpunumval);
		this.mUI = (Spinner) findViewById(R.id.uival);
		if (!Config.enable_SDL_libs)
			this.mUI.setEnabled(false);

		this.mRamSize = (Spinner) findViewById(R.id.rammemval);

		this.mKernel = (Spinner) findViewById(R.id.kernelval);
		// this.mKernel.setEnabled(false);
		this.mInitrd = (Spinner) findViewById(R.id.initrdval);
		// this.mInitrd.setEnabled(false);

		this.mHDA = (Spinner) findViewById(R.id.hdimgval);
		this.mHDB = (Spinner) findViewById(R.id.hdbimgval);
		this.mHDC = (Spinner) findViewById(R.id.hdcimgval);
		this.mHDD = (Spinner) findViewById(R.id.hddimgval);
		this.mCD = (Spinner) findViewById(R.id.cdromimgval);
		this.mFDA = (Spinner) findViewById(R.id.floppyimgval);
		this.mFDB = (Spinner) findViewById(R.id.floppybimgval);
		this.mSD = (Spinner) findViewById(R.id.sdcardimgval);

		this.mHDAenable = (CheckBox) findViewById(R.id.hdimgcheck);
		this.mHDBenable = (CheckBox) findViewById(R.id.hdbimgcheck);
		this.mHDCenable = (CheckBox) findViewById(R.id.hdcimgcheck);
		this.mHDDenable = (CheckBox) findViewById(R.id.hddimgcheck);
		this.mCDenable = (CheckBox) findViewById(R.id.cdromimgcheck);
		this.mFDAenable = (CheckBox) findViewById(R.id.floppyimgcheck);
		this.mFDBenable = (CheckBox) findViewById(R.id.floppybimgcheck);
		this.mSDenable = (CheckBox) findViewById(R.id.sdcardimgcheck);

		this.mBootDevices = (Spinner) findViewById(R.id.bootfromval);
		this.mNetConfig = (Spinner) findViewById(R.id.netcfgval);
		this.mNetDevices = (Spinner) findViewById(R.id.netDevicesVal);
		this.mVGAConfig = (Spinner) findViewById(R.id.vgacfgval);
		this.mSoundCardConfig = (Spinner) findViewById(R.id.soundcfgval);
		this.mHDCacheConfig = (Spinner) findViewById(R.id.hdcachecfgval);
		this.mHDCacheConfig.setEnabled(false); // Disabled for now
		this.mACPI = (CheckBox) findViewById(R.id.acpival);
		this.mHPET = (CheckBox) findViewById(R.id.hpetval);
		this.mFDBOOTCHK = (CheckBox) findViewById(R.id.fdbootchkval);
		this.mVNCAllowExternal = (CheckBox) findViewById(R.id.vncexternalval); // No
																				// external
		// connections
		// mVNCAllowExternal.setChecked(LimboSettingsManager.getVNCAllowExternal(activity));
		mVNCAllowExternal.setChecked(false);
		this.mPrio = (CheckBox) findViewById(R.id.prioval); //
		mPrio.setChecked(LimboSettingsManager.getPrio(activity));

		this.mEnableKVM = (CheckBox) findViewById(R.id.enablekvmval); //
		mEnableKVM.setChecked(LimboSettingsManager.getEnableKVM(activity));

		this.mOrientation = (Spinner) findViewById(R.id.orientationval);

		this.mKeyboard = (Spinner) findViewById(R.id.keyboardval);

		this.mSnapshot = (Spinner) findViewById(R.id.snapshotval);

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

		mStart = (ImageButton) findViewById(R.id.startvm);
		mStart.setFocusableInTouchMode(true);
		mStart.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				onStartButton();

			}
		});

		mStop = (ImageButton) findViewById(R.id.stopvm);
		mStop.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				onStopButton(false);

			}
		});

		mRestart = (ImageButton) findViewById(R.id.restartvm);
		mRestart.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {

				onRestartButton();

			}
		});

		enableListeners();

	}

	private void triggerUpdateSpinner(final Spinner spinner) {
		// TODO Auto-generated method stub
		// trigger a change in the spinner
		final int position = (int) spinner.getSelectedItemId();
		spinner.setSelection(0);

		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				spinner.setSelection(position);
			}
		}, 100);

	}

	private static String vnc_passwd = null;

	public static String getVnc_passwd() {
		return vnc_passwd;
	}

	public static void setVnc_passwd(String vnc_passwd) {
		LimboActivity.vnc_passwd = vnc_passwd;
	}

	private static int vnc_allow_external = 0;

	// This is easier: traverse the interfaces and get the local IPs
	public static String getLocalIpAddress() {
		try {
			for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
				NetworkInterface intf = en.nextElement();
				for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress() && inetAddress.getHostAddress().toString().contains(".")) {
						return inetAddress.getHostAddress().toString();
					}
				}
			}
		} catch (SocketException ex) {
			ex.printStackTrace();
		}
		return null;
	}

	private void promptPrio(final Activity activity) {
		// TODO Auto-generated method stub

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable High Priority!");

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setId(201012010);
		textView.setText("Warning! High Priority might increase emulation speed but " + "will slow your phone down!");

		alertDialog.setView(textView);
		final Handler handler = this.handler;

		// alertDialog.setMessage(body);
		alertDialog.setButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				LimboSettingsManager.setPrio(activity, true);
			}
		});
		alertDialog.setButton2("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				mPrio.setChecked(false);
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				mPrio.setChecked(false);
			}
		});
		alertDialog.show();
	}

	private void promptKVM(final Activity activity) {
		// TODO Auto-generated method stub

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable KVM!");

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setId(201012010);
		textView.setText("Warning! Enabling KVM is an UNTESTED and EXPERIMENTAL feature. Do you want to continue?");

		alertDialog.setView(textView);
		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				LimboSettingsManager.setEnableKVM(activity, true);
			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				mEnableKVM.setChecked(false);
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				mEnableKVM.setChecked(false);
			}
		});
		alertDialog.show();
	}

	public void promptVNCAllowExternal(final Activity activity) {
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable VNC server");

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setId(201012010);
		textView.setText("VNC Server: " + this.getLocalIpAddress() + ":" + "5901\n"
				+ "Warning: VNC Connection is UNencrypted and not secure make sure you're on a private network!\n");

		EditText passwdView = new EditText(activity);
		passwdView.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
		passwdView.setHint("Password");
		passwdView.setEnabled(true);
		passwdView.setVisibility(View.VISIBLE);
		passwdView.setId(11111);
		passwdView.setSingleLine();

		RelativeLayout mLayout = new RelativeLayout(this);
		mLayout.setId(12222);

		RelativeLayout.LayoutParams textViewParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
		textViewParams.addRule(RelativeLayout.ALIGN_PARENT_TOP, mLayout.getId());
		mLayout.addView(textView, textViewParams);

		RelativeLayout.LayoutParams passwordViewParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);

		passwordViewParams.addRule(RelativeLayout.BELOW, textView.getId());
		// passwordViewParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM,
		// mLayout.getId());
		mLayout.addView(passwdView, passwordViewParams);

		alertDialog.setView(mLayout);

		final Handler handler = this.handler;

		alertDialog.setButton("Set", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				// UIUtils.log("Searching...");
				EditText a = (EditText) alertDialog.findViewById(11111);

				if (a.getText().toString().trim().equals("")) {
					Toast.makeText(getApplicationContext(), "Password cannot be empty!", Toast.LENGTH_SHORT).show();
					vnc_passwd = null;
					vnc_allow_external = 0;
					mVNCAllowExternal.setChecked(false);
					// LimboSettingsManager.setVNCAllowExternal(activity,
					// false);
					return;
				} else {
					sendHandlerMessage(handler, Config.VNC_PASSWORD, "vnc_passwd", "passwd");
					vnc_passwd = a.getText().toString();
					vnc_allow_external = 1;
					// LimboSettingsManager.setVNCAllowExternal(activity, true);
				}

			}
		});
		alertDialog.setButton2("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				vnc_passwd = null;
				vnc_allow_external = 0;
				mVNCAllowExternal.setChecked(false);
				// LimboSettingsManager.setVNCAllowExternal(activity, false);
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				mVNCAllowExternal.setChecked(false);
				// LimboSettingsManager.setVNCAllowExternal(activity, false);
				vnc_passwd = null;
				vnc_allow_external = 0;
			}
		});
		alertDialog.show();

	}

	private void loadMachine(String machine, String snapshot) {
		// TODO Auto-generated method stub

		machineLoaded = true;

		this.setUserPressed(false);

		// Load machine from DB
		this.currMachine = machineDB.getMachine(machine, snapshot);
		// Test
		// currMachine.paused = 1;

		this.populateMachineType(currMachine.machine_type);
		this.populateCPUs(currMachine.cpu);
		this.populateNetDevices(currMachine.nic_driver);

		this.setArch(currMachine.arch, false);

		// this.setMachineType(currMachine.machine_type, false);
		// this.setCPU(currMachine.cpu, false);

		this.setCPUNum(currMachine.cpuNum, false);
		this.setRAM(currMachine.memory, false);
		this.setKernel(currMachine.kernel, false);
		this.setInitrd(currMachine.initrd, false);
		if (currMachine.append != null)
			mAppend.setText(currMachine.append);
		else
			mAppend.setText("");

		setCDROM(currMachine.cd_iso_path, false);

		// Floppy
		this.setFDA(currMachine.fda_img_path, false);
		this.setFDB(currMachine.fdb_img_path, false);

		// SD Card
		this.setSD(currMachine.sd_img_path, false);

		// HDD
		this.setHDA(currMachine.hda_img_path, false);
		this.setHDB(currMachine.hdb_img_path, false);
		this.setHDC(currMachine.hdc_img_path, false);
		this.setHDD(currMachine.hdd_img_path, false);

		// Advance
		this.setBootDevice(currMachine.bootdevice, false);
		this.setNetCfg(currMachine.net_cfg, false);
		// this.setNicDevice(currMachine.nic_driver, false);
		this.setVGA(currMachine.vga_type, false);
		this.setHDCache(currMachine.hd_cache, false);
		this.setSoundcard(currMachine.soundcard, false);
		this.setUI(LimboSettingsManager.getLastUI(activity), false);

		this.userPressedACPI = false;
		this.mACPI.setChecked(currMachine.disableacpi == 1 ? true : false);
		this.userPressedHPET = false;
		this.mHPET.setChecked(currMachine.disablehpet == 1 ? true : false);
		this.userPressedFDBOOTCHK = false;
		this.mFDBOOTCHK.setChecked(currMachine.disablefdbootchk == 1 ? true : false);
		this.userPressedBluetoothMouse = false;

		if (this.currMachine == null) {
			return;
		}
		// We finished loading now when a user change a setting it will fire an
		// event

		this.enableNonRemovableDeviceOptions(true);
		this.enableRemovableDeviceOptions(true);

		if (currMachine.fda_img_path != null) {
			mFDAenable.setChecked(true);
		} else
			mFDAenable.setChecked(false);

		if (currMachine.fdb_img_path != null) {
			mFDBenable.setChecked(true);
		} else
			mFDBenable.setChecked(false);

		if (currMachine.hda_img_path != null) {
			mHDAenable.setChecked(true);
		} else
			mHDAenable.setChecked(false);

		if (currMachine.hdb_img_path != null) {
			mHDBenable.setChecked(true);
		} else
			mHDBenable.setChecked(false);

		if (currMachine.hdc_img_path != null) {
			mHDCenable.setChecked(true);
		} else
			mHDCenable.setChecked(false);

		if (currMachine.hdd_img_path != null) {
			mHDDenable.setChecked(true);
		} else
			mHDDenable.setChecked(false);

		if (currMachine.cd_iso_path != null) {
			mCDenable.setChecked(true);
		} else
			mCDenable.setChecked(false);

		if (currMachine.sd_img_path != null) {
			mSDenable.setChecked(true);
		} else
			mSDenable.setChecked(false);

		mMachine.setEnabled(false);

		if (currMachine.paused == 1) {
			sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", "PAUSED");
			enableNonRemovableDeviceOptions(false);
			enableRemovableDeviceOptions(false);
		} else {
			sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", "READY");
			enableNonRemovableDeviceOptions(true);
			enableRemovableDeviceOptions(true);
		}

		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				setUserPressed(true);
				machineLoaded = false;
				mMachine.setEnabled(true);
			}
		}, 1000);

	}

	public static Machine currMachine = null;
	public static Handler OShandler;
	public static boolean enableCDROM;
	public static boolean enableFDA;
	public static boolean enableFDB;
	public static boolean enableSD;

	public void promptMachineName(final Activity activity) {
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Machine Name");
		EditText searchView = new EditText(activity);
		searchView.setEnabled(true);
		searchView.setVisibility(View.VISIBLE);
		searchView.setId(201012010);
		searchView.setSingleLine();
		alertDialog.setView(searchView);
		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", (DialogInterface.OnClickListener) null);

		alertDialog.show();

		Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
		button.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				EditText a = (EditText) alertDialog.findViewById(201012010);
				if (a.getText().toString().trim().equals(""))
					UIUtils.toastLong(activity, "Machine name cannot be empty");
				else {
					sendHandlerMessage(handler, Config.VM_CREATED, "machine_name", a.getText().toString());
					alertDialog.dismiss();
				}
			}
		});

	}

	public void promptImageName(final Activity activity, String hd) {
		final String hd_string = hd;
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Image Name");

		RelativeLayout mLayout = new RelativeLayout(this);
		mLayout.setId(12222);

		EditText imageNameView = new EditText(activity);
		imageNameView.setEnabled(true);
		imageNameView.setVisibility(View.VISIBLE);
		imageNameView.setId(201012010);
		imageNameView.setSingleLine();
		RelativeLayout.LayoutParams searchViewParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(imageNameView, searchViewParams);

		final Spinner size = new Spinner(this);
		RelativeLayout.LayoutParams setPlusParams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
		size.setId(201012044);

		String[] arraySpinner = new String[7];
		for (int i = 0; i < arraySpinner.length; i++) {

			if (i < 5) {
				arraySpinner[i] = (i + 1) + " GB";
			}

		}
		arraySpinner[5] = "10 GB";
		arraySpinner[6] = "20 GB";

		ArrayAdapter<?> sizeAdapter = new ArrayAdapter<Object>(this, android.R.layout.simple_spinner_item,
				arraySpinner);
		sizeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		size.setAdapter(sizeAdapter);
		setPlusParams.addRule(RelativeLayout.BELOW, imageNameView.getId());
		mLayout.addView(size, setPlusParams);

		// TODO: Not working for now
		// final TextView preallocText = new TextView(this);
		// preallocText.setText("Preallocate? ");
		// preallocText.setTextSize(15);
		// RelativeLayout.LayoutParams preallocTParams = new
		// RelativeLayout.LayoutParams(
		// RelativeLayout.LayoutParams.WRAP_CONTENT,
		// RelativeLayout.LayoutParams.WRAP_CONTENT);
		// preallocTParams.addRule(RelativeLayout.BELOW, size.getId());
		// mLayout.addView(preallocText, preallocTParams);
		// preallocText.setId(64512044);
		//
		// final CheckBox prealloc = new CheckBox(this);
		// RelativeLayout.LayoutParams preallocParams = new
		// RelativeLayout.LayoutParams(
		// RelativeLayout.LayoutParams.WRAP_CONTENT,
		// RelativeLayout.LayoutParams.WRAP_CONTENT);
		// preallocParams.addRule(RelativeLayout.BELOW, size.getId());
		// preallocParams.addRule(RelativeLayout.RIGHT_OF,
		// preallocText.getId());
		// preallocParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT,
		// preallocText.getId());
		// mLayout.addView(prealloc, preallocParams);
		// prealloc.setId(64512344);

		alertDialog.setView(mLayout);

		final Handler handler = this.handler;

		// alertDialog.setMessage(body);
		alertDialog.setButton("Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				int sizeSel = size.getSelectedItemPosition();
				String templateImage = "hd1g.qcow2";
				if (sizeSel < 5) {
					templateImage = "hd" + (sizeSel + 1) + "g.qcow2";
				} else if (sizeSel == 5) {
					templateImage = "hd10g.qcow2";
				} else if (sizeSel == 6) {
					templateImage = "hd20g.qcow2";
				}

				// UIUtils.log("Searching...");
				EditText a = (EditText) alertDialog.findViewById(201012010);
				progDialog = ProgressDialog.show(activity, "Please Wait", "Creating HD Image...", true);
				// CreateImage createImg = new
				// CreateImage(a.getText().toString(),
				// hd_string, sizeInt, prealloc.isChecked());
				// CreateImage createImg = new
				// CreateImage(a.getText().toString(),
				// hd_string, sizeInt, false);
				// createImg.execute();

				String image = a.getText().toString();
				if (!image.endsWith(".qcow2")) {
					image += ".qcow2";
				}
				createImg(templateImage, image, hd_string);

			}
		});
		alertDialog.show();

	}

	protected boolean createImg(String templateImage, String destImage, String hd_string) {
		// TODO Auto-generated method stub

		boolean fileCreated = FileInstaller.installFile(activity, templateImage,
				Config.machinedir + currMachine.machinename, "hdtemplates", destImage);
		try {
			sendHandlerMessage(handler, Config.IMG_CREATED, new String[] { "image_name", "hd" },
					new String[] { destImage, hd_string });
		} catch (Exception e) {
			e.printStackTrace();
		}
		return fileCreated;
	}

	private class ExportMachines extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {

			// Export
			String machinesToExport = machineDB.exportMachines();
			FileUtils.saveFileContents(Config.DBFile, machinesToExport);

			return null;
		}

		@Override
		protected void onPostExecute(Void test) {

			sendHandlerMessage(handler, Config.VM_EXPORT);

		}
	}

	private class ImportMachines extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {
			// Import
			ArrayList<Machine> machines = FileUtils.getVMs(Config.DBFile);
			if (machines == null) {
				return null;
			}
			for (int i = 0; i < machines.size(); i++) {
				Machine machine = machines.get(i);
				if (machineDB.getMachine(machine.machinename, "") != null) {
					machineDB.deleteMachine(machine);
				}
				machineDB.insertMachine(machine);
				addDriveToList(machine.cd_iso_path, "cdrom");
				addDriveToList(machine.hda_img_path, "hda");
				addDriveToList(machine.hdb_img_path, "hdb");
				addDriveToList(machine.fda_img_path, "fda");
				addDriveToList(machine.fdb_img_path, "fdb");
				addDriveToList(machine.sd_img_path, "sd");
				addDriveToList(machine.kernel, "kernel");
				addDriveToList(machine.initrd, "initrd");
			}

			return null;
		}

		@Override
		protected void onPostExecute(Void test) {

			sendHandlerMessage(handler, Config.VM_IMPORT);

		}
	}

	public boolean onKeyDown(int keyCode, KeyEvent event) {

		if (keyCode == KeyEvent.KEYCODE_BACK) {
			moveTaskToBack(true);
			return true; // return
		}

		return false;
	}

	public void promptStateName(final Activity activity) {
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Snapshot/State Name");
		EditText searchView = new EditText(activity);
		searchView.setEnabled(true);
		searchView.setVisibility(View.VISIBLE);
		searchView.setId(201012010);
		searchView.setSingleLine();
		alertDialog.setView(searchView);
		final Handler handler = this.handler;

		// alertDialog.setMessage(body);
		alertDialog.setButton("Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				// UIUtils.log("Searching...");
				EditText a = (EditText) alertDialog.findViewById(201012010);
				sendHandlerMessage(handler, Config.SNAPSHOT_CREATED, new String[] { "snapshot_name" },
						new String[] { a.getText().toString() });
				return;
			}
		});
		alertDialog.show();

	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		if (resultCode == Config.FILEMAN_RETURN_CODE) {
			// Read from activity
			String currDir = LimboSettingsManager.getLastDir(this);
			String file = "";
			String fileType = "";
			Bundle b = data.getExtras();
			fileType = b.getString("fileType");
			file = b.getString("file");
			currDir = b.getString("currDir");
			if (currDir != null && !currDir.trim().equals("")) {
				LimboSettingsManager.setLastDir(this, currDir);
			}
			if (fileType != null && file != null) {
				setDriveAttr(fileType, file);
			}

		} else if (resultCode == Config.VNC_RESET_RESULT_CODE) {
			Toast.makeText(getApplicationContext(), "Resizing Display", Toast.LENGTH_SHORT).show();
			this.startvnc();

		} else if (resultCode == Config.SDL_QUIT_RESULT_CODE) {

			Toast.makeText(getApplicationContext(), "SDL Quit", Toast.LENGTH_SHORT).show();
			if (LimboActivity.vmexecutor != null) {
				LimboActivity.vmexecutor.stopvm(0);
			} else if (activity.getParent() != null) {
				activity.getParent().finish();
			}

			activity.finish();

		} else if (requestCode == Config.REQUEST_SDCARD_CODE) {
			if (data != null) {
				Uri uri = data.getData();
				DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
				String file = uri.toString();
				final int takeFlags = data.getFlags()
						& (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
				getContentResolver().takePersistableUriPermission(uri, takeFlags);
				setDriveAttr(filetype, file);

			}

		}

		// Check if says open

	}

	private void setDriveAttr(String fileType, String file) {
		// TODO Auto-generated method stub
		this.addDriveToList(file, fileType);
		if (fileType != null && fileType.startsWith("hd") && file != null && !file.trim().equals("")) {

			if (fileType.startsWith("hda")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.HDA, file);
				if (this.hdaAdapter.getPosition(file) < 0) {
					this.hdaAdapter.add(file);
				}
				this.setHDA(file, false);
			} else if (fileType.startsWith("hdb")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.HDB, file);
				if (this.hdbAdapter.getPosition(file) < 0) {
					this.hdbAdapter.add(file);
				}
				this.setHDB(file, false);
			} else if (fileType.startsWith("hdc")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.HDC, file);
				if (this.hdcAdapter.getPosition(file) < 0) {
					this.hdcAdapter.add(file);
				}
				this.setHDC(file, false);
			} else if (fileType.startsWith("hdd")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.HDD, file);
				if (this.hddAdapter.getPosition(file) < 0) {
					this.hddAdapter.add(file);
				}
				this.setHDD(file, false);
			}
		} else if (fileType != null && fileType.startsWith("cd") && file != null && !file.trim().equals("")) {
			int ret = machineDB.update(currMachine, MachineOpenHelper.CDROM, file);
			if (this.cdromAdapter.getPosition(file) < 0) {
				this.cdromAdapter.add(file);
			}
			setCDROM(file, false);
		} else if (fileType != null && fileType.startsWith("fd") && file != null && !file.trim().equals("")) {
			if (fileType.startsWith("fda")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.FDA, file);
				if (this.fdaAdapter.getPosition(file) < 0) {
					this.fdaAdapter.add(file);
				}
				this.setFDA(file, false);
			} else if (fileType.startsWith("fdb")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.FDB, file);
				if (this.fdbAdapter.getPosition(file) < 0) {
					this.fdbAdapter.add(file);
				}
				this.setFDB(file, false);
			}
		} else if (fileType != null && fileType.startsWith("sd") && file != null && !file.trim().equals("")) {
			if (fileType.startsWith("sd")) {
				int ret = machineDB.update(currMachine, MachineOpenHelper.SD, file);
				if (this.sdAdapter.getPosition(file) < 0) {
					this.sdAdapter.add(file);
				}
				this.setSD(file, false);
			}
		} else if (fileType != null && fileType.startsWith("kernel") && file != null && !file.trim().equals("")) {

			int ret = machineDB.update(currMachine, MachineOpenHelper.KERNEL, file);
			if (this.kernelAdapter.getPosition(file) < 0) {
				this.kernelAdapter.add(file);
			}
			this.setKernel(file, false);

		} else if (fileType != null && fileType.startsWith("initrd") && file != null && !file.trim().equals("")) {

			int ret = machineDB.update(currMachine, MachineOpenHelper.INITRD, file);
			if (this.initrdAdapter.getPosition(file) < 0) {
				this.initrdAdapter.add(file);
			}
			this.setInitrd(file, false);

		}

		int res = this.mHDA.getSelectedItemPosition();
		if (res == 1) {
			this.mHDA.setSelection(0);
		}
		res = this.mHDB.getSelectedItemPosition();
		if (res == 1) {
			this.mHDB.setSelection(0);
		}

		res = this.mHDC.getSelectedItemPosition();
		if (res == 1) {
			this.mHDC.setSelection(0);
		}

		res = this.mHDD.getSelectedItemPosition();
		if (res == 1) {
			this.mHDD.setSelection(0);
		}

		res = this.mCD.getSelectedItemPosition();
		if (res == 1) {
			this.mCD.setSelection(0);
		}

		res = this.mFDA.getSelectedItemPosition();
		if (res == 1) {
			this.mFDA.setSelection(0);
		}

		res = this.mFDB.getSelectedItemPosition();
		if (res == 1) {
			this.mFDB.setSelection(0);

		}

		res = this.mSD.getSelectedItemPosition();
		if (res == 1) {
			this.mSD.setSelection(0);

		}

		res = this.mKernel.getSelectedItemPosition();
		if (res == 1) {
			this.mKernel.setSelection(0);

		}

		res = this.mInitrd.getSelectedItemPosition();
		if (res == 1) {
			this.mInitrd.setSelection(0);

		}
	}

	@Override
	public void onStop() {
		super.onStop();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		this.stopTimeListener();

	}

	private ConnectionBean selected;
	private String filetype;

	private void startvnc() {

		// Wait till Qemu settles
		try {
			Thread.sleep(2000);
		} catch (InterruptedException ex) {
			Logger.getLogger(LimboActivity.class.getName()).log(Level.SEVERE, null, ex);
		}

		if (this.mVNCAllowExternal.isChecked() && vnc_passwd != null && !vnc_passwd.equals("")) {
			String ret = vmexecutor.change_vnc_password();
			// toastLong(activity, ret);
			// toastLong(activity, "You can now connect with your External VNC
			// Viewer");
			promptConnectLocally(activity);
		} else {
			Intent intent = new Intent(this, LimboVNCActivity.class);
			startActivityForResult(intent, Config.VNC_REQUEST_CODE);
		}

	}

	public void promptConnectLocally(final Activity activity) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("VNC Started");
		TextView stateView = new TextView(activity);
		stateView.setText("VNC Server started: " + this.getLocalIpAddress() + ":" + "5901\n"
				+ "Warning: VNC Connection is Unencrypted and not secure make sure you're on a private network!\n");

		stateView.setId(201012555);
		stateView.setPadding(5, 5, 5, 5);
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				alertDialog.dismiss();
			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Connect Locally", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				Intent intent = new Intent(activity, LimboVNCActivity.class);
				startActivityForResult(intent, Config.VNC_REQUEST_CODE);
			}
		});
		alertDialog.show();

	}

	private void startsdl() {

		Intent intent = null;
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			intent = new Intent(this, LimboSDLActivityCompat.class);
		} else {
			intent = new Intent(this, LimboSDLActivity.class);
		}
		android.content.ContentValues values = new android.content.ContentValues();
		startActivityForResult(intent, Config.SDL_REQUEST_CODE);
	}

	// Start calling the JNI interface
	public static void startvm(Activity activity, int UI) {
		if (UI == Config.UI_VNC) {
			// disable sound card with VNC
			vmexecutor.enablevnc = 1;
			vmexecutor.enablespice = 0;
			vmexecutor.sound_card = null;
			vmexecutor.vnc_allow_external = vnc_allow_external;
			vmexecutor.vnc_passwd = vnc_passwd;
		} else if (UI == Config.UI_SDL) {
			vmexecutor.enablevnc = 0;
			vmexecutor.enablespice = 0;
		} else if (UI == Config.UI_SPICE) {
			vmexecutor.vnc_allow_external = vnc_allow_external;
			vmexecutor.vnc_passwd = vnc_passwd;
			vmexecutor.enablevnc = 0;
			vmexecutor.enablespice = 1;
		}
		vmexecutor.startvm(activity);

	}

	public void restartvm() {
		if (vmexecutor != null) {

			output = vmexecutor.stopvm(1);
			sendHandlerMessage(handler, Config.VM_RESTARTED);
		} else {

			sendHandlerMessage(handler, Config.VM_NOTRUNNING);
		}

	}

	public void savevm(String name) {
		if (vmexecutor != null) {
			if (//
			(currMachine.hda_img_path == null || currMachine.hda_img_path.equals(""))
					&& (currMachine.hdb_img_path == null || currMachine.hdb_img_path.equals(""))
					&& (currMachine.hdc_img_path == null || currMachine.hdc_img_path.equals(""))
					&& (currMachine.hdd_img_path == null || currMachine.hdd_img_path.equals(""))) {
				sendHandlerMessage(handler, Config.VM_NO_QCOW2);
			} else {

				output = vmexecutor.savevm("test_snapshot");
				sendHandlerMessage(handler, Config.VM_SAVED);
			}
		} else {

			sendHandlerMessage(handler, Config.VM_NOTRUNNING);
		}

	}

	public void resumevm() {
		if (vmexecutor != null) {
			output = vmexecutor.resumevm();
			sendHandlerMessage(handler, Config.VM_RESTARTED);
		} else {

			sendHandlerMessage(handler, Config.VM_NOTRUNNING);
		}

	}

	// Set Hard Disk
	private void populateRAM() {
		this.userPressedRAM = false;

		String[] arraySpinner = new String[128];

		arraySpinner[0] = 4 + "";
		for (int i = 1; i < arraySpinner.length; i++) {
			arraySpinner[i] = i * 8 + "";
		}
		;

		ramAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		ramAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mRamSize.setAdapter(ramAdapter);
		this.userPressedRAM = false;
		this.mRamSize.invalidate();
	}

	private void populateCPUNum() {
		this.userPressedCPUNum = false;

		String[] arraySpinner = new String[4];

		for (int i = 0; i < arraySpinner.length; i++) {
			arraySpinner[i] = (i + 1) + "";
		}
		;

		cpuNumAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		cpuNumAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mCPUNum.setAdapter(cpuNumAdapter);
		this.userPressedCPUNum = false;
		this.mCPUNum.invalidate();
	}

	// Set Hard Disk
	private void setRAM(int ram, boolean userPressed) {
		this.userPressedRAM = userPressed;
		if (ram != 0) {
			int pos = ramAdapter.getPosition(ram + "");
			mRamSize.setSelection(pos);
		} else {
			this.userPressedRAM = true;
		}
	}

	private void setCPUNum(int cpuNum, boolean userPressed) {
		this.userPressedCPUNum = userPressed;
		if (cpuNum != 0) {
			int pos = cpuNumAdapter.getPosition(cpuNum + "");
			mCPUNum.setSelection(pos);
		} else {
			this.userPressedCPUNum = true;
		}
	}

	// Set Hard Disk
	private void populateBootDevices() {

		String[] arraySpinner = { "Default", "CD Rom", "Floppy", "Hard Disk" };

		bootDevAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		bootDevAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mBootDevices.setAdapter(bootDevAdapter);
		this.mBootDevices.invalidate();
	}

	// Set Net Cfg
	private void populateNet() {
		String[] arraySpinner = { "None", "User", "TAP" };
		netAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		netAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mNetConfig.setAdapter(netAdapter);
		this.mNetConfig.invalidate();
	}

	// Set VGA Cfg
	private void populateVGA() {

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		arrList.add("std");
		arrList.add("cirrus");
		arrList.add("vmware");

		if (Config.enable_SPICE_menu)
			arrList.add("qxl");

		// Add XEN
		// "xenfb",
		// None for console only
		// "none"

		vgaAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
		vgaAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mVGAConfig.setAdapter(vgaAdapter);
		this.mVGAConfig.invalidate();
	}

	private void populateOrientation() {

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		arrList.add("Orientation (Auto)");
		arrList.add("Landscape");
		arrList.add("Landscape Reverse");
		// arrList.add("Portrait");
		// arrList.add("Portrait Reverse");

		orientationAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
		orientationAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mOrientation.setAdapter(orientationAdapter);
		this.mOrientation.invalidate();

		int pos = LimboSettingsManager.getOrientationSetting(activity);
		if (pos >= 0) {
			this.mOrientation.setSelection(pos);
		}
	}

	private void populateKeyboardLayout() {

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		arrList.add("English");
		arrList.add("Spanish");
		arrList.add("French");

		keyboardAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
		keyboardAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mKeyboard.setAdapter(keyboardAdapter);
		this.mKeyboard.invalidate();

		int pos = LimboSettingsManager.getKeyboardSetting(activity);
		if (pos >= 0) {
			this.mKeyboard.setSelection(pos);
		}
	}

	private void populateSoundcardConfig() {

		String[] arraySpinner = { "None", "sb16", "ac97", "adlib", "cs4231a", "gus", "es1370", "hda", "pcspk", "all" };

		sndAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		sndAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mSoundCardConfig.setAdapter(sndAdapter);
		this.mSoundCardConfig.invalidate();
	}

	// Set Cache Cfg
	private void populateHDCacheConfig() {

		String[] arraySpinner = { "default", "none", "writeback", "writethrough" };

		hdCacheAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		hdCacheAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mHDCacheConfig.setAdapter(hdCacheAdapter);
		this.mHDCacheConfig.invalidate();
	}

	// Set Hard Disk
	private void populateNetDevices(String nic) {

		String[] arraySpinner = { "e1000", "pcnet", "rtl8139", "ne2k_pci", "i82551", "i82557b", "i82559er", "virtio" };

		ArrayList<String> arrList = new ArrayList<String>();

		if (currMachine != null) {
			if (currMachine.arch.equals("x86")) {

				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
			} else if (currMachine.arch.equals("x64")) {
				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
			} else if (currMachine.arch.equals("ARM")) {
				arrList.add("smc91c111");
			}
		} else {
			arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		}

		if (nicCfgAdapter == null) {
			nicCfgAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
			nicCfgAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			this.mNetDevices.setAdapter(nicCfgAdapter);
		} else {
			nicCfgAdapter.clear();
			for (int i = 0; i < arrList.size(); i++) {
				nicCfgAdapter.add(arrList.get(i));
			}

		}

		this.mNetDevices.invalidate();

		int pos = this.nicCfgAdapter.getPosition(nic);
		if (pos >= 0) {
			this.mNetDevices.setSelection(pos);
		}
	}

	private void setMachine(String machine) {
		if (machine != null) {
			int pos = machineAdapter.getPosition(machine);
			this.mMachine.setSelection(pos);
		} else {
			userPressedMachine = true;
		}
	}

	// Set Hard Disk
	private void populateMachines() {
		this.userPressedMachine = false;
		ArrayList<String> machines = machineDB.getMachines();
		int length = 0;
		if (machines == null || machines.size() == 0) {
			length = 0;
		} else {
			length = machines.size();
		}

		String[] arraySpinner = new String[machines.size() + 2];
		arraySpinner[0] = "None";
		arraySpinner[1] = "New";
		int index = 2;
		Iterator<String> i = machines.iterator();
		while (i.hasNext()) {
			String file = (String) i.next();
			if (file != null) {
				arraySpinner[index++] = file;
			}
		}

		machineAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		machineAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mMachine.setAdapter(machineAdapter);
		this.mMachine.invalidate();

	}

	// Set Hard Disk
	private void setCPU(String cpu, boolean userPressed) {
		this.userPressedCPU = userPressed;

		if (cpu != null) {
			int pos = cpuAdapter.getPosition(cpu);

			mCPU.setSelection(pos);
		} else {
			this.userPressedCPU = true;

		}
	}

	private void setArch(String arch, boolean userPressed) {
		this.userPressedArch = userPressed;

		if (arch != null) {
			int pos = archAdapter.getPosition(arch);

			mArch.setSelection(pos);
		} else {
			this.userPressedArch = true;

		}
	}

	private void setMachineType(String machineType, boolean userPressed) {
		this.userPressedMachineType = userPressed;

		if (machineType != null) {
			int pos = machineTypeAdapter.getPosition(machineType);
			mMachineType.setSelection(pos);
		} else {
			this.userPressedMachineType = true;

		}
	}

	private void setCDROM(String cdrom, boolean userPressed) {
		this.userPressedCDROM = userPressed;
		this.currMachine.cd_iso_path = cdrom;

		if (cdrom != null) {
			int pos = cdromAdapter.getPosition(cdrom);

			if (pos > 1) {
				mCD.setSelection(pos);
			} else {
				mCD.setSelection(0);
			}
		} else {
			mCD.setSelection(0);

		}
	}

	private void setKernel(String kernel, boolean userPressed) {
		this.userPressedKernel = userPressed;

		currMachine.kernel = kernel;
		if (kernel != null) {
			int pos = kernelAdapter.getPosition(kernel);

			if (pos >= 0) {
				mKernel.setSelection(pos);
			} else {
				mKernel.setSelection(0);
			}
		} else {
			mKernel.setSelection(0);

		}
	}

	private void setInitrd(String initrd, boolean userPressed) {
		this.userPressedInitrd = userPressed;

		currMachine.initrd = initrd;
		if (initrd != null) {
			int pos = initrdAdapter.getPosition(initrd);

			if (pos >= 0) {
				mInitrd.setSelection(pos);
			} else {
				mInitrd.setSelection(0);
			}
		} else {
			mInitrd.setSelection(0);

		}
	}

	private void setHDA(String hda, boolean userPressed) {
		this.userPressedHDA = userPressed;
		currMachine.hda_img_path = hda;
		if (hda != null) {
			int pos = hdaAdapter.getPosition(hda);

			if (pos >= 0) {
				mHDA.setSelection(pos);
			} else {
				mHDA.setSelection(0);
			}
		} else {
			mHDA.setSelection(0);

		}
	}

	private void setHDB(String hdb, boolean userPressed) {
		this.userPressedHDB = userPressed;
		this.currMachine.hdb_img_path = hdb;

		if (hdb != null) {
			int pos = hdbAdapter.getPosition(hdb);

			if (pos >= 0) {
				mHDB.setSelection(pos);
			} else {
				mHDB.setSelection(0);
			}
		} else {
			mHDB.setSelection(0);

		}
	}

	private void setHDC(String hdc, boolean userPressed) {
		this.userPressedHDC = userPressed;
		this.currMachine.hdc_img_path = hdc;

		if (hdc != null) {
			int pos = hdcAdapter.getPosition(hdc);

			if (pos >= 0) {
				mHDC.setSelection(pos);
			} else {
				mHDC.setSelection(0);
			}
		} else {
			mHDC.setSelection(0);

		}
	}

	private void setHDD(String hdd, boolean userPressed) {
		this.userPressedHDD = userPressed;
		this.currMachine.hdd_img_path = hdd;

		if (hdd != null) {
			int pos = hddAdapter.getPosition(hdd);

			if (pos >= 0) {
				mHDD.setSelection(pos);
			} else {
				mHDD.setSelection(0);
			}
		} else {
			mHDD.setSelection(0);

		}
	}

	private void setFDA(String fda, boolean userPressed) {
		this.userPressedFDA = userPressed;
		this.currMachine.fda_img_path = fda;

		if (fda != null) {
			int pos = fdaAdapter.getPosition(fda);

			if (pos >= 0) {
				mFDA.setSelection(pos);
			} else {
				mFDA.setSelection(0);
			}
		} else {
			mFDA.setSelection(0);

		}
	}

	private void setFDB(String fdb, boolean userPressed) {
		this.userPressedFDB = userPressed;
		this.currMachine.fdb_img_path = fdb;

		if (fdb != null) {
			int pos = fdbAdapter.getPosition(fdb);

			if (pos >= 0) {
				mFDB.setSelection(pos);
			} else {
				mFDB.setSelection(0);
			}
		} else {
			mFDB.setSelection(0);

		}
	}

	private void setSD(String sd, boolean userPressed) {
		this.userPressedFDB = userPressed;
		this.currMachine.sd_img_path = sd;

		if (sd != null) {
			int pos = sdAdapter.getPosition(sd);

			if (pos >= 0) {
				mSD.setSelection(pos);
			} else {
				mSD.setSelection(0);
			}
		} else {
			mSD.setSelection(0);

		}
	}

	private void setHDCache(String hdcache, boolean userPressed) {
		this.userPressedHDCacheCfg = userPressed;

		if (hdcache != null) {
			int pos = this.hdCacheAdapter.getPosition(hdcache);

			if (pos >= 0) {
				this.mHDCacheConfig.setSelection(pos);
			} else {
				mHDCacheConfig.setSelection(0);
			}
		} else {
			mHDCacheConfig.setSelection(0);

		}
	}

	private void setSoundcard(String soundcard, boolean userPressed) {
		this.userPressedSoundcardCfg = userPressed;

		if (soundcard != null) {
			int pos = this.sndAdapter.getPosition(soundcard);

			if (pos >= 0) {
				this.mSoundCardConfig.setSelection(pos);
			} else {
				mSoundCardConfig.setSelection(0);
			}
		} else {
			mSoundCardConfig.setSelection(0);
		}
	}

	private void setUI(String ui, boolean userPressed) {
		this.userPressedUI = userPressed;

		if (ui != null) {
			int pos = this.uiAdapter.getPosition(ui);

			if (pos >= 0) {
				this.mUI.setSelection(pos);
			} else {
				mUI.setSelection(0);
			}
		} else {
			mUI.setSelection(0);

		}
	}

	private void setVGA(String vga, boolean userPressed) {
		this.userPressedVGACfg = userPressed;

		if (vga != null) {
			int pos = vgaAdapter.getPosition(vga);

			if (pos >= 0) {
				this.mVGAConfig.setSelection(pos);
			} else {
				mVGAConfig.setSelection(0);
			}
		} else {
			mVGAConfig.setSelection(0);

		}
	}

	private void setNetCfg(String net, boolean userPressed) {
		this.userPressedNetCfg = userPressed;

		if (net != null) {
			int pos = this.netAdapter.getPosition(net);

			if (pos >= 0) {
				this.mNetConfig.setSelection(pos);
			} else {
				mNetConfig.setSelection(0);
			}
		} else {
			mNetConfig.setSelection(0);

		}
	}

	private void setBootDevice(String bootDevice, boolean userPressed) {
		this.userPressedBootDev = userPressed;

		if (bootDevice != null) {
			int pos = this.bootDevAdapter.getPosition(bootDevice);

			if (pos >= 0) {
				this.mBootDevices.setSelection(pos);
			} else {
				mBootDevices.setSelection(0);
			}
		} else {
			mBootDevices.setSelection(0);

		}
	}

	private void setSnapshot(String snapshot, boolean userPressed) {
		this.userPressedSnapshot = userPressed;
		if (snapshot != null && !snapshot.equals("")) {
			int pos = this.snapshotAdapter.getPosition(snapshot);

			if (pos >= 0) {
				this.mSnapshot.setSelection(pos);
				this.mSnapshot.invalidate();
			} else {
				mSnapshot.setSelection(0);
			}
		} else {
			mSnapshot.setSelection(0);

		}

	}

	private void setNicDevice(String nic, boolean userPressed) {
		this.userPressedNicCfg = userPressed;
		if (nic != null) {
			int pos = this.nicCfgAdapter.getPosition(nic);

			if (pos >= 0) {
				this.mNetDevices.setSelection(pos);
			} else {
				mNetDevices.setSelection(3);
			}
		} else {
			mNetDevices.setSelection(3);

		}
	}

	private void populateCPUs(String cpu) {
		this.userPressedCPU = false;

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		
		//x86 cpus 32 bit
		ArrayList<String> arrX86 = new ArrayList<String>();
		arrX86.add("Default");
		arrX86.add("qemu32");// QEMU Virtual CPU version 2.3.0
		arrX86.add("kvm32");// Common 32-bit KVM processor
		arrX86.add("coreduo");// Genuine Intel(R) CPU T2600 @ 2.16GHz
		arrX86.add("486");
		arrX86.add("pentium");
		arrX86.add("pentium2");
		arrX86.add("pentium3");
		arrX86.add("athlon"); //QEMU Virtual CPU version 2.3.0                  
		arrX86.add("n270"); //Intel(R) Atom(TM) CPU N270   @ 1.60GHz     
		
		//x86 cpus 64 bit
		ArrayList<String> arrX86_64 = new ArrayList<String>();
		arrX86_64.add("Default");
		arrX86_64.add("qemu64");// QEMU Virtual CPU version 2.3.0
		arrX86_64.add("kvm64");// Common KVM processor
		arrX86_64.add("phenom");// AMD Phenom(tm) 9550 Quad-Core Processor
		arrX86_64.add("core2duo");// Intel(R) Core(TM)2 Duo CPU T7700 @
								// 2.40GHz
		
		arrX86_64.add("Conroe");// Intel Celeron_4x0 (Conroe/Merom Class Core
								// 2)
		arrX86_64.add("Penryn");// Intel Core 2 Duo P9xxx (Penryn Class Core
								// 2)
		arrX86_64.add("Nehalem");// Intel Core i7 9xx (Nehalem Class Core i7)
		arrX86_64.add("Westmer");// e Westmere E56xx/L56xx/X56xx (Nehalem-C)
		arrX86_64.add("SandyBridge");// Intel Xeon E312xx (Sandy Bridge)
		arrX86_64.add("IvyBridge");// Intel Xeon E3-12xx v2 (Ivy Bridge)
		arrX86_64.add("Haswell-noTSX");// Intel Core Processor (Haswell, no
										// TSX)
		arrX86_64.add("Haswell");// Intel Core Processor (Haswell)
		arrX86_64.add("Broadwell-noTSX");// Intel Core Processor (Broadwell,
										// no TSX)
		arrX86_64.add("Broadwell");// Intel Core Processor (Broadwell)
		arrX86_64.add("Opteron_G1");// AMD Opteron 240 (Gen 1 Class Opteron)
		arrX86_64.add("Opteron_G2");// AMD Opteron 22xx (Gen 2 Class Opteron)
		arrX86_64.add("Opteron_G3");// AMD Opteron 23xx (Gen 3 Class Opteron)
		arrX86_64.add("Opteron_G4");// AMD Opteron 62xx class CPU
		arrX86_64.add("Opteron_G5");// AMD Opteron 63xx class CPU
		
		//ARM cpus
		ArrayList<String> arrARM = new ArrayList<String>();
		arrARM.add("arm926");
		arrARM.add("arm946");
		arrARM.add("arm1026");
		arrARM.add("arm1136");
		arrARM.add("arm1136-r2");
		arrARM.add("arm1176");
		arrARM.add("arm11mpcore");
		arrARM.add("cortex-m3");
		arrARM.add("cortex-a8");
		arrARM.add("cortex-a8-r2");
		arrARM.add("cortex-a9");
		arrARM.add("cortex-a15");

		// "arm1136 (arm)", "arm1136-r2 (arm)", "arm1176 (arm)",
		// "arm11mpcore (arm)", "cortex-m3 (arm)", "cortex-a8 (arm)",
		// "cortex-a8-r2 (arm)", "cortex-a9 (arm)", "cortex-a15 (arm)",

		// "ti925t (arm)", "pxa250 (arm)", "sa1100 (arm)", "sa1110
		// (arm)",
		// "pxa255 (arm)", "pxa260 (arm)", "pxa261 (arm)", "pxa262
		// (arm)",
		// "pxa270 (arm)", "pxa270-a0 (arm)", "pxa270-a1 (arm)",
		// "pxa270-b0 (arm)", "pxa270-b1 (arm)", "pxa270-c0 (arm)",
		// "pxa270-c5 (arm)", "any (arm)"
		
		if (currMachine != null) {
			if (currMachine.arch.equals("x86")){
				arrList.addAll(arrX86);
			} else if (currMachine.arch.equals("x64")) {
				arrList.addAll(arrX86_64);
			}else if (currMachine.arch.equals("ARM")) {
				arrList.addAll(arrARM);
			}
		} else {
			arrList.addAll(arrX86);
		}

		if (cpuAdapter == null) {
			cpuAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
			cpuAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			this.mCPU.setAdapter(cpuAdapter);
		} else {
			cpuAdapter.clear();
			for (int i = 0; i < arrList.size(); i++) {
				cpuAdapter.add(arrList.get(i));
			}

		}

		this.mCPU.invalidate();

		int pos = this.cpuAdapter.getPosition(cpu);
		if (pos >= 0) {
			this.mCPU.setSelection(pos);
		}

	}

	private void populateArch() {
		this.userPressedArch = false;

		String[] arraySpinner = { "x86", "x64" };

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));

		if (Config.enable_ARM)
			arrList.add("ARM");

		archAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);

		archAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mArch.setAdapter(archAdapter);

		this.mArch.invalidate();

	}

	private void populateMachineType(String machineType) {
		this.userPressedMachineType = false;

		String[] arraySpinner = {
				// "None", "Default"

				// "beagle - Beagle board (OMAP3530)",
				// "beaglexm - Beagle board XM (OMAP3630)",
				//// "collie - Collie PDA (SA-1110)",
				// "nuri - Samsung NURI board (Exynos4210)",
				// "smdkc210 - Samsung SMDKC210 board (Exynos4210)",
				//// "connex - Gumstix Connex (PXA255)",
				//// "verdex - Gumstix Verdex (PXA270)",
				//// "highbank - Calxeda Highbank (ECX-1000)",
				// "integratorcp - ARM Integrator/CP (ARM926EJ-S) (default)",
				//// "mainstone - Mainstone II (PXA27x)",
				//// "musicpal - Marvell 88w8618 / MusicPal (ARM926EJ-S)",
				// "n800 - Nokia N800 tablet aka. RX-34 (OMAP2420)", "n810 -
				// Nokia N810 tablet aka. RX-44 (OMAP2420)",
				// "n900 - Nokia N900 (OMAP3)", "netduino2 - Netduino 2
				// Machine",
				//// "sx1 - Siemens SX1 (OMAP310) V2",
				//// "sx1-v1 - Siemens SX1 (OMAP310) V1",
				//// "overo - Gumstix Overo board (OMAP3530)",
				//// "cheetah - Palm Tungsten|E aka. Cheetah PDA (OMAP310)",
				//// "realview-eb - ARM RealView Emulation Baseboard
				// (ARM926EJ-S)",
				//// "realview-eb-mpcore - ARM RealView Emulation Baseboard
				// (ARM11MPCore)",
				// //"realview-pb-a8 - ARM RealView Platform Baseboard for
				// Cortex-A8",
				// "realview-pbx-a9 - ARM RealView Platform Baseboard Explore
				// for Cortex-A9",
				//// "akita - Akita PDA (PXA270)",
				//// "spitz - Spitz PDA (PXA270)",
				// "smdkc210 - Samsung SMDKC210 board (Exynos4210)",
				//// "borzoi - Borzoi PDA (PXA270)",
				//// "terrier - Terrier PDA (PXA270)",
				//// "lm3s811evb - Stellaris LM3S811EVB",
				//// "lm3s6965evb - Stellaris LM3S6965EVB",
				//// "tosa - Tosa PDA (PXA255)",
				// "versatilepb - ARM Versatile/PB (ARM926EJ-S)", "versatileab -
				// ARM Versatile/AB (ARM926EJ-S)",
				// "vexpress-a9 - ARM Versatile Express for Cortex-A9",
				// "vexpress-a15 - ARM Versatile Express for Cortex-A15", "virt
				// - ARM Virtual Machine",
				// "xilinx-zynq-a9 - Xilinx Zynq Platform Baseboard for
				// Cortex-A9",
				//// "z2 - Zipit Z2 (PXA27x)",
		};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));

		if (currMachine != null) {
			if (currMachine.arch.equals("x86")) {
				arrList.add("pc");
				arrList.add("q35");
				arrList.add("isapc");
			} else if (currMachine.arch.equals("x64")) {
				arrList.add("pc");
				arrList.add("q35");
				arrList.add("isapc");
			} else if (currMachine.arch.equals("ARM")) {
				arrList.add("integratorcp - ARM Integrator/CP (ARM926EJ-S) (default)");
				arrList.add("versatilepb - ARM Versatile/PB (ARM926EJ-S)");
				arrList.add("n800 - Nokia N800 tablet aka. RX-34 (OMAP2420)");
				arrList.add("n810 - Nokia N810 tablet aka. RX-44 (OMAP2420)");
				arrList.add("n900 - Nokia N900 (OMAP3)");
				arrList.add("netduino2 - Netduino 2 Machine");
				arrList.add("vexpress-a9 - ARM Versatile Express for Cortex-A9");
				arrList.add("vexpress-a15 - ARM Versatile Express for Cortex-A15");
				arrList.add("beagle - Beagle board (OMAP3530)");
				arrList.add("beaglexm - Beagle board XM (OMAP3630)");
				arrList.add("xilinx-zynq-a9 - Xilinx Zynq Platform Baseboard for Cortex-A9");
				arrList.add("smdkc210 - Samsung SMDKC210 board (Exynos4210)");
				arrList.add("virt - ARM Virtual Machine");
			}
		} else {
			arrList.add("pc");
			arrList.add("q35");
			arrList.add("isapc");
		}

		if (machineTypeAdapter == null) {
			machineTypeAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
			machineTypeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			this.mMachineType.setAdapter(machineTypeAdapter);
		} else {
			machineTypeAdapter.clear();
			for (int i = 0; i < arrList.size(); i++) {
				machineTypeAdapter.add(arrList.get(i));
			}

		}

		this.mMachineType.invalidate();
		int pos = this.machineTypeAdapter.getPosition(machineType);
		if (pos >= 0) {
			this.mMachineType.setSelection(pos);
		}

	}

	private void populateUI() {
		this.userPressedUI = false;

		String[] arraySpinner = { "VNC" };

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		if (Config.enable_SDL_menu)
			arrList.add("SDL");
		if (Config.enable_SPICE_menu)
			arrList.add("SPICE");

		uiAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arrList);
		uiAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mUI.setAdapter(uiAdapter);
		this.mUI.invalidate();
	}

	private void populateKernel() {
		// Add from History
		ArrayList<String> kernels = favDB.getFavURL("kernel");
		int length = 0;
		if (kernels == null || kernels.size() == 0) {
			length = 0;
		} else {
			length = kernels.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("Open");
		Iterator<String> i = kernels.iterator();
		while (i.hasNext()) {
			String file = (String) i.next();
			if (file != null) {
				arraySpinner.add(file);
			}
		}

		kernelAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		kernelAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mKernel.setAdapter(kernelAdapter);
		this.mKernel.invalidate();

	}

	private void populateInitrd() {
		// Add from History
		ArrayList<String> initrds = favDB.getFavURL("initrd");
		int length = 0;
		if (initrds == null || initrds.size() == 0) {
			length = 0;
		} else {
			length = initrds.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("Open");
		Iterator<String> i = initrds.iterator();
		while (i.hasNext()) {
			String file = (String) i.next();
			if (file != null) {
				arraySpinner.add(file);
			}
		}

		initrdAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		initrdAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mInitrd.setAdapter(initrdAdapter);
		this.mInitrd.invalidate();

	}

	// Set Hard Disk
	private void populateHD(String fileType) {
		// Add from History
		ArrayList<String> oldHDs = favDB.getFavURL(fileType);
		int length = 0;
		if (oldHDs == null || oldHDs.size() == 0) {
			length = 0;
		} else {
			length = oldHDs.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("New");
		arraySpinner.add("Open");
		Iterator<String> i = oldHDs.iterator();
		while (i.hasNext()) {
			String file = (String) i.next();
			if (file != null) {
				arraySpinner.add(file);
			}
		}

		if (fileType.equals("hda")) {

			hdaAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			hdaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mHDA.setAdapter(hdaAdapter);
			this.mHDA.invalidate();
		} else if (fileType.equals("hdb")) {
			hdbAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			hdbAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mHDB.setAdapter(hdbAdapter);
			this.mHDB.invalidate();
		} else if (fileType.equals("hdc")) {
			hdcAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			hdcAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mHDC.setAdapter(hdcAdapter);
			this.mHDC.invalidate();
		} else if (fileType.equals("hdd")) {
			hddAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			hddAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mHDD.setAdapter(hddAdapter);
			this.mHDD.invalidate();
		}
	}

	private void populateSnapshot() {
		// Add from History
		ArrayList<String> oldSnapshots = null;
		if (currMachine != null) {
			oldSnapshots = machineDB.getSnapshots(currMachine);
		}

		int length = 0;
		if (oldSnapshots == null) {
			length = 0;
		} else {
			length = oldSnapshots.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		if (oldSnapshots != null) {
			Iterator<String> i = oldSnapshots.iterator();
			while (i.hasNext()) {
				String file = (String) i.next();
				if (file != null) {
					arraySpinner.add(file);
				}
			}
		}
		this.userPressedSnapshot = false;

		snapshotAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, arraySpinner);
		snapshotAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.mSnapshot.setAdapter(snapshotAdapter);
		this.mSnapshot.invalidate();

		if (oldSnapshots == null) {
			this.mSnapshot.setEnabled(false);
		} else {
			this.mSnapshot.setEnabled(true);
		}

	}

	// Set CDROM
	private void populateCDRom(String fileType) {
		this.userPressedCDROM = false;
		// Add from History
		ArrayList<String> oldCDs = favDB.getFavURL(fileType);
		int length = 0;
		if (oldCDs == null || oldCDs.size() == 0) {
			length = 0;
		} else {
			length = oldCDs.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("Open");
		if (oldCDs != null) {
			Iterator<String> i = oldCDs.iterator();
			while (i.hasNext()) {
				String file = (String) i.next();
				if (file != null) {
					arraySpinner.add(file);
				}
			}
		}
		cdromAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		cdromAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mCD.setAdapter(cdromAdapter);
		this.mCD.invalidate();
	}

	// Set Hard Disk
	private void populateFloppy(String fileType) {
		// Add from History
		ArrayList<String> oldFDs = favDB.getFavURL(fileType);
		int length = 0;
		if (oldFDs == null || oldFDs.size() == 0) {
			length = 0;
		} else {
			length = oldFDs.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("Open");
		if (oldFDs != null) {
			Iterator<String> i = oldFDs.iterator();
			while (i.hasNext()) {
				String file = (String) i.next();
				if (file != null) {
					arraySpinner.add(file);
				}
			}
		}

		if (fileType.equals("fda")) {
			fdaAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			fdaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mFDA.setAdapter(fdaAdapter);
			this.mFDA.invalidate();
		} else if (fileType.equals("fdb")) {
			fdbAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			fdbAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mFDB.setAdapter(fdbAdapter);
			this.mFDB.invalidate();
		}
	}

	private void populateSDCard(String fileType) {
		// Add from History
		ArrayList<String> oldSDs = favDB.getFavURL(fileType);
		int length = 0;
		if (oldSDs == null || oldSDs.size() == 0) {
			length = 0;
		} else {
			length = oldSDs.size();
		}

		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		arraySpinner.add("Open");
		if (oldSDs != null) {
			Iterator<String> i = oldSDs.iterator();
			while (i.hasNext()) {
				String file = (String) i.next();
				if (file != null) {
					arraySpinner.add(file);
				}
			}
		}

		if (fileType.equals("sd")) {
			sdAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
			sdAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			this.mSD.setAdapter(sdAdapter);
			this.mSD.invalidate();
		}
	}

	public void browse(String fileType) {
		// Check if SD card is mounted
		String state = Environment.getExternalStorageState();
		if (!Environment.MEDIA_MOUNTED.equals(state)) {
			Toast.makeText(getApplicationContext(), "Error: SD card is not mounted", Toast.LENGTH_LONG).show();
			return;
		}

		if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP) {

			String dir = null;
			// GET THE LAST ACCESSED DIR FROM THE REG
			String lastDir = LimboSettingsManager.getLastDir(this);
			try {
				Intent i = null;
				i = getFileManIntent();
				Bundle b = new Bundle();
				b.putString("lastDir", lastDir);
				b.putString("fileType", fileType);
				i.putExtras(b);
				startActivityForResult(i, Config.FILEMAN_REQUEST_CODE);

			} catch (Exception e) {
				e.printStackTrace();
			}
		} else {
			this.filetype = fileType;
			LimboFileManager.promptSDCardAccess(activity, fileType);
		}
	}

	public Intent getFileManIntent() {
		return new Intent(LimboActivity.this, com.max2idea.android.limbo.main.LimboFileManager.class);
	}

	public Intent getVNCIntent() {
		return new Intent(LimboActivity.this, com.max2idea.android.limbo.main.LimboVNCActivity.class);
	}

	private void addDriveToList(String file, String type) {

		if (file == null)
			return;

		int res = favDB.getFavUrlSeq(file, type);
		if (res == -1) {
			if (type.equals("hda")) {
				this.mHDA.getAdapter().getCount();
			} else if (type.equals("hdb")) {
				this.mHDB.getAdapter().getCount();
			} else if (type.equals("hdc")) {
				this.mHDC.getAdapter().getCount();
			} else if (type.equals("hdd")) {
				this.mHDD.getAdapter().getCount();
			} else if (type.equals("cd")) {
				this.mCD.getAdapter().getCount();
			} else if (type.equals("fda")) {
				this.mFDA.getAdapter().getCount();
			} else if (type.equals("fdb")) {
				this.mFDB.getAdapter().getCount();
			} else if (type.equals("sd")) {
				this.mSD.getAdapter().getCount();
			}
			favDB.insertFavURL(file, type);
		}

	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		menu.clear();

		menu.add(0, INSTALL, 0, "Install Roms").setIcon(R.drawable.install);
		menu.add(0, DELETE, 0, "Delete Machine").setIcon(R.drawable.delete);
		menu.add(0, EXPORT, 0, "Export Machines").setIcon(R.drawable.exportvms);
		menu.add(0, IMPORT, 0, "Import Machines").setIcon(R.drawable.importvms);
		menu.add(0, HELP, 0, "Help").setIcon(R.drawable.help);
		menu.add(0, CHANGELOG, 0, "Changelog").setIcon(android.R.drawable.ic_menu_help);
		menu.add(0, LICENSE, 0, "License").setIcon(android.R.drawable.ic_menu_help);
		menu.add(0, QUIT, 0, "Exit").setIcon(android.R.drawable.ic_lock_power_off);

		return true;

	}

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {

		super.onOptionsItemSelected(item);
		if (item.getItemId() == this.INSTALL) {
			this.install();
		} else if (item.getItemId() == this.DELETE) {
			this.onDeleteMachine();
		} else if (item.getItemId() == this.EXPORT) {
			this.onExportMachines();
		} else if (item.getItemId() == this.IMPORT) {
			this.onImportMachines();
		} else if (item.getItemId() == this.HELP) {
			this.onHelp();
		} else if (item.getItemId() == this.CHANGELOG) {
			this.onChangeLog();
		} else if (item.getItemId() == this.LICENSE) {
			this.onLicense();
		} else if (item.getItemId() == this.QUIT) {
			this.exit();
		}
		return true;
	}

	public void stopVM(boolean exit) {
		if (vmexecutor == null && !exit) {
			if (this.currMachine != null && this.currMachine.paused == 1) {
				new AlertDialog.Builder(this).setTitle("Discard VM State")
						.setMessage("The VM is Paused. If you discard the state you might lose data. Continue?")
						.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								currMachine.paused = 0;
								machineDB.update(currMachine, machineDB.PAUSED, 0 + "");
								sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", "READY");
								enableNonRemovableDeviceOptions(true);
								enableRemovableDeviceOptions(true);

							}
						}).setNegativeButton("No", new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
							}
						}).show();
				return;
			} else {
				sendHandlerMessage(handler, Config.VM_NOTRUNNING);
				return;
			}
		}

		new AlertDialog.Builder(this).setTitle("Shutdown VM")
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

	public void saveSnapshotDB(String snapshot_name) {
		currMachine.snapshot_name = snapshot_name;
		int ret = machineDB.deleteMachine(currMachine);
		ret = machineDB.insertMachine(currMachine);
		if (this.snapshotAdapter.getPosition(snapshot_name) < 0) {
			this.snapshotAdapter.add(snapshot_name);
		}
	}

	public void saveStateVMDB() {
		machineDB.update(currMachine, machineDB.PAUSED, 1 + "");
	}

	public void stopTimeListener() {

		synchronized (this.lockTime) {
			this.timeQuit = true;
			this.lockTime.notifyAll();
		}
	}

	public void onPause() {
		super.onPause();

		this.stopTimeListener();
	}

	public void onResume() {
		super.onResume();

		execTimeListener();
	}

	public void timeListener() {
		while (timeQuit != true) {
			if (vmexecutor != null) {
				String status = checkStatus();
				if (!status.equals(currStatus)) {
					currStatus = status;
					sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", status);
				}
			}

			try {
				Thread.sleep(1000);
			} catch (InterruptedException ex) {
				ex.printStackTrace();
			}
		}

	}

	void execTimeListener() {

		Thread t = new Thread(new Runnable() {
			public void run() {
				startTimeListener();
			}
		});
		t.start();
	}

	public void startTimeListener() {
		this.stopTimeListener();

		timeQuit = false;
		try {

			timeListener();
			synchronized (lockTime) {
				while (timeQuit == false) {
					lockTime.wait();
				}
				lockTime.notifyAll();
			}
		} catch (Exception ex) {
			ex.printStackTrace();

		}

	}

	private String checkStatus() {
		String state = "READY";
		if (vmexecutor != null && vmexecutor.libLoaded && vmexecutor.get_state().equals("RUNNING")) {
			state = "RUNNING";
		} else if (vmexecutor != null) {
			String save_state = vmexecutor.get_save_state();
			String pause_state = vmexecutor.get_pause_state();

			// Shutdown if paused done
			if (pause_state.equals("SAVING")) {
				return pause_state;
			} else if (pause_state.equals("DONE")) {
				if (LimboActivity.vmexecutor != null) {
					LimboActivity.vmexecutor.stopvm(0);
				}
			} else if (save_state.equals("SAVING")) {
				state = save_state;
			} else {
				state = "READY";
			}
		} else {
			state = "READY";
		}

		return state;
	}

	public static int get_fd(String path) {
		int fd = 0;
		if (path == null)
			return 0;

		if (path.startsWith("/content") || path.startsWith("content://")) {
			path = path.replaceFirst("/content", "content:");

			try {
				ParcelFileDescriptor pfd = activity.getContentResolver().openFileDescriptor(Uri.parse(path), "rw");
				fd = pfd.getFd();
				fds.put(fd, pfd);
			} catch (final FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				new Handler(Looper.getMainLooper()).post(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(LimboActivity.activity, "Error: " + e, Toast.LENGTH_SHORT).show();
					}
				});
			}
		} else {
			try {
				File file = new File(path);
				if (!file.exists())
					file.createNewFile();
				ParcelFileDescriptor pfd = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_WRITE_ONLY);
				fd = pfd.getFd();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		}
		return fd;
	}

	static HashMap<Integer, ParcelFileDescriptor> fds = new HashMap<Integer, ParcelFileDescriptor>();

	public static int close_fd(int fd) {

		if (fds.containsKey(fd)) {
			ParcelFileDescriptor pfd = fds.get(fd);
			try {
				pfd.close();
				fds.remove(fd);
				return 0; // success for Native side
			} catch (IOException e) {
				e.printStackTrace();
			}

		}
		return -1;
	}

}
