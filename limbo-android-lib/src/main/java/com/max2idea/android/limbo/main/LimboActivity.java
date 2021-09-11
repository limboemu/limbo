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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.res.Configuration;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.Display;
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
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.widget.NestedScrollView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.dialog.DialogUtils;
import com.max2idea.android.limbo.files.FileInstaller;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.help.Help;
import com.max2idea.android.limbo.install.Installer;
import com.max2idea.android.limbo.keyboard.KeyboardUtils;
import com.max2idea.android.limbo.links.LinksManager;
import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.machine.ArchDefinitions;
import com.max2idea.android.limbo.machine.Machine;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.machine.MachineAction;
import com.max2idea.android.limbo.machine.MachineController;
import com.max2idea.android.limbo.machine.MachineController.MachineStatus;
import com.max2idea.android.limbo.machine.MachineExporter;
import com.max2idea.android.limbo.machine.MachineFilePaths;
import com.max2idea.android.limbo.machine.MachineImporter;
import com.max2idea.android.limbo.machine.MachineProperty;
import com.max2idea.android.limbo.network.NetworkUtils;
import com.max2idea.android.limbo.toast.ToastUtils;
import com.max2idea.android.limbo.ui.SpinnerAdapter;
import com.max2idea.android.limbo.updates.UpdateChecker;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Observable;
import java.util.Observer;

public class LimboActivity extends AppCompatActivity
        implements MachineController.OnMachineStatusChangeListener,
        MachineController.OnEventListener, Observer {

    private static final String TAG = "LimboActivity";

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
    private static final int DISCARD_VM_STATE = 11;
    private static final int SETTINGS = 13;
    private static final int TOOLS = 14;

    // disk mapping
    private static final Hashtable<FileType, DiskInfo> diskMapping = new Hashtable<>();

    private static boolean libLoaded;
    public View parent;
    private boolean machineLoaded;
    private FileType browseFileType = null;

    //Widgets
    private ImageView mStatus;
    private EditText mDNS;
    private EditText mHOSTFWD;
    private EditText mAppend;
    private EditText mExtraParams;
    private TextView mStatusText;
    private Spinner mMachine;
    private Spinner mCPU;
    private Spinner mMachineType;
    private Spinner mCPUNum;
    private Spinner mKernel;
    private Spinner mInitrd;
    // HDD
    private Spinner mHDA;
    private Spinner mHDB;
    private Spinner mHDC;
    private Spinner mHDD;
    private Spinner mSharedFolder;

    //removable
    private Spinner mCD;
    private Spinner mFDA;
    private Spinner mFDB;
    private Spinner mSD;
    private CheckBox mCDenable;
    private CheckBox mFDAenable;
    private CheckBox mFDBenable;
    private CheckBox mSDenable;

    // misc
    private Spinner mRamSize;
    private Spinner mBootDevices;
    private Spinner mNetworkCard;
    private Spinner mNetConfig;
    private Spinner mVGAConfig;
    private Spinner mSoundCard;
    private Spinner mUI;
    private CheckBox mDisableACPI;
    private CheckBox mDisableHPET;
    private CheckBox mDisableTSC;
    private CheckBox mEnableKVM;
    private CheckBox mEnableMTTCG;
    private Spinner mKeyboard;
    private Spinner mMouse;

    // buttons
    private ImageButton mStart;
    private ImageButton mPause;
    private ImageButton mStop;
    private ImageButton mRestart;

    //sections
    private LinearLayout mCPUSectionDetails;
    private LinearLayout mStorageSectionDetails;
    private LinearLayout mUserInterfaceSectionDetails;
    private LinearLayout mAdvancedSectionDetails;
    private LinearLayout mBootSectionDetails;
    private LinearLayout mGraphicsSectionDetails;
    private LinearLayout mRemovableStorageSectionDetails;
    private LinearLayout mNetworkSectionDetails;
    private LinearLayout mAudioSectionDetails;

    //summary
    private TextView mUISectionSummary;
    private TextView mCPUSectionSummary;
    private TextView mStorageSectionSummary;
    private TextView mRemovableStorageSectionSummary;
    private TextView mGraphicsSectionSummary;
    private TextView mAudioSectionSummary;
    private TextView mNetworkSectionSummary;
    private TextView mBootSectionSummary;
    private TextView mAdvancedSectionSummary;

    //layouts
    private NestedScrollView mScrollView;
    private boolean firstMTTCGCheck;
    private ViewListener viewListener;

    public void changeStatus(final MachineStatus status_changed) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (MachineController.getInstance().isRunning() || status_changed == MachineStatus.Running) {
                    mStatus.setImageResource(R.drawable.on);
                    if (mUI.getSelectedItemPosition() == 0) {
                        // VNC
                        mStatusText.setText(R.string.Running);
                        //XXX: we block the user from changing the drives
                        // from this activitybecause sdl is suspended and the thread will block
                        // so they have to change it from within the SDL Activity
                        enableRemovableDiskValues(true);
                    } else {
                        // SDL is always suspend in the background
                        mStatusText.setText(R.string.Suspended);
                        enableRemovableDiskValues(false);
                    }
                    unlockRemovableDevices(false);
                    enableNonRemovableDeviceOptions(false);
                    mMachine.setEnabled(false);
                } else if (status_changed == MachineStatus.Ready || status_changed == MachineStatus.Stopped) {
                    mStatus.setImageResource(R.drawable.off);
                    mStatusText.setText(R.string.Stopped);
                    unlockRemovableDevices(true);
                    enableRemovableDiskValues(true);
                    enableNonRemovableDeviceOptions(true);
                } else if (status_changed == MachineStatus.Saving) {
                    mStatus.setImageResource(R.drawable.on);
                    mStatusText.setText(R.string.savingState);
                    unlockRemovableDevices(false);
                    enableRemovableDiskValues(false);
                    enableNonRemovableDeviceOptions(false);
                } else if (status_changed == MachineStatus.Paused) {
                    mStatus.setImageResource(R.drawable.on);
                    mStatusText.setText(R.string.paused);
                    unlockRemovableDevices(false);
                    enableRemovableDiskValues(false);
                    enableNonRemovableDeviceOptions(false);
                }
            }
        });
    }

    private void onTap() {
        String userid = LimboApplication.getUserId(this);
        if (!(new File("/dev/net/tun")).exists()) {
            LimboActivityCommon.tapNotSupported(this, userid);
            return;
        }
        LimboActivityCommon.promptTap(this, userid);
    }

    public void setUserPressed(boolean pressed) {
        if (pressed) {
            setupMiscOptions();
            setupNonRemovableDiskListeners();
            enableRemovableDiskListeners();
        } else {
            disableListeners();
            disableRemovableDiskListeners();
        }
    }

    private void disableRemovableDiskListeners() {
        disableRemovableDiskListener(mCDenable, mCD);
        disableRemovableDiskListener(mCDenable, mCD);
        disableRemovableDiskListener(mCDenable, mCD);
        disableRemovableDiskListener(mCDenable, mCD);
    }

    private void disableRemovableDiskListener(CheckBox enableDrive, Spinner spinner) {
        enableDrive.setOnCheckedChangeListener(null);
        spinner.setOnItemSelectedListener(null);
    }

    private void enableRemovableDiskListeners() {
        enableRemovableDiskListener(mCD, mCDenable, MachineProperty.CDROM, FileType.CDROM);
        enableRemovableDiskListener(mFDA, mFDAenable, MachineProperty.FDA, FileType.FDA);
        enableRemovableDiskListener(mFDB, mFDBenable, MachineProperty.FDB, FileType.FDB);
        enableRemovableDiskListener(mSD, mSDenable, MachineProperty.SD, FileType.SD);
    }

    private void enableRemovableDiskListener(final Spinner spinner, final CheckBox driveEnable,
                                             final MachineProperty driveName,
                                             final FileType fileType) {
        spinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String value = (String) ((ArrayAdapter<?>) spinner.getAdapter()).getItem(position);
                if (position == 1 && driveEnable.isChecked()) {
                    browseFileType = fileType;
                    LimboFileManager.browse(LimboActivity.this, browseFileType, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    spinner.setSelection(0);
                } else {
                    notifyFieldChange(MachineProperty.REMOVABLE_DRIVE, new Object[]{driveName, value});
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
        driveEnable.setOnCheckedChangeListener(
                new OnCheckedChangeListener() {
                    public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                        spinner.setEnabled(isChecked);
                        notifyFieldChange(MachineProperty.DRIVE_ENABLED, new Object[]{driveName, isChecked});
                        triggerUpdateSpinner(spinner);
                    }

                }
        );
    }


    private void setupMiscOptions() {

        mCPU.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String cpu = (String) ((ArrayAdapter<?>) mCPU.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.CPU, cpu);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mMachineType.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String machineType = (String) ((ArrayAdapter<?>) mMachineType.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.MACHINETYPE, machineType);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mUI.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String ui = (String) ((ArrayAdapter<?>) mUI.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.UI, ui);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mCPUNum.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                final String cpuNum = (String) ((ArrayAdapter<?>) mCPUNum.getAdapter()).getItem(position);
                if (position > 0 && getMachine().getEnableMTTCG() != 1 && getMachine().getEnableKVM() != 1 && !firstMTTCGCheck) {
                    firstMTTCGCheck = true;
                    promptMultiCPU(cpuNum);
                } else {
                    notifyFieldChange(MachineProperty.CPUNUM, cpuNum);
                }
                mDisableTSC.setChecked(position > 0 && (LimboApplication.arch == Config.Arch.x86 ||
                        LimboApplication.arch == Config.Arch.x86_64));
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mRamSize.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String ram = (String) ((ArrayAdapter<?>) mRamSize.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.MEMORY, ram);
            }

            public void onNothingSelected(AdapterView<?> parentView) {

            }
        });

        mKernel.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String kernel = (String) ((ArrayAdapter<?>) mKernel.getAdapter()).getItem(position);
                if (position == 0) {
                    notifyFieldChange(MachineProperty.KERNEL, null);
                } else if (position == 1) {
                    browseFileType = FileType.KERNEL;
                    LimboFileManager.browse(LimboActivity.this, browseFileType, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mKernel.setSelection(0);
                } else if (position > 1) {
                    notifyFieldChange(MachineProperty.KERNEL, kernel);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mInitrd.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String initrd = (String) ((ArrayAdapter<?>) mInitrd.getAdapter()).getItem(position);
                if (position == 0) {
                    notifyFieldChange(MachineProperty.INITRD, initrd);
                } else if (position == 1) {
                    browseFileType = FileType.INITRD;
                    LimboFileManager.browse(LimboActivity.this, browseFileType, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mInitrd.setSelection(0);
                } else if (position > 1) {
                    notifyFieldChange(MachineProperty.INITRD, initrd);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mBootDevices.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;

                String bootDev = (String) ((ArrayAdapter<?>) mBootDevices.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.BOOT_CONFIG, bootDev);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mNetConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;

                String netcfg = (String) ((ArrayAdapter<?>) mNetConfig.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.NETCONFIG, netcfg);
                if (position > 0 && getMachine().getPaused() == 0
                        && MachineController.getInstance().getCurrStatus() != MachineStatus.Running) {
                    mNetworkCard.setEnabled(true);
                    mDNS.setEnabled(true);
                    mHOSTFWD.setEnabled(true);
                } else {
                    mNetworkCard.setEnabled(false);
                    mDNS.setEnabled(false);
                    mHOSTFWD.setEnabled(false);
                }

                if (netcfg.equals("TAP")) {
                    onTap();
                } else if (netcfg.equals("User")) {
                    LimboActivityCommon.onNetworkUser(LimboActivity.this);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mNetworkCard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                if (position < 0 || position >= mNetworkCard.getCount()) {
                    mNetworkCard.setSelection(0);
                    return;
                }
                String niccfg = (String) ((ArrayAdapter<?>) mNetworkCard.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.NICCONFIG, niccfg);
            }

            public void onNothingSelected(final AdapterView<?> parentView) {
            }
        });

        mVGAConfig.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String vgacfg = (String) ((ArrayAdapter<?>) mVGAConfig.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.VGA, vgacfg);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mSoundCard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String sndcfg = (String) ((ArrayAdapter<?>) mSoundCard.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.SOUNDCARD, sndcfg);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mDisableACPI.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if (getMachine() == null)
                    return;
                notifyFieldChange(MachineProperty.DISABLE_ACPI, isChecked);
            }
        });

        mDisableHPET.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if (getMachine() == null)
                    return;
                notifyFieldChange(MachineProperty.DISABLE_HPET, isChecked);
            }
        });

        mDisableTSC.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if (getMachine() == null)
                    return;
                notifyFieldChange(MachineProperty.DISABLE_TSC, isChecked);
            }
        });

        mDNS.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if (getMachine() == null)
                    return;
                if (!hasFocus) {
                    setDNSServer(mDNS.getText().toString());
                    LimboSettingsManager.setDNSServer(LimboActivity.this, mDNS.getText().toString());
                }
            }
        });

        mHOSTFWD.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if (getMachine() == null)
                    return;
                if (!hasFocus) {
                    notifyFieldChange(MachineProperty.HOSTFWD, mHOSTFWD.getText().toString());
                }
            }
        });

        mAppend.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if (getMachine() == null)
                    return;
                if (!hasFocus) {
                    notifyFieldChange(MachineProperty.APPEND, mAppend.getText().toString());
                }
            }
        });

        mExtraParams.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View view, boolean hasFocus) {
                if (getMachine() == null)
                    return;
                if (!hasFocus) {
                    notifyFieldChange(MachineProperty.EXTRA_PARAMS, mExtraParams.getText().toString());
                }
            }
        });

        OnClickListener resetClickListener = new OnClickListener() {
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
        mEnableKVM.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if (getMachine() == null)
                    return;
                if (isChecked) {
                    promptKVM();
                } else {
                    notifyFieldChange(MachineProperty.ENABLE_KVM, isChecked);
                }

            }

        });

        mEnableMTTCG.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton viewButton, boolean isChecked) {
                if (getMachine() == null)
                    return;
                if (isChecked) {
                    promptEnableMTTCG();
                } else {
                    notifyFieldChange(MachineProperty.ENABLE_MTTCG, isChecked);
                }
            }
        });

        mKeyboard.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;
                String keyboardCfg = (String) ((ArrayAdapter<?>) mKeyboard.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.KEYBOARD, keyboardCfg);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });

        mMouse.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String mouseCfg = (String) ((ArrayAdapter<?>) mMouse.getAdapter()).getItem(position);
                notifyFieldChange(MachineProperty.MOUSE, mouseCfg);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
    }

    private void setCPUOptions() {
        if (MachineController.getInstance().getCurrStatus() != MachineStatus.Running &&
                (LimboApplication.arch == Config.Arch.x86 || LimboApplication.arch == Config.Arch.x86_64)) {
            mDisableACPI.setEnabled(true);
            mDisableHPET.setEnabled(true);
            mDisableTSC.setEnabled(true);
        } else {
            mDisableACPI.setEnabled(false);
            mDisableHPET.setEnabled(false);
            mDisableTSC.setEnabled(false);
        }
    }

    private void setArchOptions() {
        if (!machineLoaded) {
            populateMachineType(getMachine().getMachineType());
            populateCPUs(getMachine().getCpu());
            populateNetDevices(getMachine().getNetworkCard());
        }
    }

    private void promptKVM() {
        DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                notifyFieldChange(MachineProperty.ENABLE_KVM, true);
                mEnableMTTCG.setChecked(false);
            }
        };

        DialogInterface.OnClickListener cancelListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mEnableKVM.setChecked(false);
                        notifyFieldChange(MachineProperty.ENABLE_KVM, false);
                    }
                };

        DialogInterface.OnClickListener helpListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mEnableKVM.setChecked(false);
                        notifyFieldChange(MachineProperty.ENABLE_KVM, false);
                        LimboActivityCommon.goToURL(LimboActivity.this, Config.kvmLink);
                    }
                };

        DialogUtils.UIAlert(LimboActivity.this, getString(R.string.EnableKVM),
                getString(R.string.EnableKVMWarning),
                16, false, getString(android.R.string.ok),
                okListener, getString(android.R.string.cancel),
                cancelListener, getString(R.string.KVMHelp), helpListener);
    }

    private void promptEnableMTTCG() {
        DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                notifyFieldChange(MachineProperty.ENABLE_MTTCG, true);
                mEnableKVM.setChecked(false);
            }
        };
        DialogInterface.OnClickListener cancelListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        notifyFieldChange(MachineProperty.ENABLE_MTTCG, false);
                        mEnableMTTCG.setChecked(false);
                    }
                };
        DialogInterface.OnClickListener helpListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mEnableMTTCG.setChecked(false);
                        notifyFieldChange(MachineProperty.ENABLE_MTTCG, false);
                        LimboActivityCommon.goToURL(LimboActivity.this, Config.faqLink);
                    }
                };
        DialogUtils.UIAlert(LimboActivity.this, getString(R.string.enableMTTCG),
                getString(R.string.enableMTTCGWarning),
                16, false, getString(android.R.string.ok), okListener,
                getString(android.R.string.cancel)
                , cancelListener, getString(R.string.mttcgHelp), helpListener);
    }

    private void promptMultiCPU(final String cpuNum) {
        DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                notifyFieldChange(MachineProperty.CPUNUM, cpuNum);
            }
        };
        DialogInterface.OnClickListener cancelListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mCPUNum.setSelection(0);
                    }
                };
        DialogInterface.OnClickListener helpListener =
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mCPUNum.setSelection(0);
                        LimboActivityCommon.goToURL(LimboActivity.this, Config.faqLink);
                    }
                };
        DialogUtils.UIAlert(LimboActivity.this, getString(R.string.multipleVCPU),
                getString(R.string.multipleVCPUWarning)
                        + ((LimboApplication.arch == Config.Arch.x86_64) ?
                        getString(R.string.disableTSCInstructions) : "")
                        + " " + getString(R.string.DoYouWantToContinue),
                16, false, getString(android.R.string.ok), okListener,
                getString(android.R.string.cancel), cancelListener, getString(R.string.vCPUHelp), helpListener);
    }

    private void setupNonRemovableDiskListeners() {
        setupNonRemovableDiskListener(mHDA, MachineProperty.HDA, FileType.HDA);
        setupNonRemovableDiskListener(mHDB, MachineProperty.HDB, FileType.HDB);
        setupNonRemovableDiskListener(mHDC, MachineProperty.HDC, FileType.HDC);
        setupNonRemovableDiskListener(mHDD, MachineProperty.HDD, FileType.HDD);
        setupSharedFolderDisk();
    }

    private void setupNonRemovableDiskListener(final Spinner diskSpinner,
                                               final MachineProperty machineDriveName,
                                               final FileType diskFileType) {
        diskSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                if (getMachine() == null)
                    return;

                String hdName = (String) ((ArrayAdapter<?>) diskSpinner.getAdapter()).getItem(position);
                if (position == 1) {
                    promptImageName(LimboActivity.this, diskFileType);
                    diskSpinner.setSelection(0);
                } else if (position == 2) {
                    browseFileType = diskFileType;
                    LimboFileManager.browse(LimboActivity.this, browseFileType, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    diskSpinner.setSelection(0);
                } else {
                    notifyFieldChange(MachineProperty.NON_REMOVABLE_DRIVE, new Object[]{machineDriveName, hdName});
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
    }


    public void setupSharedFolderDisk() {
        mSharedFolder.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {

                if (getMachine() == null)
                    return;

                String shared_folder = (String) ((ArrayAdapter<?>) mSharedFolder.getAdapter()).getItem(position);
                if (position == 0) {
                    notifyFieldChange(MachineProperty.SHARED_FOLDER, shared_folder);
                } else if (position == 1) {
                    browseFileType = FileType.SHARED_DIR;
                    LimboFileManager.browse(LimboActivity.this, browseFileType, Config.OPEN_SHARED_DIR_REQUEST_CODE);
                    mSharedFolder.setSelection(0);
                } else if (position > 1) {
                    notifyFieldChange(MachineProperty.SHARED_FOLDER, shared_folder);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
    }

    protected synchronized void setDNSServer(String string) {

        File resolvConf = new File(LimboApplication.getBasefileDir() + "/etc/resolv.conf");
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
        mUI.setOnItemSelectedListener(null);
        mKeyboard.setOnItemSelectedListener(null);
        mMouse.setOnItemSelectedListener(null);
        mMachineType.setOnItemSelectedListener(null);
        mCPU.setOnItemSelectedListener(null);
        mCPUNum.setOnItemSelectedListener(null);
        mRamSize.setOnItemSelectedListener(null);
        mDisableACPI.setOnCheckedChangeListener(null);
        mDisableHPET.setOnCheckedChangeListener(null);
        mDisableTSC.setOnCheckedChangeListener(null);
        mEnableKVM.setOnCheckedChangeListener(null);
        mEnableMTTCG.setOnCheckedChangeListener(null);
        mHDA.setOnItemSelectedListener(null);
        mHDB.setOnItemSelectedListener(null);
        mHDC.setOnItemSelectedListener(null);
        mHDD.setOnItemSelectedListener(null);
        mSharedFolder.setOnItemSelectedListener(null);
        mBootDevices.setOnItemSelectedListener(null);
        mKernel.setOnItemSelectedListener(null);
        mInitrd.setOnItemSelectedListener(null);
        mAppend.setOnFocusChangeListener(null);
        mVGAConfig.setOnItemSelectedListener(null);
        mSoundCard.setOnItemSelectedListener(null);
        mNetConfig.setOnItemSelectedListener(null);
        mNetworkCard.setOnItemSelectedListener(null);
        mDNS.setOnFocusChangeListener(null);
        mHOSTFWD.setOnFocusChangeListener(null);
        mExtraParams.setOnFocusChangeListener(null);
    }

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        clearNotifications();
        setupStrictMode();
        setContentView(R.layout.limbo_main);
        setupWidgets();
        setupController();
        setupDiskMapping();
        createListeners();
        populateAttributesUI();
        checkFirstLaunch();
        setupToolbar();
        checkUpdate();
        checkLog();
        checkAndLoadLibs();
        restore();
        setupListeners();
        addGenericOperatingSystems();
    }

    private void setupController() {
        setViewListener(LimboApplication.getViewListener());
    }

    public void setViewListener(ViewListener viewListener) {
        this.viewListener = viewListener;
    }

    private void setupListeners() {
        MachineController.getInstance().addOnStatusChangeListener(this);
        MachineController.getInstance().addOnEventListener(this);
    }

    private void restoreUI(final String machine) {
        int position = SpinnerAdapter.getItemPosition(mMachine, machine);
        mMachine.setSelection(position);
    }

    private void restore() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                if (MachineController.getInstance().isRunning()) {
                    restoreUI(MachineController.getInstance().getMachineName());
                }
            }
        }, 1000);
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
        addDiskMapping(FileType.HDA, mHDA, null, MachineProperty.HDA);
        addDiskMapping(FileType.HDB, mHDB, null, MachineProperty.HDB);
        addDiskMapping(FileType.HDC, mHDC, null, MachineProperty.HDC);
        addDiskMapping(FileType.HDD, mHDD, null, MachineProperty.HDD);
        addDiskMapping(FileType.SHARED_DIR, mSharedFolder, null, MachineProperty.SHARED_FOLDER);

        addDiskMapping(FileType.CDROM, mCD, mCDenable, MachineProperty.CDROM);
        addDiskMapping(FileType.FDA, mFDA, mFDAenable, MachineProperty.FDA);
        addDiskMapping(FileType.FDB, mFDB, mFDBenable, MachineProperty.FDB);
        addDiskMapping(FileType.SD, mSD, mSDenable, MachineProperty.SD);

        addDiskMapping(FileType.KERNEL, mKernel, null, MachineProperty.KERNEL);
        addDiskMapping(FileType.INITRD, mInitrd, null, MachineProperty.INITRD);
    }

    private void addDiskMapping(FileType fileType, Spinner spinner,
                                CheckBox enableCheckBox, MachineProperty dbColName) {
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
                    if (!MachineController.getInstance().isRunning())
                        notifyAction(MachineAction.LOAD_VM, null);
                } else if (position == 1) {
                    mMachine.setSelection(0);
                    promptMachineName(LimboActivity.this);
                } else {
                    final String machine = (String) ((ArrayAdapter<?>) mMachine.getAdapter()).getItem(position);
                    setUserPressed(false);
                    machineLoaded = true;
                    notifyAction(MachineAction.LOAD_VM, machine);
                }
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
        mPause.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                onPauseButton();
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

    private void onPauseButton() {
        if (MachineController.getInstance().isRunning()) {
            if (MachineController.getInstance().isVNCEnabled())
                LimboActivityCommon.promptPause(this, viewListener);
            else {
                LimboSDLActivity.pendingPause = true;
                startSDL();
            }
        }

    }

    private void savePendingEditText() {
        View currentView = getCurrentFocus();
        if (currentView instanceof EditText) {
            currentView.setFocusable(false);
        }
    }

    private void checkFirstLaunch() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                if (LimboSettingsManager.isFirstLaunch(LimboActivity.this)) {
                    onFirstLaunch();
                }
            }
        });
        t.start();
    }

    private void checkLog() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                if (LimboSettingsManager.getExitCode(LimboActivity.this) != Config.EXIT_SUCCESS) {
                    if (MachineController.getInstance().isRunning())
                        LimboSettingsManager.setExitCode(LimboActivity.this, Config.EXIT_UNKNOWN);
                    else
                        LimboSettingsManager.setExitCode(LimboActivity.this, Config.EXIT_SUCCESS);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            com.max2idea.android.limbo.log.Logger.promptShowLog(LimboActivity.this);
                        }
                    });
                }
            }
        });
        t.start();
    }

    //XXX: this needs to be called from the main thread otherwise
    //  qemu crashes when it is started later
    public void setupNativeLibs() {
        if (libLoaded)
            return;
        //Compatibility lib
        System.loadLibrary("compat-limbo");

        //Glib deps
        System.loadLibrary("compat-musl");

        //Glib
        System.loadLibrary("glib-2.0");

        //Pixman for qemu
        System.loadLibrary("pixman-1");

        // SDL library
        if (Config.enable_SDL) {
            System.loadLibrary("SDL2");
        }

        System.loadLibrary("compat-SDL2-ext");

        //Limbo needed for vmexecutor
        System.loadLibrary("limbo");

        // qemu arch specific lib
        loadQEMULib();

        libLoaded = true;
    }

    protected void loadQEMULib() {

    }

    public void setupToolbar() {
        Toolbar tb = findViewById(R.id.toolbar);
        setSupportActionBar(tb);

        final ActionBar ab = getSupportActionBar();
        if (ab != null) {
            ab.setHomeAsUpIndicator(R.drawable.limbo);
            ab.setDisplayShowHomeEnabled(true);
            ab.setDisplayHomeAsUpEnabled(true);
            ab.setDisplayShowCustomEnabled(true);
            ab.setDisplayShowTitleEnabled(true);
            ab.setTitle(R.string.app_name);
        }
    }

    public void checkUpdate() {
        Thread tsdl = new Thread(new Runnable() {
            public void run() {
                UpdateChecker.checkNewVersion(LimboActivity.this);
            }
        });
        tsdl.start();
    }

    private void setupStrictMode() {
        if (Config.debugStrictMode) {
            StrictMode.setThreadPolicy(
                    new StrictMode.ThreadPolicy.Builder().detectDiskReads().detectDiskWrites().detectNetwork()
                            .penaltyLog().build());
            StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().detectLeakedSqlLiteObjects()
                    .detectLeakedClosableObjects().penaltyLog()
                    .build());
        }
    }

    private void populateAttributesUI() {
        populateMachines(null);
        populateMachineType(null);
        populateCPUs(null);
        populateCPUNum();
        populateRAM();
        populateDisks();
        populateBootDevices();
        populateNet();
        populateNetDevices(null);
        populateVGA();
        populateSoundcardConfig();
        populateUI();
        populateKeyboardLayout();
        populateMouse();
    }

    private void populateDisks() {

        //disks
        populateDiskAdapter(mHDA, FileType.HDA, true);
        populateDiskAdapter(mHDB, FileType.HDB, true);
        populateDiskAdapter(mHDC, FileType.HDC, true);
        populateDiskAdapter(mHDD, FileType.HDD, true);
        populateDiskAdapter(mSharedFolder, FileType.SHARED_DIR, false);

        //removables drives
        populateDiskAdapter(mCD, FileType.CDROM, false);
        populateDiskAdapter(mFDA, FileType.FDA, false);
        populateDiskAdapter(mFDB, FileType.FDB, false);
        populateDiskAdapter(mSD, FileType.SD, false);

        //boot
        populateDiskAdapter(mKernel, FileType.KERNEL, false);
        populateDiskAdapter(mInitrd, FileType.INITRD, false);

    }

    public void onFirstLaunch() {
        promptLicense();
    }

    private void createMachine(String machineName) {
        notifyAction(MachineAction.CREATE_VM, machineName);
    }

    private void machineCreated() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                showOperatingSystems();
                populateMachines(getMachine().getName());
                enableNonRemovableDeviceOptions(true);
                enableRemovableDeviceOptions(true);
                setArchOptions();
            }
        });
    }

    protected void showOperatingSystems() {
        if (!Config.osImages.isEmpty()) {
            LinksManager manager = new LinksManager(this);
            manager.show();
        }
    }

    private void onDeleteMachine() {
        if (getMachine() == null) {
            ToastUtils.toastShort(this, getString(R.string.SelectAMachineFirst));
            return;
        }
        Thread t = new Thread(new Runnable() {
            public void run() {
                final String name = getMachine().getName();
                notifyAction(MachineAction.DELETE_VM, getMachine());
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        disableListeners();
                        disableRemovableDiskListeners();
                        mMachine.setSelection(0);
                        notifyAction(MachineAction.LOAD_VM, null);
                        populateAttributesUI();
                        ToastUtils.toastShort(LimboActivity.this, getString(R.string.MachineDeleted) + ": " + name);
                        setupMiscOptions();
                        setupNonRemovableDiskListeners();
                        enableRemovableDiskListeners();
                    }
                });
            }
        });
        t.start();

    }

    public void importMachines(String importFilePath) {
        disableListeners();
        disableRemovableDiskListeners();
        mMachine.setSelection(0);
        notifyAction(MachineAction.IMPORT_VMS, importFilePath);
    }

    private void promptLicense() {
        final PackageInfo finalPInfo = LimboApplication.getPackageInfo(this);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    LimboActivityCommon.promptLicense(LimboActivity.this, Config.APP_NAME + " v" + finalPInfo.versionName,
                            FileUtils.LoadFile(LimboActivity.this, "LICENSE", false));
                } catch (IOException e) {

                    e.printStackTrace();
                }
            }
        });

    }

    public void exit() {
        if (MachineController.getInstance().isRunning())
            onStopButton(true);
        else
            System.exit(0);
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
        if (MachineController.getInstance().isRunning())
            flag = false;

        //ui
        mUI.setEnabled(flag);
        mKeyboard.setEnabled(Config.enableKeyboardLayoutOption && flag);
        mMouse.setEnabled(Config.enableMouseOption && flag);

        // Enable everything except removable devices
        mMachineType.setEnabled(flag);
        mCPU.setEnabled(flag);
        mCPUNum.setEnabled(flag);
        mRamSize.setEnabled(flag);
        mEnableKVM.setEnabled(flag && Config.enableKVM);
        mEnableMTTCG.setEnabled(flag && Config.enableMTTCG);

        //drives
        mHDA.setEnabled(flag);
        mHDB.setEnabled(flag);
        mHDC.setEnabled(flag);
        mHDD.setEnabled(flag);
        mSharedFolder.setEnabled(flag);

        //boot
        mBootDevices.setEnabled(flag);
        mKernel.setEnabled(flag);
        mInitrd.setEnabled(flag);
        mAppend.setEnabled(flag);

        //graphics
        mVGAConfig.setEnabled(flag);

        //audio
        if (Config.enableSDLSound && getMachine() != null
                && getMachine().getEnableVNC() != 1
                && getMachine().getPaused() == 0)
            mSoundCard.setEnabled(flag);
        else
            mSoundCard.setEnabled(false);

        //net
        mNetConfig.setEnabled(flag);
        mNetworkCard.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);
        mDNS.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);
        mHOSTFWD.setEnabled(flag && mNetConfig.getSelectedItemPosition() > 0);

        //advanced
        mDisableACPI.setEnabled(flag);
        mDisableHPET.setEnabled(flag);
        mDisableTSC.setEnabled(flag);
        mExtraParams.setEnabled(flag);

    }

    // Main event function
    // Retrives values from saved preferences
    private void onStartButton() {

        if (mMachine.getSelectedItemPosition() == 0 || getMachine() == null) {
            ToastUtils.toastShort(LimboActivity.this, getString(R.string.SelectOrCreateVirtualMachineFirst));
            return;
        }
        if (!validateFiles()) {
            return;
        }

        try {
            createMachineDir(MachineController.getInstance().getMachineSaveDir());
        } catch (Exception ex) {
            ToastUtils.toastLong(LimboActivity.this, getString(R.string.Error) + ": " + ex);
            return;
        }

        // XXX: save the user defined dns server before we start the vm
        LimboSettingsManager.setDNSServer(this, mDNS.getText().toString());

        //XXX: make sure that bios files are installed in case we ran out of space in the last run
        FileInstaller.installFiles(LimboActivity.this, false);

        if (getMachine().getEnableVNC() == 1) {
            startVNC();
        } else {
            startSDL();
        }
    }

    private void createMachineDir(String dir) throws Exception {
        File destDir = new File(dir);
        if (!destDir.exists()) {
            if (!destDir.mkdirs())
                throw new Exception(getString(R.string.failToCreateMachineDirError));
        }
    }

    /**
     * Starts the SDL Activity that will later start the native process via the service.
     * This is done so that the java SDL part is initialized prior to starting the vm.
     */
    public void startSDL() {
        Intent intent = new Intent(LimboActivity.this, LimboSDLActivity.class);
        startActivityForResult(intent, Config.SDL_REQUEST_CODE);
    }

    /**
     * Start the vm with VNC Suport via the Controller which will later call the native process
     * via the service. We do this since we don't have a built-in VNC client anymore.
     */
    public void startVNC() {
        if (LimboSettingsManager.getEnableExternalVNC(this)) {
            // VNC external connections
            LimboActivityCommon.promptVNCServer(this,
                    getString(R.string.ExternalVNCEnabledWarning), viewListener);
        } else if (!LimboSettingsManager.getVNCEnablePassword(this)) {
            // VNC Password is not enabled
            LimboActivityCommon.promptVNCServer(this,
                    getString(R.string.VNCPasswordNotEnabledWarning), viewListener);
        } else if (LimboSettingsManager.getVNCEnablePassword(this)
                && LimboSettingsManager.getVNCPass(this) == null) {
            // VNC Password is missing
            ToastUtils.toastShort(this, getString(R.string.VNCPasswordMissing));
        } else {
            notifyAction(MachineAction.START_VM, null);
        }
    }

    private boolean validateFiles() {
        return FileUtils.fileValid(getMachine().getHdaImagePath())
                && FileUtils.fileValid(getMachine().getHdbImagePath())
                && FileUtils.fileValid(getMachine().getHdcImagePath())
                && FileUtils.fileValid(getMachine().getHddImagePath())
                && FileUtils.fileValid(getMachine().getFdaImagePath())
                && FileUtils.fileValid(getMachine().getFdbImagePath())
                && FileUtils.fileValid(getMachine().getSdImagePath())
                && FileUtils.fileValid(getMachine().getCdImagePath())
                && FileUtils.fileValid(getMachine().getKernel())
                && FileUtils.fileValid(getMachine().getInitRd());
    }

    private void onStopButton(boolean exitApp) {
        KeyboardUtils.hideKeyboard(this, mScrollView);
        if (MachineController.getInstance().isRunning()) {
            if (MachineController.getInstance().isVNCEnabled())
                LimboActivityCommon.promptStopVM(this, viewListener);
            else {
                LimboSDLActivity.pendingStop = true;
                startSDL();
            }
        } else {
            if (getMachine() != null
                    && MachineController.getInstance().isPaused() && !exitApp) {
                promptDiscardVMState();
            } else {
                ToastUtils.toastShort(LimboActivity.this, getString(R.string.vmNotRunning));
            }
        }
    }

    private void onRestartButton() {
        if (!MachineController.getInstance().isRunning()) {
            if (getMachine() != null && getMachine().getPaused() == 1) {
                promptDiscardVMState();
            } else {
                ToastUtils.toastShort(LimboActivity.this, getString(R.string.VMNotRunning));
            }
        }
        LimboActivityCommon.promptResetVM(this, viewListener);
    }

    public void toggleSectionVisibility(View view) {
        if (view.getVisibility() == View.VISIBLE) {
            view.setVisibility(View.GONE);
        } else if (view.getVisibility() == View.GONE || view.getVisibility() == View.INVISIBLE) {
            view.setVisibility(View.VISIBLE);
        }
    }

    public void setupWidgets() {
        setupSections();
        mScrollView = findViewById(R.id.scroll_view);
        mStatus = findViewById(R.id.statusVal);
        mStatus.setImageResource(R.drawable.off);
        mStatusText = findViewById(R.id.statusStr);

        mStart = findViewById(R.id.startvm);
        mPause = findViewById(R.id.pausevm);
        mStop = findViewById(R.id.stopvm);
        mRestart = findViewById(R.id.restartvm);

        //Machine
        mMachine = findViewById(R.id.machineval);
        if (MachineController.getInstance().isRunning())
            mMachine.setEnabled(false);

        //ui
        if (!Config.enable_SDL)
            mUI.setEnabled(false);

        mKeyboard = findViewById(R.id.keyboardval);
        mMouse = findViewById(R.id.mouseval);

        //cpu/board
        mCPU = findViewById(R.id.cpuval);
        mMachineType = findViewById(R.id.machinetypeval);
        mCPUNum = findViewById(R.id.cpunumval);
        mUI = findViewById(R.id.uival);
        mRamSize = findViewById(R.id.rammemval);
        mEnableKVM = findViewById(R.id.enablekvmval);
        mEnableMTTCG = findViewById(R.id.enablemttcgval);
        mDisableACPI = findViewById(R.id.acpival);
        mDisableHPET = findViewById(R.id.hpetval);
        mDisableTSC = findViewById(R.id.tscval);

        //disks
        mHDA = findViewById(R.id.hdimgval);
        mHDB = findViewById(R.id.hdbimgval);
        mHDC = findViewById(R.id.hdcimgval);
        mHDD = findViewById(R.id.hddimgval);

        LinearLayout sharedFolderLayout = findViewById(R.id.sharedfolderl);
        if (!Config.enableSharedFolder)
            sharedFolderLayout.setVisibility(View.GONE);
        mSharedFolder = findViewById(R.id.sharedfolderval);

        //Removable storage
        mCD = findViewById(R.id.cdromimgval);
        mFDA = findViewById(R.id.floppyimgval);
        mFDB = findViewById(R.id.floppybimgval);
        if (!Config.enableEmulatedFloppy) {
            LinearLayout mFDALayout = findViewById(R.id.floppyimgl);
            mFDALayout.setVisibility(View.GONE);
            LinearLayout mFDBLayout = findViewById(R.id.floppybimgl);
            mFDBLayout.setVisibility(View.GONE);
        }
        mSD = findViewById(R.id.sdcardimgval);
        if (!Config.enableEmulatedSDCard) {
            LinearLayout mSDCardLayout = findViewById(R.id.sdcardimgl);
            mSDCardLayout.setVisibility(View.GONE);
        }
        mCDenable = findViewById(R.id.cdromimgcheck);
        mFDAenable = findViewById(R.id.floppyimgcheck);
        mFDBenable = findViewById(R.id.floppybimgcheck);
        mSDenable = findViewById(R.id.sdcardimgcheck);

        //boot
        mBootDevices = findViewById(R.id.bootfromval);
        mKernel = findViewById(R.id.kernelval);
        mInitrd = findViewById(R.id.initrdval);
        mAppend = findViewById(R.id.appendval);

        //display
        mVGAConfig = findViewById(R.id.vgacfgval);

        //sound
        mSoundCard = findViewById(R.id.soundcfgval);

        //network
        mNetConfig = findViewById(R.id.netcfgval);
        mNetworkCard = findViewById(R.id.netDevicesVal);
        mDNS = findViewById(R.id.dnsval);
        setDefaultDNServer();
        mHOSTFWD = findViewById(R.id.hostfwdval);

        // advanced
        mExtraParams = findViewById(R.id.extraparamsval);

        disableFeatures();
        enableRemovableDeviceOptions(false);
        enableNonRemovableDeviceOptions(false);
    }

    private void disableFeatures() {

        LinearLayout mAudioSectionLayout = findViewById(R.id.audiosectionl);
        if (!Config.enableSDLSound) {
            mAudioSectionLayout.setVisibility(View.GONE);
        }

        LinearLayout mDisableTSCLayout = findViewById(R.id.tscl);
        LinearLayout mDisableACPILayout = findViewById(R.id.acpil);
        LinearLayout mDisableHPETLayout = findViewById(R.id.hpetl);
        LinearLayout mEnableKVMLayout = findViewById(R.id.kvml);

        if (LimboApplication.arch != Config.Arch.x86 && LimboApplication.arch != Config.Arch.x86_64) {
            mDisableTSCLayout.setVisibility(View.GONE);
            mDisableACPILayout.setVisibility(View.GONE);
            mDisableHPETLayout.setVisibility(View.GONE);
        }
        if (LimboApplication.arch != Config.Arch.x86 && LimboApplication.arch != Config.Arch.x86_64
                && LimboApplication.arch != Config.Arch.arm && LimboApplication.arch != Config.Arch.arm64) {
            mEnableKVMLayout.setVisibility(View.GONE);
        }
    }

    private void setDefaultDNServer() {

        Thread thread = new Thread(new Runnable() {
            public void run() {
                final String defaultDNSServer = LimboSettingsManager.getDNSServer(LimboActivity.this);
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
            mCPUSectionDetails = findViewById(R.id.cpusectionDetails);
            mCPUSectionDetails.setVisibility(View.GONE);
            mCPUSectionSummary = findViewById(R.id.cpusectionsummaryStr);
            LinearLayout mCPUSectionHeader = findViewById(R.id.cpusectionheaderl);
            mCPUSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mCPUSectionDetails);
                    enableListenersDelayed();
                }
            });

            mStorageSectionDetails = findViewById(R.id.storagesectionDetails);
            mStorageSectionDetails.setVisibility(View.GONE);
            mStorageSectionSummary = findViewById(R.id.storagesectionsummaryStr);
            LinearLayout mStorageSectionHeader = findViewById(R.id.storageheaderl);
            mStorageSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mStorageSectionDetails);
                    enableListenersDelayed();
                }
            });

            mUserInterfaceSectionDetails = findViewById(R.id.userInterfaceDetails);
            mUserInterfaceSectionDetails.setVisibility(View.GONE);
            mUISectionSummary = findViewById(R.id.uisectionsummaryStr);
            LinearLayout mUserInterfaceSectionHeader = findViewById(R.id.userinterfaceheaderl);
            mUserInterfaceSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mUserInterfaceSectionDetails);
                    enableListenersDelayed();
                }
            });


            mRemovableStorageSectionDetails = findViewById(R.id.removableStoragesectionDetails);
            mRemovableStorageSectionDetails.setVisibility(View.GONE);
            mRemovableStorageSectionSummary = findViewById(R.id.removablesectionsummaryStr);
            LinearLayout mRemovableStorageSectionHeader = findViewById(R.id.removablestorageheaderl);
            mRemovableStorageSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mRemovableStorageSectionDetails);
                    enableListenersDelayed();
                }
            });

            mGraphicsSectionDetails = findViewById(R.id.graphicssectionDetails);
            mGraphicsSectionDetails.setVisibility(View.GONE);
            mGraphicsSectionSummary = findViewById(R.id.graphicssectionsummaryStr);
            LinearLayout mGraphicsSectionHeader = findViewById(R.id.graphicsheaderl);
            mGraphicsSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mGraphicsSectionDetails);
                    enableListenersDelayed();
                }
            });
            mAudioSectionDetails = findViewById(R.id.audiosectionDetails);
            mAudioSectionDetails.setVisibility(View.GONE);
            mAudioSectionSummary = findViewById(R.id.audiosectionsummaryStr);
            LinearLayout mAudioSectionHeader = findViewById(R.id.audioheaderl);
            mAudioSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mAudioSectionDetails);
                    enableListenersDelayed();
                }
            });

            mNetworkSectionDetails = findViewById(R.id.networksectionDetails);
            mNetworkSectionDetails.setVisibility(View.GONE);
            mNetworkSectionSummary = findViewById(R.id.networksectionsummaryStr);
            View mNetworkSectionHeader = findViewById(R.id.networkheaderl);
            mNetworkSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mNetworkSectionDetails);
                    enableListenersDelayed();
                }
            });

            mBootSectionDetails = findViewById(R.id.bootsectionDetails);
            mBootSectionDetails.setVisibility(View.GONE);
            mBootSectionSummary = findViewById(R.id.bootsectionsummaryStr);
            View mBootSectionHeader = findViewById(R.id.bootheaderl);
            mBootSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mBootSectionDetails);
                    enableListenersDelayed();
                }
            });

            mAdvancedSectionDetails = findViewById(R.id.advancedSectionDetails);
            mAdvancedSectionDetails.setVisibility(View.GONE);
            mAdvancedSectionSummary = findViewById(R.id.advancedsectionsummaryStr);
            LinearLayout mAdvancedSectionHeader = findViewById(R.id.advancedheaderl);
            mAdvancedSectionHeader.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    disableListeners();
                    disableRemovableDiskListeners();
                    toggleSectionVisibility(mAdvancedSectionDetails);
                    enableListenersDelayed();
                }
            });
        }
    }

    private void enableListenersDelayed() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                setupMiscOptions();
                setupNonRemovableDiskListeners();
                enableRemovableDiskListeners();
            }
        }, 500);
    }

    public void updateUISummary(boolean clear) {
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mUISectionSummary.setText("");
        else {
            String text = getString(R.string.display) + ": " + (getMachine().getEnableVNC() == 1 ? "VNC" : "SDL");
            if (getMachine().getEnableVNC() == 1) {
                text += ", " + getString(R.string.server);
                text += ": " + NetworkUtils.getVNCAddress(this) + ":" + Config.defaultVNCPort;
            }
            if (getMachine().getKeyboard() != null) {
                text += ", " + getString(R.string.keyboard) + ": " + getMachine().getKeyboard();
            }
            if (getMachine().getMouse() != null) {
                text += ", " + getString(R.string.mouse) + ": " + getMachine().getMouse();
            }
            mUISectionSummary.setText(text);
        }
    }

    private Machine getMachine() {
        return MachineController.getInstance().getMachine();
    }

    public void updateCPUSummary(boolean clear) {
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mCPUSectionSummary.setText("");
        else {
            String text = "Machine Type: " + getMachine().getMachineType()
                    + ", CPU: " + getMachine().getCpu()
                    + ", " + getMachine().getCpuNum() + " CPU" + ((getMachine().getCpuNum() > 1) ? "s" : "")
                    + ", " + getMachine().getMemory() + " MB";
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
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mStorageSectionSummary.setText("");
        else {
            String text = null;
            text = appendDriveFilename(getMachine().getHdaImagePath(), text, "HDA", false);
            text = appendDriveFilename(getMachine().getHdbImagePath(), text, "HDB", false);
            text = appendDriveFilename(getMachine().getHdcImagePath(), text, "HDC", false);
            text = appendDriveFilename(getMachine().getHddImagePath(), text, "HDD", false);

            if (Config.enableSharedFolder)
                text = appendDriveFilename(getMachine().getSharedFolderPath(), text,
                        getString(R.string.SharedFolder), false);

            if (text == null || text.equals("'"))
                text = "None";
            mStorageSectionSummary.setText(text);
        }
    }

    public void updateRemovableStorageSummary(boolean clear) {
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mRemovableStorageSectionSummary.setText("");
        else {
            String text = null;

            text = appendDriveFilename(getMachine().getCdImagePath(), text, "CDROM", true);
            text = appendDriveFilename(getMachine().getFdaImagePath(), text, "FDA", true);
            text = appendDriveFilename(getMachine().getFdbImagePath(), text, "FDB", true);
            text = appendDriveFilename(getMachine().getSdImagePath(), text, "SD", true);


            if (text == null || text.equals(""))
                text = "None";

            mRemovableStorageSectionSummary.setText(text);
        }
    }

    public void updateBootSummary(boolean clear) {
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mBootSectionSummary.setText("");
        else {
            String text = "Boot from: " + getMachine().getBootDevice();
            text = appendDriveFilename(getMachine().getKernel(), text, "kernel", false);
            text = appendDriveFilename(getMachine().getInitRd(), text, "initrd", false);
            text = appendDriveFilename(getMachine().getAppend(), text, "append", false);
            mBootSectionSummary.setText(text);
        }
    }

    private String appendDriveFilename(String driveFile, String text, String drive, boolean allowEmptyDrive) {

        String file = null;
        if (driveFile != null) {
            if ((driveFile.equals("") || driveFile.equals("None")) && allowEmptyDrive) {
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
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mGraphicsSectionSummary.setText("");
        else {
            String text = "Video Card: " + getMachine().getVga();
            mGraphicsSectionSummary.setText(text);
        }
    }

    public void updateAudioSummary(boolean clear) {
        if (clear || getMachine() == null
                || mMachine.getSelectedItemPosition() < 2)
            mAudioSectionSummary.setText("");
        else {
            String soundCard = getMachine().getSoundCard();
            String text = getString(R.string.AudioCard) + ": " + (soundCard != null ? soundCard : "None");
            mAudioSectionSummary.setText(text);
        }
    }

    public void updateNetworkSummary(boolean clear) {
        if (clear || getMachine() == null
                || mMachine.getSelectedItemPosition() < 2)
            mNetworkSectionSummary.setText("");
        else {
            String netCfg = getMachine().getNetwork();
            String text = getString(R.string.Network) + ": " + (netCfg != null ? netCfg : "None");
            if (netCfg != null && !netCfg.equals("None")) {
                String nicCard = getMachine().getNetworkCard();
                text += ", " + getString(R.string.NicCard) + ": " + (nicCard != null ? nicCard : "None");
                text += ", " + getString(R.string.DNSServer) + ": " + mDNS.getText();
                String hostFWD = getMachine().getHostFwd();
                if (hostFWD != null && !hostFWD.equals(""))
                    text += ", " + getString(R.string.HostForward) + ": " + hostFWD;
            }
            mNetworkSectionSummary.setText(text);
        }
    }

    public void updateAdvancedSummary(boolean clear) {
        if (clear || getMachine() == null || mMachine.getSelectedItemPosition() < 2)
            mAdvancedSectionSummary.setText("");
        else {
            String text = "";
            if (getMachine().getExtraParams() != null
                    && !getMachine().getExtraParams().equals(""))
                text = appendOption(getString(R.string.ExtraParams) + ": " +
                        getMachine().getExtraParams(), text);
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

    private void loadMachine() {

        setUserPressed(false);
        if (getMachine() == null) {
            return;
        }
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            public void run() {
                loadMachineUI();
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        postLoadMachineUI();
                    }
                }, 1000);
                setCPUOptions();
                getMachine().addObserver(LimboActivity.this);
            }
        });
    }

    private void postLoadMachineUI() {

        mFDAenable.setChecked(getMachine().getFdaImagePath() != null);
        mFDBenable.setChecked(getMachine().getFdbImagePath() != null);
        mCDenable.setChecked(getMachine().getCdImagePath() != null);
        mSDenable.setChecked(getMachine().getSdImagePath() != null);

        changeStatus(MachineController.getInstance().getCurrStatus());
        if (getMachine().getPaused() == 1) {
            enableNonRemovableDeviceOptions(false);
            enableRemovableDeviceOptions(false);
        } else {
            enableNonRemovableDeviceOptions(true);
            enableRemovableDeviceOptions(true);
        }
        setUserPressed(true);
        machineLoaded = false;
        mMachine.setEnabled(!MachineController.getInstance().isRunning());
    }

    private void loadMachineUI() {
        populateMachineType(getMachine().getMachineType());
        populateCPUs(getMachine().getCpu());
        populateNetDevices(getMachine().getNetworkCard());
        SpinnerAdapter.setDiskAdapterValue(mCPUNum, getMachine().getCpuNum() + "");
        SpinnerAdapter.setDiskAdapterValue(mRamSize, getMachine().getMemory() + "");
        seMachineDriveValue(FileType.KERNEL, getMachine().getKernel());
        seMachineDriveValue(FileType.INITRD, getMachine().getInitRd());
        if (getMachine().getAppend() != null)
            mAppend.setText(getMachine().getAppend());
        else
            mAppend.setText("");

        if (getMachine().getHostFwd() != null)
            mHOSTFWD.setText(getMachine().getHostFwd());
        else
            mHOSTFWD.setText("");

        if (getMachine().getExtraParams() != null)
            mExtraParams.setText(getMachine().getExtraParams());
        else
            mExtraParams.setText("");

        // CDROM
        seMachineDriveValue(FileType.CDROM, getMachine().getCdImagePath());

        // Floppy
        seMachineDriveValue(FileType.FDA, getMachine().getFdaImagePath());
        seMachineDriveValue(FileType.FDB, getMachine().getFdbImagePath());

        // SD Card
        seMachineDriveValue(FileType.SD, getMachine().getSdImagePath());

        // HDD
        seMachineDriveValue(FileType.HDA, getMachine().getHdaImagePath());
        seMachineDriveValue(FileType.HDB, getMachine().getHdbImagePath());
        seMachineDriveValue(FileType.HDC, getMachine().getHdcImagePath());
        seMachineDriveValue(FileType.HDD, getMachine().getHddImagePath());

        //sharedfolder
        seMachineDriveValue(FileType.SHARED_DIR, getMachine().getSharedFolderPath());

        // Advance
        SpinnerAdapter.setDiskAdapterValue(mBootDevices, getMachine().getBootDevice());
        SpinnerAdapter.setDiskAdapterValue(mNetConfig, getMachine().getNetwork());
        SpinnerAdapter.setDiskAdapterValue(mVGAConfig, getMachine().getVga());
        SpinnerAdapter.setDiskAdapterValue(mSoundCard, getMachine().getSoundCard());
        SpinnerAdapter.setDiskAdapterValue(mUI, getMachine().getEnableVNC() == 1 ? "VNC" : "SDL");
        SpinnerAdapter.setDiskAdapterValue(mMouse, fixMouseValue(getMachine().getMouse()));
        SpinnerAdapter.setDiskAdapterValue(mKeyboard, getMachine().getKeyboard());

        // motherboard settings
        mDisableACPI.setChecked(getMachine().getDisableAcpi() == 1);
        mDisableHPET.setChecked(getMachine().getDisableHPET() == 1);
        if (LimboApplication.arch == Config.Arch.x86 || LimboApplication.arch == Config.Arch.x86_64)
            mDisableTSC.setChecked(getMachine().getDisableTSC() == 1);
        mEnableKVM.setChecked(getMachine().getEnableKVM() == 1);
        mEnableMTTCG.setChecked(getMachine().getEnableMTTCG() == 1);

        enableNonRemovableDeviceOptions(true);
        enableRemovableDeviceOptions(!MachineController.getInstance().isRunning());

        if (Config.enableSDLSound) {
            mSoundCard.setEnabled(getMachine().getEnableVNC() != 1 && getMachine().getPaused() == 0);
        } else
            mSoundCard.setEnabled(false);

        mMachine.setEnabled(false);
    }

    private String fixMouseValue(String mouse) {
        if (mouse != null) {
            if (mouse.startsWith("usb-tablet"))
                mouse += " " + getString(R.string.fixesMouseParen);
        }
        return mouse;
    }

    private synchronized void updateSummary() {
        updateUISummary(false);
        updateCPUSummary(false);
        updateStorageSummary(false);
        updateRemovableStorageSummary(false);
        updateGraphicsSummary(false);
        updateAudioSummary(false);
        updateNetworkSummary(false);
        updateBootSummary(false);
        updateAdvancedSummary(false);
    }

    public void promptMachineName(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(getString(R.string.NewMachineName));
        final EditText vmNameTextView = new EditText(activity);
        vmNameTextView.setPadding(20, 20, 20, 20);
        vmNameTextView.setEnabled(true);
        vmNameTextView.setVisibility(View.VISIBLE);
        vmNameTextView.setSingleLine();
        alertDialog.setView(vmNameTextView);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Create), (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (vmNameTextView.getText().toString().trim().equals(""))
                    ToastUtils.toastShort(activity, getString(R.string.MachineNameCannotBeEmpty));
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
        alertDialog.setTitle(getString(R.string.ImageName));

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

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Create), (DialogInterface.OnClickListener) null);
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, getString(R.string.ChangeDirectory), (DialogInterface.OnClickListener) null);

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
                } else if (sizeSel == 4) {
                    templateImage = "hd20g.qcow2";
                }

                String image = imageNameView.getText().toString();
                if (image.trim().equals(""))
                    ToastUtils.toastShort(activity, getString(R.string.ImageFilenameCannotBeEmpty));
                else {
                    if (!image.endsWith(".qcow2")) {
                        image += ".qcow2";
                    }
                    String filePath = FileUtils.createImgFromTemplate(LimboActivity.this, templateImage, image, fileType);
                    if (filePath!=null) {
                        updateDrive(fileType, filePath);
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

    public void changeImagesDir() {
        ToastUtils.toastLong(LimboActivity.this, getString(R.string.chooseDirToCreateImage));
        LimboFileManager.browse(LimboActivity.this, FileType.IMAGE_DIR, Config.OPEN_IMAGE_DIR_REQUEST_CODE);
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            moveTaskToBack(true);
            return true; // return
        }

        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == Config.SDL_QUIT_RESULT_CODE) {
            if (getParent() != null) {
                getParent().finish();
            }
            finish();
            if (MachineController.getInstance().isRunning()) {
                notifyAction(MachineAction.STOP_VM, null);
            }
        } else if (requestCode == Config.OPEN_IMPORT_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMPORT_FILE_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_IMPORT_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, false);
            } else {
                file = FileUtils.getFilePathFromIntent(this, data);
            }
            if (file != null)
                importMachines(file);
        } else if (requestCode == Config.OPEN_EXPORT_DIR_REQUEST_CODE || requestCode == Config.OPEN_EXPORT_DIR_ASF_REQUEST_CODE) {
            String exportDir;
            if (requestCode == Config.OPEN_EXPORT_DIR_ASF_REQUEST_CODE) {
                exportDir = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                exportDir = FileUtils.getDirPathFromIntent(this, data);
            }
            if (exportDir != null)
                LimboSettingsManager.setExportDir(this, exportDir);
        } else if (requestCode == Config.OPEN_IMAGE_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                browseFileType = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getFilePathFromIntent(this, data);
            }
            if (file != null)
                updateDrive(browseFileType, file);
        } else if (requestCode == Config.OPEN_IMAGE_DIR_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_DIR_ASF_REQUEST_CODE) {
            String imageDir;
            if (requestCode == Config.OPEN_IMAGE_DIR_ASF_REQUEST_CODE) {
                imageDir = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                imageDir = FileUtils.getDirPathFromIntent(this, data);
            }
            if (imageDir != null)
                LimboSettingsManager.setImagesDir(this, imageDir);

        } else if (requestCode == Config.OPEN_SHARED_DIR_REQUEST_CODE || requestCode == Config.OPEN_SHARED_DIR_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_SHARED_DIR_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                browseFileType = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getDirPathFromIntent(this, data);
            }
            if (file != null) {
                updateDrive(browseFileType, file);
                LimboSettingsManager.setSharedDir(this, file);
            }
        } else if (requestCode == Config.OPEN_LOG_FILE_DIR_REQUEST_CODE || requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                file = FileUtils.getDirPathFromIntent(this, data);
            }
            if (file != null) {
                FileUtils.saveLogToFile(LimboActivity.this, file);
            }
        }

    }

    private void updateDrive(FileType fileType, String diskValue) {
        //FIXME: sometimes the array adapters try to set invalid values
        if (fileType == null || diskValue == null) {
            return;
        }
        Spinner spinner = getSpinner(fileType);
        if (!diskValue.trim().isEmpty()) {
            if (SpinnerAdapter.getPositionFromSpinner(spinner, diskValue) < 0) {
                android.widget.SpinnerAdapter adapter = spinner.getAdapter();
                if (adapter instanceof ArrayAdapter) {
                    SpinnerAdapter.addItem(spinner, diskValue);
                }
            }
            notifyAction(MachineAction.INSERT_FAV, new Object[]{diskValue, fileType});
            seMachineDriveValue(fileType, diskValue);
        }
        int res = spinner.getSelectedItemPosition();
        if (res == 1) {
            spinner.setSelection(0);
        }
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

    private MachineProperty getProperty(FileType fileType) {
        if (diskMapping.containsKey(fileType))
            return diskMapping.get(fileType).colName;
        return null;
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroy() {
        savePendingEditText();
        MachineController.getInstance().removeOnStatusChangeListener(this);
        getMachine().deleteObserver(LimboActivity.this);
        setViewListener(null);
        super.onDestroy();
    }

    private void populateRAM() {
        String[] arraySpinner = new String[4 * 256];
        arraySpinner[0] = 4 + "";
        for (int i = 1; i < arraySpinner.length; i++) {
            arraySpinner[i] = i * 8 + "";
        }
        ArrayAdapter<String> ramAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arraySpinner);
        ramAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mRamSize.setAdapter(ramAdapter);
        mRamSize.invalidate();
    }

    private void populateCPUNum() {
        String[] arraySpinner = new String[Config.MAX_CPU_NUM];
        for (int i = 0; i < arraySpinner.length; i++) {
            arraySpinner[i] = (i + 1) + "";
        }
        ArrayAdapter<String> cpuNumAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arraySpinner);
        cpuNumAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mCPUNum.setAdapter(cpuNumAdapter);
        mCPUNum.invalidate();
    }

    private void populateBootDevices() {
        ArrayList<String> bootDevicesList = new ArrayList<>();
        bootDevicesList.add("Default");
        bootDevicesList.add("CDROM");
        bootDevicesList.add("Hard Disk");
        if (Config.enableEmulatedFloppy)
            bootDevicesList.add("Floppy");

        String[] arraySpinner = bootDevicesList.toArray(new String[0]);

        ArrayAdapter<String> bootDevAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arraySpinner);
        bootDevAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mBootDevices.setAdapter(bootDevAdapter);
        mBootDevices.invalidate();
    }

    private void populateNet() {
        String[] arraySpinner = {"None", "User", "TAP"};
        ArrayAdapter<String> netAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arraySpinner);
        netAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mNetConfig.setAdapter(netAdapter);
        mNetConfig.invalidate();
    }

    private void populateVGA() {
        ArrayList<String> arrList = ArchDefinitions.getVGAValues(this);
        ArrayAdapter<String> vgaAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        vgaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mVGAConfig.setAdapter(vgaAdapter);
        mVGAConfig.invalidate();
    }

    private void populateKeyboardLayout() {
        ArrayList<String> arrList = ArchDefinitions.getKeyboardValues(this);
        ArrayAdapter<String> keyboardAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        keyboardAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mKeyboard.setAdapter(keyboardAdapter);
        mKeyboard.invalidate();
        //TODO: for now we use only English keyboard, add more layouts
        mKeyboard.setSelection(0);
    }

    private void populateMouse() {
        ArrayList<String> arrList = ArchDefinitions.getMouseValues(this);
        ArrayAdapter<String> mouseAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        mouseAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mMouse.setAdapter(mouseAdapter);
        mMouse.invalidate();
    }

    private void populateSoundcardConfig() {
        ArrayList<String> soundCards = new ArrayList<>();
        soundCards.add("None");
        soundCards.addAll(ArchDefinitions.getSoundcards(this));
        ArrayAdapter<String> sndAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, soundCards);
        sndAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mSoundCard.setAdapter(sndAdapter);
        mSoundCard.invalidate();
    }

    private void populateNetDevices(String nic) {
        ArrayList<String> networkCards = ArchDefinitions.getNetworkDevices(this);
        ArrayAdapter<String> nicCfgAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, networkCards);
        nicCfgAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mNetworkCard.setAdapter(nicCfgAdapter);
        mNetworkCard.invalidate();

        int pos = nicCfgAdapter.getPosition(nic);
        if (pos >= 0) {
            mNetworkCard.setSelection(pos);
        }
    }

    private void populateMachines(final String machineValue) {
        Thread thread = new Thread(new Runnable() {
            public void run() {
                final ArrayList<String> machinesList = ArchDefinitions.getMachineValues(LimboActivity.this);
                ArrayList<String> machinesDB = MachineController.getInstance().getStoredMachines();
                machinesList.addAll(machinesDB);
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        ArrayAdapter<String> machineAdapter = new ArrayAdapter<>(LimboActivity.this, R.layout.custom_spinner_item, machinesList);
                        machineAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        mMachine.setAdapter(machineAdapter);
                        mMachine.invalidate();
                        if (machineValue != null)
                            SpinnerAdapter.setDiskAdapterValue(mMachine, machineValue);
                    }
                });
            }
        });
        thread.setPriority(Thread.MIN_PRIORITY);
        thread.start();
    }

    private void seMachineDriveValue(FileType fileType, final String diskValue) {
        Spinner spinner = getSpinner(fileType);
        if (spinner != null)
            SpinnerAdapter.setDiskAdapterValue(spinner, diskValue);
    }

    private void populateCPUs(String cpu) {
        ArrayList<String> arrList = ArchDefinitions.getCpuValues(this);
        ArrayAdapter<String> cpuAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        cpuAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mCPU.setAdapter(cpuAdapter);
        mCPU.invalidate();
        int pos = cpuAdapter.getPosition(cpu);
        if (pos >= 0) {
            mCPU.setSelection(pos);
        }
    }

    private void populateMachineType(String machineType) {
        ArrayList<String> arrList = ArchDefinitions.getMachineTypeValues(this);

        ArrayAdapter<String> machineTypeAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        machineTypeAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mMachineType.setAdapter(machineTypeAdapter);

        mMachineType.invalidate();
        int pos = machineTypeAdapter.getPosition(machineType);
        mMachineType.setSelection(Math.max(pos, 0));

    }

    private void populateUI() {
        ArrayList<String> arrList = ArchDefinitions.getUIValues();
        ArrayAdapter<String> uiAdapter = new ArrayAdapter<>(this, R.layout.custom_spinner_item, arrList);
        uiAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        mUI.setAdapter(uiAdapter);
        mUI.invalidate();
    }

    public void populateDiskAdapter(final Spinner spinner, final FileType fileType, final boolean createOption) {
        Thread t = new Thread(new Runnable() {
            public void run() {
                ArrayList<String> oldHDs = MachineFilePaths.getRecentFilePaths(fileType);
                final ArrayList<String> arraySpinner = new ArrayList<>();
                arraySpinner.add("None");
                if (createOption)
                    arraySpinner.add("New");
                arraySpinner.add(getString(R.string.open));
                final int index = arraySpinner.size();
                if (oldHDs != null) {
                    for (String file : oldHDs) {
                        if (file != null) {
                            arraySpinner.add(file);
                        }
                    }
                }
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        SpinnerAdapter adapter = new SpinnerAdapter(LimboActivity.this, R.layout.custom_spinner_item, arraySpinner, index);
                        adapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        spinner.setAdapter(adapter);
                        spinner.invalidate();
                    }
                });
            }
        });
        t.start();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        invalidateOptionsMenu();
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.clear();
        menu.add(0, HELP, 0, R.string.help).setIcon(R.drawable.help);
        if(!MachineController.getInstance().isRunning()) {
            menu.add(0, INSTALL, 0, R.string.InstallRoms).setIcon(R.drawable.install);
            if (getMachine() != null && getMachine().getPaused() == 1)
                menu.add(0, DISCARD_VM_STATE, 0, R.string.DiscardSavedState).setIcon(R.drawable.close);
            menu.add(0, CREATE, 0, R.string.CreateMachine).setIcon(R.drawable.machinetype);
            menu.add(0, DELETE, 0, R.string.DeleteMachine).setIcon(R.drawable.delete);
            menu.add(0, EXPORT, 0, R.string.ExportMachines).setIcon(R.drawable.exportvms);
            menu.add(0, IMPORT, 0, R.string.ImportMachines).setIcon(R.drawable.importvms);
        }
        menu.add(0, SETTINGS, 0, R.string.Settings).setIcon(R.drawable.settings);
        menu.add(0, TOOLS, 0, R.string.advancedTools).setIcon(R.drawable.advanced);
        menu.add(0, VIEWLOG, 0, R.string.ViewLog).setIcon(android.R.drawable.ic_menu_view);
        menu.add(0, HELP, 0, R.string.help).setIcon(R.drawable.help);
        menu.add(0, CHANGELOG, 0, R.string.Changelog).setIcon(android.R.drawable.ic_menu_help);
        menu.add(0, LICENSE, 0, R.string.License).setIcon(android.R.drawable.ic_menu_help);
        menu.add(0, QUIT, 0, R.string.Exit).setIcon(android.R.drawable.ic_lock_power_off);


        int maxMenuItemsShown = 3;
        int actionShow = MenuItem.SHOW_AS_ACTION_IF_ROOM;
        if (isLandscapeOrientation(this)) {
            maxMenuItemsShown = 4;
            actionShow = MenuItem.SHOW_AS_ACTION_ALWAYS;
        }

        for (int i = 0; i < menu.size() && i < maxMenuItemsShown; i++) {
            menu.getItem(i).setShowAsAction(actionShow);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {

        super.onOptionsItemSelected(item);
        if (item.getItemId() == INSTALL) {
            Installer.installFiles(this, true);
        } else if (item.getItemId() == DELETE) {
            promptDeleteMachine();
        } else if (item.getItemId() == DISCARD_VM_STATE) {
            promptDiscardVMState();
        } else if (item.getItemId() == CREATE) {
            promptMachineName(this);
        } else if (item.getItemId() == SETTINGS) {
            showSettings();
        } else if (item.getItemId() == TOOLS) {
            LimboActivityCommon.goToURL(this, Config.toolsLink);
        } else if (item.getItemId() == EXPORT) {
            MachineExporter.promptExport(this);
        } else if (item.getItemId() == IMPORT) {
            MachineImporter.promptImportMachines(this);
        } else if (item.getItemId() == HELP) {
            Help.showHelp(this);
        } else if (item.getItemId() == VIEWLOG) {
            Logger.viewLimboLog(LimboActivity.this);
        } else if (item.getItemId() == CHANGELOG) {
            LimboActivityCommon.showChangelog(LimboActivity.this);
        } else if (item.getItemId() == LICENSE) {
            promptLicense();
        } else if (item.getItemId() == QUIT) {
            exit();
        }
        return true;
    }

    private void showSettings() {
        Intent i = new Intent(this, LimboSettingsManager.class);
        startActivity(i);
    }

    public void promptDeleteMachine() {
        if (getMachine() == null) {
            ToastUtils.toastShort(this, getString(R.string.NoMachineSelected));
            return;
        }
        new AlertDialog.Builder(this).setTitle(getString(R.string.DeleteVM) + ": " + getMachine().getName())
                .setMessage(R.string.deleteVMWarning)
                .setPositiveButton(getString(android.R.string.yes), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        onDeleteMachine();
                    }
                }).setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }

    public void promptDiscardVMState() {
        new AlertDialog.Builder(this).setTitle(R.string.discardVMState)
                .setMessage(R.string.discardVMInstructions)
                .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        notifyFieldChange(MachineProperty.PAUSED, 0);
                        changeStatus(MachineStatus.Ready);
                        enableNonRemovableDeviceOptions(true);
                        enableRemovableDeviceOptions(true);

                    }
                }).setNegativeButton(getString(android.R.string.no), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }

    public void onPause() {
        View currentView = getCurrentFocus();
        if (currentView instanceof EditText) {
            currentView.setFocusable(false);
        }
        super.onPause();
    }

    public void onResume() {
        super.onResume();
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                updateValues();
            }
        }, 1000);

    }

    private void updateValues() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        changeStatus(MachineController.getInstance().getCurrStatus());
                        updateRemovableDiskValues();
                        updateSummary();
                    }
                });
            }
        });
        t.start();
    }

    private void updateRemovableDiskValues() {
        if (getMachine() != null) {
            disableRemovableDiskListeners();
            updateDrive(FileType.CDROM, getMachine().getCdImagePath());
            updateDrive(FileType.FDA, getMachine().getFdaImagePath());
            updateDrive(FileType.FDB, getMachine().getFdbImagePath());
            updateDrive(FileType.SD, getMachine().getSdImagePath());
            enableRemovableDiskListeners();
        }
    }

    public boolean isLandscapeOrientation(Activity activity) {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point screenSize = new Point();
        display.getSize(screenSize);
        return screenSize.x >= screenSize.y;
    }

    @Override
    public void onMachineStatusChanged(Machine machine, MachineStatus status, Object o) {
        switch (status) {
            case SaveFailed:
                LimboActivityCommon.promptPausedErrorVM(this, (String) o, viewListener);
                break;
            case SaveCompleted:
                LimboActivityCommon.promptPausedVM(this, viewListener);
                break;
            default:
                changeStatus(status);
        }
    }

    @Override
    public void onEvent(Machine machine, MachineController.Event event, Object o) {
        switch (event) {
            case MachineCreateFailed:
                if (o instanceof Integer) {
                    ToastUtils.toastShort(LimboActivity.this, getString((int) o));
                } else if (o instanceof String) {
                    ToastUtils.toastShort(LimboActivity.this, (String) o);
                }
                break;
            case MachineCreated:
                machineCreated();
                break;
            case MachineLoaded:
                loadMachine();
                break;
            case MachinesImported:
                onMachinesImported((ArrayList<Machine>) o);
                break;
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                updateSummary();
            }
        });
    }


    private void onMachinesImported(ArrayList<Machine> machines) {
        populateAttributesUI();
        setupMiscOptions();
        setupNonRemovableDiskListeners();
        enableRemovableDiskListeners();
        updateFavAdapters();
        LimboActivityCommon.promptMachinesImported(this, machines);
    }

    private void updateFavAdapters() {
        mHDA.getAdapter().getCount();
        mHDB.getAdapter().getCount();
        mHDC.getAdapter().getCount();
        mHDD.getAdapter().getCount();
        mCD.getAdapter().getCount();
        mFDA.getAdapter().getCount();
        mFDB.getAdapter().getCount();
        mSD.getAdapter().getCount();
        mKernel.getAdapter().getCount();
        mInitrd.getAdapter().getCount();
    }

    private void addGenericOperatingSystems() {
        Config.osImages.put(getString(R.string.other), new LinksManager.LinkInfo("Other",
                getString(R.string.otherOperatingSystem),
                null,
                LinksManager.LinkType.OTHER));
    }

    @Override
    public void update(Observable observable, final Object o) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (o instanceof MachineProperty) {
                    MachineProperty property = (MachineProperty) o;
                    if (property == MachineProperty.UI) {
                        if (getMachine().getEnableVNC() != 1)
                            mSoundCard.setEnabled(true);
                        else
                            mSoundCard.setEnabled(true);
                    }
                }
                updateSummary();
            }
        });

    }

    public void notifyFieldChange(MachineProperty property, Object value) {
        if (viewListener != null)
            viewListener.onFieldChange(property, value);
    }

    public void notifyAction(MachineAction action, Object value) {
        if (viewListener != null)
            viewListener.onAction(action, value);
    }

    static class DiskInfo {
        public CheckBox enableCheckBox;
        public Spinner spinner;
        public MachineProperty colName;

        public DiskInfo(Spinner spinner, CheckBox enableCheckbox, MachineProperty dbColName) {
            this.spinner = spinner;
            this.enableCheckBox = enableCheckbox;
            this.colName = dbColName;
        }
    }

}