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
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.machine.Machine;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.machine.MachineAction;
import com.max2idea.android.limbo.machine.MachineController;
import com.max2idea.android.limbo.machine.MachineFilePaths;
import com.max2idea.android.limbo.machine.MachineProperty;
import com.max2idea.android.limbo.ui.SpinnerAdapter;

import java.util.ArrayList;
import java.util.Observable;
import java.util.Observer;

/** A simple custom dialog that serves as a way to change removable drives for Limbo.
 * This class communicates passively with the ViewController which observes for changes on the ui.
 */
public class DrivesDialogBox extends Dialog implements Observer {
    private static final String TAG = "DrivesDialogBox";

    private final Machine currMachine;
    public Spinner mCD;
    public Spinner mFDA;
    public Spinner mSD;
    public Spinner mFDB;
    public LinearLayout mCDLayout;
    public LinearLayout mFDALayout;
    public LinearLayout mFDBLayout;
    public LinearLayout mSDLayout;
    public FileType fileType;
    private Activity activity;
    private ViewListener viewListener;


    public DrivesDialogBox(Activity activity, int theme, Machine currMachine) {
        super(activity, theme);
        this.activity = activity;
        this.currMachine = currMachine;
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND, WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        setContentView(R.layout.dev_dialog);
        this.setTitle(R.string.RemovableDrives);
        getWidgets();
        initUI();
        setupController();
        setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialogInterface) {
                setViewListener(null);
                MachineController.getInstance().getMachine().deleteObserver(DrivesDialogBox.this);
            }
        });
        MachineController.getInstance().getMachine().addObserver(this);
    }

    private void setupController() {
        setViewListener(LimboApplication.getViewListener());
    }

    public void setViewListener(ViewListener viewListener) {
        this.viewListener = viewListener;
    }

    @Override
    public void onBackPressed() {
        this.dismiss();
    }

    private void getWidgets() {
        mCD = (Spinner) findViewById(R.id.cdromimgval);
        mCDLayout = findViewById(R.id.cdromimgl);

        mFDA = (Spinner) findViewById(R.id.floppyimgval);
        mFDALayout = findViewById(R.id.floppyimgl);

        mFDB = (Spinner) findViewById(R.id.floppybimgval);
        mFDBLayout = findViewById(R.id.floppybimgl);

        mSD = (Spinner) findViewById(R.id.sdimgval);
        mSDLayout = findViewById(R.id.sdimgl);

    }

    private void setupListeners() {
        setupListener(mCD, MachineProperty.CDROM, FileType.CDROM);
        setupListener(mFDA, MachineProperty.FDA, FileType.FDA);
        setupListener(mFDB, MachineProperty.FDB, FileType.FDB);
        setupListener(mSD, MachineProperty.SD, FileType.SD);
    }

    private void setupListener(final Spinner spinner, final MachineProperty machineDrive,
                               final FileType fileType) {
        spinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                setDriveValue(spinner, position, machineDrive, fileType);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
            }
        });
    }

    private void setDriveValue(Spinner spinner, int position, MachineProperty driveLabel,
                               FileType filetype) {
        String diskValue = (String) ((ArrayAdapter<?>) spinner.getAdapter()).getItem(position);
        if (position == 0) {
            notifyFieldChange(MachineProperty.REMOVABLE_DRIVE, new Object[]{ driveLabel, ""});
        } else if (position == 1) {
            this.fileType = filetype;
            LimboFileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
            spinner.setSelection(0);
        } else if (position > 1) {
            notifyFieldChange(MachineProperty.REMOVABLE_DRIVE, new Object[]{ driveLabel, diskValue});
        }
    }

    public void populateDiskAdapter(final Spinner spinner, final FileType fileType, final boolean createOption,
                                    final String value) {
        Thread t = new Thread(new Runnable() {
            public void run() {
                ArrayList<String> oldHDs = MachineFilePaths.getRecentFilePaths(fileType);
                final ArrayList<String> arraySpinner = new ArrayList<>();
                arraySpinner.add("None");
                if (createOption)
                    arraySpinner.add("New");
                arraySpinner.add(activity.getString(R.string.open));
                final int index = arraySpinner.size();
                for (String file : oldHDs) {
                    if (file != null) {
                        arraySpinner.add(file);
                    }
                }
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        SpinnerAdapter adapter = new SpinnerAdapter(activity, R.layout.custom_spinner_item, arraySpinner, index);
                        adapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
                        spinner.setAdapter(adapter);
                        spinner.invalidate();
                        setDiskValue(spinner, value);
                    }
                });
            }
        });
        t.start();
    }

    private void initUI() {
        Thread thread = new Thread(new Runnable() {
            public void run() {
                if (currMachine.isEnableCDROM()) {
                    populateDiskAdapter(mCD, FileType.CDROM, false, currMachine.getCdImagePath());
                } else {
                    mCDLayout.setVisibility(View.GONE);
                }
                if (currMachine.isEnableFDA()) {
                    populateDiskAdapter(mFDA, FileType.FDA, false, currMachine.getFdaImagePath());
                } else {
                    mFDALayout.setVisibility(View.GONE);
                }
                if (currMachine.isEnableFDB()) {
                    populateDiskAdapter(mFDB, FileType.FDB, false, currMachine.getFdbImagePath());
                } else {
                    mFDBLayout.setVisibility(View.GONE);
                }
                if (currMachine.isEnableSD()) {
                    populateDiskAdapter(mSD, FileType.SD, false, currMachine.getCdImagePath());
                } else {
                    mSDLayout.setVisibility(View.GONE);
                }
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    public void run() {
                        setupListeners();
                    }
                }, 500);
            }
        });
        thread.setPriority(Thread.MIN_PRIORITY);
        thread.start();
    }

    public void setDriveAttr(FileType fileType, final String file) {
        notifyAction(MachineAction.INSERT_FAV, new Object[]{file, fileType});
        if (fileType == FileType.CDROM && file != null && !file.trim().equals("")) {
            notifyFieldChange(MachineProperty.CDROM, file);
            setSpinnerValue(mCD, file);
        } else if (fileType == FileType.SD && file != null && !file.trim().equals("")) {
            notifyFieldChange(MachineProperty.SD, file);
            setSpinnerValue(mSD, file);
        } else if (file != null && !file.trim().equals("") && fileType == FileType.FDA) {
            notifyFieldChange(MachineProperty.FDA, file);
            setSpinnerValue(mFDA, file);
        } else if (file != null && !file.trim().equals("") && fileType == FileType.FDB) {
            notifyFieldChange(MachineProperty.FDB, file);
            setSpinnerValue(mFDB, file);
        }
    }

    private void setSpinnerValue(final Spinner spinner, final String value) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            public void run() {
                if (SpinnerAdapter.getItemPosition(spinner, value) < 0) {
                    SpinnerAdapter.addItem(spinner, value);
                }
                setDiskValue(spinner, value);
                int res = spinner.getSelectedItemPosition();
                if (res == 1) {
                    spinner.setSelection(0);
                }
            }
        });
    }


    private void setDiskValue(final Spinner spinner, final String value) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (spinner != null) {
                    int pos = SpinnerAdapter.getItemPosition(spinner, value);
                    if (pos > 1) {
                        spinner.setSelection(pos);
                    } else {
                        spinner.setSelection(0);
                    }
                }
            }
        });

    }

    public void notifyFieldChange(MachineProperty property, Object value) {
        if(viewListener !=null)
            viewListener.onFieldChange(property, value);
    }

    public void notifyAction(MachineAction action, Object value) {
        if(viewListener !=null)
            viewListener.onAction(action, value);
    }

    @Override
    public void update(Observable observable, Object o) {
        Log.v(TAG, "Observable updated param: " + o);
        Object[] params = (Object[]) o;
        MachineProperty property = (MachineProperty) params[0];
        Object value = params[1];
        //XXX: if the executor is not able to change the drive then we reset the spinner
        switch(property) {
            case CDROM:
                if(value == null)
                    setDiskValue(mCD, "");
                break;
            case FDA:
                if(value == null)
                    setDiskValue(mFDA, "");
                break;
            case FDB:
                if(value == null)
                    setDiskValue(mFDB, "");
                break;
            case SD:
                if(value == null)
                    setDiskValue(mSD, "");
                break;
        }
    }
}
