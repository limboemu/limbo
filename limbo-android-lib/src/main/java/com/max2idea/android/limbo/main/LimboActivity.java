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
import android.androidVNC.RfbProto;
import android.androidVNC.VncCanvas;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.net.Uri;
import android.net.wifi.WifiManager.WifiLock;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.PowerManager.WakeLock;
import android.os.StrictMode;
import android.support.v4.view.MenuItemCompat;
import android.support.v4.widget.NestedScrollView;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.InputType;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
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

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.jni.VMExecutor;
import com.max2idea.android.limbo.utils.FavOpenHelper;
import com.max2idea.android.limbo.utils.FileInstaller;
import com.max2idea.android.limbo.utils.FileManager;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.LinksManager;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.OSDialogBox;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;
import com.max2idea.android.limbo.utils.UIUtils.LimboFileSpinnerAdapter;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

public class LimboActivity extends AppCompatActivity {

    public static final String TAG = "LIMBO";

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
    private static final int DISCARD_VM_STATE = 11;
    private static final int ENABLE_FILEMANAGER = 12;
    private static final int SETTINGS = 13;

    public static VMStatus currStatus = VMStatus.Ready;
    public static boolean vmStarted = false;
    public static VMExecutor vmexecutor;
    public static Machine currMachine = null;
    private static LimboActivity activity = null;
    private static String vnc_passwd = null;
    private static int vnc_allow_external = 0;
    private static int qmp_allow_external = 0;
    public ProgressDialog progDialog;
    public View parent;
    private InstallerTask installerTaskTask;
    private boolean machineLoaded;
    private boolean timeQuit = false;
    private Object lockTime = new Object();

    //Widgets
    private ImageView mStatus;
    private EditText mDNS;
    private EditText mHOSTFWD;
    private EditText mAppend;
    private EditText mExtraParams;
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
    private Spinner mNicCard;
    private Spinner mNetConfig;
    private Spinner mVGAConfig;
    private Spinner mSoundCard;
    private Spinner mHDCacheConfig;
    private Spinner mUI;
    private CheckBox mDisableACPI;
    private CheckBox mDisableHPET;
    private CheckBox mDisableTSC;

    //TODO:
    // private CheckBox mSnapshot;

    private CheckBox mVNCAllowExternal;
    private CheckBox mQMPAllowExternal;
    private CheckBox mPrio;
    private CheckBox mEnableKVM;
    private CheckBox mEnableMTTCG;

    private CheckBox mToolBar;
    private CheckBox mFullScreen;
    private Spinner mSnapshot;
    private Spinner mOrientation;
    private Spinner mKeyboard;
    private Spinner mMouse;
    private ImageButton mStart;
    private ImageButton mStop;
    private ImageButton mRestart;
    private ImageButton mSave;

    private LinearLayout mCPUSectionDetails;
    private LinearLayout mCPUSectionHeader;
    private LinearLayout mStorageSectionDetails;
    private LinearLayout mStorageSectionHeader;
    private LinearLayout mUserInterfaceSectionDetails;
    private LinearLayout mUserInterfaceSectionHeader;
    private LinearLayout mAdvancedSectionDetails;
    private LinearLayout mAdvancedSectionHeader;
    private View mBootSectionHeader;
    private LinearLayout mBootSectionDetails;
    private LinearLayout mGraphicsSectionDetails;
    private LinearLayout mGraphicsSectionHeader;
    private LinearLayout mRemovableStorageSectionDetails;
    private LinearLayout mRemovableStorageSectionHeader;
    private LinearLayout mNetworkSectionDetails;
    private View mNetworkSectionHeader;
    private LinearLayout mAudioSectionDetails;
    private LinearLayout mAudioSectionHeader;
    private ConnectionBean selected;
    private FileType filetype;
    private TextView mUISectionSummary;
    private TextView mCPUSectionSummary;
    private TextView mStorageSectionSummary;
    private TextView mRemovableStorageSectionSummary;
    private TextView mGraphicsSectionSummary;
    private TextView mAudioSectionSummary;
    private TextView mNetworkSectionSummary;
    private TextView mBootSectionSummary;
    private TextView mAdvancedSectionSummary;
    private CheckBox mDesktopMode;
    private FavOpenHelper favinstance;
    private NestedScrollView mScrollView;
    private boolean libLoaded;
    private OnClickListener resetClickListener;
    private Hashtable<FileType, DiskInfo> diskMapping = new Hashtable<>();
    private String fixMouseDescr = " (Fixes Mouse)";
    private boolean firstMTTCGCheck;

    public static void quit() {
        activity.finish();
    }

    static private void onInstall(boolean force) {
        FileInstaller.installFiles(activity, force);
    }

    public static String getVnc_passwd() {
        return LimboActivity.vnc_passwd;
    }

    public static void setVnc_passwd(String vnc_passwd) {
        LimboActivity.vnc_passwd = vnc_passwd;
    }

    public static String getLocalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
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
        QmpClient.allow_external = (qmp_allow_external == 1);
        vmexecutor.qmp_allow_external = qmp_allow_external;

        if (UI == Config.UI_VNC) {
            // disable sound card with VNC
            vmexecutor.enablevnc = 1;
            vmexecutor.enablespice = 0;
            vmexecutor.sound_card = null;
            vmexecutor.vnc_allow_external = vnc_allow_external;
            RfbProto.allow_external = (vnc_allow_external == 1);
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

    public static LimboActivity getInstance() {
        return activity;
    }

    public static void cleanup() {

        if (getInstance() != null && getInstance().mMachine != null) {
            vmStarted = false;

            //XXX flush and close all file descriptors if we haven't already
            FileUtils.close_fds();

            try {
                MachineOpenHelper.getInstance(activity).close();
            } catch (Exception ex) {
                Log.e(TAG, "Could not close machine db: " + ex);
            }


            try {
                FavOpenHelper.getInstance(activity).close();
            } catch (Exception ex) {
                Log.e(TAG, "Could not close fav db: " + ex);
            }


            ////XXX; we wait till fds flush and close
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            //set the exit code
            LimboSettingsManager.setExitCode(activity, 1);

            //XXX: SDL seems to lock the keyboard events
            // unless we finish the starting activity
            activity.finish();

            Log.v(TAG, "Exit");
            //XXX: We exit here to force unload the native libs
            System.exit(0);

        }
    }

    public static void saveStateVMDB() {
        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.PAUSED,
                1 + "");
    }

    public void changeStatus(final VMStatus status_changed) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (status_changed == VMStatus.Running) {
                    mStatus.setImageResource(R.drawable.on);
                    mStatusText.setText("Running");
                    unlockRemovableDevices(false);
                    enableRemovableDiskValues(true);
                    enableNonRemovableDeviceOptions(false);
                    vmStarted = true;
                } else if (status_changed == VMStatus.Ready || status_changed == VMStatus.Stopped) {
                    mStatus.setImageResource(R.drawable.off);
                    mStatusText.setText("Stopped");
                    unlockRemovableDevices(true);
                    enableRemovableDiskValues(true);
                    enableNonRemovableDeviceOptions(true);
                } else if (status_changed == VMStatus.Saving) {
                    mStatus.setImageResource(R.drawable.on);
                    mStatusText.setText("Saving State");
                    unlockRemovableDevices(false);
                    enableRemovableDiskValues(false);
                    enableNonRemovableDeviceOptions(false);
                } else if (status_changed == VMStatus.Paused) {
                    mStatus.setImageResource(R.drawable.on);
                    mStatusText.setText("Paused");
                    unlockRemovableDevices(false);
                    enableRemovableDiskValues(false);
                    enableNonRemovableDeviceOptions(false);
                }
            }
        });

    }

    private void install(boolean force) {
        progDialog = ProgressDialog.show(activity, "Please Wait", "Installing BIOS...", true);
        installerTaskTask = new InstallerTask();
        installerTaskTask.force = force;
        installerTaskTask.execute();
    }

    public void UIAlertLicense(String title, String body, final Activity activity) {

        AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);

        TextView textView = new TextView(activity);
        textView.setText(body);
        textView.setTextSize(10);
        textView.setPadding(20, 20, 20, 20);

        ScrollView scrollView = new ScrollView(activity);
        scrollView.addView(textView);

        alertDialog.setView(scrollView);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "I Acknowledge", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                if (LimboSettingsManager.isFirstLaunch(activity)) {
                    install(true);
                    UIUtils.onHelp(activity);
                    UIUtils.onChangeLog(activity);
                }
                LimboSettingsManager.setFirstLaunch(activity);
                return;
            }
        });
        alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                if (LimboSettingsManager.isFirstLaunch(activity)) {
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

    private void onTap() {
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
            UIUtils.UIAlert(this,"TAP - User Id: " + userid,
                    "Your device doesn't support TAP, use \"User\" network mode instead ");
            return;
        }


        DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                updateSummary(false);
            }
        };


        DialogInterface.OnClickListener helpListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        goToURL(Config.faqLink);
                        return;
                    }
                };

        UIUtils.UIAlert(activity,
                "TAP Device found",
                "Warning! Make sure device /dev/net/tun has appropriate permissions\nUser ID: " + userid
                        +"/n",
                16, false, "OK", okListener,
                null, null, "TAP Help", helpListener);
    }


    private void onNetworkUser() {
        ApplicationInfo pInfo = null;

        DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                updateSummary(false);
            }
        };

        DialogInterface.OnClickListener helpListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        goToURL(Config.faqLink);
                        return;
                    }
                };

        UIUtils.UIAlert(activity,
                "Network",
                "Warning! Enabling external network for images you don't trust or containing old OSes is not recommended. " +
                        "If not use network \"None\" before running the virtual machine.\n",
                16, false, "OK", okListener,
                null, null, "FAQ", helpListener);
    }

    public void setUserPressed(boolean pressed) {

        if (pressed) {
            enableListeners();
            enableRemovableDiskListeners();
        } else {
            disableListeners();
            disableRemovableDiskListeners();
        }
    }

    private void disableRemovableDiskListeners() {

        mCDenable.setOnCheckedChangeListener(null);
        mFDAenable.setOnCheckedChangeListener(null);
        mFDBenable.setOnCheckedChangeListener(null);
        mSDenable.setOnCheckedChangeListener(null);
        mCD.setOnItemSelectedListener(null);
        mFDA.setOnItemSelectedListener(null);
        mFDB.setOnItemSelectedListener(null);
        mSD.setOnItemSelectedListener(null);
    }

    private void enableRemovableDiskListeners() {
        mCD.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String cd = (String) ((ArrayAdapter<?>) mCD.getAdapter()).getItem(position);
                if (
                        position == 0 && mCDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM, "");
                    currMachine.cd_iso_path = "";
                } else if (
                        (position == 0 || !mCDenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
                            null);
                    currMachine.cd_iso_path = null;
                } else if (
                        position == 1 && mCDenable.isChecked()) {
                    filetype = FileType.CD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mCD.setSelection(0);
                } else if (
                        position > 1 && mCDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM, cd);
                    currMachine.cd_iso_path = cd;
                }
                if (
                        currStatus == VMStatus.Running && position > 1 && mCDenable.isChecked()) {
                    mCD.setEnabled(false);
                    vmexecutor.change_dev("ide1-cd0", currMachine.cd_iso_path);
                    mCD.setEnabled(true);
                } else if (
                        mCDenable.isChecked() &&
                                currStatus == VMStatus.Running && position == 0) {
                    mCD.setEnabled(false);
                    vmexecutor.change_dev("ide1-cd0", null); // Eject
                    mCD.setEnabled(true);
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mFDA.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String fda = (String) ((ArrayAdapter<?>) mFDA.getAdapter()).getItem(position);
                if (
                        position == 0 && mFDAenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, "");
                    currMachine.fda_img_path = "";
                } else if (
                        (position == 0 || !mFDAenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, null);
                    currMachine.fda_img_path = null;
                } else if (
                        position == 1 && mFDAenable.isChecked()) {
                    filetype = FileType.FDA;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mFDA.setSelection(0);
                } else if (
                        position > 1 && mFDAenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA, fda);
                    currMachine.fda_img_path = fda;
                }
                if (
                        currStatus == VMStatus.Running && position > 1 && mFDAenable.isChecked()) {
                    mFDA.setEnabled(false);
                    vmexecutor.change_dev("floppy0", currMachine.fda_img_path);
                    mFDA.setEnabled(true);
                } else if (
                        currStatus == VMStatus.Running && position == 0 && mFDAenable.isChecked()) {
                    mFDA.setEnabled(false);
                    vmexecutor.change_dev("floppy0", null); // Eject
                    mFDA.setEnabled(true);
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mFDB.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String fdb = (String) ((ArrayAdapter<?>) mFDB.getAdapter()).getItem(position);
                if (
                        position == 0 && mFDBenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, "");
                    currMachine.fdb_img_path = "";
                } else if (
                        (position == 0 || !mFDBenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, null);
                    currMachine.fdb_img_path = null;
                } else if (
                        position == 1 && mFDBenable.isChecked()) {
                    filetype = FileType.FDB;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mFDB.setSelection(0);
                } else if (
                        position > 1 && mFDBenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB, fdb);
                    currMachine.fdb_img_path = fdb;
                    // TODO: If Machine is running eject and set floppy img
                }
                if (
                        currStatus == VMStatus.Running && position > 1 && mFDBenable.isChecked()) {
                    mFDB.setEnabled(false);
                    vmexecutor.change_dev("floppy1", currMachine.fdb_img_path);
                    mFDB.setEnabled(true);
                } else if (
                        currStatus == VMStatus.Running && position == 0 && mFDBenable.isChecked()) {
                    mFDB.setEnabled(false);
                    vmexecutor.change_dev("floppy1", null); // Eject
                    mFDB.setEnabled(true);
                }
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mSD.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String sd = (String) ((ArrayAdapter<?>) mSD.getAdapter()).getItem(position);
                if (
                        position == 0 && mSDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, "");
                    currMachine.sd_img_path = "";
                } else if (
                        (position == 0 || !mSDenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, null);
                    currMachine.sd_img_path = null;
                } else if (
                        position == 1 && mSDenable.isChecked()) {
                    filetype = FileType.SD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mSD.setSelection(0);
                } else if (
                        position > 1 && mSDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, sd);
                    currMachine.sd_img_path = sd;
                    // TODO: If Machine is running eject and set floppy img
                }
                if (
                        currStatus == VMStatus.Running && position > 1 && mSDenable.isChecked()) {
                } else if (
                        currStatus == VMStatus.Running && position == 0 && mSDenable.isChecked()) {
                }
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mCDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
                                                 public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                                                     if(currMachine == null)
                                                         return;

                                                     mCD.setEnabled(isChecked);

                                                         currMachine.enableCDROM = isChecked;
                                                         if (isChecked) {
                                                             currMachine.cd_iso_path = "";
                                                             MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
                                                                     "");
                                                             mHDCenable.setChecked(false);
                                                         } else {
                                                             currMachine.cd_iso_path = null;
                                                             MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CDROM,
                                                                     null);
                                                         }

                                                     triggerUpdateSpinner(mCD);
                                                     updateSummary(false);
                                                 }

                                             }

        );
        mFDAenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mFDA.setEnabled(isChecked);

                    currMachine.enableFDA = isChecked;
                    if (isChecked) {
                        currMachine.fda_img_path = "";
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA,
                                "");
                    } else {
                        currMachine.fda_img_path = null;
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDA,
                                null);
                    }

                triggerUpdateSpinner(mFDA);
                updateSummary(false);
            }

        });
        mFDBenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mFDB.setEnabled(isChecked);

                    currMachine.enableFDB = isChecked;
                    if (isChecked) {
                        currMachine.fdb_img_path = "";
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB,
                                "");
                    } else {
                        currMachine.fdb_img_path = null;
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.FDB,
                                null);
                    }
                triggerUpdateSpinner(mFDB);
                updateSummary(false);
            }

        });
        mSDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                mSD.setEnabled(isChecked);

                    currMachine.enableSD = isChecked;
                    if (isChecked) {
                        currMachine.sd_img_path = "";
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD, "");
                    } else {
                        currMachine.sd_img_path = null;
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SD,
                                null);
                    }

                triggerUpdateSpinner(mSD);
                updateSummary(false);
            }

        });

    }

    private void enableListeners() {


        mArch.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                // your code here
                if(currMachine == null)
                    return;

                String arch = (String) ((ArrayAdapter<?>) mArch.getAdapter()).getItem(position);

                currMachine.arch = arch;
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.ARCH, arch);

                if (currMachine.arch.equals("ARM") || currMachine.arch.equals("ARM64")) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null)
                            currMachine.machine_type = "versatilepb";
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null)
                            currMachine.cpu = "Default";
                        populateCPUs(currMachine.cpu);
                        if(currMachine.nic_card==null)
                            currMachine.nic_card = "Default";
                        populateNetDevices(currMachine.nic_card);
                    }

                } else if (currMachine.arch.equals("MIPS")) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null)
                            currMachine.machine_type = "malta";
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null)
                            currMachine.cpu = "Default";
                        populateCPUs(currMachine.cpu);

                    }

                } else if (currMachine.arch.equals("PPC") || currMachine.arch.equals("PPC64")) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null) {
                            if(currMachine.arch.equals("PPC"))
                                currMachine.machine_type = "g3beige";
                            else if (currMachine.arch.equals("PPC64")){
                                currMachine.machine_type = "Default";
                            }
                        }
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null)
                            currMachine.cpu = "Default";
                        populateCPUs(currMachine.cpu);
                        if(currMachine.nic_card==null)
                            currMachine.nic_card = "Default";
                        populateNetDevices(currMachine.nic_card);

                    }

                } else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null)
                            currMachine.machine_type = "pc";
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null) {
                            if(currMachine.arch.equals("x86"))
                                currMachine.cpu = "n270";
                            else  if(currMachine.arch.equals("x64"))
                                currMachine.cpu = "phenom";
                        }
                        populateCPUs(currMachine.cpu);
                        if(currMachine.nic_card==null)
                            currMachine.nic_card = "Default";
                        populateNetDevices(currMachine.nic_card);
                    }

                } else if (currMachine.arch.equals("m68k")) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null)
                            currMachine.machine_type = "Default";
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null)
                            currMachine.cpu = "Default";
                        populateCPUs(currMachine.cpu);

                    }

                } else if (currMachine.arch.equals("SPARC") || currMachine.arch.equals("SPARC64") ) {

                    if (!machineLoaded) {
                        if(currMachine.machine_type==null)
                            currMachine.machine_type = "Default";
                        populateMachineType(currMachine.machine_type);
                        if(currMachine.cpu==null)
                            currMachine.cpu = "Default";
                        populateCPUs(currMachine.cpu);
                        if(currMachine.nic_card==null)
                            currMachine.nic_card = "Default";
                        populateNetDevices(currMachine.nic_card);

                    }

                }

                    if (currStatus == VMStatus.Running
                            || currMachine.arch.equals("ARM")
                            || currMachine.arch.equals("ARM64")
                            || currMachine.arch.equals("MIPS")
                            || currMachine.arch.equals("m68k") || currMachine.arch.equals("PPC")
                            || currMachine.arch.equals("SPARC") || currMachine.arch.equals("SPARC64") ) {
                        mDisableACPI.setEnabled(false);
                        mDisableHPET.setEnabled(false);
                        mDisableTSC.setEnabled(false);

                    } else if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {

                        mDisableACPI.setEnabled(true);
                        mDisableHPET.setEnabled(true);
                        mDisableTSC.setEnabled(true);

                    }

                updateSummary(false);

                machineLoaded = false;
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }

        });

        mSnapshot.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

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
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });



        mCPU.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String cpu = (String) ((ArrayAdapter<?>) mCPU.getAdapter()).getItem(position);


                currMachine.cpu = cpu;
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPU, cpu);

                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mMachineType.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String machineType = (String) ((ArrayAdapter<?>) mMachineType.getAdapter()).getItem(position);
                currMachine.machine_type = machineType;
                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.MACHINE_TYPE, machineType);

                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mUI.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String ui = (String) ((ArrayAdapter<?>) mUI.getAdapter()).getItem(position);
                currMachine.ui = ui;
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.UI, ui);

                if (position == 0) {
                    mVNCAllowExternal.setEnabled(true);
                    if (mSnapshot.getSelectedItemPosition() == 0)
                        mSoundCard.setEnabled(false);
                } else {
                    mVNCAllowExternal.setEnabled(false);
                    if (mSnapshot.getSelectedItemPosition() == 0) {
                        if (Config.enable_SDL_sound)
                            mSoundCard.setEnabled(true);
                    }
                }
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mCPUNum.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                final String cpuNum = (String) ((ArrayAdapter<?>) mCPUNum.getAdapter()).getItem(position);

                if (position>0 && currMachine.enableMTTCG!=1 && currMachine.enableKVM!=1 && !firstMTTCGCheck) {
                    firstMTTCGCheck = true;
                    DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            currMachine.cpuNum = Integer.parseInt(cpuNum);
                            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPUNUM,
                                    cpuNum);
                            updateSummary(false);
                        }
                    };

                    DialogInterface.OnClickListener cancelListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    mCPUNum.setSelection(0);
                                    return;
                                }
                            };

                    DialogInterface.OnClickListener helpListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    mCPUNum.setSelection(0);
                                    goToURL(Config.faqLink);
                                    return;
                                }
                            };

                    UIUtils.UIAlert(activity,
                            "Multiple vCPUs",
                            "Warning! Setting Multiple Virtual CPUs will NOT result in additional performance unless you use KVM or MTTCG. " +
                                    "Your device might even be slow or the Guest OS might hang so it is advised to use 1 CPU. " +
                                    "\n\n" + ((Config.enable_X86 || Config.enable_X86_64) ?
                                    "If your guest OS is not able to boot check option 'Disable TSC' and restart the VM. ": "")
                                    +"Do you want to continue?",
                            16, false, "OK", okListener, "Cancel", cancelListener, "vCPU Help", helpListener);

                } else {
                    currMachine.cpuNum = Integer.parseInt(cpuNum);
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.CPUNUM,
                            cpuNum);

                }
                if(position>0 && (Config.enable_X86 || Config.enable_X86_64))
                    mDisableTSC.setChecked(true);
                else
                    mDisableTSC.setChecked(false);


                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mRamSize.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String ram = (String) ((ArrayAdapter<?>) mRamSize.getAdapter()).getItem(position);
                    currMachine.memory = Integer.parseInt(ram);
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MEMORY,
                            ram);



                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mKernel.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String kernel = (String) ((ArrayAdapter<?>) mKernel.getAdapter()).getItem(position);
                if (
                        position == 0) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KERNEL,
                            null);
                    currMachine.kernel = null;
                } else if (
                        position == 1) {
                    filetype = FileType.KERNEL;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mKernel.setSelection(0);
                } else if (
                        position > 1) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KERNEL,
                            kernel);
                    currMachine.kernel = kernel;
                    // TODO: If Machine is running eject and set floppy img
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mInitrd.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String initrd = (String) ((ArrayAdapter<?>) mInitrd.getAdapter()).getItem(position);
                if (
                        position == 0) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.INITRD,
                            null);
                    currMachine.initrd = null;
                } else if (
                        position == 1) {
                    filetype = FileType.INITRD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mInitrd.setSelection(0);
                } else if (
                        position > 1) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.INITRD,
                            initrd);
                    currMachine.initrd = initrd;

                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mHDA.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

                if(currMachine == null)
                    return;

                String hda = (String) ((ArrayAdapter<?>) mHDA.getAdapter()).getItem(position);
                if (
                        position == 0 && mHDAenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, "");
                    currMachine.hda_img_path = "";
                } else if (
                        (position == 0 || !mHDAenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, null);
                    currMachine.hda_img_path = null;
                } else if (
                        position == 1 && mHDAenable.isChecked()) {
                    promptImageName(activity, FileType.HDA);
                    mHDA.setSelection(0);
                } else if (
                        position == 2 && mHDAenable.isChecked()) {
                    filetype =FileType.HDA;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mHDA.setSelection(0);
                } else if (
                        position > 2 && mHDAenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA, hda);
                    currMachine.hda_img_path = hda;
                }
                updateSummary(false);


            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mHDB.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String hdb = (String) ((ArrayAdapter<?>) mHDB.getAdapter()).getItem(position);

                if (
                        position == 0 && mHDBenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, "");
                    currMachine.hdb_img_path = "";
                } else if (
                        (position == 0 || !mHDBenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, null);
                    currMachine.hdb_img_path = null;
                } else if (
                        position == 1 && mHDBenable.isChecked()) {
                    promptImageName(activity, FileType.HDB);
                    mHDB.setSelection(0);
                } else if (
                        position == 2 && mHDBenable.isChecked()) {
                    filetype = FileType.HDB;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mHDB.setSelection(0);
                } else if (
                        position > 2 && mHDBenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB, hdb);
                    currMachine.hdb_img_path = hdb;
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mHDC.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;


                String hdc = (String) ((ArrayAdapter<?>) mHDC.getAdapter()).getItem(position);
                if (
                        position == 0 && mHDCenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, "");
                    currMachine.hdc_img_path = "";
                } else if (
                        (position == 0 || !mHDCenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, null);
                    currMachine.hdc_img_path = null;
                } else if (
                        position == 1 && mHDCenable.isChecked()) {
                    promptImageName(activity, FileType.HDC);
                    mHDC.setSelection(0);
                } else if (
                        position == 2 && mHDCenable.isChecked()) {
                    filetype = FileType.HDC;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mHDC.setSelection(0);
                } else if (
                        position > 2 && mHDCenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC, hdc);
                    currMachine.hdc_img_path = hdc;
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mHDD.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String hdd = (String) ((ArrayAdapter<?>) mHDD.getAdapter()).getItem(position);
                if (
                        position == 0 && mHDDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, "");
                    currMachine.hdd_img_path = "";
                } else if (
                        (position == 0 || !mHDDenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, null);
                    currMachine.hdd_img_path = null;
                } else if (
                        position == 1 && mHDDenable.isChecked()) {
                    promptImageName(activity, FileType.HDD);
                    mHDD.setSelection(0);
                } else if (
                        position == 2 && mHDDenable.isChecked()) {
                    filetype = FileType.HDD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mHDD.setSelection(0);
                } else if (
                        position > 2 && mHDDenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD, hdd);
                    currMachine.hdd_img_path = hdd;
                }
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mSharedFolder.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

                if(currMachine == null)
                    return;

                String shared_folder = (String) ((ArrayAdapter<?>) mSharedFolder.getAdapter()).getItem(position);
                if (
                        position == 0 && mSharedFolderenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SHARED_FOLDER, "");
                    currMachine.shared_folder = "";
                } else if (
                        (position == 0 || !mSharedFolderenable.isChecked())) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SHARED_FOLDER, null);
                    currMachine.shared_folder = null;
                } else if (
                        position == 1 && mSharedFolderenable.isChecked()) {
                    filetype = FileType.SHARED_DIR;
                    FileManager.browse(activity, filetype, Config.OPEN_SHARED_DIR_REQUEST_CODE);
                    mSharedFolder.setSelection(0);
                } else if (
                        position > 1 && mSharedFolderenable.isChecked()) {
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SHARED_FOLDER, shared_folder);
                    currMachine.shared_folder = shared_folder;
                }
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });


        mHDAenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mHDA.setEnabled(isChecked);
                    if (isChecked) {
                        currMachine.hda_img_path = "";
                    } else {
                        currMachine.hda_img_path = null;
                    }
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDA,
                            currMachine.hda_img_path);

                triggerUpdateSpinner(mHDA);
                updateSummary(false);
            }

        });
        mHDBenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mHDB.setEnabled(isChecked);
                    if (isChecked) {
                        currMachine.hdb_img_path = "";
                    } else {
                        currMachine.hdb_img_path = null;
                    }
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDB,
                            currMachine.hdb_img_path);

                triggerUpdateSpinner(mHDB);
                updateSummary(false);
            }

        });
        mHDCenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mHDC.setEnabled(isChecked);
                    if (isChecked) {
                        currMachine.hdc_img_path = "";
                        mCDenable.setChecked(false);
                    } else {
                        currMachine.hdc_img_path = null;
                    }
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDC,
                            currMachine.hdc_img_path);

                triggerUpdateSpinner(mHDC);
                updateSummary(false);
            }

        });
        mHDDenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                mHDD.setEnabled(isChecked);
                    if (isChecked) {
                        currMachine.hdd_img_path = "";
                        mSharedFolderenable.setChecked(false);
                    } else {
                        currMachine.hdd_img_path = null;
                    }
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HDD,
                            currMachine.hdd_img_path);

                triggerUpdateSpinner(mHDD);
                updateSummary(false);
            }

        });


        mSharedFolderenable.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                if(isChecked) {
                    promptSharedFolder(activity);
                }
                mSharedFolder.setEnabled(isChecked);
                    if (isChecked) {
                        currMachine.shared_folder = "";
                        mHDDenable.setChecked(false);
                    } else {
                        currMachine.shared_folder = null;
                    }
                    MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.SHARED_FOLDER,
                            currMachine.shared_folder);

                triggerUpdateSpinner(mSharedFolder);
                updateSummary(false);
            }

        });



        mBootDevices.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String bootDev = (String) ((ArrayAdapter<?>) mBootDevices.getAdapter()).getItem(position);
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.BOOT_CONFIG,
                        bootDev);
                currMachine.bootdevice = bootDev;
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        this.mNetConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String netfcg = (String) ((ArrayAdapter<?>) mNetConfig.getAdapter()).getItem(position);
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NET_CONFIG,
                        netfcg);
                currMachine.net_cfg = netfcg;
                if (position > 0 && currMachine.paused == 0
                        && currStatus != VMStatus.Running) {
                    mNicCard.setEnabled(true);
                    mDNS.setEnabled(true);
                    mHOSTFWD.setEnabled(true);
                } else {
                    mNicCard.setEnabled(false);
                    mDNS.setEnabled(false);
                    mHOSTFWD.setEnabled(false);
                }

                ApplicationInfo pInfo = null;

                if (netfcg.equals("TAP")) {
                    onTap();
                } else if (netfcg.equals("User")) {
                    onNetworkUser();
                }

                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        this.mNicCard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                if (position < 0 || position >= mNicCard.getCount()) {
                    mNicCard.setSelection(0);
                    updateSummary(false);
                    return;
                }
                String niccfg = (String) ((ArrayAdapter<?>) mNicCard.getAdapter()).getItem(position);
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.NIC_CONFIG,
                        niccfg);
                currMachine.nic_card = niccfg;

                updateSummary(false);
            }

            public void onNothingSelected(final AdapterView<?> parentView) {

            }
        });

        mVGAConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String vgacfg = (String) ((ArrayAdapter<?>) mVGAConfig.getAdapter()).getItem(position);

                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.VGA,
                        vgacfg);
                currMachine.vga_type = vgacfg;
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        this.mSoundCard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String sndcfg = (String) ((ArrayAdapter<?>) mSoundCard.getAdapter()).getItem(position);

                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.SOUNDCARD_CONFIG, sndcfg);
                currMachine.soundcard = sndcfg;
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mHDCacheConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String hdcfg = (String) ((ArrayAdapter<?>) mHDCacheConfig.getAdapter()).getItem(position);

                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.HDCACHE_CONFIG, hdcfg);
                currMachine.hd_cache = hdcfg;
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mDisableACPI.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.DISABLE_ACPI, ((isChecked ? 1 : 0) + ""));
                currMachine.disableacpi = (isChecked ? 1 : 0);
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mDisableHPET.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.DISABLE_HPET, ((isChecked ? 1 : 0) + ""));
                currMachine.disablehpet = (isChecked ? 1 : 0);
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mDisableTSC.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                MachineOpenHelper.getInstance(activity).update(currMachine,
                        MachineOpenHelper.DISABLE_TSC, ((isChecked ? 1 : 0) + ""));
                currMachine.disabletsc = (isChecked ? 1 : 0);
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mDNS.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if(currMachine == null)
                    return;

                if (!hasFocus) {
                    setDNSServer(mDNS.getText().toString());
                    LimboSettingsManager.setDNSServer(activity, mDNS.getText().toString());
                    updateSummary(false);
                }
            }
        });

        mHOSTFWD.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if(currMachine == null)
                    return;

                if (!hasFocus) {

                        currMachine.hostfwd = mHOSTFWD.getText().toString();
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.HOSTFWD,
                                mHOSTFWD.getText().toString());

                    updateSummary(false);
                }
            }
        });

        mAppend.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if(currMachine == null)
                    return;

                if (!hasFocus) {
                        currMachine.append = mAppend.getText() + "";
                        MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.APPEND,
                                mAppend.getText() + "");

                    updateSummary(false);
                }
            }
        });

        mExtraParams.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if(currMachine == null)
                    return;

                if (!hasFocus) {
                        currMachine.extra_params = mExtraParams.getText() + "";
                        MachineOpenHelper.getInstance(activity).update(currMachine,
                                MachineOpenHelper.EXTRA_PARAMS, mExtraParams.getText() + "");

                    updateSummary(false);
                }
            }
        });


        resetClickListener = new OnClickListener() {
            @Override
            public void onClick(View view) {
                view.setFocusableInTouchMode(true);
                view.setFocusable(true);
            }
        };

        mDNS.setOnClickListener(resetClickListener);
        mAppend.setOnClickListener(resetClickListener);
        mHOSTFWD.setOnClickListener(resetClickListener);
        mExtraParams.setOnClickListener(resetClickListener);


        mVNCAllowExternal.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

                if (isChecked) {
                    promptVNCAllowExternal(activity);
                } else {
                    vnc_passwd = null;
                    vnc_allow_external = 0;
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mQMPAllowExternal.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

                if (isChecked) {
                    promptQMPAllowExternal(activity);
                } else {
                    qmp_allow_external = 0;
                }
                updateSummary(false);

            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mDesktopMode.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {

                if (isChecked) {
                    LimboSettingsManager.setDesktopMode(activity, true);
                    Config.mouseMode = Config.MouseMode.External;
                } else {
                    LimboSettingsManager.setDesktopMode(activity, false);
                    Config.mouseMode = Config.MouseMode.Trackpad;
                }
                updateSummary(false);

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
                updateSummary(false);
            }
        });
        mEnableKVM.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                if (isChecked) {
                    DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            MachineOpenHelper.getInstance(activity).update(currMachine,
                                    MachineOpenHelper.ENABLE_KVM, 1 + "");
                            currMachine.enableKVM = 1;
                            updateSummary(false);
                            mEnableMTTCG.setChecked(false);
                        }
                    };

                    DialogInterface.OnClickListener cancelListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    MachineOpenHelper.getInstance(activity).update(currMachine,
                                            MachineOpenHelper.ENABLE_KVM, 0 + "");
                                    mEnableKVM.setChecked(false);
                                    currMachine.enableKVM = 0;
                                    updateSummary(false);
                                    return;
                                }
                            };

                    DialogInterface.OnClickListener helpListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    MachineOpenHelper.getInstance(activity).update(currMachine,
                                            MachineOpenHelper.ENABLE_KVM, 0 + "");
                                    mEnableKVM.setChecked(false);
                                    currMachine.enableKVM = 0;
                                    updateSummary(false);
                                    goToURL(Config.kvmLink);
                                    return;
                                }
                            };

                    UIUtils.UIAlert(activity,
                            "Enable KVM",
                            "Warning! You'll need an Android Device with the same architecture as the emulated Guest. " +
                                    "Make sure you have installed an Android kernel with KVM support." +
                                    "If you don't know what this is press Cancel now.\n\nIf you experience crashes disable this option. Do you want to continue?",
                            16, false, "OK", okListener, "Cancel", cancelListener, "KVM Help", helpListener);

                } else {
                    MachineOpenHelper.getInstance(activity).update(currMachine,
                            MachineOpenHelper.ENABLE_KVM, 0 + "");
                    currMachine.enableKVM = 0;

                }
                updateSummary(false);
            }

        });

        mEnableMTTCG.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if(currMachine == null)
                    return;

                if (isChecked) {
                    DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            MachineOpenHelper.getInstance(activity).update(currMachine,
                                    MachineOpenHelper.ENABLE_MTTCG, 1 + "");
                            currMachine.enableMTTCG = 1;
                            updateSummary(false);
                            mEnableKVM.setChecked(false);
                        }
                    };

                    DialogInterface.OnClickListener cancelListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    MachineOpenHelper.getInstance(activity).update(currMachine,
                                            MachineOpenHelper.ENABLE_MTTCG, 0 + "");
                                    mEnableMTTCG.setChecked(false);
                                    currMachine.enableMTTCG = 0;
                                    updateSummary(false);
                                    return;
                                }
                            };

                    DialogInterface.OnClickListener helpListener =
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    MachineOpenHelper.getInstance(activity).update(currMachine,
                                            MachineOpenHelper.ENABLE_MTTCG, 0 + "");
                                    mEnableMTTCG.setChecked(false);
                                    currMachine.enableMTTCG = 0;
                                    updateSummary(false);
                                    goToURL(Config.faqLink);
                                    return;
                                }
                            };


                    UIUtils.UIAlert(activity,
                            "Enable MTTCG",
                            "Warning! Enabling MTTCG under Limbo is not fully tested. " +
                                    "\nYou need to have an Android Devices with multi core CPU. " +
                                    "If you don't know what this is press Cancel." +
                                    "\n\nIf you experience app crashing or freezing disable this option. Do you want to continue?",
                            16, false,"OK", okListener, "Cancel", cancelListener, "MTTCG Help", helpListener);
                } else {
                    MachineOpenHelper.getInstance(activity).update(currMachine,
                            MachineOpenHelper.ENABLE_MTTCG, 0 + "");
                    currMachine.enableMTTCG = 1;

                }

                updateSummary(false);
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
                updateSummary(false);
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
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mOrientation.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String orientationCfg = (String) ((ArrayAdapter<?>) mOrientation.getAdapter()).getItem(position);
                LimboSettingsManager.setOrientationSetting(activity, position);
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mKeyboard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String keyboardCfg = (String) ((ArrayAdapter<?>) mKeyboard.getAdapter()).getItem(position);

                currMachine.keyboard = keyboardCfg;
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.KEYBOARD, keyboardCfg);

                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mMouse.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if(currMachine == null)
                    return;

                String mouseCfg = (String) ((ArrayAdapter<?>) mMouse.getAdapter()).getItem(position);
                String mouseDB = mouseCfg.split(" ")[0];
                currMachine.mouse = mouseDB;
                MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.MOUSE, mouseDB);
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
    }

    protected synchronized void setDNSServer(String string) {

        File resolvConf = new File(Config.getBasefileDir() + "/etc/resolv.conf");
        FileOutputStream fileStream = null;
        try {
            fileStream = new FileOutputStream(resolvConf);
            String str = "nameserver " + string + "\n\n";
            byte[] data = str.getBytes();
            fileStream.write(data);
        } catch (Exception ex) {
            Log.e(TAG, "Could not write DNS to file: " + ex);
        } finally {
            if (fileStream != null)
                try {
                    fileStream.close();
                } catch (IOException e) {

                    e.printStackTrace();
                }
        }
    }

    private void disableListeners() {

        if (mMachine == null)
            return;


        mSnapshot.setOnItemSelectedListener(null);

        mUI.setOnItemSelectedListener(null);
        mKeyboard.setOnItemSelectedListener(null);
        mMouse.setOnItemSelectedListener(null);
        mOrientation.setOnItemSelectedListener(null);
        mVNCAllowExternal.setOnCheckedChangeListener(null);
        mDesktopMode.setOnCheckedChangeListener(null);
        mToolBar.setOnCheckedChangeListener(null);
        mFullScreen.setOnCheckedChangeListener(null);

        mArch.setOnItemSelectedListener(null);
        mMachineType.setOnItemSelectedListener(null);
        mCPU.setOnItemSelectedListener(null);
        mCPUNum.setOnItemSelectedListener(null);
        mRamSize.setOnItemSelectedListener(null);
        mDisableACPI.setOnCheckedChangeListener(null);
        mDisableHPET.setOnCheckedChangeListener(null);
        mDisableTSC.setOnCheckedChangeListener(null);
        mEnableKVM.setOnCheckedChangeListener(null);
        mEnableMTTCG.setOnCheckedChangeListener(null);

        mHDAenable.setOnCheckedChangeListener(null);
        mHDBenable.setOnCheckedChangeListener(null);
        mHDCenable.setOnCheckedChangeListener(null);
        mHDDenable.setOnCheckedChangeListener(null);
        mSharedFolderenable.setOnCheckedChangeListener(null);
        mHDA.setOnItemSelectedListener(null);
        mHDB.setOnItemSelectedListener(null);
        mHDC.setOnItemSelectedListener(null);
        mHDD.setOnItemSelectedListener(null);
        mSharedFolder.setOnItemSelectedListener(null);
        mHDCacheConfig.setOnItemSelectedListener(null);


        mBootDevices.setOnItemSelectedListener(null);
        mKernel.setOnItemSelectedListener(null);
        mInitrd.setOnItemSelectedListener(null);
        mAppend.setOnFocusChangeListener(null);

        mVGAConfig.setOnItemSelectedListener(null);

        mSoundCard.setOnItemSelectedListener(null);

        mNetConfig.setOnItemSelectedListener(null);
        mNicCard.setOnItemSelectedListener(null);
        mDNS.setOnFocusChangeListener(null);
        mHOSTFWD.setOnFocusChangeListener(null);

        mPrio.setOnCheckedChangeListener(null);
        mExtraParams.setOnFocusChangeListener(null);

    }

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        activity = this;
        super.onCreate(savedInstanceState);

        clearNotifications();
        setupStrictMode();
        setupFolders();
        setContentView(R.layout.limbo_main);
        setupWidgets();
        setupDiskMapping();
        createListeners();
        populateAttributes();
        execTimer();
        checkFirstLaunch();
        setupToolbar();
        checkUpdate();
        checkLog();
        checkAndLoadLibs();
        setupLinks();

    }

    private void setupLinks() {

        Config.osImages.put("Advanced Tools", new LinksManager.LinkInfo("Advanced Tools",
                "Get Full Qwerty Keyboard, Download Manager for ISO and virtual disks images, SSH/FTP Client for network connection",
                Config.toolsLink,
                LinksManager.LinkType.TOOL));
    }

    private void checkAndLoadLibs() {
        if (Config.loadNativeLibsEarly)
            if (Config.loadNativeLibsMainThread)
                setupNativeLibs();
            else
                setupNativeLibsAsync();
    }

    private void clearNotifications() {
        NotificationManager notificationManager = (NotificationManager) getApplicationContext().getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancelAll();
    }

    private void setupDiskMapping() {

        diskMapping.clear();
        addDiskMapping(FileType.HDA, mHDA, mHDAenable, MachineOpenHelper.HDA);
        addDiskMapping(FileType.HDB, mHDB, mHDBenable, MachineOpenHelper.HDB);
        addDiskMapping(FileType.HDC, mHDC, mHDCenable, MachineOpenHelper.HDC);
        addDiskMapping(FileType.HDD, mHDD, mHDDenable, MachineOpenHelper.HDD);
        addDiskMapping(FileType.SHARED_DIR, mSharedFolder, mSharedFolderenable, MachineOpenHelper.SHARED_FOLDER);

        addDiskMapping(FileType.CD, mCD, mCDenable, MachineOpenHelper.CDROM);
        addDiskMapping(FileType.FDA, mFDA, mFDAenable, MachineOpenHelper.FDA);
        addDiskMapping(FileType.FDB, mFDB, mFDBenable, MachineOpenHelper.FDB);
        addDiskMapping(FileType.SD, mSD, mSDenable, MachineOpenHelper.SD);

        addDiskMapping(FileType.KERNEL, mKernel, null, MachineOpenHelper.KERNEL);
        addDiskMapping(FileType.INITRD, mInitrd, null, MachineOpenHelper.INITRD);
    }

    private void addDiskMapping(FileType fileType, Spinner spinner, CheckBox enableCheckBox, String dbColName) {
        spinner.setTag(fileType);

        diskMapping.put(fileType, new DiskInfo(spinner, enableCheckBox, dbColName));
    }

    private void setupNativeLibsAsync() {

        Thread thread = new Thread(new Runnable() {
            public void run() {
                setupNativeLibs();
            }
        });
        thread.setPriority(Thread.MIN_PRIORITY);
        thread.start();

    }

    private void createListeners() {

        mMachine.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

                if (position == 0) {
                    enableNonRemovableDeviceOptions(false);
                    enableRemovableDeviceOptions(false);
                    mVNCAllowExternal.setEnabled(false);
                    currMachine = null;
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
                updateSummary(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mScrollView.setOnScrollChangeListener(new NestedScrollView.OnScrollChangeListener() {
            @Override
            public void onScrollChange(NestedScrollView v, int scrollX, int scrollY, int oldScrollX, int oldScrollY) {
                savePendingEditText();
            }
        });

        mStart.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {

                if (!Config.loadNativeLibsEarly && Config.loadNativeLibsMainThread) {
                    setupNativeLibs();
                }

                Thread thread = new Thread(new Runnable() {
                    public void run() {

                        if (!Config.loadNativeLibsEarly && !Config.loadNativeLibsMainThread) {
                            setupNativeLibs();
                        }
                        onStartButton();
                    }
                });
                thread.setPriority(Thread.MIN_PRIORITY);
                thread.start();

            }
        });
        mStop.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                onStopButton(false);

            }
        });
        mRestart.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {

                onRestartButton();

            }
        });
    }

    private void savePendingEditText() {
        View currentView = getCurrentFocus();
        if (currentView != null && currentView instanceof EditText) {
            ((EditText) currentView).setFocusable(false);
        }
    }

    private void checkFirstLaunch() {
        Thread t = new Thread(new Runnable() {
            public void run() {

                if (LimboSettingsManager.isFirstLaunch(activity)) {
                    onFirstLaunch();
                }

            }
        });
        t.start();
    }

    private void checkLog() {

        Thread t = new Thread(new Runnable() {
            public void run() {

                if (LimboSettingsManager.getExitCode(activity) != 1) {
                    LimboSettingsManager.setExitCode(activity, 1);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            UIUtils.promptShowLog(activity);
                        }
                    });
                }
            }
        });
        t.start();
    }

    private void setupFolders() {
        Thread t = new Thread(new Runnable() {
            public void run() {

                Config.cacheDir = getCacheDir().getAbsolutePath();
                Config.storagedir = Environment.getExternalStorageDirectory().toString();

                // Create Temp folder
                File folder = new File(Config.getTmpFolder());
                if (!folder.exists())
                    folder.mkdirs();


            }
        });
        t.start();
    }

    //XXX: sometimes this needs to be called from the main thread otherwise
    //  qemu crashes when it is started later
    public void setupNativeLibs() {

        if (libLoaded)
            return;

        //Some devices need stl loaded upfront
        //System.loadLibrary("stlport_shared");

        //Compatibility lib
        System.loadLibrary("compat-limbo");

        //Glib deps
        System.loadLibrary("compat-musl");


        //Glib
        System.loadLibrary("glib-2.0");

        //Pixman for qemu
        System.loadLibrary("pixman-1");

        //Spice server
        if (Config.enable_SPICE) {
            System.loadLibrary("crypto");
            System.loadLibrary("ssl");
            System.loadLibrary("spice");
        }

        // //Load SDL library
        if (Config.enable_SDL) {
            System.loadLibrary("SDL2");
        }

        System.loadLibrary("compat-SDL2-ext");

        //Limbo needed for vmexecutor
        System.loadLibrary("limbo");

        loadQEMULib();

        libLoaded = true;
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

                UIUtils.checkNewVersion(activity);

            }
        });
        tsdl.start();
    }

    private void setupStrictMode() {

        if (Config.debugStrictMode) {
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


    private void populateAttributes() {


        Thread t = new Thread(new Runnable() {
            public void run() {
                favinstance = FavOpenHelper.getInstance(activity);
                updateGlobalSettings();

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        populateAttributesUI();
                    }
                });
            }
        });
        t.start();
    }

    private void populateAttributesUI() {

        this.populateMachines(null);
        this.populateArch();
        this.populateMachineType(null);
        this.populateCPUs(null);
        this.populateCPUNum();
        this.populateRAM();
        this.populateDisks();
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
        this.populateMouse();
    }

    private void populateDisks() {

        //disks
        this.populateDiskAdapter(mHDA, FileType.HDA, true);
        this.populateDiskAdapter(mHDB, FileType.HDB, true);
        this.populateDiskAdapter(mHDC, FileType.HDC, true);
        this.populateDiskAdapter(mHDD, FileType.HDD, true);
        this.populateDiskAdapter(mSharedFolder, FileType.SHARED_DIR, false);

        //removable
        this.populateDiskAdapter(mCD, FileType.CD, false);
        this.populateDiskAdapter(mFDA, FileType.FDA, false);
        this.populateDiskAdapter(mFDB, FileType.FDB, false);
        this.populateDiskAdapter(mSD, FileType.SD, false);

        //boot
        this.populateDiskAdapter(mKernel, FileType.KERNEL, false);
        this.populateDiskAdapter(mInitrd, FileType.INITRD, false);

    }

    public void onFirstLaunch() {
        onLicense();
    }

    private void createMachine(String machineValue) {

        if (MachineOpenHelper.getInstance(activity).getMachine(machineValue, "") != null) {
            UIUtils.toastShort(activity, "VM Name \"" + machineValue + "\" exists please choose another name!");
            return;
        }

        showDownloadLinks();

        currMachine = new Machine(machineValue);

        // We set SDL as default interface
        currMachine.ui = "SDL";

        // default settings for each architecture, this helps user with the most common setup
        if (Config.enable_X86 || Config.enable_X86_64) {
            currMachine.arch = "x86";
            currMachine.cpu = "n270";
            currMachine.machine_type = "pc";
            currMachine.nic_card = "Default";
            currMachine.disabletsc = 1;
        } else if (Config.enable_ARM || Config.enable_ARM64) {
            currMachine.arch = "ARM";
            currMachine.machine_type = "versatilepb";
            currMachine.cpu = "Default";
            currMachine.nic_card = "Default";
        } else if (Config.enable_MIPS) {
            currMachine.arch = "MIPS";
            currMachine.machine_type = "malta";
        } else if (Config.enable_PPC || Config.enable_PPC64) {
            currMachine.arch = "PPC";
            currMachine.machine_type = "g3beige";
            currMachine.nic_card = "Default";
        } else if (Config.enable_m68k) {
            currMachine.arch = "m68k";
            currMachine.machine_type = "Default";
        } else if (Config.enable_sparc || Config.enable_sparc64) {
            currMachine.arch = "SPARC";
            currMachine.vga_type="cg3";
            currMachine.machine_type = "Default";
            currMachine.nic_card = "Default";
        }

        MachineOpenHelper.getInstance(activity).insertMachine(currMachine);

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
        this.mQMPAllowExternal.setEnabled(true);

    }

    protected void showDownloadLinks() {

        if (!Config.osImages.isEmpty()) {
            OSDialogBox oses = new OSDialogBox(activity);
            oses.setCanceledOnTouchOutside(false);
            oses.setCancelable(false);
            oses.show();
        }
    }

    private void onDeleteMachine() {
        if (currMachine == null) {
            UIUtils.toastShort(this, "Select a machine first!");
            return;
        }
        Thread t = new Thread(new Runnable() {
            public void run() {
                MachineOpenHelper.getInstance(activity).deleteMachineDB(currMachine);
                final String name = currMachine.machinename;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        disableListeners();
                        disableRemovableDiskListeners();
                        mMachine.setSelection(0);
                        currMachine = null;
                        populateAttributes();
                        UIUtils.toastShort(activity, "Machine " + name + " deleted");
                        enableListeners();
                        enableRemovableDiskListeners();
                    }
                });

            }
        });
        t.start();

    }

    private void onExportMachines() {
        promptExportName(this);
    }

    public void exportMachines(String filePath) {
        progDialog = ProgressDialog.show(activity, "Please Wait", "Exporting Machines...", true);
        ExportMachines exporter = new ExportMachines();
        exporter.exportFilePath = filePath;
        exporter.execute();

    }

    private void onImportMachines() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Import Machines");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setPadding(20, 20, 20, 20);

        TextView imageNameView = new TextView(activity);
        imageNameView.setVisibility(View.VISIBLE);
        imageNameView.setText(
                "Choose the <export>.csv file you want to import.\n"
                        + "WARNING: Any machine with the same name will be replaced!\n"
                        );

        LinearLayout.LayoutParams searchViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(imageNameView, searchViewParams);
        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                promptForImportDir();
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

    private void promptForImportDir() {
        FileManager.browse(this, FileType.IMPORT_FILE, Config.OPEN_IMPORT_FILE_REQUEST_CODE);
    }


    public void importFile(String importFilePath) {
        // For each line create a Machine
        progDialog = ProgressDialog.show(activity, "Please Wait", "Importing Machines...", true);
        disableListeners();
        disableRemovableDiskListeners();
        currMachine = null;
        mMachine.setSelection(0);
        ImportMachines importer = new ImportMachines();
        importer.importFilePath = importFilePath;
        importer.execute();
    }

    private void onLicense() {
        PackageInfo pInfo = null;

        try {
            pInfo = getPackageManager().getPackageInfo(getClass().getPackage().getName(), PackageManager.GET_META_DATA);
        } catch (NameNotFoundException e) {
            e.printStackTrace();
            return;
        }

        final PackageInfo finalPInfo = pInfo;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    UIAlertLicense(Config.APP_NAME + " v" + finalPInfo.versionName,
                            FileUtils.LoadFile(activity, "LICENSE", false),
                            activity);
                } catch (IOException e) {

                    e.printStackTrace();
                }
            }
        });

    }

    public void exit() {
        onStopButton(true);
    }

    private void unlockRemovableDevices(boolean flag) {

        mCDenable.setEnabled(flag);
        mFDAenable.setEnabled(flag);
        mFDBenable.setEnabled(flag);
        mSDenable.setEnabled(flag);

    }

    private void enableRemovableDeviceOptions(boolean flag) {

        unlockRemovableDevices(flag);
        enableRemovableDiskValues(flag);
    }

    private void enableRemovableDiskValues(boolean flag) {
        mCD.setEnabled(flag && mCDenable.isChecked());
        mFDA.setEnabled(flag && mFDAenable.isChecked());
        mFDB.setEnabled(flag && mFDBenable.isChecked());
        mSD.setEnabled(flag && mSDenable.isChecked());
    }

    private void enableNonRemovableDeviceOptions(boolean flag) {

        //snapshot
        this.mSnapshot.setEnabled(flag);


        //ui
        this.mUI.setEnabled(flag);
        this.mKeyboard.setEnabled(Config.enableKeyboardLayoutOption && flag);
        this.mMouse.setEnabled(Config.enableMouseOption && flag);
        //XXX: disabled for now, use the Emulated mouse property to fix mouse
//        this.mDesktopMode.setEnabled(false);

        // Everything Except removable devices
        this.mArch.setEnabled(flag);
        this.mMachineType.setEnabled(flag);
        this.mCPU.setEnabled(flag);
        this.mCPUNum.setEnabled(flag);
        this.mRamSize.setEnabled(flag);
        this.mEnableKVM.setEnabled(flag && (Config.enable_KVM || !flag));
        this.mEnableMTTCG.setEnabled(flag && (Config.enableMTTCG || !flag));

        //lock drives
        mHDAenable.setEnabled(flag);
        mHDBenable.setEnabled(flag);
        mHDCenable.setEnabled(flag);
        mHDDenable.setEnabled(flag);
        mSharedFolderenable.setEnabled(flag);

        //drives
        mHDA.setEnabled(flag && mHDAenable.isChecked());
        mHDB.setEnabled(flag && mHDBenable.isChecked());
        mHDC.setEnabled(flag && mHDCenable.isChecked());
        mHDD.setEnabled(flag && mHDDenable.isChecked());
        mSharedFolder.setEnabled(flag && mSharedFolderenable.isChecked());
        mHDCacheConfig.setEnabled(flag && (Config.enableHDCache || !flag));

        //boot
        this.mBootDevices.setEnabled(flag);
        this.mKernel.setEnabled(flag);
        this.mInitrd.setEnabled(flag);
        this.mAppend.setEnabled(flag);

        //graphics
        this.mVGAConfig.setEnabled(flag);

        //audio
        if (Config.enable_SDL_sound
                && currMachine != null && currMachine.ui != null
                && currMachine.ui.equals("SDL") && currMachine.paused == 0)
            this.mSoundCard.setEnabled(flag);
        else
            this.mSoundCard.setEnabled(false);

        //net
        this.mNetConfig.setEnabled(flag);
        this.mNicCard.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);
        this.mDNS.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);
        this.mHOSTFWD.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);

        //advanced
        this.mDisableACPI.setEnabled(flag);
        this.mDisableHPET.setEnabled(flag);
        this.mDisableTSC.setEnabled(flag);
        this.mPrio.setEnabled(flag);
        this.mExtraParams.setEnabled(flag);

    }

    // Main event function
    // Retrives values from saved preferences
    private void onStartButton() {

        if (this.mMachine.getSelectedItemPosition() == 0 || this.currMachine == null ) {
            UIUtils.toastShort(activity, "Select or Create a Virtual Machine first");
            return;
        }
        try {
            validateFiles();
        } catch (Exception ex) {
            UIUtils.toastLong(this, ex.toString());
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
            UIUtils.toastLong(LimboActivity.this, "Couldn't find a Kernel image\nPlease attach a Kernel image first!");
            return;
        }

        if (currMachine != null && currMachine.machine_type != null && currMachine.cpu.endsWith("(arm)")
                && currMachine.machine_type.equals("None")) {
            UIUtils.toastLong(LimboActivity.this, "Please select an ARM machine type first!");
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

        // Append only when kernel is set

        if (currMachine.kernel != null && !currMachine.kernel.equals(""))
            vmexecutor.append = mAppend.getText().toString();

        vmexecutor.paused = currMachine.paused;

        if (!vmStarted) {
            UIUtils.toastShort(LimboActivity.this, "Starting VM, please wait");
            //XXX: make sure that bios files are installed in case we ran out of space in the last
            //  run
            FileInstaller.installFiles(activity, false);
        } else {
            UIUtils.toastShort(LimboActivity.this, "Connecting to VM, please wait");
        }

        if (mUI.getSelectedItemPosition() == 0) { // VNC
            vmexecutor.enableqmp = 1; // We enable qemu monitor
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
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.PAUSED,
                    0 + "");
        } else if (mUI.getSelectedItemPosition() == 2) { // SPICE
            startSPICE();
            currMachine.paused = 0;
            MachineOpenHelper.getInstance(activity).update(currMachine, MachineOpenHelper.PAUSED,
                    0 + "");
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (vmStarted) {
                    //do nothing
                    mQMPAllowExternal.setEnabled(false);
                } else if (vmexecutor.paused == 1) {
                    UIUtils.toastShort(LimboActivity.this, "VM Resuming, Please Wait");
                } else {
//					if (!vmStarted) {
//						UIUtils.toastLong(LimboActivity.this, "VM Started\nPause the VM instead so you won't have to boot again!");
//					}
                    enableNonRemovableDeviceOptions(false);
                    mStart.setImageResource(R.drawable.play);
                }
            }
        });

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
            activity.startvnc();
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

    private boolean validateFiles() {


        if (!FileUtils.fileValid(this, currMachine.hda_img_path)
                || !FileUtils.fileValid(this, currMachine.hdb_img_path)
                || !FileUtils.fileValid(this, currMachine.hdc_img_path)
                || !FileUtils.fileValid(this, currMachine.hdd_img_path)
                || !FileUtils.fileValid(this, currMachine.fda_img_path)
                || !FileUtils.fileValid(this, currMachine.fdb_img_path)
                || !FileUtils.fileValid(this, currMachine.sd_img_path)
                || !FileUtils.fileValid(this, currMachine.cd_iso_path)
                || !FileUtils.fileValid(this, currMachine.kernel)
                || !FileUtils.fileValid(this, currMachine.initrd)
                )
            return false;

        return true;
    }

    private void onStopButton(boolean exit) {
        stopVM(exit);
    }

    private void onRestartButton() {

        execTimer();

        if (vmexecutor == null) {
            if (this.currMachine != null && this.currMachine.paused == 1) {
                promptDiscardVMState();
                return;
            } else {
                UIUtils.toastShort(LimboActivity.this, "VM not running");
                return;
            }
        }

        Machine.resetVM(activity);
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
        } else if (view.getVisibility() == View.GONE || view.getVisibility() == View.INVISIBLE) {
            view.setVisibility(View.VISIBLE);
        }
    }

    // Setting up the UI
    public void setupWidgets() {

        setupSections();

        this.mScrollView = findViewById(R.id.scroll_view);

        this.mStatus = (ImageView) findViewById(R.id.statusVal);
        this.mStatus.setImageResource(R.drawable.off);
        this.mStatusText = (TextView) findViewById(R.id.statusStr);

        mStart = (ImageButton) findViewById(R.id.startvm);
        mStop = (ImageButton) findViewById(R.id.stopvm);
        mRestart = (ImageButton) findViewById(R.id.restartvm);

        //Machine
        this.mMachine = (Spinner) findViewById(R.id.machineval);

        //snapshots not used currently
        this.mSnapshot = (Spinner) findViewById(R.id.snapshotval);

        //ui
        if (!Config.enable_SDL)
            this.mUI.setEnabled(false);
        this.mVNCAllowExternal = (CheckBox) findViewById(R.id.vncexternalval);
        mVNCAllowExternal.setChecked(false);
        this.mQMPAllowExternal = (CheckBox) findViewById(R.id.qmpexternalval);
        mQMPAllowExternal.setChecked(false);

        this.mDesktopMode = (CheckBox) findViewById(R.id.desktopmodeval);
        this.mKeyboard = (Spinner) findViewById(R.id.keyboardval);
        this.mMouse = (Spinner) findViewById(R.id.mouseval);
        this.mOrientation = (Spinner) findViewById(R.id.orientationval);
        this.mToolBar = (CheckBox) findViewById(R.id.showtoolbarval);
        this.mFullScreen = (CheckBox) findViewById(R.id.fullscreenval);

        //cpu/board
        this.mCPU = (Spinner) findViewById(R.id.cpuval);
        this.mArch = (Spinner) findViewById(R.id.archtypeval);
        this.mMachineType = (Spinner) findViewById(R.id.machinetypeval);
        this.mCPUNum = (Spinner) findViewById(R.id.cpunumval);
        this.mUI = (Spinner) findViewById(R.id.uival);
        this.mRamSize = (Spinner) findViewById(R.id.rammemval);
        this.mEnableKVM = (CheckBox) findViewById(R.id.enablekvmval);
        this.mEnableMTTCG = (CheckBox) findViewById(R.id.enablemttcgval);
        this.mDisableACPI = (CheckBox) findViewById(R.id.acpival);
        this.mDisableHPET = (CheckBox) findViewById(R.id.hpetval);
        this.mDisableTSC = (CheckBox) findViewById(R.id.tscval);

        //disks
        this.mHDAenable = (CheckBox) findViewById(R.id.hdimgcheck);
        this.mHDBenable = (CheckBox) findViewById(R.id.hdbimgcheck);
        this.mHDCenable = (CheckBox) findViewById(R.id.hdcimgcheck);
        this.mHDDenable = (CheckBox) findViewById(R.id.hddimgcheck);
        this.mHDA = (Spinner) findViewById(R.id.hdimgval);
        this.mHDB = (Spinner) findViewById(R.id.hdbimgval);
        this.mHDC = (Spinner) findViewById(R.id.hdcimgval);
        this.mHDD = (Spinner) findViewById(R.id.hddimgval);
        this.mSharedFolderenable = (CheckBox) findViewById(R.id.sharedfoldercheck);
        this.mSharedFolder = (Spinner) findViewById(R.id.sharedfolderval);
        this.mHDCacheConfig = (Spinner) findViewById(R.id.hdcachecfgval);
        this.mHDCacheConfig.setEnabled(false); // Disabled for now

        //Removable storage
        this.mCD = (Spinner) findViewById(R.id.cdromimgval);
        this.mFDA = (Spinner) findViewById(R.id.floppyimgval);
        this.mFDB = (Spinner) findViewById(R.id.floppybimgval);
        if(!Config.enableEmulatedFloppy) {
            LinearLayout mFDALayout = (LinearLayout) findViewById(R.id.floppyimgl);
            mFDALayout.setVisibility(View.GONE);
            LinearLayout mFDBLayout = (LinearLayout) findViewById(R.id.floppybimgl);
            mFDBLayout.setVisibility(View.GONE);
        }
        this.mSD = (Spinner) findViewById(R.id.sdcardimgval);
        if(!Config.enableEmulatedSDCard) {
            LinearLayout mSDCardLayout = (LinearLayout) findViewById(R.id.sdcardimgl);
            mSDCardLayout.setVisibility(View.GONE);
        }
        this.mCDenable = (CheckBox) findViewById(R.id.cdromimgcheck);
        this.mFDAenable = (CheckBox) findViewById(R.id.floppyimgcheck);
        this.mFDBenable = (CheckBox) findViewById(R.id.floppybimgcheck);
        this.mSDenable = (CheckBox) findViewById(R.id.sdcardimgcheck);

        //boot
        this.mBootDevices = (Spinner) findViewById(R.id.bootfromval);
        this.mKernel = (Spinner) findViewById(R.id.kernelval);
        this.mInitrd = (Spinner) findViewById(R.id.initrdval);
        this.mAppend = (EditText) findViewById(R.id.appendval);

        //display
        this.mVGAConfig = (Spinner) findViewById(R.id.vgacfgval);

        //sound
        this.mSoundCard = (Spinner) findViewById(R.id.soundcfgval);

        //network
        this.mNetConfig = (Spinner) findViewById(R.id.netcfgval);
        this.mNicCard = (Spinner) findViewById(R.id.netDevicesVal);
        this.mDNS = (EditText) findViewById(R.id.dnsval);
        setDefaultDNServer();
        this.mHOSTFWD = (EditText) findViewById(R.id.hostfwdval);

        // advanced
        this.mPrio = (CheckBox) findViewById(R.id.prioval);
        this.mExtraParams = (EditText) findViewById(R.id.extraparamsval);

        disableFeatures();
        enableRemovableDeviceOptions(false);
        enableNonRemovableDeviceOptions(false);

    }

    private void disableFeatures() {

        LinearLayout mAudioSectionLayout = (LinearLayout) findViewById(R.id.audiosectionl);
        if(!Config.enable_SDL_sound) {
            mAudioSectionLayout.setVisibility(View.GONE);
        }

        LinearLayout mDisableTSCLayout = (LinearLayout) findViewById(R.id.tscl);
        LinearLayout mDisableACPILayout = (LinearLayout) findViewById(R.id.acpil);
        LinearLayout mDisableHPETLayout = (LinearLayout) findViewById(R.id.hpetl);
        LinearLayout mEnableKVMLayout = (LinearLayout) findViewById(R.id.kvml);

        if(!Config.enable_X86 && !Config.enable_X86_64) {
            mDisableTSCLayout.setVisibility(View.GONE);
            mDisableACPILayout.setVisibility(View.GONE);
            mDisableHPETLayout.setVisibility(View.GONE);
        }
        if(!Config.enable_X86 && !Config.enable_X86_64
                && !Config.enable_ARM && !Config.enable_ARM64) {
            mEnableKVMLayout.setVisibility(View.GONE);
        }
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
            mCPUSectionSummary = (TextView) findViewById(R.id.cpusectionsummaryStr);
            mCPUSectionHeader = (LinearLayout) findViewById(R.id.cpusectionheaderl);
            mCPUSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mCPUSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mStorageSectionDetails = (LinearLayout) findViewById(R.id.storagesectionDetails);
            mStorageSectionDetails.setVisibility(View.GONE);
            mStorageSectionSummary = (TextView) findViewById(R.id.storagesectionsummaryStr);
            mStorageSectionHeader = (LinearLayout) findViewById(R.id.storageheaderl);
            mStorageSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mStorageSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mUserInterfaceSectionDetails = (LinearLayout) findViewById(R.id.userInterfaceDetails);
            mUserInterfaceSectionDetails.setVisibility(View.GONE);
            mUISectionSummary = (TextView) findViewById(R.id.uisectionsummaryStr);
            mUserInterfaceSectionHeader = (LinearLayout) findViewById(R.id.userinterfaceheaderl);
            mUserInterfaceSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mUserInterfaceSectionDetails);
                    enableListenersDelayed(500);
                }
            });


            mRemovableStorageSectionDetails = (LinearLayout) findViewById(R.id.removableStoragesectionDetails);
            mRemovableStorageSectionDetails.setVisibility(View.GONE);
            mRemovableStorageSectionSummary = (TextView) findViewById(R.id.removablesectionsummaryStr);
            mRemovableStorageSectionHeader = (LinearLayout) findViewById(R.id.removablestorageheaderl);
            mRemovableStorageSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mRemovableStorageSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mGraphicsSectionDetails = (LinearLayout) findViewById(R.id.graphicssectionDetails);
            mGraphicsSectionDetails.setVisibility(View.GONE);
            mGraphicsSectionSummary = (TextView) findViewById(R.id.graphicssectionsummaryStr);
            mGraphicsSectionHeader = (LinearLayout) findViewById(R.id.graphicsheaderl);
            mGraphicsSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mGraphicsSectionDetails);
                    enableListenersDelayed(500);
                }
            });
            mAudioSectionDetails = (LinearLayout) findViewById(R.id.audiosectionDetails);
            mAudioSectionDetails.setVisibility(View.GONE);
            mAudioSectionSummary = (TextView) findViewById(R.id.audiosectionsummaryStr);
            mAudioSectionHeader = (LinearLayout) findViewById(R.id.audioheaderl);
            mAudioSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mAudioSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mNetworkSectionDetails = (LinearLayout) findViewById(R.id.networksectionDetails);
            mNetworkSectionDetails.setVisibility(View.GONE);
            mNetworkSectionSummary = (TextView) findViewById(R.id.networksectionsummaryStr);
            mNetworkSectionHeader = (LinearLayout) findViewById(R.id.networkheaderl);
            mNetworkSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mNetworkSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mBootSectionDetails = (LinearLayout) findViewById(R.id.bootsectionDetails);
            mBootSectionDetails.setVisibility(View.GONE);
            mBootSectionSummary = (TextView) findViewById(R.id.bootsectionsummaryStr);
            mBootSectionHeader = (LinearLayout) findViewById(R.id.bootheaderl);
            mBootSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mBootSectionDetails);
                    enableListenersDelayed(500);
                }
            });

            mAdvancedSectionDetails = (LinearLayout) findViewById(R.id.advancedSectionDetails);
            mAdvancedSectionDetails.setVisibility(View.GONE);
            mAdvancedSectionSummary = (TextView) findViewById(R.id.advancedsectionsummaryStr);
            mAdvancedSectionHeader = (LinearLayout) findViewById(R.id.advancedheaderl);
            mAdvancedSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleVisibility(mAdvancedSectionDetails);
                    enableListenersDelayed(500);
                }
            });
        }
    }

    private void enableListenersDelayed(int msecs) {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                enableListeners();
                enableRemovableDiskListeners();
            }
        },msecs);
    }

    public void updateUISummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mUISectionSummary.setText("");
        else {
            String text = currMachine.ui
                    + ", Orientation: " + mOrientation.getSelectedItem().toString()
                    + ", Keyboard: " + mKeyboard.getSelectedItem().toString()
                    + ", Mouse: " + mMouse.getSelectedItem().toString()
                    + (mToolBar.isChecked() ? ", Toolbar On" : "")
                    + (mFullScreen.isChecked() ? ", Fullscreen" : "")
                    + (mVNCAllowExternal.isChecked() ? ", External VNC Server: On" : "")
                    + (mQMPAllowExternal.isChecked() ? ", External QMP Server: On" : "")
                    + (mDesktopMode.isChecked() ? ", Desktop Mode" : "");

            mUISectionSummary.setText(text);
        }
    }

    public void updateCPUSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mCPUSectionSummary.setText("");
        else {
            String text = "Arch: " + currMachine.arch
                    + ", Machine Type: " + currMachine.machine_type
                    + ", CPU: " + currMachine.cpu
                    + ", " + currMachine.cpuNum + " CPU" + ((currMachine.cpuNum > 1) ? "s" : "")
                    + ", " + currMachine.memory + " MB";
            if (mEnableMTTCG.isChecked())
                text = appendOption("Enable MTTCG", text);
            if (mEnableKVM.isChecked())
                text = appendOption("Enable KVM", text);
            if (mDisableACPI.isChecked())
                text = appendOption("Disable ACPI", text);
            if (mDisableHPET.isChecked())
                text = appendOption("Disable HPET", text);
            if (mDisableTSC.isChecked())
                text = appendOption("Disable TSC", text);
            mCPUSectionSummary.setText(text);
        }
    }

    public void updateStorageSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mStorageSectionSummary.setText("");
        else {
            String text = null;
            text = appendDriveFilename(currMachine.hda_img_path, text, "HDA", false);
            text = appendDriveFilename(currMachine.hdb_img_path, text, "HDB", false);
            text = appendDriveFilename(currMachine.hdc_img_path, text, "HDC", false);
            text = appendDriveFilename(currMachine.hdd_img_path, text, "HDD", false);

            text = appendDriveFilename(currMachine.shared_folder, text, "Shared Folder", false);

            if (text == null || text.equals("'"))
                text = "None";
            mStorageSectionSummary.setText(text);
        }
    }

    public void updateRemovableStorageSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mRemovableStorageSectionSummary.setText("");
        else {
            String text = null;

            text = appendDriveFilename(currMachine.cd_iso_path, text, "CDROM", true);
            text = appendDriveFilename(currMachine.fda_img_path, text, "FDA", true);
            text = appendDriveFilename(currMachine.fdb_img_path, text, "FDB", true);
            text = appendDriveFilename(currMachine.sd_img_path, text, "SD", true);


            if (text == null || text.equals(""))
                text = "None";

            mRemovableStorageSectionSummary.setText(text);
        }
    }

    public void updateBootSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mBootSectionSummary.setText("");
        else {
            String text = "Boot from: " + currMachine.bootdevice;
            text = appendDriveFilename(currMachine.kernel, text, "kernel", false);
            text = appendDriveFilename(currMachine.initrd, text, "initrd", false);
            text = appendDriveFilename(currMachine.append, text, "append", false);

            mBootSectionSummary.setText(text);
        }
    }

    private String appendDriveFilename(String driveFile, String text, String drive, boolean allowEmptyDrive) {

        String file = null;
        if (driveFile != null) {
            if((driveFile.equals("") || driveFile.equals("None") ) && allowEmptyDrive){
                file = drive + ": Empty";
            } else if (!driveFile.equals("") && !driveFile.equals("None"))
                file = drive + ": " + FileUtils.getFilenameFromPath(driveFile);
        }
        if (text == null && file != null)
            text = file;
        else if (file != null)
            text += (", " + file);
        return text;
    }

    public void updateGraphicsSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mGraphicsSectionSummary.setText("");
        else {
            String text = "Video Card: " + currMachine.vga_type;
            mGraphicsSectionSummary.setText(text);
        }
    }

    public void updateAudioSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mAudioSectionSummary.setText("");
        else {
            String text = mUI.getSelectedItemPosition() == 1 ? ("Audio Card: " + currMachine.soundcard) : "Audio Card: None";

            mAudioSectionSummary.setText(text);
        }
    }

    public void updateNetworkSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mNetworkSectionSummary.setText("");
        else {
            String text = "Net: " + currMachine.net_cfg;
            if (!currMachine.net_cfg.equals("None")) {
                text = text + ", NIC Card: " + currMachine.nic_card
                        + ", DNS Server: " + mDNS.getText();
                if (!(mHOSTFWD.getText() + "").equals(""))
                    text += (", Host Forward: " + mHOSTFWD.getText());
            }
            mNetworkSectionSummary.setText(text);
        }
    }


    public void updateAdvancedSummary(boolean clear) {
        if (clear || currMachine == null || mMachine.getSelectedItemPosition() < 2)
            mAdvancedSectionSummary.setText("");
        else {
            String text = null;
            if (mPrio.isChecked())
                text = appendOption("High Priority", text);
            if (currMachine.extra_params != null && !currMachine.extra_params.equals(""))
                text = appendOption("Extra Params: " + currMachine.extra_params, text);
            mAdvancedSectionSummary.setText(text);
        }
    }

    private String appendOption(String option, String text) {

        if (text == null && option != null)
            text = option;
        else if (option != null)
            text += (", " + option);
        return text;
    }

    private void triggerUpdateSpinner(final Spinner spinner) {

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
        textView.setPadding(20, 20, 20, 20);
        textView.setText("Warning! High Priority might increase emulation speed but " + "will slow your phone down!");

        alertDialog.setView(textView);

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


    private void promptSharedFolder(final Activity activity) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Enable Shared Folder!");

        TextView textView = new TextView(activity);
        textView.setVisibility(View.VISIBLE);
        textView.setPadding(20, 20, 20, 20);
        textView.setText(
                "Warning! Enabling Shared folder is buggy under Limbo. " +
                        "\nIf you use Storage Cleaning apps exclude this folder. " +
                        "\nMake sure you keep a backup of your files. " +
                        "\nPausing the Virtual Machine is not supported with this feature." +
                        "\nIf you experience crashes disable this option. " +
                        "\nDo you want to continue?");

        ScrollView scrollView = new ScrollView(activity);
        scrollView.addView(textView);

        alertDialog.setView(scrollView);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                mSharedFolderenable.setChecked(false);
                return;
            }
        });
        alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mSharedFolderenable.setChecked(false);
            }
        });
        alertDialog.setCancelable(false);
        alertDialog.show();
    }

    public void promptVNCAllowExternal(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Enable VNC server");

        TextView textView = new TextView(activity);
        textView.setVisibility(View.VISIBLE);
        textView.setPadding(20, 20, 20, 20);
        textView.setText("VNC Server: " + this.getLocalIpAddress() + ":" + Config.defaultVNCPort + "\n"
                + "Warning: Allowing VNC to serve external connections is not Recommended! " +
                "VNC connections are Unencrypted therefore NOT secure. " +
                "Make sure you're on a private network and you trust other apps installed in your device!\n");

        final EditText passwdView = new EditText(activity);
        passwdView.setInputType(InputType.TYPE_CLASS_TEXT |
                InputType.TYPE_TEXT_VARIATION_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        passwdView.setSelection(passwdView.getText().length());
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
        passwdView.setTag(false);
        passwdView.setTransformationMethod(new PasswordTransformationMethod());
        passwdView.setSelection(passwdView.getText().length());

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Set", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {


                if (passwdView.getText().toString().trim().equals("")) {
                    UIUtils.toastShort(getApplicationContext(), "Password cannot be empty!");
                    vnc_passwd = null;
                    vnc_allow_external = 0;
                    mVNCAllowExternal.setChecked(false);
                    return;
                } else {
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

//        alertDialog.setButton(DialogInterface.BUTTON_NEUTRAL, " ", new DialogInterface.OnClickListener() {
//            public void onClick(DialogInterface dialog, int which) {
//            }
//        });
        try {
            Button passwordButton = alertDialog.getButton(AlertDialog.BUTTON_NEUTRAL);
//        passwordButton.setWidth(24);
//        passwordButton.setHeight(24);
            passwordButton.setVisibility(View.VISIBLE);
            passwordButton.setBackgroundResource(android.R.drawable.ic_menu_view);
            passwordButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if ((boolean) passwdView.getTag()) {
                        passwdView.setTransformationMethod(new PasswordTransformationMethod());
                    } else
                        passwdView.setTransformationMethod(new HideReturnsTransformationMethod());
                    passwdView.setTag(!(boolean) passwdView.getTag());
                    passwdView.setSelection(passwdView.getText().length());
                }
            });
        } finally{}
    }


    public void promptQMPAllowExternal(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Enable QMP server");

        TextView textView = new TextView(activity);
        textView.setVisibility(View.VISIBLE);
        textView.setPadding(20, 20, 20, 20);
        textView.setText("QMP Server: " + this.getLocalIpAddress() + ":" + Config.QMPPort + "\n"
                + "Warning: Allowing QMP to serve external connections is NOT Recommended! " +
                "QMP connections are Passwordless and Unencrypted therefore NOT secure. " +
                "Make sure you're on a private network and you trust other apps installed in your device!\n");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        LinearLayout.LayoutParams textViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(textView, textViewParams);

        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Continue", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                    qmp_allow_external = 1;
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                qmp_allow_external = 0;
                mQMPAllowExternal.setChecked(false);
                return;
            }
        });



        alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mQMPAllowExternal.setChecked(false);
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
                mQMPAllowExternal.setChecked(false);

                populateMachineType(currMachine.machine_type);
                populateCPUs(currMachine.cpu);
                populateNetDevices(currMachine.nic_card);

                setAdapter(mArch, currMachine.arch);
                setCPUNum(currMachine.cpuNum);
                setRAM(currMachine.memory);
                setDiskValue(FileType.KERNEL, currMachine.kernel);
                setDiskValue(FileType.INITRD, currMachine.initrd);
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
                setDiskValue(FileType.CD, currMachine.cd_iso_path);

                // Floppy
                setDiskValue(FileType.FDA, currMachine.fda_img_path);
                setDiskValue(FileType.FDB, currMachine.fdb_img_path);

                // SD Card
                setDiskValue(FileType.SD, currMachine.sd_img_path);

                // HDD
                setDiskValue(FileType.HDA, currMachine.hda_img_path);
                setDiskValue(FileType.HDB, currMachine.hdb_img_path);
                setDiskValue(FileType.HDC, currMachine.hdc_img_path);
                setDiskValue(FileType.HDD, currMachine.hdd_img_path);

                //sharedfolder
                setDiskValue(FileType.SHARED_DIR, currMachine.shared_folder);

                // Advance
                setBootDevice(currMachine.bootdevice);
                setNetCfg(currMachine.net_cfg, false);
                // this.setNicDevice(currMachine.nic_card, false);
                setVGA(currMachine.vga_type);
                setHDCache(currMachine.hd_cache);
                setSoundcard(currMachine.soundcard);
                setUI(currMachine.ui);
                setMouse(currMachine.mouse);
                setKeyboard(currMachine.keyboard);
                mDisableACPI.setChecked(currMachine.disableacpi == 1 ? true : false);
                mDisableHPET.setChecked(currMachine.disablehpet == 1 ? true : false);
                if(Config.enable_X86 || Config.enable_X86_64)
                    mDisableTSC.setChecked(currMachine.disabletsc == 1 ? true : false);
                mEnableKVM.setChecked(currMachine.enableKVM == 1 ? true : false);
                mEnableMTTCG.setChecked(currMachine.enableMTTCG == 1 ? true : false);

//				userPressedBluetoothMouse = false;


                // We finished loading now when a user change a setting it will
                // fire an
                // event

                enableNonRemovableDeviceOptions(true);
                enableRemovableDeviceOptions(true);

                if (Config.enable_SDL_sound) {
                    if (currMachine.ui != null && currMachine.ui.equals("SDL") && currMachine.paused == 0) {
                        mSoundCard.setEnabled(true);
                    } else
                        mSoundCard.setEnabled(false);
                } else
                    mSoundCard.setEnabled(false);

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
                            changeStatus(VMStatus.Paused);
                            enableNonRemovableDeviceOptions(false);
                            enableRemovableDeviceOptions(false);
                        } else {
                            changeStatus(VMStatus.Ready);
                            enableNonRemovableDeviceOptions(true);
                            enableRemovableDeviceOptions(true);
                        }

                        setUserPressed(true);
                        machineLoaded = false;
                        mMachine.setEnabled(true);
                        updateSummary(false);

                    }
                }, 1000);

            }
        });

    }

    private void updateSummary(boolean clear) {

        updateUISummary(clear);
        updateCPUSummary(clear);
        updateStorageSummary(clear);
        updateRemovableStorageSummary(clear);
        updateGraphicsSummary(clear);
        updateAudioSummary(clear);
        updateNetworkSummary(clear);
        updateBootSummary(clear);
        updateAdvancedSummary(clear);
    }

    public void promptMachineName(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("New Machine Name");
        final EditText vmNameTextView = new EditText(activity);
        vmNameTextView.setPadding(20, 20, 20, 20);
        vmNameTextView.setEnabled(true);
        vmNameTextView.setVisibility(View.VISIBLE);
        vmNameTextView.setSingleLine();
        alertDialog.setView(vmNameTextView);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (vmNameTextView.getText().toString().trim().equals(""))
                    UIUtils.toastShort(activity, "Machine name cannot be empty");
                else {
                    createMachine(vmNameTextView.getText().toString());
                    alertDialog.dismiss();
                }
            }
        });
        alertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(vmNameTextView.getWindowToken(), 0);
            }
        });
    }

    public void promptImageName(final Activity activity, final FileType fileType) {

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

        String[] arraySpinner = new String[5];
        arraySpinner[0] = "1GB (Growable)";
        arraySpinner[1] = "2GB (Growable)";
        arraySpinner[2] = "4GB (Growable)";
        arraySpinner[3] = "10 GB (Growable)";
        arraySpinner[4] = "20 GB (Growable)";

        ArrayAdapter<?> sizeAdapter = new ArrayAdapter<Object>(this, R.layout.custom_spinner_item, arraySpinner);
        sizeAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        size.setAdapter(sizeAdapter);
        mLayout.addView(size, spinnerParams);

        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", (DialogInterface.OnClickListener) null);
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Change Directory", (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button positiveButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        positiveButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                if (LimboSettingsManager.getImagesDir(LimboActivity.this) == null) {
                    changeImagesDir();
                    return;
                }

                int sizeSel = size.getSelectedItemPosition();
                String templateImage = "hd1g.qcow2";
                if (sizeSel == 0) {
                    templateImage = "hd1g.qcow2";
                } else if (sizeSel == 1) {
                    templateImage = "hd2g.qcow2";
                } else if (sizeSel == 2) {
                    templateImage = "hd4g.qcow2";
                } else if (sizeSel == 3) {
                    templateImage = "hd10g.qcow2";
                }else if (sizeSel == 4) {
                    templateImage = "hd20g.qcow2";
                }

                String image = imageNameView.getText().toString();
                if (image.trim().equals(""))
                    UIUtils.toastShort(activity, "Image filename cannot be empty");
                else {
                    if (!image.endsWith(".qcow2")) {
                        image += ".qcow2";
                    }
                    boolean res = createImgFromTemplate(templateImage, image, fileType);
                    if (res) {
                        alertDialog.dismiss();
                    }

                }
            }
        });

        Button negativeButton = alertDialog.getButton(AlertDialog.BUTTON_NEGATIVE);
        negativeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                changeImagesDir();

            }
        });

    }



    public void promptExportName(final Activity activity) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Export Filename");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        final EditText exportNameView = new EditText(activity);
        exportNameView.setEnabled(true);
        exportNameView.setVisibility(View.VISIBLE);
        exportNameView.setSingleLine();
        LinearLayout.LayoutParams imageNameViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(exportNameView, imageNameViewParams);

        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Export", (DialogInterface.OnClickListener) null);
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Change Directory", (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button positiveButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        positiveButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                if (LimboSettingsManager.getExportDir(LimboActivity.this) == null) {
                    changeExportDir();
                    return;
                }

                String exportFilename = exportNameView.getText().toString();
                if (exportFilename.trim().equals(""))
                    UIUtils.toastShort(activity, "Export filename cannot be empty");
                else {

                    if(!exportFilename.endsWith(".csv"))
                        exportFilename += ".csv";

                    exportMachines(exportFilename);
                    alertDialog.dismiss();
                }
            }
        });

        Button negativeButton = alertDialog.getButton(AlertDialog.BUTTON_NEGATIVE);
        negativeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                changeExportDir();

            }
        });

    }

    public void changeExportDir() {
        UIUtils.toastLong(LimboActivity.this, "Choose a directory to export your VMs");
        FileManager.browse(LimboActivity.this, FileType.EXPORT_DIR, Config.OPEN_EXPORT_DIR_REQUEST_CODE);
    }

    public void changeImagesDir() {
        UIUtils.toastLong(LimboActivity.this, "Choose a directory to create your image");
        FileManager.browse(LimboActivity.this, FileType.IMAGE_DIR, Config.OPEN_IMAGE_DIR_REQUEST_CODE);
    }

    public void changeSharedDir() {
        UIUtils.toastLong(LimboActivity.this, "Choose a directory to shared files with the VM");
        FileManager.browse(LimboActivity.this, FileType.SHARED_DIR, Config.OPEN_SHARED_DIR_REQUEST_CODE);
    }


    protected boolean createImgFromTemplate(String templateImage, String destImage, FileType imgType) {

        String imagesDir = LimboSettingsManager.getImagesDir(this);
        String displayName = null;
        String filePath = null;
        if(imagesDir.startsWith("content://")) {
            Uri imagesDirUri = Uri.parse(imagesDir);
            Uri fileCreatedUri = FileInstaller.installImageTemplateToSDCard(activity, templateImage,
                    imagesDirUri, "hdtemplates", destImage);
            displayName = FileUtils.getFullPathFromDocumentFilePath(fileCreatedUri.toString());
            filePath = fileCreatedUri.toString();
        } else {
            filePath = FileInstaller.installImageTemplateToExternalStorage(activity, templateImage, imagesDir, "hdtemplates", destImage);
            displayName = filePath;
        }

        if (progDialog != null && progDialog.isShowing()) {
            progDialog.dismiss();
        }

        if (displayName != null) {
            UIUtils.toastShort(activity, "Image Created: " + displayName);
            updateDrive(imgType, filePath);

            return true;
        }
        return false;
    }

    protected String exportMachinesToFile(String exportFileName) {

        String exportDir = LimboSettingsManager.getExportDir(this);
        String displayName = null;
        if(exportDir.startsWith("content://")) {
            Uri exportDirUri = Uri.parse(exportDir);
            Uri fileCreatedUri = FileUtils.exportFileSDCard(activity,
                    exportDirUri, exportFileName);
            displayName = FileUtils.getFullPathFromDocumentFilePath(fileCreatedUri.toString());
        } else {
            String filePath = FileUtils.exportFileLegacy(this, exportDir, exportFileName);
            displayName = filePath;
        }

        if (progDialog != null && progDialog.isShowing()) {
            progDialog.dismiss();
        }

        return displayName;
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

        // alertDialog.setMessage(body);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                savevm(stateNameView.getText() + "");

                return;
            }
        });
        alertDialog.show();

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == Config.VNC_RESET_RESULT_CODE) {
            Log.d(TAG, "Reconnecting");
            this.startvnc();

        } else if (resultCode == Config.SDL_QUIT_RESULT_CODE) {

            UIUtils.toastShort(getApplicationContext(), "SDL Quit");
            if (LimboActivity.vmexecutor != null) {
                LimboActivity.vmexecutor.stopvm(0);
            } else if (activity.getParent() != null) {
                activity.getParent().finish();
            }
            activity.finish();
        } else if (requestCode == Config.OPEN_IMPORT_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMPORT_FILE_ASF_REQUEST_CODE) {
            String file = null;
            if(requestCode == Config.OPEN_IMPORT_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, false);
            } else {
                file = FileUtils.getFilePathFromIntent(this, data);
            }
            if(file!=null)
                importFile(file);
        } else if (requestCode == Config.OPEN_EXPORT_DIR_REQUEST_CODE || requestCode == Config.OPEN_EXPORT_DIR_ASF_REQUEST_CODE) {
            String exportDir = null;
            if(requestCode == Config.OPEN_EXPORT_DIR_ASF_REQUEST_CODE) {
                exportDir = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                exportDir = FileUtils.getDirPathFromIntent(this, data);
            }
            if(exportDir != null)
                LimboSettingsManager.setExportDir(this, exportDir);

        } else if (requestCode == Config.OPEN_IMAGE_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
            String file = null;
            if(requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                filetype = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getFilePathFromIntent(this, data);
            }
            if(file!=null)
                updateDrive(filetype, file);


        } else if (requestCode == Config.OPEN_IMAGE_DIR_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_DIR_ASF_REQUEST_CODE) {
            String imageDir = null;
            if(requestCode == Config.OPEN_IMAGE_DIR_ASF_REQUEST_CODE) {
                imageDir = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                imageDir = FileUtils.getDirPathFromIntent(this, data);
            }
            if(imageDir != null)
                LimboSettingsManager.setImagesDir(this, imageDir);

        } else if (requestCode == Config.OPEN_SHARED_DIR_REQUEST_CODE || requestCode == Config.OPEN_SHARED_DIR_ASF_REQUEST_CODE) {
            String file = null;
            if(requestCode == Config.OPEN_SHARED_DIR_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                filetype = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getDirPathFromIntent(this, data);
            }
            if(file!=null) {
                updateDrive(filetype, file);
                LimboSettingsManager.setSharedDir(this, file);
            }
        } else if (requestCode == Config.OPEN_LOG_FILE_DIR_REQUEST_CODE|| requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
            String file = null;
            if(requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                file = FileUtils.getDirPathFromIntent(this, data);
            }
            if(file!=null) {
                FileUtils.saveLogToFile(activity, file);
            }
        }

    }

    private void updateDrive(FileType fileType, String diskValue) {

        //FIXME: sometimes the array adapters try to set invalid values
        if (fileType == null || diskValue == null) {
            return;
        }

        Spinner spinner = getSpinner(fileType);
        ArrayAdapter adapter = getAdapter(fileType);

        if (fileType != null && diskValue != null && !diskValue.trim().equals("")) {
            String colName = getColName(fileType);

            MachineOpenHelper.getInstance(activity).update(currMachine, colName, diskValue);
            if (adapter.getPosition(diskValue) < 0) {
                adapter.add(diskValue);
            }
            this.addDriveToList(diskValue, fileType);
            this.setDiskValue(fileType, diskValue);
        }

        int res = spinner.getSelectedItemPosition();
        if (res == 1) {
            spinner.setSelection(0);
        }

    }

    private String getColName(FileType fileType) {
        if (diskMapping.containsKey(fileType))
            return diskMapping.get(fileType).colName;
        return null;
    }

    private ArrayAdapter getAdapter(FileType fileType) {
        Spinner spinner = getSpinner(fileType);
        return (ArrayAdapter) spinner.getAdapter();
    }

    private Spinner getSpinner(FileType fileType) {
        if (diskMapping.containsKey(fileType))
            return diskMapping.get(fileType).spinner;
        return null;
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroy() {
        savePendingEditText();
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

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mVNCAllowExternal.setEnabled(false);
            }
        });


        if (this.mVNCAllowExternal.isChecked()
                && vnc_passwd != null && !vnc_passwd.equals("")) {
            vmexecutor.change_vnc_password();

            if (currMachine.paused != 1)
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
                stateView.setText("VNC Server started: " + getLocalIpAddress() + ":" + Config.defaultVNCPort + "\n"
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
        //UIUtils.toastShort(LimboActivity.this, "Connecting to VM Display");
        Intent intent = getVNCIntent();
        startActivityForResult(intent, Config.VNC_REQUEST_CODE);
    }

    public void startsdl() {

        Intent intent = null;

        intent = new Intent(this, LimboSDLActivity.class);

        android.content.ContentValues values = new android.content.ContentValues();
        startActivityForResult(intent, Config.SDL_REQUEST_CODE);
    }

    public void savevm(String name) {
        if (vmexecutor != null) {
            if (//
                    (currMachine.hda_img_path == null || currMachine.hda_img_path.equals(""))
                            && (currMachine.hdb_img_path == null || currMachine.hdb_img_path.equals(""))
                            && (currMachine.hdc_img_path == null || currMachine.hdc_img_path.equals(""))
                            && (currMachine.hdd_img_path == null || currMachine.hdd_img_path.equals(""))) {
                UIUtils.toastLong(LimboActivity.this, "Couldn't find a QCOW2 image\nPlease attach an HDA or HDB image first!");
            } else {
                vmexecutor.savevm("test_snapshot");
                UIUtils.toastShort(LimboActivity.this, "VM Saved");
            }
        } else {

            UIUtils.toastShort(LimboActivity.this, "VM not running");
        }

    }

    public void resumevm() {
        if (vmexecutor != null) {
            vmexecutor.resumevm();
            UIUtils.toastShort(LimboActivity.this, "VM Reset");
        } else {

            UIUtils.toastShort(LimboActivity.this, "VM not running");
        }

    }

    // Set Hard Disk
    private void populateRAM() {

        String[] arraySpinner = new String[256];

        arraySpinner[0] = 4 + "";
        for (int i = 1; i < arraySpinner.length; i++) {
            arraySpinner[i] = i * 8 + "";
        }
        ;

        ArrayAdapter<String> ramAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        ramAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mRamSize.setAdapter(ramAdapter);
        this.mRamSize.invalidate();
    }

    private void populateCPUNum() {
        String[] arraySpinner = new String[Config.MAX_CPU_NUM];

        for (int i = 0; i < arraySpinner.length; i++) {
            arraySpinner[i] = (i + 1) + "";
        }


        ArrayAdapter<String> cpuNumAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        cpuNumAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mCPUNum.setAdapter(cpuNumAdapter);
        this.mCPUNum.invalidate();
    }

    // Set Hard Disk
    private void setRAM(final int ram) {

        this.mRamSize.post(new Runnable() {
            public void run() {
                if (ram != 0) {
                    int pos = ((ArrayAdapter<String>) mRamSize.getAdapter()).getPosition(ram + "");
                    mRamSize.setSelection(pos);
                }
            }
        });

    }

    private void setCPUNum(final int cpuNum) {

        this.mCPUNum.post(new Runnable() {
            public void run() {
                if (cpuNum != 0) {
                    int pos = ((ArrayAdapter<String>) mCPUNum.getAdapter()).getPosition(cpuNum + "");
                    mCPUNum.setSelection(pos);
                }
            }
        });

    }

    // Set Hard Disk
    private void populateBootDevices() {
        ArrayList<String> bootDevicesList = new ArrayList<String>();
        bootDevicesList.add("Default");
        bootDevicesList.add("CD Rom");
        bootDevicesList.add("Hard Disk");
        if(Config.enableEmulatedFloppy)
            bootDevicesList.add("Floppy");

        String[] arraySpinner = bootDevicesList.toArray(new String[bootDevicesList.size()]);

        ArrayAdapter<String> bootDevAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        bootDevAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mBootDevices.setAdapter(bootDevAdapter);
        this.mBootDevices.invalidate();
    }

    // Set Net Cfg
    private void populateNet() {
        String[] arraySpinner = {"None", "User", "TAP"};
        ArrayAdapter<String> netAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        netAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mNetConfig.setAdapter(netAdapter);
        this.mNetConfig.invalidate();
    }

    // Set VGA Cfg
    private void populateVGA() {
        ArrayList<String> arrList = new ArrayList<String>();


        if (Config.enable_X86 || Config.enable_X86_64
        || Config.enable_ARM || Config.enable_ARM64
                || Config.enable_PPC || Config.enable_PPC64
                ) {
            arrList.add("std");
        }

        if (Config.enable_X86 || Config.enable_X86_64
                ) {
            arrList.add("cirrus");
            arrList.add("vmware");
        }

        //override vga for sun4m (32bit) machines to cg3
        if (Config.enable_sparc) {
            arrList.add("cg3");
        }

        if (Config.enable_ARM || Config.enable_ARM64) {
            arrList.add("virtio-gpu-pci");
        }

        if (Config.enable_SPICE)
            arrList.add("qxl");

        //XXX: some archs don't support vga on QEMU like SPARC64
        arrList.add("nographic");

        //TODO: Add XEN???
        // "xenfb"

        ArrayAdapter<String> vgaAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        vgaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mVGAConfig.setAdapter(vgaAdapter);
        this.mVGAConfig.invalidate();
    }

    private void populateOrientation() {

        ArrayList<String> arrList = new ArrayList<String>();
        arrList.add("Auto");
        arrList.add("Landscape");
        arrList.add("Landscape Reverse");
        arrList.add("Portrait");
        arrList.add("Portrait Reverse");

        ArrayAdapter<String> orientationAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        orientationAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mOrientation.setAdapter(orientationAdapter);
        this.mOrientation.invalidate();

        int pos = LimboSettingsManager.getOrientationSetting(activity);
        if (pos >= 0) {
            this.mOrientation.setSelection(pos);
        }
    }

    private void populateKeyboardLayout() {
        ArrayList<String> arrList = new ArrayList<String>();
        arrList.add("en-us");


        ArrayAdapter<String> keyboardAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        keyboardAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mKeyboard.setAdapter(keyboardAdapter);
        this.mKeyboard.invalidate();

        //TODO: for now we use only English keyboard, add more layouts
        int pos = 0;
        if (pos >= 0) {
            this.mKeyboard.setSelection(pos);
        }
    }

    private void populateMouse() {
        ArrayList<String> arrList = new ArrayList<String>();
        arrList.add("ps2");
        arrList.add("usb-mouse");
        arrList.add("usb-tablet" + fixMouseDescr);

        ArrayAdapter<String> mouseAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        mouseAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mMouse.setAdapter(mouseAdapter);
        this.mMouse.invalidate();
//
//        int pos = LimboSettingsManager.getMouseSetting(activity);
//        if (pos >= 0) {
//            this.mMouse.setSelection(pos);
//        }
    }

    private void populateSoundcardConfig() {

        String[] arraySpinner = {"None", "sb16", "ac97", "adlib", "cs4231a", "gus", "es1370", "hda", "pcspk", "all"};

        ArrayAdapter<String> sndAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        sndAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mSoundCard.setAdapter(sndAdapter);
        this.mSoundCard.invalidate();
    }

    // Set Cache Cfg
    private void populateHDCacheConfig() {

        String[] arraySpinner = {"default", "none", "writeback", "writethrough"};

        ArrayAdapter<String> hdCacheAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arraySpinner);
        hdCacheAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mHDCacheConfig.setAdapter(hdCacheAdapter);
        this.mHDCacheConfig.invalidate();
    }

    // Set Hard Disk
    private void populateNetDevices(String nic) {

        String[] arraySpinner = {"Default","e1000", "pcnet", "rtl8139", "ne2k_pci", "i82551", "i82557b",
                "i82559er", "virtio"};

        ArrayList<String> arrList = new ArrayList<String>();
        arrList.add("Default");

        if (currMachine != null && currMachine.arch != null) {
            if (currMachine.arch.equals("x86")) {

                arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
            } else if (currMachine.arch.equals("x64")) {
                arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
            } else if (currMachine.arch.equals("ARM")
                    || currMachine.arch.equals("ARM64")) {
                arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
                arrList.add("smc91c111");
                arrList.add("xgmac");
                arrList.add("lan9118");
                arrList.add("cadence_gem");
                arrList.add("allwinner-emac");
                arrList.add("mv88w8618");
            } else if (currMachine.arch.equals("PPC") || currMachine.arch.equals("PPC64")) {
                arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
            } else if (currMachine.arch.equals("SPARC") || currMachine.arch.equals("SPARC64")) {
                arrList.add("lance");
            }
        } else {
            arrList = new ArrayList<String>(Arrays.asList(arraySpinner));
        }

        ArrayAdapter<String> nicCfgAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        nicCfgAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mNicCard.setAdapter(nicCfgAdapter);


        this.mNicCard.invalidate();

        int pos = nicCfgAdapter.getPosition(nic);
        if (pos >= 0) {
            this.mNicCard.setSelection(pos);
        }
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
                        ArrayAdapter<String> machineAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
                        machineAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        mMachine.setAdapter(machineAdapter);
                        mMachine.invalidate();
                        if (machineValue != null)
                            setAdapter(mMachine, machineValue);
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
                    int pos = ((ArrayAdapter<String>) mCPU.getAdapter()).getPosition(cpu);

                    mCPU.setSelection(pos);
                }
            }
        });

    }

    private void setAdapter(final Spinner spinner, final String value) {

        spinner.post(new Runnable() {
            public void run() {
                if (value != null) {
                    int pos = ((ArrayAdapter<String>) spinner.getAdapter()).getPosition(value);
                    spinner.setSelection(pos);
                }
            }
        });

    }

    private void setMachineFieldValue(FileType type, final String diskValue) {
        if (type == FileType.HDA) {
            currMachine.hda_img_path = diskValue;
        } else if (type == FileType.HDB) {
            currMachine.hdb_img_path = diskValue;
        } else if (type == FileType.HDC) {
            currMachine.hdc_img_path = diskValue;
        } else if (type == FileType.HDD) {
            currMachine.hdd_img_path = diskValue;
        } else if (type == FileType.SHARED_DIR) {
            currMachine.shared_folder = diskValue;
        } else if (type == FileType.CD) {
            currMachine.cd_iso_path = diskValue;
        } else if (type == FileType.FDA) {
            currMachine.fda_img_path = diskValue;
        } else if (type == FileType.FDB) {
            currMachine.fdb_img_path = diskValue;
        } else if (type == FileType.SD) {
            currMachine.sd_img_path = diskValue;
        } else if (type ==  FileType.KERNEL) {
            currMachine.kernel = diskValue;
        } else if (type == FileType.INITRD) {
            currMachine.initrd = diskValue;
        }
    }

    private void setDiskValue(FileType fileType, final String diskValue) {
        Spinner spinner = getSpinner(fileType);
        ArrayAdapter adapter = getAdapter(fileType);

        setMachineFieldValue(fileType, diskValue);
        setDiskAdapter(spinner, diskValue);
    }

    private void setDiskAdapter(final Spinner spinner, final String value) {
        spinner.post(new Runnable() {
            public void run() {
                if (value != null) {
                    int pos = ((ArrayAdapter<String>) spinner.getAdapter()).getPosition(value);

                    if (pos >= 0) {
                        spinner.setSelection(pos);
                    } else {
                        spinner.setSelection(0);
                    }
                } else {
                    spinner.setSelection(0);
                }
            }
        });
    }

    private void setHDA(final String hda) {
        currMachine.hda_img_path = hda;
        setDiskAdapter(mHDA, hda);
    }

    private void setHDB(final String hdb) {
        this.currMachine.hdb_img_path = hdb;
        setDiskAdapter(mHDB, hdb);
    }

    private void setHDC(final String hdc) {

        this.currMachine.hdc_img_path = hdc;
        setDiskAdapter(mHDC, hdc);
    }

    private void setHDD(final String hdd) {
        this.currMachine.hdd_img_path = hdd;
        setDiskAdapter(mHDD, hdd);
    }

    private void setFDA(final String fda) {
        this.currMachine.fda_img_path = fda;
        setDiskAdapter(mFDA, fda);
    }

    private void setFDB(final String fdb) {
        this.currMachine.fdb_img_path = fdb;
        setDiskAdapter(mFDB, fdb);
    }

    private void setSD(final String sd) {
        this.currMachine.sd_img_path = sd;
        setDiskAdapter(mSD, sd);
    }


    //XXX: Not supported
    private void setHDCache(final String hdcache) {

        mHDCacheConfig.post(new Runnable() {
            public void run() {
                if (hdcache != null) {
                    int pos = ((ArrayAdapter<String>) mHDCacheConfig.getAdapter()).getPosition(hdcache);

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

        this.mSoundCard.post(new Runnable() {
            public void run() {
                if (soundcard != null) {
                    int pos = ((ArrayAdapter<String>) mSoundCard.getAdapter()).getPosition(soundcard);

                    if (pos >= 0) {
                        mSoundCard.setSelection(pos);
                    } else {
                        mSoundCard.setSelection(0);
                    }
                } else {
                    mSoundCard.setSelection(0);
                }
            }
        });

    }

    private void setUI(final String ui) {

        this.mUI.post(new Runnable() {
            public void run() {
                if (ui != null) {
                    int pos = ((ArrayAdapter<String>) mUI.getAdapter()).getPosition(ui);

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
                    int pos = ((ArrayAdapter<String>) mVGAConfig.getAdapter()).getPosition(vga);

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


    private void setMouse(final String mouse) {

        this.mMouse.post(new Runnable() {
            public void run() {
                if (mouse!= null) {
                    String mouseStr = mouse;
                    if(mouseStr.startsWith("usb-tablet"))
                        mouseStr+= fixMouseDescr;
                    int pos = ((ArrayAdapter<String>) mMouse.getAdapter()).getPosition(mouseStr);

                    if (pos >= 0) {
                        mMouse.setSelection(pos);
                    } else {
                        mMouse.setSelection(0);
                    }
                } else {
                    mMouse.setSelection(0);

                }
            }
        });

    }

    private void setKeyboard(final String keyboard) {

        this.mKeyboard.post(new Runnable() {
            public void run() {
                if (keyboard!= null) {
                    String mouseStr = keyboard;
                    int pos = ((ArrayAdapter<String>) mKeyboard.getAdapter()).getPosition(keyboard);

                    if (pos >= 0) {
                        mKeyboard.setSelection(pos);
                    } else {
                        mKeyboard.setSelection(0);
                    }
                } else {
                    mKeyboard.setSelection(0);

                }
            }
        });

    }


    private void setNetCfg(final String net, boolean userPressed) {
//		this.userPressedNetCfg = userPressed;

        this.mNetConfig.post(new Runnable() {
            public void run() {
                if (net != null) {
                    int pos = ((ArrayAdapter<String>) mNetConfig.getAdapter()).getPosition(net);

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
                    int pos = ((ArrayAdapter<String>) mBootDevices.getAdapter()).getPosition(bootDevice);

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
                    int pos = ((ArrayAdapter<String>) mSnapshot.getAdapter()).getPosition(snapshot);

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

        this.mNicCard.post(new Runnable() {
            public void run() {
                if (nic != null) {
                    int pos = ((ArrayAdapter<String>) mNicCard.getAdapter()).getPosition(nic);

                    if (pos >= 0) {
                        mNicCard.setSelection(pos);
                    } else {
                        mNicCard.setSelection(3);
                    }
                } else {
                    mNicCard.setSelection(3);

                }
            }
        });

    }

    private void populateCPUs(String cpu) {

        ArrayList<String> arrList = new ArrayList<String>();

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
        arrARM.add("Default");
        arrARM.add("arm926");
        arrARM.add("arm920t");
        arrARM.add("arm946");
        arrARM.add("arm1026");
        arrARM.add("arm1136");
        arrARM.add("arm1136-r2");
        arrARM.add("arm1176");
        arrARM.add("arm11mpcore");
        arrARM.add("cortex-m3");
        arrARM.add("cortex-m4");
        arrARM.add("cortex-m5");
        arrARM.add("cortex-a7");
        arrARM.add("cortex-a8");
        arrARM.add("cortex-a9");
        arrARM.add("cortex-a15");
        arrARM.add("cortex-r5");
        arrARM.add("pxa250");
        arrARM.add("pxa255");
        arrARM.add("pxa260");
        arrARM.add("pxa261");
        arrARM.add("pxa262");
        arrARM.add("pxa270-a0");
        arrARM.add("pxa270-a1");
        arrARM.add("pxa270");
        arrARM.add("pxa270-b0");
        arrARM.add("pxa270-b1");
        arrARM.add("pxa270-c0");
        arrARM.add("pxa270-c5");
        arrARM.add("sa1100");
        arrARM.add("sa1110");
        arrARM.add("ti925t");

        ArrayList<String> arrARM64 = new ArrayList<String>();
        arrARM64.add("Default");
        arrARM64.add("cortex-a53");
        arrARM64.add("cortex-a57");

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
        //XXX: something is wrong with quoting and doesn't let qemu find the cpu
        //  when it contains a space for now we don't add any cpus
//        arrSparc.add("Fujitsu MB86904");
//        arrSparc.add("Fujitsu MB86907");
//        arrSparc.add("TI MicroSparc I");
//        arrSparc.add("TI MicroSparc II");
//        arrSparc.add("TI MicroSparc IIep");
//        arrSparc.add("TI SuperSparc 40");
//        arrSparc.add("TI SuperSparc 50");
//        arrSparc.add("TI SuperSparc 51");
//        arrSparc.add("TI SuperSparc 60");
//        arrSparc.add("TI SuperSparc 61");
//        arrSparc.add("TI SuperSparc II");
//        arrSparc.add("LEON2");
//        arrSparc.add("LEON3");

        // Sparc
        ArrayList<String> arrSparc64 = new ArrayList<String>();
        arrSparc64.add("Default");
        //XXX: something is wrong with quoting and doesn't let qemu find the cpu
        //  when it contains a space for now we don't add any cpus
//        arrSparc64.add("Fujitsu Sparc64");
//        arrSparc64.add("Fujitsu Sparc64 III");
//        arrSparc64.add("Fujitsu Sparc64 IV");
//        arrSparc64.add("Fujitsu Sparc64 V");
//        arrSparc64.add("TI UltraSparc I");
//        arrSparc64.add("TI UltraSparc II");
//        arrSparc64.add("TI UltraSparc IIi");
//        arrSparc64.add("TI UltraSparc IIe");
//        arrSparc64.add("Sun UltraSparc III");
//        arrSparc64.add("Sun UltraSparc III");
//        arrSparc64.add("Sun UltraSparc IIIi");
//        arrSparc64.add("Sun UltraSparc IV");
//        arrSparc64.add("Sun UltraSparc IV+");
//        arrSparc64.add("Sun UltraSparc IIIi+");
//        arrSparc64.add("Sun UltraSparc T1");
//        arrSparc64.add("Sun UltraSparc T2");
//        arrSparc64.add("NEC UltraSparc I");


        if (currMachine != null && currMachine.arch != null)  {
            if (currMachine.arch.equals("x86")) {
                arrList.addAll(arrX86);
            } else if (currMachine.arch.equals("x64")) {
                arrList.addAll(arrX86_64);
            } else if (currMachine.arch.equals("ARM")) {
                arrList.addAll(arrARM);
            } else if (currMachine.arch.equals("ARM64")) {
                arrList.addAll(arrARM64);
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
            } else if (currMachine.arch.equals("SPARC64")) {
                arrList.addAll(arrSparc64);
            }
        } else {
            arrList.addAll(arrX86);
        }

        if(Config.enable_X86 || Config.enable_X86_64 || Config.enable_ARM || Config.enable_ARM64)
            arrList.add("host");

        ArrayAdapter<String> cpuAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        cpuAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mCPU.setAdapter(cpuAdapter);

        this.mCPU.invalidate();

        int pos = cpuAdapter.getPosition(cpu);
        if (pos >= 0) {
            this.mCPU.setSelection(pos);
        }

    }

    private void populateArch() {

        ArrayList<String> arrList = new ArrayList<String>();

        if (Config.enable_X86)
            arrList.add("x86");
        if (Config.enable_X86_64)
            arrList.add("x64");

        if (Config.enable_ARM)
            arrList.add("ARM");
        if (Config.enable_ARM64)
            arrList.add("ARM64");

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

        if (Config.enable_sparc64)
            arrList.add("SPARC64");

        ArrayAdapter<String> archAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        archAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);

        this.mArch.setAdapter(archAdapter);

        this.mArch.invalidate();

    }

    private void populateMachineType(String machineType) {

        ArrayList<String> arrList = new ArrayList<String>();

        if (currMachine != null && currMachine.arch!=null) {
            if (currMachine.arch.equals("x86") || currMachine.arch.equals("x64")) {
                arrList.add("Default");
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

            } else if (currMachine.arch.equals("ARM") || currMachine.arch.equals("ARM64")) {
//                arrList.add("Default"); //no default for ARM
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
                arrList.add("xlnx-ep108");
                arrList.add("xlnx-zcu102");
                arrList.add("z2");

            } else if (currMachine.arch.equals("MIPS")) {
                arrList.add("Default");
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
                arrList.add("none");
                arrList.add("powernv");
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

            } if (currMachine.arch.equals("SPARC64")) {
                arrList.add("Default");
                arrList.add("niagara");
                arrList.add("sun4u");
                arrList.add("sun4v");
                arrList.add("none");

            }

        }

        ArrayAdapter<String> machineTypeAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        machineTypeAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mMachineType.setAdapter(machineTypeAdapter);

        this.mMachineType.invalidate();
        int pos = machineTypeAdapter.getPosition(machineType);
        if (pos >= 0) {
            this.mMachineType.setSelection(pos);
        } else {
            this.mMachineType.setSelection(0);
        }

    }

    private void populateUI() {


        ArrayList<String> arrList = new ArrayList<String>();
        arrList.add("VNC");
        if (Config.enable_SDL)
            arrList.add("SDL");
        if (Config.enable_SPICE)
            arrList.add("SPICE");

        ArrayAdapter<String> uiAdapter = new ArrayAdapter<String>(this, R.layout.custom_spinner_item, arrList);
        uiAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        this.mUI.setAdapter(uiAdapter);
        this.mUI.invalidate();
    }

    // Set File Adapters
    public void populateDiskAdapter(final Spinner spinner, final FileType fileType, final boolean createOption) {

        Thread t = new Thread(new Runnable() {
            public void run() {

                ArrayList<String> oldHDs = FavOpenHelper.getInstance(activity).getFav(fileType.toString().toLowerCase());
                int length = 0;
                if (oldHDs == null || oldHDs.size() == 0) {
                    length = 0;
                } else {
                    length = oldHDs.size();
                }

                final ArrayList<String> arraySpinner = new ArrayList<String>();
                arraySpinner.add("None");
                if (createOption)
                    arraySpinner.add("New");
                arraySpinner.add("Open");
                final int index = arraySpinner.size();
                Iterator<String> i = oldHDs.iterator();
                while (i.hasNext()) {
                    String file = (String) i.next();
                    if (file != null) {
                        arraySpinner.add(file);
                    }
                }

                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        LimboFileSpinnerAdapter adapter = new LimboFileSpinnerAdapter(activity, R.layout.custom_spinner_item, arraySpinner, index);
                        adapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        spinner.setAdapter(adapter);
                        spinner.invalidate();
                    }
                });
            }
        });
        t.start();
    }

    private void populateSnapshot() {
        Thread t = new Thread(new Runnable() {
            public void run() {
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

                final ArrayList<String> arraySpinner = new ArrayList<String>();
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

                final ArrayList<String> finalOldSnapshots = oldSnapshots;
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        ArrayAdapter<String> snapshotAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
                        snapshotAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        mSnapshot.setAdapter(snapshotAdapter);
                        mSnapshot.invalidate();

                        if (finalOldSnapshots == null) {
                            mSnapshot.setEnabled(false);
                        } else {
                            mSnapshot.setEnabled(true);
                        }
                    }
                });


            }
        });
        t.start();


    }


    public Intent getVNCIntent() {
        return new Intent(LimboActivity.this, com.max2idea.android.limbo.main.LimboVNCActivity.class);

    }

    public enum FileType {
        HDA, HDB, HDC,HDD, CD,FDA,FDB,SD,KERNEL, INITRD, SHARED_DIR, EXPORT_DIR, IMAGE_DIR, LOG_DIR, IMPORT_FILE
    }

    private void addDriveToList(String file, FileType type) {

        if (file == null)
            return;

        int res = FavOpenHelper.getInstance(activity).getFavSeq(file, type.toString().toLowerCase());
        if (res == -1) {
            if (type == FileType.HDA) {
                this.mHDA.getAdapter().getCount();
            } else if (type == FileType.HDB) {
                this.mHDB.getAdapter().getCount();
            } else if (type == FileType.HDC) {
                this.mHDC.getAdapter().getCount();
            } else if (type == FileType.HDD) {
                this.mHDD.getAdapter().getCount();
            } else if (type == FileType.SHARED_DIR) {
                this.mSharedFolder.getAdapter().getCount();
            } else if (type == FileType.CD) {
                this.mCD.getAdapter().getCount();
            } else if (type == FileType.FDA) {
                this.mFDA.getAdapter().getCount();
            } else if (type == FileType.FDB) {
                this.mFDB.getAdapter().getCount();
            } else if (type == FileType.SD) {
                this.mSD.getAdapter().getCount();
            }
            if (file != null && !file.equals("")) {
                FavOpenHelper.getInstance(activity).insertFav(file, type.toString().toLowerCase());
            }
        }

    }


    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        this.invalidateOptionsMenu();
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

        menu.add(0, HELP, 0, "Help").setIcon(R.drawable.help);
        menu.add(0, INSTALL, 0, "Install Roms").setIcon(R.drawable.install);
        menu.add(0, CREATE, 0, "Create machine").setIcon(R.drawable.machinetype);
        menu.add(0, DELETE, 0, "Delete Machine").setIcon(R.drawable.delete);
        menu.add(0, SETTINGS, 0, "Settings").setIcon(R.drawable.settings);
        if (currMachine != null && currMachine.paused == 1)
            menu.add(0, DISCARD_VM_STATE, 0, "Discard Saved State").setIcon(R.drawable.close);
        menu.add(0, EXPORT, 0, "Export Machines").setIcon(R.drawable.exportvms);
        menu.add(0, IMPORT, 0, "Import Machines").setIcon(R.drawable.importvms);
        menu.add(0, VIEWLOG, 0, "View Log").setIcon(android.R.drawable.ic_menu_view);
        menu.add(0, HELP, 0, "Help").setIcon(R.drawable.help);
        menu.add(0, CHANGELOG, 0, "Changelog").setIcon(android.R.drawable.ic_menu_help);
        menu.add(0, LICENSE, 0, "License").setIcon(android.R.drawable.ic_menu_help);
        menu.add(0, QUIT, 0, "Exit").setIcon(android.R.drawable.ic_lock_power_off);


        int maxMenuItemsShown = 3;
        int actionShow = MenuItemCompat.SHOW_AS_ACTION_IF_ROOM;
        if(UIUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 4;
            actionShow = MenuItemCompat.SHOW_AS_ACTION_ALWAYS;
        }

        for (int i = 0; i < menu.size() && i < maxMenuItemsShown; i++) {
            MenuItemCompat.setShowAsAction(menu.getItem(i), actionShow);
        }

        return true;

    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {

        super.onOptionsItemSelected(item);
        if (item.getItemId() == this.INSTALL) {
            this.install(true);
        } else if (item.getItemId() == this.DELETE) {
            this.promptDeleteMachine();
        } else if (item.getItemId() == this.DISCARD_VM_STATE) {
                promptDiscardVMState();
        } else if (item.getItemId() == this.CREATE) {
            this.promptMachineName(this);
        } else if (item.getItemId() == this.SETTINGS) {
            this.goToSettings();
        } else if (item.getItemId() == this.EXPORT) {
            this.onExportMachines();
        } else if (item.getItemId() == this.IMPORT) {
            this.onImportMachines();
        } else if (item.getItemId() == this.HELP) {
            UIUtils.onHelp(this);
        } else if (item.getItemId() == this.VIEWLOG) {
            this.onViewLog();
        } else if (item.getItemId() == this.CHANGELOG) {
            UIUtils.onChangeLog(activity);
        } else if (item.getItemId() == this.LICENSE) {
            this.onLicense();
        } else if (item.getItemId() == this.QUIT) {
            this.exit();
        }
        return true;
    }

    private void goToSettings() {
        Intent i = new Intent(this, LimboSettingsManager.class);
        activity.startActivity(i);
    }

    public void onViewLog() {

        Thread t = new Thread(new Runnable() {
            public void run() {
                FileUtils.viewLimboLog(activity);
            }
        });
        t.start();
    }

    private void goToURL(String url) {

        Intent i = new Intent(Intent.ACTION_VIEW);
        i.setData(Uri.parse(url));
        activity.startActivity(i);

    }

    public void promptDeleteMachine() {
        if(currMachine == null) {
            UIUtils.toastShort(this, "No Machine selected");
            return;
        }
        new AlertDialog.Builder(this).setTitle("Delete VM: " + currMachine.machinename)
                .setMessage("Delete VM? Only machine definition and state will be deleted, disk images files will not be deleted")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        onDeleteMachine();
                    }
                }).setNegativeButton("No", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }


    public void promptDiscardVMState() {
        new AlertDialog.Builder(this).setTitle("Discard VM State")
                .setMessage("The VM is Paused. If you discard the state you might lose data. Continue?")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        currMachine.paused = 0;
                        MachineOpenHelper.getInstance(activity).update(currMachine,
                                MachineOpenHelper.PAUSED, 0 + "");
                        changeStatus(VMStatus.Ready);
                        enableNonRemovableDeviceOptions(true);
                        enableRemovableDeviceOptions(true);

                    }
                }).setNegativeButton("No", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }

    public void stopVM(boolean exit) {
        execTimer();
        if (vmexecutor == null && !exit) {
            if (this.currMachine != null && this.currMachine.paused == 1) {
                promptDiscardVMState();
                return;
            } else {

                UIUtils.toastShort(LimboActivity.this, "VM not running");
                return;
            }
        }

        Machine.stopVM(activity);
    }

    public void saveSnapshotDB(String snapshot_name) {
        currMachine.snapshot_name = snapshot_name;
        MachineOpenHelper.getInstance(activity).deleteMachineDB(currMachine);
        MachineOpenHelper.getInstance(activity).insertMachine(currMachine);
        if (((ArrayAdapter<String>) mSnapshot.getAdapter()).getPosition(snapshot_name) < 0) {
            ((ArrayAdapter<String>) mSnapshot.getAdapter()).add(snapshot_name);
        }
    }

    public void stopTimeListener() {

        synchronized (this.lockTime) {
            this.timeQuit = true;
            this.lockTime.notifyAll();
        }
    }

    public void onPause() {
        View currentView = getCurrentFocus();
        if (currentView != null && currentView instanceof EditText) {
            ((EditText) currentView).setFocusable(false);
        }
        super.onPause();
        this.stopTimeListener();
    }

    public void onResume() {

        super.onResume();
        updateValues();
        execTimer();
    }

    private void updateValues() {

        Thread t = new Thread(new Runnable() {
            public void run() {
                checkAndUpdateStatus(true);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateRemovableDiskValues();
                        updateGlobalSettings();
                        updateSummary(false);
                    }
                });
            }
        });
        t.start();
    }

    private void updateGlobalSettings() {
        final boolean enableFullscreen = LimboSettingsManager.getFullscreen(activity);
        final boolean enablehighPriority = LimboSettingsManager.getPrio(activity);
        final boolean enableDesktopMode = LimboSettingsManager.getDesktopMode(activity);
        final boolean enableShowToolbar = LimboSettingsManager.getAlwaysShowMenuToolbar(activity);

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mFullScreen.setChecked(enableFullscreen);
                mPrio.setChecked(enablehighPriority);
                mDesktopMode.setChecked(enableDesktopMode);
                if(enableDesktopMode) {
                    Config.mouseMode = Config.MouseMode.External;
                } else
                    Config.mouseMode = Config.MouseMode.Trackpad;
                mToolBar.setChecked(enableShowToolbar);

            }
        });

    }

    private void updateRemovableDiskValues() {
        if(currMachine!=null) {
            disableRemovableDiskListeners();
            this.updateDrive(FileType.CD, currMachine.cd_iso_path);
            this.updateDrive(FileType.FDA, currMachine.fda_img_path);
            this.updateDrive(FileType.FDB, currMachine.fdb_img_path);
            this.updateDrive(FileType.SD, currMachine.sd_img_path);
            enableRemovableDiskListeners();
        }
    }

    public void timer() {
        //XXX: No timers just ping a few times
        for (int i = 0; i < 3; i++) {
            checkAndUpdateStatus(false);

            try {
                Thread.sleep(1000);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }

    }

    private void checkAndUpdateStatus(boolean force) {
        if (vmexecutor != null) {
            VMStatus status = checkStatus();
            if (force || status != currStatus) {
                currStatus = status;
                changeStatus(status);
            }
        }
    }

    void execTimer() {

        Thread t = new Thread(new Runnable() {
            public void run() {
                startTimer();
            }
        });
        t.start();
    }

    public void startTimer() {
        this.stopTimeListener();

        timeQuit = false;
        try {
            timer();
        } catch (Exception ex) {
            ex.printStackTrace();

        }

    }


    public enum VMStatus {
        Ready, Stopped, Saving, Paused, Completed, Failed, Unknown, Running
    }

    private VMStatus checkStatus() {
        VMStatus state = VMStatus.Ready;
        if (vmexecutor != null && libLoaded && vmexecutor.get_state().toUpperCase().equals("RUNNING")) {
            state = VMStatus.Running;
        }
        return state;
    }

    private class InstallerTask extends AsyncTask<Void, Void, Void> {
        public boolean force;

        @Override
        protected Void doInBackground(Void... arg0) {
            onInstall(force);
            if (progDialog.isShowing()) {
                progDialog.dismiss();
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void test) {

        }
    }

    private class ExportMachines extends AsyncTask<Void, Void, Void> {

        public String exportFilePath;
        private String displayName;

        @Override
        protected Void doInBackground(Void... arg0) {

            // Export
            displayName = exportMachinesToFile(exportFilePath);

            return null;
        }

        @Override
        protected void onPostExecute(Void test) {
            if (progDialog.isShowing()) {
                progDialog.dismiss();
            }
            if(displayName != null)
            UIUtils.toastLong(LimboActivity.this, "Machines are exported in " + displayName);

        }
    }

    private class ImportMachines extends AsyncTask<Void, Void, Void> {

        private String importFilePath;
        private ArrayList<Machine> machines;
        private String displayImportFilePath;

        @Override
        protected Void doInBackground(Void... arg0) {
            // Import
            displayImportFilePath = FileUtils.getFullPathFromDocumentFilePath(importFilePath);
            machines = FileUtils.getVMsFromFile(activity, importFilePath);
            if (machines == null) {
                return null;
            }

            for (int i = 0; i < machines.size(); i++) {
                Machine machine = machines.get(i);
                if (MachineOpenHelper.getInstance(activity).getMachine(machine.machinename, "") != null) {
                    MachineOpenHelper.getInstance(activity).deleteMachineDB(machine);
                }
                MachineOpenHelper.getInstance(activity).insertMachine(machine);
                addDriveToList(machine.cd_iso_path, FileType.CD);
                addDriveToList(machine.hda_img_path,FileType.HDA );
                addDriveToList(machine.hdb_img_path, FileType.HDB);
                addDriveToList(machine.hdc_img_path, FileType.HDC);
                addDriveToList(machine.hdd_img_path, FileType.HDD);
                addDriveToList(machine.shared_folder, FileType.SHARED_DIR);
                addDriveToList(machine.fda_img_path, FileType.FDA);
                addDriveToList(machine.fdb_img_path, FileType.FDB);
                addDriveToList(machine.sd_img_path, FileType.SD);
                addDriveToList(machine.kernel, FileType.KERNEL);
                addDriveToList(machine.initrd, FileType.INITRD);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void test) {
            if (progDialog.isShowing()) {
                progDialog.dismiss();
            }
            if(machines!=null) {
                UIUtils.toastLong(LimboActivity.this, "Machines are imported from " + displayImportFilePath);
                populateAttributes();
                enableListeners();
                enableRemovableDiskListeners();
            }

        }
    }

    private class DiskInfo {
        public CheckBox enableCheckBox;
        public Spinner spinner;
        public String colName;
        ;

        public DiskInfo(Spinner spinner, CheckBox enableCheckbox, String dbColName) {
            this.spinner = spinner;
            this.enableCheckBox = enableCheckbox;
            this.colName = dbColName;
        }
    }
}
