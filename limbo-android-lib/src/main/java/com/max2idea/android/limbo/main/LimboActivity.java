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

import android.androidVNC.ConnectionBean;
import android.androidVNC.VncCanvas;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.NotificationManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.net.wifi.WifiManager.WifiLock;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager.WakeLock;
import android.os.StrictMode;
import android.preference.PreferenceManager;
import android.support.v4.provider.DocumentFile;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
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
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.jni.VMExecutor;
import com.max2idea.android.limbo.utils.FavOpenHelper;
import com.max2idea.android.limbo.utils.FileInstaller;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.OSDialogBox;
import com.max2idea.android.limbo.utils.UIUtils;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

//import com.max2idea.android.limbo.main.R;

public class LimboActivity extends AppCompatActivity {

	public static final String TAG = "LIMBO";
	// Static
	private static final int HELP = 0;
	private static final int QUIT = 1;
	private static final int INSTALL = 2;
	private static final int DELETE = 3;
	private static final int EXPORT = 4;
	private static final int IMPORT = 5;
	private static final int CHANGELOG = 6;
	private static final int LICENSE = 7;
	private static final int VIEWLOG = 8;
	private static final int CREATE = 9;
	private static final int ISOSIMAGES = 10;
	public static boolean vmStarted = false;
	public static LimboActivity activity = null;
	public static VMExecutor vmexecutor;
	public static String currStatus = "READY";
	static public ProgressDialog progDialog;
	public static Machine currMachine = null;
	public static Handler OShandler;
	private static Installer a;
	private static TextWatcher appendChangeListener;
	private static TextWatcher extraParamsChangeListener;
	private static TextWatcher dnsChangeListener;
	private static String output;
	private static String vnc_passwd = null;
	private static int vnc_allow_external = 0;
	public View parent;
	public TextView mOutput;
	public AutoScrollView mLyricsScroll;
	private boolean machineLoaded;
    //Widgets
	private ImageView mStatus;
	private EditText mDNS;
	private EditText mHOSTFWD;
	private EditText mAppend;
	private EditText mExtraParams;
	private boolean timeQuit = false;
	private Object lockTime = new Object();
	private TextView mStatusText;
	private WakeLock mWakeLock;
	private WifiLock wlock;
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
	private Spinner mSharedFolder;
	private CheckBox mHDAenable;
	private CheckBox mHDBenable;
	private CheckBox mHDCenable;
	private CheckBox mHDDenable;
	private CheckBox mCDenable;
	private CheckBox mFDAenable;
	private CheckBox mFDBenable;
	private CheckBox mSDenable;
	private CheckBox mSharedFolderenable;
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
	private CheckBox mToolBar;
	private CheckBox mFullScreen;
	private Spinner mSnapshot;
	private Spinner mOrientation;
	private Spinner mKeyboard;
	private ImageButton mStart;
	private ImageButton mStop;
	private ImageButton mRestart;
	private ImageButton mSave;
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
	private ArrayAdapter<String> sharedFolderAdapter;
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
	public Handler handler = new Handler() {
		@Override
		public synchronized void handleMessage(Message msg) {
			Bundle b = msg.getData();
			Integer messageType = (Integer) b.get("message_type");

			if (messageType != null && messageType == Config.VM_PAUSED) {
                UIUtils.toastShort(LimboActivity.this, "VM Paused");

			}
			if (messageType != null && messageType == Config.VM_RESUMED) {
                UIUtils.toastShort(LimboActivity.this, "VM Resuming, Please Wait");
			}
			if (messageType != null && messageType == Config.VM_STARTED) {
				if (!vmStarted) {
                    UIUtils.toastLong(LimboActivity.this, "VM Started\nPause the VM instead so you won't have to boot again!");
				}
				enableNonRemovableDeviceOptions(false);
				mStart.setImageResource(R.drawable.play);

			}
			if (messageType != null && messageType == Config.VM_STOPPED) {
                UIUtils.toastShort(LimboActivity.this, "VM Shutdown");
				mStart.setImageResource(R.drawable.play);

			}
			if (messageType != null && messageType == Config.VM_RESTARTED) {
                UIUtils.toastShort(LimboActivity.this, "VM Reset");
			}
			if (messageType != null && messageType == Config.VM_SAVED) {
                UIUtils.toastShort(LimboActivity.this, "VM Saved");
			}
			if (messageType != null && messageType == Config.VM_NO_QCOW2) {
                UIUtils.toastLong(LimboActivity.this, "Couldn't find a QCOW2 image\nPlease attach an HDA or HDB image first!");
			}
			if (messageType != null && messageType == Config.VM_NO_KERNEL) {
                UIUtils.toastLong(LimboActivity.this, "Couldn't find a Kernel image\nPlease attach a Kernel image first!");
			}
			if (messageType != null && messageType == Config.VM_NO_INITRD) {
                UIUtils.toastLong(LimboActivity.this, "Couldn't find a initrd image\nPlease attach an initrd image first!");
			}
			if (messageType != null && messageType == Config.VM_ARM_NOMACHINE) {
                UIUtils.toastLong(LimboActivity.this, "Please select an ARM machine type first!");
			}
			if (messageType != null && messageType == Config.VM_NOTRUNNING) {
                UIUtils.toastShort(LimboActivity.this, "VM not running");
			}
			if (messageType != null && messageType == Config.VM_CREATED) {
				String machineValue = (String) b.get("machine_name");
				createMachine(machineValue);

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
				UIUtils.UIAlertHtml(title, body, activity);
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
                UIUtils.toastLong(LimboActivity.this, "Machines are exported in " + Config.DBFile);
			}
			if (messageType != null && messageType == Config.VM_IMPORT) {
				if (progDialog.isShowing()) {
					progDialog.dismiss();
				}
                UIUtils.toastLong(LimboActivity.this, "Machines have been imported from " + Config.DBFile);
				populateAttributes();
			}

		}
	};
	private LinearLayout mCPUSectionDetails;
	private TextView mCPUSectionHeader;
	private LinearLayout mStorageSectionDetails;
	private TextView mStorageSectionHeader;
	private LinearLayout mUserInterfaceSectionDetails;
	private TextView mUserInterfaceSectionHeader;
	private LinearLayout mAdvancedSectionDetails;
	private TextView mAdvancedSectionHeader;
	private View mBootSectionHeader;
	private LinearLayout mBootSectionDetails;
	private LinearLayout mGraphicsSectionDetails;
	private TextView mGraphicsSectionHeader;
	private LinearLayout mRemovableStorageSectionDetails;
	private TextView mRemovableStorageSectionHeader;
	private LinearLayout mNetworkSectionDetails;
	private View mNetworkSectionHeader;
	private LinearLayout mAudioSectionDetails;
	private TextView mAudioSectionHeader;
	private ConnectionBean selected;
	private String filetype;

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

	public static void quit() {
		activity.finish();
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
	}

	static private void install() {
		progDialog = ProgressDialog.show(activity, "Please Wait", "Installing Files...", true);
		a = new Installer();
		a.execute();
	}

    public static void UIAlertLicense(String title, String html, final Activity activity) {

		AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle(title);
		WebView webview = new WebView(activity);
		webview.loadData(html, "text/html", "UTF-8");
		alertDialog.setView(webview);

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "I Acknowledge", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				if (isFirstLaunch()) {
					install();
					UIUtils.onHelp(activity);
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

	// Another Generic Messanger
	public static void sendHandlerMessage(Handler handler, int message_type) {
		Message msg1 = handler.obtainMessage();
		Bundle b = new Bundle();
		b.putInt("message_type", message_type);
		msg1.setData(b);
		handler.sendMessage(msg1);
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
			showAlertHtml("CHANGELOG", fileutils.LoadFile(activity, "CHANGELOG", false), OShandler);
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

	static private void onInstall() {
		FileInstaller.installFiles(activity);
	}

	public static String getVnc_passwd() {
		return vnc_passwd;
	}

	public static void setVnc_passwd(String vnc_passwd) {
		LimboActivity.vnc_passwd = vnc_passwd;
	}

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
		vmexecutor.startvm(activity, UI);

	}



	public void setUserPressed(boolean pressed) {

		if (pressed) {
			enableListeners();
		} else
			disableListeners();

	}

	private void enableListeners() {


    	mArch.setOnItemSelectedListener(new OnItemSelectedListener() {

			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				// your code here

				String arch = (String) ((ArrayAdapter<?>) mArch.getAdapter()).getItem(position);


					currMachine.arch = arch;
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, arch);

					if (currMachine.arch.equals("ARM")) {

						if (!machineLoaded) {
							populateMachineType("integratorcp");
							populateCPUs("arm926");
							populateNetDevices("smc91c111");
						}

					} else if (currMachine.arch.equals("MIPS")) {

						if (!machineLoaded) {
							populateMachineType("malta");
							populateCPUs("Default");
							// populateNetDevices("smc91c111");
						}

					} else if (currMachine.arch.equals("PPC")) {

						if (!machineLoaded) {
							populateMachineType("Default");
							populateCPUs("Default");
							populateNetDevices("e1000");
						}

					} else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

						if (!machineLoaded) {
							populateMachineType("pc");
							if (currMachine.arch.equals("x86")) {
								populateCPUs("Default");
							} else
								populateCPUs("Default");
							populateNetDevices("ne2k_pci");
						}

					} else if (currMachine.arch.equals("m68k")) {

						if (!machineLoaded) {
							populateMachineType("Default");
							populateCPUs("Default");
							// populateNetDevices("smc91c111");
						}

					} else if (currMachine.arch.equals("SPARC")) {

						if (!machineLoaded) {
							populateMachineType("Default");
							populateCPUs("Default");
							populateNetDevices("lance");
						}

					}

				if (currMachine != null)
					if (currMachine.arch.equals("ARM") || currMachine.arch.equals("MIPS")
							|| currMachine.arch.equals("m68k") || currMachine.arch.equals("PPC")
							|| currMachine.arch.equals("SPARC")) {
						mACPI.setEnabled(false);
						mHPET.setEnabled(false);
						mFDBOOTCHK.setEnabled(false);

					} else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

						mACPI.setEnabled(true);
						mHPET.setEnabled(true);
						mFDBOOTCHK.setEnabled(true);

					}
				machineLoaded = false;
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mCPU.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

				String cpu = (String) ((ArrayAdapter<?>) mCPU.getAdapter()).getItem(position);


					currMachine.cpu = cpu;
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, cpu);


			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mMachineType.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String machineType = (String) ((ArrayAdapter<?>) mMachineType.getAdapter()).getItem(position);
					currMachine.machine_type = machineType;
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.MACHINE_TYPE, machineType);
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mUI.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				// your code here

				String ui = (String) ((ArrayAdapter<?>) mUI.getAdapter()).getItem(position);

					currMachine.ui = ui;
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.UI, ui);

				if (position == 0) {
					mVNCAllowExternal.setEnabled(true);
					if (mSnapshot.getSelectedItemPosition() == 0)
						mSoundCardConfig.setEnabled(false);
				} else {
					mVNCAllowExternal.setEnabled(false);
					if (mSnapshot.getSelectedItemPosition() == 0) {
						if (Config.enable_sound)
							mSoundCardConfig.setEnabled(true);
					}
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mCPUNum.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cpuNum = (String) ((ArrayAdapter<?>) mCPUNum.getAdapter()).getItem(position);
					currMachine.cpuNum = Integer.parseInt(cpuNum);
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPUNUM,
							cpuNum);
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mRamSize.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String ram = (String) ((ArrayAdapter<?>) mRamSize.getAdapter()).getItem(position);
				if (currMachine != null) {
					currMachine.memory = Integer.parseInt(ram);
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY,
							ram);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mKernel.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String kernel = (String) ((ArrayAdapter<?>) mKernel.getAdapter()).getItem(position);
				if (
						position == 0) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KERNEL,
							null);
					currMachine.kernel = null;
				} else if (
						position == 1) {
					browse("kernel");
					mKernel.setSelection(0);
				} else if (
						position > 1) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KERNEL,
							kernel);
					currMachine.kernel = kernel;
					// TODO: If Machine is running eject and set floppy img
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mInitrd.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String initrd = (String) ((ArrayAdapter<?>) mInitrd.getAdapter()).getItem(position);
				if (
						position == 0) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.INITRD,
							null);
					currMachine.initrd = null;
				} else if (
						position == 1) {
					browse("initrd");
					mInitrd.setSelection(0);
				} else if (
						position > 1) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.INITRD,
							initrd);
					currMachine.initrd = initrd;

				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDA.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hda = (String) ((ArrayAdapter<?>) mHDA.getAdapter()).getItem(position);
				if (
						position == 0 && mHDAenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, "");
					currMachine.hda_img_path = "";
				} else if (
						(position == 0 || !mHDAenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, null);
					currMachine.hda_img_path = null;
				} else if (
						position == 1 && mHDAenable.isChecked()) {
					promptImageName(activity, "hda");

				} else if (
						position == 2 && mHDAenable.isChecked()) {
					browse("hda");
					mHDA.setSelection(0);
				} else if (
						position > 2 && mHDAenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, hda);
					currMachine.hda_img_path = hda;
				}



			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDB.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdb = (String) ((ArrayAdapter<?>) mHDB.getAdapter()).getItem(position);

				if (
						position == 0 && mHDBenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, "");
					currMachine.hdb_img_path = "";
				} else if (
						(position == 0 || !mHDBenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, null);
					currMachine.hdb_img_path = null;
				} else if (
						position == 1 && mHDBenable.isChecked()) {
					promptImageName(activity, "hdb");

				} else if (
						position == 2 && mHDBenable.isChecked()) {
					browse("hdb");
					mHDB.setSelection(0);
				} else if (
						position > 2 && mHDBenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, hdb);
					currMachine.hdb_img_path = hdb;
				}


			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDC.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdc = (String) ((ArrayAdapter<?>) mHDC.getAdapter()).getItem(position);
				if (
						position == 0 && mHDCenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, "");
					currMachine.hdc_img_path = "";
				} else if (
						(position == 0 || !mHDCenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, null);
					currMachine.hdc_img_path = null;
				} else if (
						position == 1 && mHDCenable.isChecked()) {
					promptImageName(activity, "hdc");

				} else if (
						position == 2 && mHDCenable.isChecked()) {
					browse("hdc");
					mHDC.setSelection(0);
				} else if (
						position > 2 && mHDCenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, hdc);
					currMachine.hdc_img_path = hdc;
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHDD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdd = (String) ((ArrayAdapter<?>) mHDD.getAdapter()).getItem(position);
				if (
						position == 0 && mHDDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, "");
					currMachine.hdd_img_path = "";
				} else if (
						(position == 0 || !mHDDenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, null);
					currMachine.hdd_img_path = null;
				} else if (
						position == 1 && mHDDenable.isChecked()) {
					promptImageName(activity, "hdd");
				} else if (
						position == 2 && mHDDenable.isChecked()) {
					browse("hdd");
					mHDD.setSelection(0);
				} else if (
						position > 2 && mHDDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, hdd);
					currMachine.hdd_img_path = hdd;
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mSnapshot.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String snapshot_name = (String) ((ArrayAdapter<?>) mSnapshot.getAdapter()).getItem(position);
				if (
						position == 0) {
					currMachine.snapshot_name = "";

					Thread thread = new Thread(new Runnable() {
						public void run() {
							loadMachine(currMachine.machinename, currMachine.snapshot_name);
						}
					});
					thread.setPriority(Thread.MIN_PRIORITY);
					thread.start();

					mStart.setImageResource(R.drawable.play);
				} else if (
                        position > 0) {
					currMachine.snapshot_name = snapshot_name;

					Thread thread = new Thread(new Runnable() {
						public void run() {
							loadMachine(currMachine.machinename, currMachine.snapshot_name);
						}
					});
					thread.setPriority(Thread.MIN_PRIORITY);
					thread.start();

					mStart.setImageResource(R.drawable.play);
					enableNonRemovableDeviceOptions(false);
					enableRemovableDeviceOptions(false);
					mSnapshot.setEnabled(true);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mCD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cd = (String) ((ArrayAdapter<?>) mCD.getAdapter()).getItem(position);
				if (
				        position == 0 && mCDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM, "");
					currMachine.cd_iso_path = "";
				} else if (
						(position == 0 || !mCDenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
							null);
					currMachine.cd_iso_path = null;
				} else if (
						position == 1 && mCDenable.isChecked()) {
					browse("cd");
					mCD.setSelection(0);
				} else if (
						position > 1 && mCDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM, cd);
					currMachine.cd_iso_path = cd;
				}
				if (
						currStatus.equals("RUNNING") && position > 1 && mCDenable.isChecked()) {
					mCD.setEnabled(false);
					vmexecutor.change_dev("ide1-cd0", currMachine.cd_iso_path);
					mCD.setEnabled(true);
				} else if (
                                mCDenable.isChecked() &&
                                currStatus.equals("RUNNING") && position == 0) {
					mCD.setEnabled(false);
					vmexecutor.change_dev("ide1-cd0", null); // Eject
					mCD.setEnabled(true);
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mFDA.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String fda = (String) ((ArrayAdapter<?>) mFDA.getAdapter()).getItem(position);
				if (
						position == 0 && mFDAenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, "");
					currMachine.fda_img_path = "";
				} else if (
						(position == 0 || !mFDAenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, null);
					currMachine.fda_img_path = null;
				} else if (
						position == 1 && mFDAenable.isChecked()) {
					browse("fda");
					mFDA.setSelection(0);
				} else if (
						position > 1 && mFDAenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, fda);
					currMachine.fda_img_path = fda;
				}
				if (
						currStatus.equals("RUNNING") && position > 1 && mFDAenable.isChecked()) {
					mFDA.setEnabled(false);
					vmexecutor.change_dev("floppy0", currMachine.fda_img_path);
					mFDA.setEnabled(true);
				} else if (
						currStatus.equals("RUNNING") && position == 0 && mFDAenable.isChecked()) {
					mFDA.setEnabled(false);
					vmexecutor.change_dev("floppy0", null); // Eject
					mFDA.setEnabled(true);
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mFDB.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String fdb = (String) ((ArrayAdapter<?>) mFDB.getAdapter()).getItem(position);
				if (
						position == 0 && mFDBenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, "");
					currMachine.fdb_img_path = "";
				} else if (
						(position == 0 || !mFDBenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, null);
					currMachine.fdb_img_path = null;
				} else if (
						position == 1 && mFDBenable.isChecked()) {
					browse("fdb");
					mFDB.setSelection(0);
				} else if (
						position > 1 && mFDBenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, fdb);
					currMachine.fdb_img_path = fdb;
					// TODO: If Machine is running eject and set floppy img
				}
				if (
						currStatus.equals("RUNNING") && position > 1 && mFDBenable.isChecked()) {
					mFDB.setEnabled(false);
					vmexecutor.change_dev("floppy1", currMachine.fdb_img_path);
					mFDB.setEnabled(true);
				} else if (
						currStatus.equals("RUNNING") && position == 0 && mFDBenable.isChecked()) {
					mFDB.setEnabled(false);
					vmexecutor.change_dev("floppy1", null); // Eject
					mFDB.setEnabled(true);
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mSD.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String sd = (String) ((ArrayAdapter<?>) mSD.getAdapter()).getItem(position);
				if (
						position == 0 && mSDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, "");
					currMachine.sd_img_path = "";
				} else if (
						(position == 0 || !mSDenable.isChecked())) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, null);
					currMachine.sd_img_path = null;
				} else if (
						position == 1 && mSDenable.isChecked()) {
					browse("sd");
					mSD.setSelection(0);
				} else if (
						position > 1 && mSDenable.isChecked()) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, sd);
					currMachine.sd_img_path = sd;
					// TODO: If Machine is running eject and set floppy img
				}
				if (
						currStatus.equals("RUNNING") && position > 1 && mSDenable.isChecked()) {
				} else if (
						currStatus.equals("RUNNING") && position == 0 && mSDenable.isChecked()) {
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mSharedFolder.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String sharedFolder = (String) ((ArrayAdapter<?>) mSharedFolder.getAdapter()).getItem(position);

				if (
						(position == 0
                                //|| !mSharedFolderenable.isChecked()
                        )) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.SHARED_FOLDER, null);
					currMachine.shared_folder = null;
					currMachine.shared_folder_mode = 0;
				} else if (
						mSharedFolderenable.isChecked()) {
					String[] shared_folder = sharedFolder.split("\\(");
					currMachine.shared_folder = shared_folder[0].trim();
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.SHARED_FOLDER, currMachine.shared_folder);
					if (position >= 0) {
						int folderMode = 1; // always read/write
						currMachine.shared_folder_mode = folderMode;
						ret = MachineOpenHelper.getInstance(activity).update(currMachine,
								MachineOpenHelper.SHARED_FOLDER_MODE, folderMode + "");
					}
				}

			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mCDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mCD.setEnabled(isChecked);

				if (currMachine != null) {
					currMachine.enableCDROM = isChecked;
					if (isChecked) {
						currMachine.cd_iso_path = "";
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
								"");
						mHDCenable.setChecked(false);
					} else {
						currMachine.cd_iso_path = null;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
								null);
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
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA,
							currMachine.hda_img_path);
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
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB,
							currMachine.hdb_img_path);
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
						mCDenable.setChecked(false);
					} else {
						currMachine.hdc_img_path = null;
					}
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC,
							currMachine.hdc_img_path);
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
						mSharedFolderenable.setChecked(false);
					} else {
						currMachine.hdd_img_path = null;
					}
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD,
							currMachine.hdd_img_path);
				}
				triggerUpdateSpinner(mHDD);
			}

		});
		mFDAenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mFDA.setEnabled(isChecked);

				if (currMachine != null) {
					currMachine.enableFDA = isChecked;
					if (isChecked) {
						currMachine.fda_img_path = "";
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA,
								"");
					} else {
						currMachine.fda_img_path = null;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA,
								null);
					}
				}

				triggerUpdateSpinner(mFDA);
			}

		});
		mFDBenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mFDB.setEnabled(isChecked);

				if (currMachine != null) {
					currMachine.enableFDB = isChecked;
					if (isChecked) {
						currMachine.fdb_img_path = "";
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB,
								"");
					} else {
						currMachine.fdb_img_path = null;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB,
								null);
					}
				}
				triggerUpdateSpinner(mFDB);
			}

		});
		mSDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mSD.setEnabled(isChecked);

				if (currMachine != null) {
					currMachine.enableSD = isChecked;
					if (isChecked) {
						currMachine.sd_img_path = "";
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, "");
					} else {
						currMachine.sd_img_path = null;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD,
								null);
					}
				}

				triggerUpdateSpinner(mSD);
			}

		});
		mSharedFolderenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
				mSharedFolder.setEnabled(isChecked);
				if (currMachine != null
                        ) {
					if (isChecked) {
//						currMachine.shared_folder = "";
						currMachine.shared_folder_mode = 0;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
								MachineOpenHelper.SHARED_FOLDER, currMachine.shared_folder);
						mHDDenable.setChecked(false);
					} else {
						currMachine.shared_folder = null;
						currMachine.shared_folder_mode = 0;
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
								MachineOpenHelper.SHARED_FOLDER, null);
					}
				}

				triggerUpdateSpinner(mSharedFolder);
			}

		});

		mBootDevices.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String bootDev = (String) ((ArrayAdapter<?>) mBootDevices.getAdapter()).getItem(position);
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.BOOT_CONFIG,
							bootDev);
					currMachine.bootdevice = bootDev;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		this.mNetConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String netfcg = (String) ((ArrayAdapter<?>) mNetConfig.getAdapter()).getItem(position);
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG,
							netfcg);
					currMachine.net_cfg = netfcg;
				if (position > 0) {
					mNetDevices.setEnabled(true);
					mDNS.setEnabled(true);
				} else {
					mNetDevices.setEnabled(false);
					mDNS.setEnabled(false);
				}

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
					mNetDevices.setSelection(0);
					return;
				}
				String niccfg = (String) ((ArrayAdapter<?>) mNetDevices.getAdapter()).getItem(position);
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG,
							niccfg);
					currMachine.nic_driver = niccfg;


			}

			public void onNothingSelected(final AdapterView<?> parentView) {

			}
		});

		mVGAConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String vgacfg = (String) ((ArrayAdapter<?>) mVGAConfig.getAdapter()).getItem(position);

					int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.VGA,
							vgacfg);
					currMachine.vga_type = vgacfg;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		this.mSoundCardConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String sndcfg = (String) ((ArrayAdapter<?>) mSoundCardConfig.getAdapter()).getItem(position);

					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.SOUNDCARD_CONFIG, sndcfg);
					currMachine.soundcard = sndcfg;

			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mHDCacheConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String hdcfg = (String) ((ArrayAdapter<?>) mHDCacheConfig.getAdapter()).getItem(position);

					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.HDCACHE_CONFIG, hdcfg);
					currMachine.hd_cache = hdcfg;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mACPI.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.DISABLE_ACPI, ((isChecked ? 1 : 0) + ""));
					currMachine.disableacpi = (isChecked ? 1 : 0);
			}

			public void onNothingSelected(AdapterView<?> parentView) {

			}
		});

		mHPET.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.DISABLE_HPET, ((isChecked ? 1 : 0) + ""));
					currMachine.disablehpet = (isChecked ? 1 : 0);
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mFDBOOTCHK.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
					int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
							MachineOpenHelper.DISABLE_FD_BOOT_CHK, ((isChecked ? 1 : 0) + ""));
					currMachine.disablefdbootchk = (isChecked ? 1 : 0);
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		if (dnsChangeListener == null)
			dnsChangeListener = new TextWatcher() {

				public void afterTextChanged(Editable s) {

				}

				public void beforeTextChanged(CharSequence s, int start, int count, int after) {
				}

				public void onTextChanged(CharSequence s, int start, int before, int count) {
				}
			};

		mDNS.setOnFocusChangeListener(new OnFocusChangeListener() {
			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				if (!hasFocus) {
					setDNSServer(mDNS.getText().toString());
					LimboSettingsManager.setDNSServer(activity, mDNS.getText().toString());
				}
			}
		});

		mDNS.addTextChangedListener(dnsChangeListener);

		mHOSTFWD.setOnFocusChangeListener(new OnFocusChangeListener() {
			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				if (!hasFocus) {
					if (currMachine != null) {

						currMachine.hostfwd = mHOSTFWD.getText().toString();
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HOSTFWD,
								mHOSTFWD.getText().toString());
					}
				}
			}
		});

		if (appendChangeListener == null)
			appendChangeListener = new TextWatcher() {

				public void afterTextChanged(Editable s) {
					if (currMachine != null) {
						currMachine.append = s.toString();
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.APPEND,
								s.toString());
					}
				}

				public void beforeTextChanged(CharSequence s, int start, int count, int after) {

				}

				public void onTextChanged(CharSequence s, int start, int before, int count) {

				}
			};
		mAppend.addTextChangedListener(appendChangeListener);

		if (extraParamsChangeListener == null)
			extraParamsChangeListener = new TextWatcher() {

				public void afterTextChanged(Editable s) {
					if (currMachine != null) {
						currMachine.extra_params = s.toString();
						int ret = MachineOpenHelper.getInstance(activity).update(currMachine,
								MachineOpenHelper.EXTRA_PARAMS, s.toString());
					}
				}

				public void beforeTextChanged(CharSequence s, int start, int count, int after) {

				}

				public void onTextChanged(CharSequence s, int start, int before, int count) {

				}
			};
		mExtraParams.addTextChangedListener(extraParamsChangeListener);

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

		mToolBar.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

				if (isChecked) {
					LimboSettingsManager.setAlwaysShowMenuToolbar(activity, true);
				} else {
					LimboSettingsManager.setAlwaysShowMenuToolbar(activity, false);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mFullScreen.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

				if (isChecked) {
					LimboSettingsManager.setFullscreen(activity, true);
				} else {
					LimboSettingsManager.setFullscreen(activity, false);
				}
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mOrientation.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String orientationCfg = (String) ((ArrayAdapter<?>) mOrientation.getAdapter()).getItem(position);
					LimboSettingsManager.setOrientationSetting(activity, position);
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});

		mKeyboard.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String keyboardCfg = (String) ((ArrayAdapter<?>) mKeyboard.getAdapter()).getItem(position);
					LimboSettingsManager.setKeyboardSetting(activity, position);
			}

			public void onNothingSelected(AdapterView<?> parentView) {
			}
		});
	}

	protected synchronized void setDNSServer(String string) {

		File resolvConf = new File(Config.basefiledir + "/etc/resolv.conf");
		FileOutputStream fileStream = null;
		try {
			fileStream = new FileOutputStream(resolvConf);
			String str = "nameserver " + string;
			byte[] data = str.getBytes();
			fileStream.write(data);
		} catch (Exception ex) {
			Log.e(TAG, "Could not write DNS to file: " + ex);
		} finally {
			if (fileStream != null)
				try {
					fileStream.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		}
	}

	private void disableListeners() {

		if(mMachine == null)
			return;

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
		mExtraParams.removeTextChangedListener(extraParamsChangeListener);

	}

	/**
	 * Called when the activity is first created.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
        NotificationManager notificationManager = (NotificationManager) getApplicationContext().getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancelAll();

		setupStrictMode();
		setupFolders();
		OShandler = this.handler;
		activity = this;
		this.setContentView(R.layout.main);
		this.setupWidgets();
		populateAttributes();
		execTimeListener();
		if (this.isFirstLaunch()) {
			onFirstLaunch();
		}
		setupNativeLibs();
		setupToolbar();
		checkUpdate();
	}

	private void setupFolders() {
		Thread t = new Thread(new Runnable() {
			public void run() {

				// Create Temp folder
				File folder = new File(Config.tmpFolder);
				if (!folder.exists())
					folder.mkdirs();

				// Create shared folder
				File shared_folder = new File(Config.sharedFolder);
				if (!shared_folder.exists())
					shared_folder.mkdirs();

			}
		});
		t.start();
	}

	public void setupNativeLibs() {

        //Some devices need stl loaded upfront
        System.loadLibrary("stlport_shared");

        //iconv is not really needed
        if(Config.enable_iconv) {
            System.loadLibrary("iconv");
        }

        //Glib
		System.loadLibrary("glib-2.0");
		System.loadLibrary("gthread-2.0");
		System.loadLibrary("gobject-2.0");
		System.loadLibrary("gmodule-2.0");

        //Pixman for qemu
        System.loadLibrary("pixman");

		if (Config.enable_SPICE) {
			System.loadLibrary("crypto");
			System.loadLibrary("ssl");
			System.loadLibrary("spice");
		}

		// //Load SDL libraries
		if (Config.enable_SDL) {
			System.loadLibrary("SDL2");
			System.loadLibrary("SDL2_image");
		}
		if (Config.enable_sound) {
			// System.loadLibrary("mikmod");
			System.loadLibrary("SDL2_mixer");
			// System.loadLibrary("SDL_ttf");

		}

		//main for SDL
		if (Config.enable_SDL) {
			System.loadLibrary("main");
		}

		//Limbo needed for vmexecutor
		System.loadLibrary("limbo");

		loadQEMULib();
	}

    protected void loadQEMULib() {

    }

    public void setupToolbar() {
		Toolbar tb = (Toolbar) findViewById(R.id.toolbar);
		setSupportActionBar(tb);

		// Get the ActionBar here to configure the way it behaves.
		final ActionBar ab = getSupportActionBar();
		ab.setHomeAsUpIndicator(R.drawable.limbo); // set a custom icon for the
													// default home button
		ab.setDisplayShowHomeEnabled(true); // show or hide the default home
											// button
		ab.setDisplayHomeAsUpEnabled(true);
		ab.setDisplayShowCustomEnabled(true); // enable overriding the default
												// toolbar layout
		ab.setDisplayShowTitleEnabled(true); // disable the default title
												// element here (for centered
												// title)

		ab.setTitle(R.string.app_name);
	}

	public void checkUpdate() {
		Thread tsdl = new Thread(new Runnable() {
			public void run() {
				// Check for a new Version

				checkNewVersion();

			}
		});
		tsdl.start();
	}

	private void setupStrictMode() {

		if (Config.debug) {
			StrictMode.setThreadPolicy(
					new StrictMode.ThreadPolicy.Builder().detectDiskReads().detectDiskWrites().detectNetwork()
                            //.penaltyDeath()
							.penaltyLog().build());
			StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().detectLeakedSqlLiteObjects()
					.detectLeakedClosableObjects().penaltyLog()
					// .penaltyDeath()
					.build());
		}

	}

	private void checkNewVersion() {

		if (!LimboSettingsManager.getPromptUpdateVersion(activity)) {
			return;
		}

		HttpClient client;
		HttpGet get = null;
		HttpResponse response;
		HttpEntity entity = null;
		InputStream is = null;
		try {
			client = new DefaultHttpClient();
			get = new HttpGet(Config.downloadUpdateLink);
			response = client.execute(get);
			entity = response.getEntity();

			is = entity.getContent();
			ByteArrayOutputStream bos = new ByteArrayOutputStream();

			int read = 0;
			byte[] buff = new byte[1024];
			while ((read = is.read(buff)) != -1) {
				bos.write(buff, 0, read);
			}
			byte[] streamData = bos.toByteArray();
			final String versionStr = new String(streamData);
			float version = Float.parseFloat(versionStr);

			PackageInfo pInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(),
					PackageManager.GET_META_DATA);
            int versionCheck = (int)  (version * 100);
			if (versionCheck > pInfo.versionCode) {
				new Handler(Looper.getMainLooper()).post(new Runnable() {
					@Override
					public void run() {
						promptNewVersion(activity, versionStr);
					}
				});
			}
		} catch (Exception ex) {
            //ex.printStackTrace();
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			if (entity != null) {
				try {
					entity.consumeContent();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			if (get != null) {
				get.abort();
			}

		}
	}

	public void promptNewVersion(final Activity activity, String version) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("New Version " + version);
		TextView stateView = new TextView(activity);
		stateView.setText("There is a new version available with fixes and new features. Do you want to update?");
		stateView.setPadding(20, 20, 20, 20);
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



	private void populateAttributes() {

		this.populateMachines(null);
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
		this.populateSharedFolder();
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

    public void cleanup() {
        LimboActivity.vmexecutor = null;
        if(this.mMachine!=null) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    mMachine.setEnabled(true);
                    mMachine.setSelection(0);
                    vmStarted = false;

                    //XXX: We exit here to force unload the native libs
                    System.exit(1);
                }
            });
        }
    }

    private void createMachine(String machineValue) {

        if (MachineOpenHelper.getInstance(activity).getMachine(machineValue, "") != null) {
            Toast.makeText(activity, "VM Name \"" + machineValue + "\" exists please choose another name!",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        showDownloadLinks();

        currMachine = new Machine(machineValue);
        MachineOpenHelper.getInstance(activity).insertMachine(currMachine);

        // default settings
        if (Config.enable_X86 || Config.enable_X86_64) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "x86");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE, "pc");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "User");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG,
                    "ne2k_pci");
        } else if (Config.enable_ARM) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "ARM");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE,
                    "integratorcp");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "arm926");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "User");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG,
                    "smc91c111");
        } else if (Config.enable_MIPS) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "MIPS");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE,
                    "malta");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "None");
        } else if (Config.enable_PPC || Config.enable_PPC64) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "PPC");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE,
                    "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "User");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG, "e1000");
        } else if (Config.enable_m68k) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "m68k");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE,
                    "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "None");
        } else if (Config.enable_sparc) {
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, "SPARC");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MACHINE_TYPE,
                    "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, "Default");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY, "128");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG, "User");
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG, "lance");

        }

        Toast.makeText(activity, "VM Created: " + machineValue, Toast.LENGTH_SHORT).show();
        populateMachines(machineValue);

        if (LimboActivity.currMachine != null && currMachine.cpu != null
                && (currMachine.cpu.endsWith("(arm)") || currMachine.arch.startsWith("MIPS")
                || currMachine.arch.startsWith("PPC") || currMachine.arch.startsWith("m68k"))

                ) {
            mMachineType.setEnabled(true); // Disabled for now
        }
        enableNonRemovableDeviceOptions(true);
        enableRemovableDeviceOptions(true);
        this.mVNCAllowExternal.setEnabled(true);
    }

	protected void showDownloadLinks() {

		if (!Config.osImages.isEmpty()) {
			OSDialogBox oses = new OSDialogBox(activity, R.style.Transparent, this);
			oses.show();
		}
	}

	private void onDeleteMachine() {
		if (currMachine == null) {
			Toast.makeText(this, "Select a machine first!", Toast.LENGTH_SHORT).show();
			return;
		}
		MachineOpenHelper.getInstance(activity).deleteMachine(currMachine);
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

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Import Machines");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setPadding(20,20,20,20);

		TextView imageNameView = new TextView(activity);
		imageNameView.setVisibility(View.VISIBLE);
		imageNameView.setText(
				"Step 1: Place the machine.CSV file you export previously under \"limbo\" directory in your SD card.\n"
						+ "Step 2: WARNING: Any machine with the same name will be replaced!\n"
						+ "Step 3: Press \"OK\".\n");

		LinearLayout.LayoutParams searchViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(imageNameView, searchViewParams);
		alertDialog.setView(mLayout);

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

		if (Config.enable_sound)
			if (currMachine != null && currMachine.ui != null && currMachine.ui.equals("SDL"))
				this.mSoundCardConfig.setEnabled(flag);
			else
				this.mSoundCardConfig.setEnabled(false);
		else
			this.mSoundCardConfig.setEnabled(false);

		this.mPrio.setEnabled(flag);
		if (Config.enable_KVM || !flag)
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

		this.mHOSTFWD.setEnabled(flag);

		mHDAenable.setEnabled(flag);
		mHDBenable.setEnabled(flag);
		mHDCenable.setEnabled(flag);
		mHDDenable.setEnabled(flag);
		mSharedFolderenable.setEnabled(flag);

		mHDA.setEnabled(flag && mHDAenable.isChecked());
		mHDB.setEnabled(flag && mHDBenable.isChecked());
		mHDC.setEnabled(flag && mHDCenable.isChecked());
		mHDD.setEnabled(flag && mHDDenable.isChecked());
		mSharedFolder.setEnabled(flag && mSharedFolderenable.isChecked());

		this.mExtraParams.setEnabled(flag);

	}

	// Main event function
	// Retrives values from saved preferences
	private void onStartButton() {

		if (this.mMachine.getSelectedItemPosition() == 0 || this.currMachine == null) {
			UIUtils.toastShort(getApplicationContext(), "Select or Create a Virtual Machine first");
			return;
		}
		String filenotexists = validateFiles();
		if (filenotexists != null) {
            UIUtils.toastShort(getApplicationContext(), "Could not find file: " + filenotexists);
			return;
		}
		if (currMachine.snapshot_name != null && !currMachine.snapshot_name.toLowerCase().equals("none")
				&& !currMachine.snapshot_name.toLowerCase().equals("") && currMachine.soundcard != null
				&& !currMachine.soundcard.toLowerCase().equals("none") && mUI.getSelectedItemPosition() != 1) {
            UIUtils.toastLong(getApplicationContext(),
					"Snapshot was saved with soundcard enabled please use SDL \"User Interface\"");
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


		if (vmexecutor == null) {

			try {
				vmexecutor = new VMExecutor(currMachine, this);
			} catch (Exception ex) {
				UIUtils.toastLong(activity, "Error: " + ex);
				return;

			}
		}
		if (mCDenable.isChecked() && vmexecutor.cd_iso_path == null)
			vmexecutor.cd_iso_path = "";
		if (mFDAenable.isChecked() && vmexecutor.fda_img_path == null)
			vmexecutor.fda_img_path = "";
		if (mFDBenable.isChecked() && vmexecutor.fdb_img_path == null)
			vmexecutor.fdb_img_path = "";
		if (Config.enableFlashMemoryImages && mSDenable.isChecked() && vmexecutor.sd_img_path == null)
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


		if (mUI.getSelectedItemPosition() == 0) { // VNC
			vmexecutor.enableqmp = 0; // We enable qemu monitor
			startVNC();
            //we don't flag the VNC as started yet because we need
            //  to send cont via QEMU console first
		} else if (mUI.getSelectedItemPosition() == 1) { // SDL
			// XXX: We need to enable qmp server to be able to save the state
			// We could do it via the Monitor but SDL for Android
			// doesn't support multiple Windows
			vmexecutor.enableqmp = 1;
			startSDL();
            currMachine.paused = 0;
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.getInstance(activity).PAUSED,
                    0 + "");
		} else if (mUI.getSelectedItemPosition() == 2) { // SPICE
			startSPICE();
            currMachine.paused = 0;
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.getInstance(activity).PAUSED,
                    0 + "");
		}
		if (vmStarted) {
            //do nothing
        }else if (vmexecutor.paused == 1) {
            sendHandlerMessage(handler, Config.VM_RESUMED);
            vmStarted = true;
        }
        else {
            sendHandlerMessage(handler, Config.VM_STARTED);
            vmStarted = true;
        }



		new Handler(Looper.getMainLooper()).post(new Runnable() {
			public void run() {
				mMachine.setEnabled(false);
			}
		});

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

	public void startSDL() {

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
		} else {
            LimboActivity.activity.startvnc();
        }



	}

	private void startSPICE() {

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

		int fd;
		try {
			if (!FileUtils.fileValid(this, currMachine.hda_img_path))
				return currMachine.hda_img_path;
			if (!FileUtils.fileValid(this, currMachine.hdb_img_path))
				return currMachine.hdb_img_path;
			if (!FileUtils.fileValid(this, currMachine.hdc_img_path))
				return currMachine.hdc_img_path;
			if (!FileUtils.fileValid(this, currMachine.hdd_img_path))
				return currMachine.hdd_img_path;
			if (!FileUtils.fileValid(this, currMachine.fda_img_path))
				return currMachine.fda_img_path;
			if (!FileUtils.fileValid(this, currMachine.fdb_img_path))
				return currMachine.fdb_img_path;
			if (!FileUtils.fileValid(this, currMachine.sd_img_path))
				return currMachine.sd_img_path;
			if (!FileUtils.fileValid(this, currMachine.cd_iso_path))
				return currMachine.cd_iso_path;
			if (!FileUtils.fileValid(this, currMachine.kernel))
				return currMachine.kernel;
			if (!FileUtils.fileValid(this, currMachine.initrd))
				return currMachine.initrd;

		} catch (Exception ex) {
			return "Unknown";
		}
		return null;
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
		t.start();
	}

	private void onResumeButton() {

		// TODO: This probably has no effect
		Thread t = new Thread(new Runnable() {
			public void run() {
				resumevm();
			}
		});
		t.start();
	}

	public void toggleVisibility(View view) {
		if (view.getVisibility() == View.VISIBLE) {
			view.setVisibility(View.GONE);
		} else if (view.getVisibility() == View.GONE) {
			view.setVisibility(View.VISIBLE);
		}
	}

	// Setting up the UI
	public void setupWidgets() {

		setupSections();

		this.mStatus = (ImageView) findViewById(R.id.statusVal);
		this.mStatus.setImageResource(R.drawable.off);

		this.mStatusText = (TextView) findViewById(R.id.statusStr);

		this.mDNS = (EditText) findViewById(R.id.dnsval);
		this.mDNS.setFocusableInTouchMode(true);
		this.mDNS.setFocusable(true);
		setDefaultDNServer();

		this.mHOSTFWD = (EditText) findViewById(R.id.hostfwdval);
		this.mHOSTFWD.setFocusableInTouchMode(true);
		this.mHOSTFWD.setFocusable(true);

		this.mAppend = (EditText) findViewById(R.id.appendval);
		this.mAppend.setFocusableInTouchMode(true);
		this.mAppend.setFocusable(true);

		this.mExtraParams = (EditText) findViewById(R.id.extraparamsval);
		this.mExtraParams.setFocusableInTouchMode(true);
		this.mExtraParams.setFocusable(true);

		this.mMachine = (Spinner) findViewById(R.id.machineval);

		this.mCPU = (Spinner) findViewById(R.id.cpuval);
		this.mArch = (Spinner) findViewById(R.id.archtypeval);
		this.mMachineType = (Spinner) findViewById(R.id.machinetypeval);

		View machineType = (View) findViewById(R.id.machinetypel);

		this.mCPUNum = (Spinner) findViewById(R.id.cpunumval);
		this.mUI = (Spinner) findViewById(R.id.uival);
		if (!Config.enable_SDL)
			this.mUI.setEnabled(false);

		this.mRamSize = (Spinner) findViewById(R.id.rammemval);

		this.mKernel = (Spinner) findViewById(R.id.kernelval);
		this.mInitrd = (Spinner) findViewById(R.id.initrdval);

		this.mHDA = (Spinner) findViewById(R.id.hdimgval);
		this.mHDB = (Spinner) findViewById(R.id.hdbimgval);
		this.mHDC = (Spinner) findViewById(R.id.hdcimgval);
		this.mHDD = (Spinner) findViewById(R.id.hddimgval);
		this.mCD = (Spinner) findViewById(R.id.cdromimgval);
		this.mFDA = (Spinner) findViewById(R.id.floppyimgval);
		this.mFDB = (Spinner) findViewById(R.id.floppybimgval);
		this.mSD = (Spinner) findViewById(R.id.sdcardimgval);
		this.mSharedFolder = (Spinner) findViewById(R.id.sharedfolderval);

		this.mHDAenable = (CheckBox) findViewById(R.id.hdimgcheck);
		this.mHDBenable = (CheckBox) findViewById(R.id.hdbimgcheck);
		this.mHDCenable = (CheckBox) findViewById(R.id.hdcimgcheck);
		this.mHDDenable = (CheckBox) findViewById(R.id.hddimgcheck);
		this.mCDenable = (CheckBox) findViewById(R.id.cdromimgcheck);
		this.mFDAenable = (CheckBox) findViewById(R.id.floppyimgcheck);
		this.mFDBenable = (CheckBox) findViewById(R.id.floppybimgcheck);
		this.mSDenable = (CheckBox) findViewById(R.id.sdcardimgcheck);
		this.mSharedFolderenable = (CheckBox) findViewById(R.id.sharedfoldercheck);

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
		mVNCAllowExternal.setChecked(false);
		this.mPrio = (CheckBox) findViewById(R.id.prioval);
		mPrio.setChecked(LimboSettingsManager.getPrio(activity));

		this.mEnableKVM = (CheckBox) findViewById(R.id.enablekvmval);
		mEnableKVM.setChecked(LimboSettingsManager.getEnableKVM(activity));

		this.mToolBar = (CheckBox) findViewById(R.id.showtoolbarval);
		mToolBar.setChecked(LimboSettingsManager.getAlwaysShowMenuToolbar(activity));

		this.mFullScreen = (CheckBox) findViewById(R.id.fullscreenval);
		mFullScreen.setChecked(LimboSettingsManager.getFullscreen(activity));

		this.mOrientation = (Spinner) findViewById(R.id.orientationval);

		this.mKeyboard = (Spinner) findViewById(R.id.keyboardval);

		this.mSnapshot = (Spinner) findViewById(R.id.snapshotval);

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

		mStart = (ImageButton) findViewById(R.id.startvm);
		mStart.setFocusableInTouchMode(true);
		mStart.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {

				Thread thread = new Thread(new Runnable() {
					public void run() {
						onStartButton();
					}
				});
				thread.setPriority(Thread.MIN_PRIORITY);
				thread.start();

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

		enableRemovableDeviceOptions(false);
		enableNonRemovableDeviceOptions(false);
		mVNCAllowExternal.setEnabled(false);

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
                    final String machine = (String) ((ArrayAdapter<?>) mMachine.getAdapter()).getItem(position);
                    setUserPressed(false);
                    Thread thread = new Thread(new Runnable() {
                        public void run() {
                            loadMachine(machine, "");
                        }
                    });
                    thread.setPriority(Thread.MIN_PRIORITY);
                    thread.start();
                    populateSnapshot();
                    mVNCAllowExternal.setEnabled(true);

                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

	}

	private void setDefaultDNServer() {
		// TODO Auto-generated method stub
		Thread thread = new Thread(new Runnable() {
			public void run() {
				final String defaultDNSServer = LimboSettingsManager.getDNSServer(activity);
				new Handler(Looper.getMainLooper()).post(new Runnable() {
					public void run() {
						// Code here will run in UI thread
						mDNS.setText(defaultDNSServer);
					}
				});
			}
		});
		thread.setPriority(Thread.MIN_PRIORITY);
		thread.start();

	}

	private void setupSections() {

		if (Config.collapseSections) {
			mCPUSectionDetails = (LinearLayout) findViewById(R.id.cpusectionDetails);
			mCPUSectionDetails.setVisibility(View.GONE);
			mCPUSectionHeader = (TextView) findViewById(R.id.cpusectionStr);
			mCPUSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mCPUSectionDetails);
				}
			});

			mStorageSectionDetails = (LinearLayout) findViewById(R.id.storagesectionDetails);
			mStorageSectionDetails.setVisibility(View.GONE);
			mStorageSectionHeader = (TextView) findViewById(R.id.storagesectionStr);
			mStorageSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mStorageSectionDetails);
				}
			});

			mUserInterfaceSectionDetails = (LinearLayout) findViewById(R.id.userInterfaceDetails);
			mUserInterfaceSectionDetails.setVisibility(View.GONE);
			mUserInterfaceSectionHeader = (TextView) findViewById(R.id.userInterfaceSectionStr);
			mUserInterfaceSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mUserInterfaceSectionDetails);
				}
			});

			mRemovableStorageSectionDetails = (LinearLayout) findViewById(R.id.removableStoragesectionDetails);
			mRemovableStorageSectionDetails.setVisibility(View.GONE);
			mRemovableStorageSectionHeader = (TextView) findViewById(R.id.removableStoragesectionStr);
			mRemovableStorageSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mRemovableStorageSectionDetails);
				}
			});

			mGraphicsSectionDetails = (LinearLayout) findViewById(R.id.graphicssectionDetails);
			mGraphicsSectionDetails.setVisibility(View.GONE);
			mGraphicsSectionHeader = (TextView) findViewById(R.id.graphicssectionStr);
			mGraphicsSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mGraphicsSectionDetails);
				}
			});

			mAudioSectionDetails = (LinearLayout) findViewById(R.id.audiosectionDetails);
			mAudioSectionDetails.setVisibility(View.GONE);
			mAudioSectionHeader = (TextView) findViewById(R.id.audiosectionStr);
			mAudioSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mAudioSectionDetails);
				}
			});

			mNetworkSectionDetails = (LinearLayout) findViewById(R.id.networksectionDetails);
			mNetworkSectionDetails.setVisibility(View.GONE);
			mNetworkSectionHeader = (TextView) findViewById(R.id.networksectionStr);
			mNetworkSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mNetworkSectionDetails);
				}
			});

			mBootSectionDetails = (LinearLayout) findViewById(R.id.bootsectionDetails);
			mBootSectionDetails.setVisibility(View.GONE);
			mBootSectionHeader = (TextView) findViewById(R.id.bootsectionStr);
			mBootSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mBootSectionDetails);
				}
			});

			mAdvancedSectionDetails = (LinearLayout) findViewById(R.id.advancedSectionDetails);
			mAdvancedSectionDetails.setVisibility(View.GONE);
			mAdvancedSectionHeader = (TextView) findViewById(R.id.advancedSectionStr);
			mAdvancedSectionHeader.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					toggleVisibility(mAdvancedSectionDetails);
				}
			});
		}
	}

	private void triggerUpdateSpinner(final Spinner spinner) {

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

	private void promptPrio(final Activity activity) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable High Priority!");

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setPadding(20,20,20,20);
		textView.setText("Warning! High Priority might increase emulation speed but " + "will slow your phone down!");

		alertDialog.setView(textView);
		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				LimboSettingsManager.setPrio(activity, true);
			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
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

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable KVM!");

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setPadding(20,20,20,20);		textView.setText(
				"Warning! Enabling KVM is an UNTESTED and EXPERIMENTAL feature. If you experience crashes disable this option. Do you want to continue?");

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
		textView.setPadding(20,20,20,20);		textView.setText("VNC Server: " + this.getLocalIpAddress() + ":" + "5901\n"
				+ "Warning: VNC Connection is UNencrypted and not secure make sure you're on a private network!\n");

		final EditText passwdView = new EditText(activity);
		passwdView.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
		passwdView.setHint("Password");
		passwdView.setEnabled(true);
		passwdView.setVisibility(View.VISIBLE);
		passwdView.setSingleLine();

		LinearLayout mLayout = new LinearLayout(this);
		mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        LinearLayout.LayoutParams textViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(textView, textViewParams);

        LinearLayout.LayoutParams passwordViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(passwdView, passwordViewParams);

		alertDialog.setView(mLayout);

		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Set", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {


				if (passwdView.getText().toString().trim().equals("")) {
					Toast.makeText(getApplicationContext(), "Password cannot be empty!", Toast.LENGTH_SHORT).show();
					vnc_passwd = null;
					vnc_allow_external = 0;
					mVNCAllowExternal.setChecked(false);
					return;
				} else {
					sendHandlerMessage(handler, Config.VNC_PASSWORD, "vnc_passwd", "passwd");
					vnc_passwd = passwdView.getText().toString();
					vnc_allow_external = 1;
				}

			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				vnc_passwd = null;
				vnc_allow_external = 0;
				mVNCAllowExternal.setChecked(false);
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				mVNCAllowExternal.setChecked(false);
				vnc_passwd = null;
				vnc_allow_external = 0;
			}
		});
		alertDialog.show();

	}

	private void loadMachine(String machine, String snapshot) {


		machineLoaded = true;
		this.setUserPressed(false);

		// Load machine from DB
		this.currMachine = MachineOpenHelper.getInstance(activity).getMachine(machine, snapshot);

		if (currMachine == null) {
			return;
		}
		new Handler(Looper.getMainLooper()).post(new Runnable() {
			public void run() {

                mVNCAllowExternal.setChecked(false);

				populateMachineType(currMachine.machine_type);
				populateCPUs(currMachine.cpu);
				populateNetDevices(currMachine.nic_driver);

				setArch(currMachine.arch);
				setCPUNum(currMachine.cpuNum);
				setRAM(currMachine.memory);
				setKernel(currMachine.kernel);
				setInitrd(currMachine.initrd);
				if (currMachine.append != null)
					mAppend.setText(currMachine.append);
				else
					mAppend.setText("");

				if (currMachine.hostfwd != null)
					mHOSTFWD.setText(currMachine.hostfwd);
				else
					mHOSTFWD.setText("");

				if (currMachine.extra_params != null)
					mExtraParams.setText(currMachine.extra_params);
				else
					mExtraParams.setText("");

                // CDROM
				setCDROM(currMachine.cd_iso_path);

				// Floppy
				setFDA(currMachine.fda_img_path);
				setFDB(currMachine.fdb_img_path);

				// SD Card
				setSD(currMachine.sd_img_path);

				// HDD
				setHDA(currMachine.hda_img_path);
				setHDB(currMachine.hdb_img_path);
				setHDC(currMachine.hdc_img_path);
				setHDD(currMachine.hdd_img_path);
				setSharedFolder(currMachine.shared_folder);

				// Advance
				setBootDevice(currMachine.bootdevice);
				setNetCfg(currMachine.net_cfg, false);
				// this.setNicDevice(currMachine.nic_driver, false);
				setVGA(currMachine.vga_type);
				setHDCache(currMachine.hd_cache);
				setSoundcard(currMachine.soundcard);
				setUI(currMachine.ui);
				mACPI.setChecked(currMachine.disableacpi == 1 ? true : false);
				mHPET.setChecked(currMachine.disablehpet == 1 ? true : false);
				mFDBOOTCHK.setChecked(currMachine.disablefdbootchk == 1 ? true : false);
//				userPressedBluetoothMouse = false;


				// We finished loading now when a user change a setting it will
				// fire an
				// event

				enableNonRemovableDeviceOptions(true);
				enableRemovableDeviceOptions(true);

				if (Config.enable_sound) {
					if (currMachine.ui != null && currMachine.ui.equals("SDL")) {
						mSoundCardConfig.setEnabled(true);
					} else
						mSoundCardConfig.setEnabled(false);
				} else
					mSoundCardConfig.setEnabled(false);

				mMachine.setEnabled(false);

				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {


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

						if (currMachine.shared_folder != null) {
							mSharedFolderenable.setChecked(true);
						} else
							mSharedFolderenable.setChecked(false);

						if (currMachine.paused == 1) {
							sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", "PAUSED");
							enableNonRemovableDeviceOptions(false);
							enableRemovableDeviceOptions(false);
						} else {
							sendHandlerMessage(handler, Config.STATUS_CHANGED, "status_changed", "READY");
							enableNonRemovableDeviceOptions(true);
							enableRemovableDeviceOptions(true);
						}

						setUserPressed(true);
						machineLoaded = false;
						mMachine.setEnabled(true);
					}
				}, 1000);

			}
		});
	}

	public void promptMachineName(final Activity activity) {
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Machine Name");
		final EditText vmNameTextView = new EditText(activity);
		vmNameTextView.setPadding(20, 20, 20, 20);
		vmNameTextView.setEnabled(true);
		vmNameTextView.setVisibility(View.VISIBLE);
		vmNameTextView.setSingleLine();
		alertDialog.setView(vmNameTextView);
		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", (DialogInterface.OnClickListener) null);

		alertDialog.show();

		Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
		button.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				if (vmNameTextView.getText().toString().trim().equals(""))
					UIUtils.toastShort(activity, "Machine name cannot be empty");
				else {
					sendHandlerMessage(handler, Config.VM_CREATED, "machine_name", vmNameTextView.getText().toString());
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

		LinearLayout mLayout = new LinearLayout(this);
		mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

		final EditText imageNameView = new EditText(activity);
		imageNameView.setEnabled(true);
		imageNameView.setVisibility(View.VISIBLE);
		imageNameView.setSingleLine();
        LinearLayout.LayoutParams imageNameViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(imageNameView, imageNameViewParams);

		final Spinner size = new Spinner(this);
        LinearLayout.LayoutParams spinnerParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

		String[] arraySpinner = new String[3];
		arraySpinner[0] = "5 GB (Growable)";
		arraySpinner[1] = "10 GB (Growable)";
		arraySpinner[2] = "20 GB (Growable)";

		ArrayAdapter<?> sizeAdapter = new ArrayAdapter<Object>(this, R.layout.custom_spinner_item, arraySpinner);
		sizeAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		size.setAdapter(sizeAdapter);
		mLayout.addView(size, spinnerParams);

		alertDialog.setView(mLayout);

		final Handler handler = this.handler;

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				int sizeSel = size.getSelectedItemPosition();
				String templateImage = "hd1g.qcow2";
				if (sizeSel == 0) {
					templateImage = "hd5g.qcow2";
				} else if (sizeSel == 1) {
					templateImage = "hd10g.qcow2";
				} else if (sizeSel == 2) {
					templateImage = "hd20g.qcow2";
				}


				progDialog = ProgressDialog.show(activity, "Please Wait", "Creating HD Image...", true);

				String image = imageNameView.getText().toString();
				if (!image.endsWith(".qcow2")) {
					image += ".qcow2";
				}
				createImg(templateImage, image, hd_string);

			}
		});
		alertDialog.show();

	}

	protected boolean createImg(String templateImage, String destImage, String hd_string) {

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
		final EditText stateNameView = new EditText(activity);
        stateNameView.setPadding(20, 20, 20, 20);
        stateNameView.setEnabled(true);
        stateNameView.setVisibility(View.VISIBLE);
        stateNameView.setSingleLine();
		alertDialog.setView(stateNameView);
		final Handler handler = this.handler;

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				sendHandlerMessage(handler, Config.SNAPSHOT_CREATED, new String[] { "snapshot_name" },
						new String[] { stateNameView.getText().toString() });
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
			if (data != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				Uri uri = data.getData();
				DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
				String file = uri.toString();

				activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

				final int takeFlags = Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
				getContentResolver().takePersistableUriPermission(uri, takeFlags);
                // Protect from qemu thinking it's a protocol
                file = ("/" + file).replace(":", "");
				setDriveAttr(filetype, file);

			}

		}


	}

	private void setDriveAttr(String fileType, String file) {

		this.addDriveToList(file, fileType);
		if (fileType != null && fileType.startsWith("hd") && file != null && !file.trim().equals("")) {

			if (fileType.startsWith("hda")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, file);
				if (this.hdaAdapter.getPosition(file) < 0) {
					this.hdaAdapter.add(file);
				}
				this.setHDA(file);
			} else if (fileType.startsWith("hdb")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, file);
				if (this.hdbAdapter.getPosition(file) < 0) {
					this.hdbAdapter.add(file);
				}
				this.setHDB(file);
			} else if (fileType.startsWith("hdc")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, file);
				if (this.hdcAdapter.getPosition(file) < 0) {
					this.hdcAdapter.add(file);
				}
				this.setHDC(file);
			} else if (fileType.startsWith("hdd")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, file);
				if (this.hddAdapter.getPosition(file) < 0) {
					this.hddAdapter.add(file);
				}
				this.setHDD(file);
			}
		} else if (fileType != null && fileType.startsWith("cd") && file != null && !file.trim().equals("")) {
			int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM, file);
			if (this.cdromAdapter.getPosition(file) < 0) {
				this.cdromAdapter.add(file);
			}
			setCDROM(file);
		} else if (fileType != null && fileType.startsWith("fd") && file != null && !file.trim().equals("")) {
			if (fileType.startsWith("fda")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, file);
				if (this.fdaAdapter.getPosition(file) < 0) {
					this.fdaAdapter.add(file);
				}
				this.setFDA(file);
			} else if (fileType.startsWith("fdb")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, file);
				if (this.fdbAdapter.getPosition(file) < 0) {
					this.fdbAdapter.add(file);
				}
				this.setFDB(file);
			}
		} else if (fileType != null && fileType.startsWith("sd") && file != null && !file.trim().equals("")) {
			if (fileType.startsWith("sd")) {
				int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, file);
				if (this.sdAdapter.getPosition(file) < 0) {
					this.sdAdapter.add(file);
				}
				this.setSD(file);
			}
		} else if (fileType != null && fileType.startsWith("kernel") && file != null && !file.trim().equals("")) {

			int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KERNEL, file);
			if (this.kernelAdapter.getPosition(file) < 0) {
				this.kernelAdapter.add(file);
			}
			this.setKernel(file);

		} else if (fileType != null && fileType.startsWith("initrd") && file != null && !file.trim().equals("")) {

			int ret = MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.INITRD, file);
			if (this.initrdAdapter.getPosition(file) < 0) {
				this.initrdAdapter.add(file);
			}
			this.setInitrd(file);

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

	public void startvnc() {

		// Wait till Qemu settles
		try {
			Thread.sleep(2000);
		} catch (InterruptedException ex) {
			Logger.getLogger(LimboActivity.class.getName()).log(Level.SEVERE, null, ex);
		}

        this.mVNCAllowExternal.setEnabled(false);

		if (this.mVNCAllowExternal.isChecked()
                && vnc_passwd != null && !vnc_passwd.equals("")) {
			String ret = vmexecutor.change_vnc_password();
            if(currMachine.paused != 1)
                promptConnectLocally(activity);
            else
                connectLocally();
		} else {
            connectLocally();
		}

	}

	public void promptConnectLocally(final Activity activity) {

        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                final AlertDialog alertDialog;
                alertDialog = new AlertDialog.Builder(activity).create();
                alertDialog.setTitle("VNC Started");
                TextView stateView = new TextView(activity);
                stateView.setText("VNC Server started: " + getLocalIpAddress() + ":" + "5901\n"
                        + "Warning: VNC Connection is Unencrypted and not secure make sure you're on a private network!\n");

                stateView.setPadding(20, 20, 20, 20);
                alertDialog.setView(stateView);

                alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        alertDialog.dismiss();
                    }
                });
                alertDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Connect Locally", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        connectLocally();
                    }
                });
                alertDialog.show();
            }
        }, 100);


	}

	public void connectLocally() {
        UIUtils.toastShort(LimboActivity.this, "Connecting to VM Display");
        Intent intent = getVNCIntent();
        startActivityForResult(intent, Config.VNC_REQUEST_CODE);
    }

	public void startsdl() {

		Intent intent = null;

		intent = new Intent(this, LimboSDLActivity.class);

		android.content.ContentValues values = new android.content.ContentValues();
		startActivityForResult(intent, Config.SDL_REQUEST_CODE);
	}

	public void restartvm() {
		if (vmexecutor != null) {

			output = vmexecutor.stopvm(1);
            vmStarted = true;
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

		String[] arraySpinner = new String[128];

		arraySpinner[0] = 4 + "";
		for (int i = 1; i < arraySpinner.length; i++) {
			arraySpinner[i] = i * 8 + "";
		}
		;

		ramAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		ramAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mRamSize.setAdapter(ramAdapter);
		this.mRamSize.invalidate();
	}

	private void populateCPUNum() {
		String[] arraySpinner = new String[4];

		for (int i = 0; i < arraySpinner.length; i++) {
			arraySpinner[i] = (i + 1) + "";
		}


		cpuNumAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		cpuNumAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mCPUNum.setAdapter(cpuNumAdapter);
		this.mCPUNum.invalidate();
	}

	// Set Hard Disk
	private void setRAM(final int ram) {

		this.mRamSize.post(new Runnable() {
			public void run() {
				if (ram != 0) {
					int pos = ramAdapter.getPosition(ram + "");
					mRamSize.setSelection(pos);
				}
			}
		});

	}

	private void setCPUNum(final int cpuNum) {

		this.mCPUNum.post(new Runnable() {
			public void run() {
				if (cpuNum != 0) {
					int pos = cpuNumAdapter.getPosition(cpuNum + "");
					mCPUNum.setSelection(pos);
				}
			}
		});

	}

	// Set Hard Disk
	private void populateBootDevices() {

		String[] arraySpinner = { "Default", "CD Rom", "Floppy", "Hard Disk" };

		bootDevAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		bootDevAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mBootDevices.setAdapter(bootDevAdapter);
		this.mBootDevices.invalidate();
	}

	// Set Net Cfg
	private void populateNet() {
		String[] arraySpinner = { "None", "User", "TAP" };
		netAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		netAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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

		if (Config.enable_SPICE)
			arrList.add("qxl");

		// Add XEN
		// "xenfb",
		// None for console only
		// "none"

		vgaAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
		vgaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mVGAConfig.setAdapter(vgaAdapter);
		this.mVGAConfig.invalidate();
	}

	private void populateOrientation() {

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		arrList.add("Auto");
		arrList.add("Landscape");
		arrList.add("Landscape Reverse");
		arrList.add("Portrait");
        arrList.add("Portrait Reverse");

		orientationAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
		orientationAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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
		// FIXME: Need to enable in VNC & SDL interfaces
		// arrList.add("Spanish");
		// arrList.add("French");

		keyboardAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
		keyboardAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mKeyboard.setAdapter(keyboardAdapter);
		this.mKeyboard.invalidate();

		int pos = LimboSettingsManager.getKeyboardSetting(activity);
		if (pos >= 0) {
			this.mKeyboard.setSelection(pos);
		}
	}

	private void populateSoundcardConfig() {

		String[] arraySpinner = { "None", "sb16", "ac97", "adlib", "cs4231a", "gus", "es1370", "hda", "pcspk", "all" };

		sndAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		sndAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mSoundCardConfig.setAdapter(sndAdapter);
		this.mSoundCardConfig.invalidate();
	}

	// Set Cache Cfg
	private void populateHDCacheConfig() {

		String[] arraySpinner = { "default", "none", "writeback", "writethrough" };

		hdCacheAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		hdCacheAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mHDCacheConfig.setAdapter(hdCacheAdapter);
		this.mHDCacheConfig.invalidate();
	}

	// Set Hard Disk
	private void populateNetDevices(String nic) {

		String[] arraySpinner = { "e1000", "pcnet", "rtl8139", "ne2k_pci", "i82551", "i82557b", "i82559er", "virtio" };

		// String[] arraySpinner = {
		// ne2k_pci,i82551,i82557b,i82559er,rtl8139,e1000,pcnet,virtio

		ArrayList<String> arrList = new ArrayList<String>();

		if (currMachine != null) {
			if (currMachine.arch.equals("x86")) {

				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
			} else if (currMachine.arch.equals("x64")) {
				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
			} else if (currMachine.arch.equals("ARM")) {
				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
				arrList.add("smc91c111");
			} else if (currMachine.arch.equals("PPC") || currMachine.arch.equals("PPC64") ) {
				arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
			} else if (currMachine.arch.equals("SPARC")) {
				arrList.add("lance");
			}
		} else {
			arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		}

		if (nicCfgAdapter == null) {
			nicCfgAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
			nicCfgAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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

	private void setMachine(final String machine) {

		this.mMachine.post(new Runnable() {
			public void run() {
				if (machine != null) {
					int pos = machineAdapter.getPosition(machine);

					mMachine.setSelection(pos);
				}
			}
		});

	}

	// Set Hard Disk
	private void populateMachines(final String machineValue) {

		Thread thread = new Thread(new Runnable() {
			public void run() {

				ArrayList<String> machines = MachineOpenHelper.getInstance(activity).getMachines();
				int length = 0;
				if (machines == null || machines.size() == 0) {
					length = 0;
				} else {
					length = machines.size();
				}

				final String[] arraySpinner = new String[machines.size() + 2];
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

				new Handler(Looper.getMainLooper()).post(new Runnable() {
					public void run() {
						machineAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
						machineAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
						mMachine.setAdapter(machineAdapter);
						mMachine.invalidate();
                        if(machineValue != null)
                            setMachine(machineValue);
					}
				});

			}
		});
		thread.setPriority(Thread.MIN_PRIORITY);
		thread.start();

	}

	// Set Hard Disk
	private void setCPU(final String cpu) {

		this.mCPU.post(new Runnable() {
			public void run() {
				if (cpu != null) {
					int pos = cpuAdapter.getPosition(cpu);

					mCPU.setSelection(pos);
				}
			}
		});

	}

	private void setArch(final String arch) {

		this.mArch.post(new Runnable() {
			public void run() {
				if (arch != null) {
					int pos = archAdapter.getPosition(arch);

					mArch.setSelection(pos);
				}
			}
		});

	}

	private void setMachineType(final String machineType) {

		this.mMachineType.post(new Runnable() {
			public void run() {
				if (machineType != null) {
					int pos = machineTypeAdapter.getPosition(machineType);
					mMachineType.setSelection(pos);
				}

			}
		});

	}

	private void setCDROM(final String cdrom) {

		this.currMachine.cd_iso_path = cdrom;

		mCD.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setKernel(final String kernel) {
		currMachine.kernel = kernel;
		mKernel.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setInitrd(final String initrd) {
		currMachine.initrd = initrd;

		mInitrd.post(new Runnable() {
			public void run() {
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
		});
	}

	private void setHDA(final String hda) {
		currMachine.hda_img_path = hda;

		mHDA.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setHDB(final String hdb) {
		this.currMachine.hdb_img_path = hdb;

		mHDB.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setHDC(final String hdc) {

		this.currMachine.hdc_img_path = hdc;

		mHDC.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setHDD(final String hdd) {
		this.currMachine.hdd_img_path = hdd;

		mHDD.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setFDA(final String fda) {

		this.currMachine.fda_img_path = fda;

		mFDA.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setFDB(final String fdb) {

		this.currMachine.fdb_img_path = fdb;

		mFDB.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setSD(final String sd) {

		this.currMachine.sd_img_path = sd;

		mSD.post(new Runnable() {
			public void run() {
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
		});

	}

	private void setSharedFolder(final String sharedFolder) {

		this.currMachine.shared_folder = sharedFolder;

		mSharedFolder.post(new Runnable() {
			public void run() {
				String sharedMode = null;
				if (currMachine.shared_folder_mode == 0) {
					sharedMode = "(read-only)";
				} else if (currMachine.shared_folder_mode == 1) {
					sharedMode = "(read-write)";
				}
				if (sharedFolder != null) {
					int pos = sharedFolderAdapter.getPosition(sharedFolder + " " + sharedMode);

					if (pos >= 0) {
						mSharedFolder.setSelection(pos);
					} else {
						mSharedFolder.setSelection(0);
					}
				} else {
					mSharedFolder.setSelection(0);

				}
			}
		});

	}

	//XXX: Not supported
	private void setHDCache(final String hdcache) {

		mHDCacheConfig.post(new Runnable() {
			public void run() {
				if (hdcache != null) {
					int pos = hdCacheAdapter.getPosition(hdcache);

					if (pos >= 0) {
						mHDCacheConfig.setSelection(pos);
					} else {
						mHDCacheConfig.setSelection(0);
					}
				} else {
					mHDCacheConfig.setSelection(0);

				}
			}
		});

	}

	private void setSoundcard(final String soundcard) {

		this.mSoundCardConfig.post(new Runnable() {
			public void run() {
				if (soundcard != null) {
					int pos = sndAdapter.getPosition(soundcard);

					if (pos >= 0) {
						mSoundCardConfig.setSelection(pos);
					} else {
						mSoundCardConfig.setSelection(0);
					}
				} else {
					mSoundCardConfig.setSelection(0);
				}
			}
		});

	}

	private void setUI(final String ui) {

		this.mUI.post(new Runnable() {
			public void run() {
				if (ui != null) {
					int pos = uiAdapter.getPosition(ui);

					if (pos >= 0) {
						mUI.setSelection(pos);
					} else {
						mUI.setSelection(0);
					}
				} else {
					mUI.setSelection(0);

				}
			}
		});

	}

	private void setVGA(final String vga) {

		this.mVGAConfig.post(new Runnable() {
			public void run() {
				if (vga != null) {
					int pos = vgaAdapter.getPosition(vga);

					if (pos >= 0) {
						mVGAConfig.setSelection(pos);
					} else {
						mVGAConfig.setSelection(0);
					}
				} else {
					mVGAConfig.setSelection(0);

				}
			}
		});

	}

	private void setNetCfg(final String net, boolean userPressed) {
//		this.userPressedNetCfg = userPressed;

		this.mNetConfig.post(new Runnable() {
			public void run() {
				if (net != null) {
					int pos = netAdapter.getPosition(net);

					if (pos >= 0) {
						mNetConfig.setSelection(pos);
					} else {
						mNetConfig.setSelection(0);
					}
				} else {
					mNetConfig.setSelection(0);

				}
			}
		});

	}

	private void setBootDevice(final String bootDevice) {

		this.mBootDevices.post(new Runnable() {
			public void run() {
				if (bootDevice != null) {
					int pos = bootDevAdapter.getPosition(bootDevice);

					if (pos >= 0) {
						mBootDevices.setSelection(pos);
					} else {
						mBootDevices.setSelection(0);
					}
				} else {
					mBootDevices.setSelection(0);

				}
			}
		});

	}

	private void setSnapshot(final String snapshot) {

		this.mSnapshot.post(new Runnable() {
			public void run() {
				if (snapshot != null && !snapshot.equals("")) {
					int pos = snapshotAdapter.getPosition(snapshot);

					if (pos >= 0) {
						mSnapshot.setSelection(pos);
						mSnapshot.invalidate();
					} else {
						mSnapshot.setSelection(0);
					}
				} else {
					mSnapshot.setSelection(0);

				}

			}
		});

	}

	private void setNicDevice(final String nic) {

		this.mNetDevices.post(new Runnable() {
			public void run() {
				if (nic != null) {
					int pos = nicCfgAdapter.getPosition(nic);

					if (pos >= 0) {
						mNetDevices.setSelection(pos);
					} else {
						mNetDevices.setSelection(3);
					}
				} else {
					mNetDevices.setSelection(3);

				}
			}
		});

	}

	private void populateCPUs(String cpu) {

		String[] arraySpinner = {};
		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));

		// x86 cpus 32 bit
		ArrayList<String> arrX86 = new ArrayList<String>();
		arrX86.add("Default");
		arrX86.add("qemu32");// QEMU Virtual CPU version 2.3.0
		arrX86.add("kvm32");// Common 32-bit KVM processor
		arrX86.add("coreduo");// Genuine Intel(R) CPU T2600 @ 2.16GHz
		arrX86.add("486");
		arrX86.add("pentium");
		arrX86.add("pentium2");
		arrX86.add("pentium3");
		arrX86.add("athlon"); // QEMU Virtual CPU version 2.3.0
		arrX86.add("n270"); // Intel(R) Atom(TM) CPU N270 @ 1.60GHz

		// x86 cpus 64 bit
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
		arrX86_64.add("Westmere");// e Westmere E56xx/L56xx/X56xx (Nehalem-C)
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

		// ARM cpus
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

		// Mips cpus
		ArrayList<String> arrMips = new ArrayList<String>();
		arrMips.add("Default");

		ArrayList<String> arrPpc = new ArrayList<String>();
		arrPpc.add("Default");
		arrPpc.add("601");// (alias for 601_v2)
		arrPpc.add("603");// PVR 00030100
		arrPpc.add("Vanilla");// (alias for 603)
		arrPpc.add("604");// PVR 00040103
		arrPpc.add("ppc32");// (alias for 604)
		arrPpc.add("ppc");// (alias for 604)
		arrPpc.add("default");// (alias for 604)
		arrPpc.add("602");// PVR 00050100
		arrPpc.add("Goldeneye");// (alias for 603e7t)
		arrPpc.add("740e");// PVR 00080100
		arrPpc.add("G3");// (alias for 750_v3.1)
		arrPpc.add("Arthur");// (alias for 740_v3.1)
		arrPpc.add("745");// (alias for 745_v2.8)
		arrPpc.add("Goldfinger");// (alias for 755_v2.8)
		arrPpc.add("LoneStar");// (alias for 750l_v3.2)
		arrPpc.add("Mach5");// (alias for 604r)
		arrPpc.add("G4");// (alias for 7400_v2.9)
		arrPpc.add("403");// (alias for 403GC)
		arrPpc.add("G2");// PVR 00810011
		arrPpc.add("Cobra");// PVR 10100000
		arrPpc.add("STB03");// PVR 40310000
		arrPpc.add("STB04");// PVR 41810000
		arrPpc.add("405");// (alias for 405D4)
		arrPpc.add("STB25");// PVR 51510950
		arrPpc.add("750fl");// PVR 70000203
		arrPpc.add("750gl");// PVR 70020102
		arrPpc.add("7450");// (alias for 7450_v2.1)
		arrPpc.add("Vger");// (alias for 7450_v2.1)
		arrPpc.add("7441");// (alias for 7441_v2.3)
		arrPpc.add("7451");// (alias for 7451_v2.3)
		arrPpc.add("7445");// (alias for 7445_v3.2)
		arrPpc.add("7455");// (alias for 7455_v3.2)
		arrPpc.add("7457");// (alias for 7457_v1.2)
		arrPpc.add("e600");// PVR 80040010
		arrPpc.add("7448");// (alias for 7448_v2.1)
		arrPpc.add("7410");// (alias for 7410_v1.4)
		arrPpc.add("Nitro");// (alias for 7410_v1.4)
		arrPpc.add("e500");// (alias for e500v2_v22)

        ArrayList<String> arrPpc64 = new ArrayList<String>();
        arrPpc64.add("Default");
        arrPpc64.add("601_v1");
        arrPpc64.add("601_v0");
        arrPpc64.add("601_v2");
        arrPpc64.add("601");
        arrPpc64.add("601v");
        arrPpc64.add("603");
        arrPpc64.add("mpc8240");
        arrPpc64.add("vanilla");
        arrPpc64.add("604");
        arrPpc64.add("ppc32");
        arrPpc64.add("ppc");
        arrPpc64.add("default");
        arrPpc64.add("602");
        arrPpc64.add("603e_v1.1");
        arrPpc64.add("603e_v1.2");
        arrPpc64.add("603e_v1.3");
        arrPpc64.add("603e_v1.4");
        arrPpc64.add("603e_v2.2");
        arrPpc64.add("603e_v3");
        arrPpc64.add("603e_v4");
        arrPpc64.add("603e_v4.1");
        arrPpc64.add("603e");
        arrPpc64.add("stretch");
        arrPpc64.add("603p");
        arrPpc64.add("603e7v");
        arrPpc64.add("vaillant");
        arrPpc64.add("603e7v1");
        arrPpc64.add("6030000000");
        arrPpc64.add("603e7v2");
        arrPpc64.add("603e7t");
        arrPpc64.add("603r");
        arrPpc64.add("goldeneye");
        arrPpc64.add("750_v1.0");
        arrPpc64.add("740_v1.0");
        arrPpc64.add("740e");
        arrPpc64.add("750e");
        arrPpc64.add("750_v2.0");
        arrPpc64.add("740_v2.0");
        arrPpc64.add("750_v2.1");
        arrPpc64.add("740_v2.1");
        arrPpc64.add("740_v2.2");
        arrPpc64.add("750_v2.2");
        arrPpc64.add("750_v3.0");
        arrPpc64.add("740_v3.0");
        arrPpc64.add("750_v3.1");
        arrPpc64.add("750");
        arrPpc64.add("typhoon");
        arrPpc64.add("g3");
        arrPpc64.add("740_v3.1");
        arrPpc64.add("740");
        arrPpc64.add("arthur");
        arrPpc64.add("750cx_v1.0");
        arrPpc64.add("750cx_v2.0");
        arrPpc64.add("750cx_v2.1");
        arrPpc64.add("750cx_v2.2");
        arrPpc64.add("750cx");
        arrPpc64.add("750cxe_v2.1");
        arrPpc64.add("750cxe_v2.2");
        arrPpc64.add("750cxe_v2.3");
        arrPpc64.add("750cxe_v2.4");
        arrPpc64.add("750cxe_v3.0");
        arrPpc64.add("750cxe_v3.1");
        arrPpc64.add("755_v1.0");
        arrPpc64.add("745_v1.0");
        arrPpc64.add("755_v1.1");
        arrPpc64.add("745_v1.1");
        arrPpc64.add("755_v2.0");
        arrPpc64.add("745_v2.0");
        arrPpc64.add("755_v2.1");
        arrPpc64.add("745_v2.1");
        arrPpc64.add("745_v2.2");
        arrPpc64.add("755_v2.2");
        arrPpc64.add("755_v2.3");
        arrPpc64.add("745_v2.3");
        arrPpc64.add("755_v2.4");
        arrPpc64.add("745_v2.4");
        arrPpc64.add("745_v2.5");
        arrPpc64.add("755_v2.5");
        arrPpc64.add("755_v2.6");
        arrPpc64.add("745_v2.6");
        arrPpc64.add("755_v2.7");
        arrPpc64.add("745_v2.7");
        arrPpc64.add("745_v2.8");
        arrPpc64.add("745");
        arrPpc64.add("755_v2.8");
        arrPpc64.add("755");
        arrPpc64.add("goldfinger");
        arrPpc64.add("750cxe_v2.4b");
        arrPpc64.add("750cxe_v3.1b");
        arrPpc64.add("750cxe");
        arrPpc64.add("750cxr");
        arrPpc64.add("750cl_v1.0");
        arrPpc64.add("750cl_v2.0");
        arrPpc64.add("750cl");
        arrPpc64.add("750l_v2.0");
        arrPpc64.add("750l_v2.1");
        arrPpc64.add("750l_v2.2");
        arrPpc64.add("750l_v3.0");
        arrPpc64.add("750l_v3.2");
        arrPpc64.add("750l");
        arrPpc64.add("lonestar");
        arrPpc64.add("604e_v1.0");
        arrPpc64.add("604e_v2.2");
        arrPpc64.add("604e_v2.4");
        arrPpc64.add("604e");
        arrPpc64.add("sirocco");
        arrPpc64.add("604r");
        arrPpc64.add("mach5");
        arrPpc64.add("7400_v1.0");
        arrPpc64.add("7400_v1.1");
        arrPpc64.add("7400_v2.0");
        arrPpc64.add("7400_v2.1");
        arrPpc64.add("7400_v2.2");
        arrPpc64.add("7400_v2.6");
        arrPpc64.add("7400_v2.7");
        arrPpc64.add("7400_v2.8");
        arrPpc64.add("7400_v2.9");
        arrPpc64.add("7400");
        arrPpc64.add("max");
        arrPpc64.add("g4");
        arrPpc64.add("403ga");
        arrPpc64.add("403gb");
        arrPpc64.add("403gc");
        arrPpc64.add("403");
        arrPpc64.add("403gcx");
        arrPpc64.add("401a1");
        arrPpc64.add("401b2");
        arrPpc64.add("iop480");
        arrPpc64.add("401c2");
        arrPpc64.add("401d2");
        arrPpc64.add("40100");
        arrPpc64.add("401f2");
        arrPpc64.add("401g2");
        arrPpc64.add("401");
        arrPpc64.add("g2");
        arrPpc64.add("mpc603");
        arrPpc64.add("g2hip3");
        arrPpc64.add("mpc8250_hip3");
        arrPpc64.add("mpc8255_hip3");
        arrPpc64.add("mpc8260_hip3");
        arrPpc64.add("mpc8264_hip3");
        arrPpc64.add("mpc8265_hip3");
        arrPpc64.add("mpc8266_hip3");
        arrPpc64.add("mpc8349a");
        arrPpc64.add("mpc8347ap");
        arrPpc64.add("mpc8347eap");
        arrPpc64.add("mpc8347p");
        arrPpc64.add("mpc8349");
        arrPpc64.add("mpc8343e");
        arrPpc64.add("mpc8343ea");
        arrPpc64.add("mpc8343");
        arrPpc64.add("mpc8347et");
        arrPpc64.add("mpc8347e");
        arrPpc64.add("mpc8349e");
        arrPpc64.add("mpc8347at");
        arrPpc64.add("mpc8347a");
        arrPpc64.add("mpc8349ea");
        arrPpc64.add("mpc8347eat");
        arrPpc64.add("mpc8347ea");
        arrPpc64.add("mpc8343a");
        arrPpc64.add("mpc8347t");
        arrPpc64.add("mpc8347");
        arrPpc64.add("mpc8347ep");
        arrPpc64.add("e300c1");
        arrPpc64.add("e300c2");
        arrPpc64.add("e300c3");
        arrPpc64.add("e300");
        arrPpc64.add("mpc8379");
        arrPpc64.add("mpc8378e");
        arrPpc64.add("mpc8379e");
        arrPpc64.add("mpc8378");
        arrPpc64.add("mpc8377");
        arrPpc64.add("e300c4");
        arrPpc64.add("mpc8377e");
        arrPpc64.add("750p");
        arrPpc64.add("conan/doyle");
        arrPpc64.add("740p");
        arrPpc64.add("cobra");
        arrPpc64.add("460exb");
        arrPpc64.add("460ex");
        arrPpc64.add("440epx");
        arrPpc64.add("405d2");
        arrPpc64.add("x2vp4");
        arrPpc64.add("x2vp7");
        arrPpc64.add("x2vp20");
        arrPpc64.add("x2vp50");
        arrPpc64.add("405gpa");
        arrPpc64.add("405gpb");
        arrPpc64.add("405cra");
        arrPpc64.add("405gpc");
        arrPpc64.add("405gpd");
        arrPpc64.add("405gp");
        arrPpc64.add("405crb");
        arrPpc64.add("405crc");
        arrPpc64.add("405cr");
        arrPpc64.add("405gpe");
        arrPpc64.add("stb03");
        arrPpc64.add("npe4gs3");
        arrPpc64.add("npe405h");
        arrPpc64.add("npe405h2");
        arrPpc64.add("405ez");
        arrPpc64.add("npe405l");
        arrPpc64.add("405d4");
        arrPpc64.add("405");
        arrPpc64.add("stb04");
        arrPpc64.add("405lp");
        arrPpc64.add("440epa");
        arrPpc64.add("440epb");
        arrPpc64.add("440ep");
        arrPpc64.add("405gpr");
        arrPpc64.add("405ep");
        arrPpc64.add("stb25");
        arrPpc64.add("750fx_v1.0");
        arrPpc64.add("750fx_v2.0");
        arrPpc64.add("750fx_v2.1");
        arrPpc64.add("750fx_v2.2");
        arrPpc64.add("750fl");
        arrPpc64.add("750fx_v2.3");
        arrPpc64.add("750fx");
        arrPpc64.add("750gx_v1.0");
        arrPpc64.add("750gx_v1.1");
        arrPpc64.add("750gx_v1.2");
        arrPpc64.add("750gx");
        arrPpc64.add("750gl");
        arrPpc64.add("440-xilinx");
        arrPpc64.add("440-xilinx-w-dfpu");
        arrPpc64.add("7450_v1.0");
        arrPpc64.add("7450_v1.1");
        arrPpc64.add("7450_v1.2");
        arrPpc64.add("7450_v2.0");
        arrPpc64.add("7450_v2.1");
        arrPpc64.add("7450");
        arrPpc64.add("vger");
        arrPpc64.add("7441_v2.1");
        arrPpc64.add("7441_v2.3");
        arrPpc64.add("7441");
        arrPpc64.add("7451_v2.3");
        arrPpc64.add("7451");
        arrPpc64.add("7451_v2.10");
        arrPpc64.add("7441_v2.10");
        arrPpc64.add("7455_v1.0");
        arrPpc64.add("7445_v1.0");
        arrPpc64.add("7445_v2.1");
        arrPpc64.add("7455_v2.1");
        arrPpc64.add("7445_v3.2");
        arrPpc64.add("7445");
        arrPpc64.add("7455_v3.2");
        arrPpc64.add("7455");
        arrPpc64.add("apollo6");
        arrPpc64.add("7455_v3.3");
        arrPpc64.add("7445_v3.3");
        arrPpc64.add("7455_v3.4");
        arrPpc64.add("7445_v3.4");
        arrPpc64.add("7447_v1.0");
        arrPpc64.add("7457_v1.0");
        arrPpc64.add("7457_v1.1");
        arrPpc64.add("7447_v1.1");
        arrPpc64.add("7447");
        arrPpc64.add("7457_v1.2");
        arrPpc64.add("7457");
        arrPpc64.add("apollo7");
        arrPpc64.add("7457a_v1.0");
        arrPpc64.add("apollo7pm");
        arrPpc64.add("7447a_v1.0");
        arrPpc64.add("7447a_v1.1");
        arrPpc64.add("7457a_v1.1");
        arrPpc64.add("7447a_v1.2");
        arrPpc64.add("7447a");
        arrPpc64.add("7457a_v1.2");
        arrPpc64.add("7457a");
        arrPpc64.add("e600");
        arrPpc64.add("mpc8610");
        arrPpc64.add("mpc8641d");
        arrPpc64.add("mpc8641");
        arrPpc64.add("7448_v1.0");
        arrPpc64.add("7448_v1.1");
        arrPpc64.add("7448_v2.0");
        arrPpc64.add("7448_v2.1");
        arrPpc64.add("7448");
        arrPpc64.add("7410_v1.0");
        arrPpc64.add("7410_v1.1");
        arrPpc64.add("7410_v1.2");
        arrPpc64.add("7410_v1.3");
        arrPpc64.add("7410_v1.4");
        arrPpc64.add("7410");
        arrPpc64.add("nitro");
        arrPpc64.add("e500_v10");
        arrPpc64.add("mpc8540_v10");
        arrPpc64.add("mpc8540_v21");
        arrPpc64.add("mpc8540");
        arrPpc64.add("e500_v20");
        arrPpc64.add("e500v1");
        arrPpc64.add("mpc8541_v10");
        arrPpc64.add("mpc8541e_v11");
        arrPpc64.add("mpc8541e");
        arrPpc64.add("mpc8540_v20");
        arrPpc64.add("mpc8541e_v10");
        arrPpc64.add("mpc8541_v11");
        arrPpc64.add("mpc8541");
        arrPpc64.add("mpc8555_v10");
        arrPpc64.add("mpc8548_v10");
        arrPpc64.add("mpc8543_v10");
        arrPpc64.add("mpc8543e_v10");
        arrPpc64.add("mpc8548e_v10");
        arrPpc64.add("mpc8555e_v10");
        arrPpc64.add("e500v2_v10");
        arrPpc64.add("mpc8560_v10");
        arrPpc64.add("mpc8543e_v11");
        arrPpc64.add("mpc8548e_v11");
        arrPpc64.add("mpc8555e_v11");
        arrPpc64.add("mpc8555e");
        arrPpc64.add("mpc8555_v11");
        arrPpc64.add("mpc8555");
        arrPpc64.add("mpc8548_v11");
        arrPpc64.add("mpc8543_v11");
        arrPpc64.add("mpc8547e_v20");
        arrPpc64.add("e500v2_v20");
        arrPpc64.add("mpc8560_v20");
        arrPpc64.add("mpc8545e_v20");
        arrPpc64.add("mpc8545_v20");
        arrPpc64.add("mpc8548_v20");
        arrPpc64.add("mpc8543_v20");
        arrPpc64.add("mpc8543e_v20");
        arrPpc64.add("mpc8548e_v20");
        arrPpc64.add("mpc8545_v21");
        arrPpc64.add("mpc8545");
        arrPpc64.add("mpc8548_v21");
        arrPpc64.add("mpc8548");
        arrPpc64.add("mpc8543_v21");
        arrPpc64.add("mpc8543");
        arrPpc64.add("mpc8544_v10");
        arrPpc64.add("mpc8543e_v21");
        arrPpc64.add("mpc8543e");
        arrPpc64.add("mpc8544e_v10");
        arrPpc64.add("mpc8533_v10");
        arrPpc64.add("mpc8548e_v21");
        arrPpc64.add("mpc8548e");
        arrPpc64.add("mpc8547e_v21");
        arrPpc64.add("mpc8547e");
        arrPpc64.add("mpc8560_v21");
        arrPpc64.add("mpc8560");
        arrPpc64.add("e500v2_v21");
        arrPpc64.add("mpc8533e_v10");
        arrPpc64.add("mpc8545e_v21");
        arrPpc64.add("mpc8545e");
        arrPpc64.add("mpc8533_v11");
        arrPpc64.add("mpc8533");
        arrPpc64.add("mpc8567e");
        arrPpc64.add("e500v2_v22");
        arrPpc64.add("e500");
        arrPpc64.add("e500v2");
        arrPpc64.add("mpc8533e_v11");
        arrPpc64.add("mpc8533e");
        arrPpc64.add("mpc8568e");
        arrPpc64.add("mpc8568");
        arrPpc64.add("mpc8567");
        arrPpc64.add("mpc8544e_v11");
        arrPpc64.add("mpc8544e");
        arrPpc64.add("mpc8544_v11");
        arrPpc64.add("mpc8544");
        arrPpc64.add("e500v2_v30");
        arrPpc64.add("mpc8572e");
        arrPpc64.add("mpc8572");
        arrPpc64.add("e500mc");
        arrPpc64.add("g2h4");
        arrPpc64.add("g2hip4");
        arrPpc64.add("mpc8241");
        arrPpc64.add("mpc8245");
        arrPpc64.add("mpc8250");
        arrPpc64.add("mpc8250_hip4");
        arrPpc64.add("mpc8255");
        arrPpc64.add("mpc8255_hip4");
        arrPpc64.add("mpc8260");
        arrPpc64.add("mpc8260_hip4");
        arrPpc64.add("mpc8264");
        arrPpc64.add("mpc8264_hip4");
        arrPpc64.add("mpc8265");
        arrPpc64.add("mpc8265_hip4");
        arrPpc64.add("mpc8266");
        arrPpc64.add("mpc8266_hip4");
        arrPpc64.add("g2le");
        arrPpc64.add("g2gp");
        arrPpc64.add("g2legp");
        arrPpc64.add("mpc5200_v10");
        arrPpc64.add("mpc5200_v12");
        arrPpc64.add("mpc52xx");
        arrPpc64.add("mpc5200");
        arrPpc64.add("g2legp1");
        arrPpc64.add("mpc5200_v11");
        arrPpc64.add("mpc5200b_v21");
        arrPpc64.add("mpc5200b");
        arrPpc64.add("mpc5200b_v20");
        arrPpc64.add("g2legp3");
        arrPpc64.add("mpc82xx");
        arrPpc64.add("powerquicc-ii");
        arrPpc64.add("mpc8247");
        arrPpc64.add("mpc8248");
        arrPpc64.add("mpc8270");
        arrPpc64.add("mpc8271");
        arrPpc64.add("mpc8272");
        arrPpc64.add("mpc8275");
        arrPpc64.add("mpc8280");
        arrPpc64.add("e200z5");
        arrPpc64.add("e200z6");
        arrPpc64.add("e200");
        arrPpc64.add("g2ls");
        arrPpc64.add("g2lels");



        // m68k cpus
		ArrayList<String> arrM68k = new ArrayList<String>();
		arrM68k.add("Default");
		arrM68k.add("cfv4e");
		arrM68k.add("m5206");
		arrM68k.add("m5208");
		arrM68k.add("m68000");
		arrM68k.add("m68020");
		arrM68k.add("m68030");
		arrM68k.add("m68040");
		arrM68k.add("m68060");
		arrM68k.add("any");

		// Sparc
		ArrayList<String> arrSparc = new ArrayList<String>();
		arrSparc.add("Default");

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
			if (currMachine.arch.equals("x86")) {
				arrList.addAll(arrX86);
			} else if (currMachine.arch.equals("x64")) {
				arrList.addAll(arrX86_64);
			} else if (currMachine.arch.equals("ARM")) {
				arrList.addAll(arrARM);
			} else if (currMachine.arch.equals("MIPS")) {
				arrList.addAll(arrMips);
			} else if (currMachine.arch.equals("PPC")) {
				arrList.addAll(arrPpc);
            } else if (currMachine.arch.equals("PPC64")) {
                arrList.addAll(arrPpc64);
			} else if (currMachine.arch.equals("m68k")) {
				arrList.addAll(arrM68k);
			} else if (currMachine.arch.equals("SPARC")) {
				arrList.addAll(arrSparc);
			}
		} else {
			arrList.addAll(arrX86);
		}

        arrList.add("host");

		if (cpuAdapter == null) {
			cpuAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
			cpuAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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

		String[] arraySpinner = {};

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));

		if (Config.enable_X86)
			arrList.add("x86");
		if (Config.enable_X86_64)
			arrList.add("x64");

		if (Config.enable_ARM)
			arrList.add("ARM");

		if (Config.enable_MIPS)
			arrList.add("MIPS");

		if (Config.enable_PPC)
			arrList.add("PPC");

        if (Config.enable_PPC)
            arrList.add("PPC64");

		if (Config.enable_m68k)
			arrList.add("m68k");

		if (Config.enable_sparc)
			arrList.add("SPARC");

		archAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);

		archAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mArch.setAdapter(archAdapter);

		this.mArch.invalidate();

	}

	private void populateMachineType(String machineType) {

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
			if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {
				arrList.add("pc");
				arrList.add("pc-i440fx-2.9");
				arrList.add("pc-i440fx-2.8");
				arrList.add("pc-i440fx-2.7");
				arrList.add("pc-i440fx-2.6");
				arrList.add("pc-i440fx-2.5");
				arrList.add("pc-i440fx-2.4");
				arrList.add("pc-i440fx-2.3");
				arrList.add("pc-i440fx-2.2");
				arrList.add("pc-i440fx-2.1");
				arrList.add("pc-i440fx-2.0");
				arrList.add("pc-i440fx-1.7");
				arrList.add("pc-i440fx-1.6");
				arrList.add("pc-i440fx-1.5");
				arrList.add("pc-i440fx-1.4");
				arrList.add("pc-1.3");
				arrList.add("pc-1.2");
				arrList.add("pc-1.1");
				arrList.add("pc-1.0");
				arrList.add("pc-0.15");
				arrList.add("pc-0.14");
				arrList.add("pc-0.13");
				arrList.add("pc-0.12");
				arrList.add("pc-0.11");
				arrList.add("pc-0.10");
				arrList.add("q35");
				arrList.add("pc-q35-2.9");
				arrList.add("pc-q35-2.8");
				arrList.add("pc-q35-2.7");
				arrList.add("pc-q35-2.6");
				arrList.add("pc-q35-2.5");
				arrList.add("pc-q35-2.4");
				arrList.add("isapc");
				arrList.add("none");

			} else if (currMachine.arch.equals("ARM")) {
				arrList.add("akita");
				arrList.add("ast2500-evb");
				arrList.add("borzoi");
				arrList.add("canon-a1100");
				arrList.add("cheetah");
				arrList.add("collie");
				arrList.add("connex");
				arrList.add("cubieboard");
				arrList.add("highbank");
				arrList.add("imx25-pdk");
				arrList.add("integratorcp");
				arrList.add("kzm");
				arrList.add("lm3s6965evb");
				arrList.add("lm3s811evb");
				arrList.add("mainstone");
				arrList.add("midway");
				arrList.add("musicpal");
				arrList.add("n800");
				arrList.add("n810");
				arrList.add("netduino2");
				arrList.add("none");
				arrList.add("nuri");
				arrList.add("palmetto-bmc");
				arrList.add("raspi2");
				arrList.add("realview-eb");
				arrList.add("realview-eb-mpcore");
				arrList.add("realview-pb-a8");
				arrList.add("realview-pbx-a9");
				arrList.add("romulus-bmc");
				arrList.add("sabrelite");
				arrList.add("smdkc210");
				arrList.add("spitz");
				arrList.add("sx1");
				arrList.add("sx1-v1");
				arrList.add("terrier");
				arrList.add("tosa");
				arrList.add("verdex");
				arrList.add("versatileab");
				arrList.add("versatilepb");
				arrList.add("vexpress-a15");
				arrList.add("vexpress-a9");
				arrList.add("virt-2.6");
				arrList.add("virt-2.7");
				arrList.add("virt-2.8");
				arrList.add("virt");
				arrList.add("virt-2.9");
				arrList.add("xilinx-zynq-a9");
				arrList.add("z2");

			} else if (currMachine.arch.equals("MIPS")) {
				arrList.add("malta");
				arrList.add("mips");
				arrList.add("mipssim");
				arrList.add("none");
			} else if (currMachine.arch.equals("PPC")) {
				arrList.add("Default");
				arrList.add("40p");
				arrList.add("bamboo");
				arrList.add("g3beige");
				arrList.add("mac99");
				arrList.add("mpc8544ds");
				arrList.add("none");
				arrList.add("ppce500");
				arrList.add("prep");
				arrList.add("ref405ep");
				arrList.add("taihu");
				arrList.add("virtex-ml507");
            } else if (currMachine.arch.equals("PPC64")) {
                arrList.add("Default");
                arrList.add("40p");
                arrList.add("bamboo");
                arrList.add("g3beige");
                arrList.add("mac99");
                arrList.add("mpc8544ds");
                arrList.add("none");
                arrList.add("powernv");
                arrList.add("ppce500");
                arrList.add("prep");
                arrList.add("pseries-2.1");
                arrList.add("pseries-2.10");
                arrList.add("pseries");
                arrList.add("pseries-2.11");
                arrList.add("pseries-2.2");
                arrList.add("pseries-2.3");
                arrList.add("pseries-2.4");
                arrList.add("pseries-2.5");
                arrList.add("pseries-2.6");
                arrList.add("pseries-2.7");
                arrList.add("pseries-2.8");
                arrList.add("pseries-2.9");
                arrList.add("ref405ep");
                arrList.add("taihu");
                arrList.add("virtex-ml507");




			} else if (currMachine.arch.equals("m68k")) {
				arrList.add("Default");
				arrList.add("an5206"); // Arnewsh 5206
				arrList.add("mcf5208evb"); // MCF5206EVB (default0
				arrList.add("none");
			} else if (currMachine.arch.equals("SPARC")) {
				arrList.add("Default");
				arrList.add("LX"); // Arnewsh 5206
				arrList.add("SPARCClassic"); // MCF5206EVB (default0
				arrList.add("SS-10");
				arrList.add("SS-20");
				arrList.add("SS-4");
				arrList.add("SS-5");
				arrList.add("SS-600MP");
				arrList.add("Voyager");
				arrList.add("leon3_generic");

			}

		}
		// else {
		// arrList.add("pc");
		// arrList.add("q35");
		// arrList.add("isapc");
		// }

		if (machineTypeAdapter == null) {
			machineTypeAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
			machineTypeAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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
		} else {
			this.mMachineType.setSelection(0);
		}

	}

	private void populateUI() {

		String[] arraySpinner = { "VNC" };

		ArrayList<String> arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
		if (Config.enable_SDL)
			arrList.add("SDL");
		if (Config.enable_SPICE)
			arrList.add("SPICE");

		uiAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
		uiAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mUI.setAdapter(uiAdapter);
		this.mUI.invalidate();
	}

	private void populateKernel() {

		Thread thread = new Thread(new Runnable() {
			public void run() {

				ArrayList<String> kernels = FavOpenHelper.getInstance(activity).getFavURL("kernel");
				int length = 0;
				if (kernels == null || kernels.size() == 0) {
					length = 0;
				} else {
					length = kernels.size();
				}

				final ArrayList<String> arraySpinner = new ArrayList<String>();
				arraySpinner.add("None");
				arraySpinner.add("Open");
				Iterator<String> i = kernels.iterator();
				while (i.hasNext()) {
					String file = (String) i.next();
					if (file != null) {
						arraySpinner.add(file);
					}
				}

				new Handler(Looper.getMainLooper()).post(new Runnable() {
					public void run() {
						kernelAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
						kernelAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
						mKernel.setAdapter(kernelAdapter);
						mKernel.invalidate();
					}
				});

			}
		});
		thread.setPriority(Thread.MIN_PRIORITY);

		thread.start();

	}

	private void populateInitrd() {

		Thread thread = new Thread(new Runnable() {
			public void run() {

				ArrayList<String> initrds = FavOpenHelper.getInstance(activity).getFavURL("initrd");
				int length = 0;
				if (initrds == null || initrds.size() == 0) {
					length = 0;
				} else {
					length = initrds.size();
				}

				final ArrayList<String> arraySpinner = new ArrayList<String>();
				arraySpinner.add("None");
				arraySpinner.add("Open");
				Iterator<String> i = initrds.iterator();
				while (i.hasNext()) {
					String file = (String) i.next();
					if (file != null) {
						arraySpinner.add(file);
					}
				}

				new Handler(Looper.getMainLooper()).post(new Runnable() {
					public void run() {
						// Code here will run in UI thread
						initrdAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
						initrdAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
						mInitrd.setAdapter(initrdAdapter);
						mInitrd.invalidate();
					}
				});
			}
		});
		thread.setPriority(Thread.MIN_PRIORITY);
		thread.start();

	}

	// Set Hard Disk
	private void populateHD(String fileType) {
		// Add from History
		ArrayList<String> oldHDs = FavOpenHelper.getInstance(activity).getFavURL(fileType);
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
			oldSnapshots = MachineOpenHelper.getInstance(activity).getSnapshots(currMachine);
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

		snapshotAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		snapshotAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
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

		// Add from History
		ArrayList<String> oldCDs = FavOpenHelper.getInstance(activity).getFavURL(fileType);
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
		ArrayList<String> oldFDs = FavOpenHelper.getInstance(activity).getFavURL(fileType);
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
		ArrayList<String> oldSDs = FavOpenHelper.getInstance(activity).getFavURL(fileType);
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

	private void populateSharedFolder() {
		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		//arraySpinner.add(Config.sharedFolder + " (read-only)");
		arraySpinner.add(Config.sharedFolder + " (read-write)");  //Hard Disks are always Read/Write

		sharedFolderAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
		sharedFolderAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		this.mSharedFolder.setAdapter(sharedFolderAdapter);
		this.mSharedFolder.invalidate();

	}

	public void browse(String fileType) {
		// Check if SD card is mounted
		String state = Environment.getExternalStorageState();
		if (!Environment.MEDIA_MOUNTED.equals(state)) {
			UIUtils.toastShort(this, "Error: SD card is not mounted");
			return;
		}

		if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP || !Config.enableExternalSD) {

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

		int res = FavOpenHelper.getInstance(activity).getFavUrlSeq(file, type);
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
			if (file != null && !file.equals(""))
				FavOpenHelper.getInstance(activity).insertFavURL(file, type);
		}

	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		menu.clear();
		return this.setupMenu(menu);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.clear();
		return this.setupMenu(menu);
	}

	public boolean setupMenu(Menu menu) {

		menu.add(0, HELP, 0, "Help").setIcon(android.R.drawable.ic_menu_help);
		menu.add(0, INSTALL, 0, "Install Roms").setIcon(R.drawable.install);
		menu.add(0, CREATE, 0, "Create machine").setIcon(R.drawable.cpu);
		menu.add(0, DELETE, 0, "Delete Machine").setIcon(R.drawable.delete);
		menu.add(0, ISOSIMAGES, 0, "ISOs & HD Images").setIcon(R.drawable.cdrom);
		menu.add(0, VIEWLOG, 0, "View Log").setIcon(android.R.drawable.ic_menu_view);
		menu.add(0, EXPORT, 0, "Export Machines").setIcon(R.drawable.exportvms);
		menu.add(0, IMPORT, 0, "Import Machines").setIcon(R.drawable.importvms);
		menu.add(0, HELP, 0, "Help").setIcon(R.drawable.help);
		menu.add(0, CHANGELOG, 0, "Changelog").setIcon(android.R.drawable.ic_menu_help);
		menu.add(0, LICENSE, 0, "License").setIcon(android.R.drawable.ic_menu_help);
		menu.add(0, QUIT, 0, "Exit").setIcon(android.R.drawable.ic_lock_power_off);

		for (int i = 0; i < menu.size() && i < 1; i++) {
			MenuItemCompat.setShowAsAction(menu.getItem(i), MenuItemCompat.SHOW_AS_ACTION_IF_ROOM);
		}

		return true;

	}

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {

		super.onOptionsItemSelected(item);
		if (item.getItemId() == this.INSTALL) {
			this.install();
		} else if (item.getItemId() == this.DELETE) {
			this.onDeleteMachine();
		} else if (item.getItemId() == this.CREATE) {
			this.promptMachineName(this);
		} else if (item.getItemId() == this.ISOSIMAGES) {
			this.goToURL(Config.isosImagesURL);
		} else if (item.getItemId() == this.EXPORT) {
			this.onExportMachines();
		} else if (item.getItemId() == this.IMPORT) {
			this.onImportMachines();
		} else if (item.getItemId() == this.HELP) {
			UIUtils.onHelp(this);
		} else if (item.getItemId() == this.VIEWLOG) {
			this.onViewLog();
		} else if (item.getItemId() == this.CHANGELOG) {
			this.onChangeLog();
		} else if (item.getItemId() == this.LICENSE) {
			this.onLicense();
		} else if (item.getItemId() == this.QUIT) {
			this.exit();
		}
		return true;
	}

	public void onViewLog() {
        FileUtils.viewLimboLog(this);
    }

	private void goToURL(String url) {

		Intent i = new Intent(Intent.ACTION_VIEW);
		i.setData(Uri.parse(url));
		activity.startActivity(i);

	}



	public void stopVM(boolean exit) {
		if (vmexecutor == null && !exit) {
			if (this.currMachine != null && this.currMachine.paused == 1) {
				new AlertDialog.Builder(this).setTitle("Discard VM State")
						.setMessage("The VM is Paused. If you discard the state you might lose data. Continue?")
						.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								currMachine.paused = 0;
								MachineOpenHelper.getInstance(activity).update(currMachine,
										MachineOpenHelper.getInstance(activity).PAUSED, 0 + "");
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
		int ret = MachineOpenHelper.getInstance(activity).deleteMachine(currMachine);
		ret = MachineOpenHelper.getInstance(activity).insertMachine(currMachine);
		if (this.snapshotAdapter.getPosition(snapshot_name) < 0) {
			this.snapshotAdapter.add(snapshot_name);
		}
	}

	public void saveStateVMDB() {
		MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.getInstance(activity).PAUSED,
				1 + "");
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

    private static class Installer extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {
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

	public class AutoScrollView extends ScrollView {

		public AutoScrollView(Context context, AttributeSet attrs) {
			super(context, attrs);
		}

		public AutoScrollView(Context context) {
			super(context);
		}
	}

	private class ExportMachines extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {

			// Export
			String machinesToExport = MachineOpenHelper.getInstance(activity).exportMachines();
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
				if (MachineOpenHelper.getInstance(activity).getMachine(machine.machinename, "") != null) {
					MachineOpenHelper.getInstance(activity).deleteMachine(machine);
				}
				MachineOpenHelper.getInstance(activity).insertMachine(machine);
				addDriveToList(machine.cd_iso_path, "cd");
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

}
