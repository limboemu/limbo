package com.max2idea.android.limbo.utils;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import java.util.ArrayList;
import java.util.Iterator;

public class DrivesDialogBox extends Dialog {
    public Spinner mCD;
    public LinearLayout mCDLayout;
    public LinearLayout mFDALayout;
    public LinearLayout mFDBLayout;
    public LinearLayout mSDLayout;
    public Spinner mFDA;
    public Spinner mSD;
    public Spinner mFDB;
    public static LimboActivity.FileType filetype;
    private Activity activity;
    private Machine currMachine;
    public DrivesDialogBox(Context context, int theme, Activity activity1, Machine currMachine) {
        super(context, theme);
        this.currMachine = currMachine;
        activity = activity1;
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND, WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        setContentView(R.layout.dev_dialog);
        this.setTitle("Device Manager");
        getWidgets();
        initUI();


    }

    @Override
    public void onBackPressed() {
        this.dismiss();
    }

    private void getWidgets() {
        mCD = (Spinner) findViewById(R.id.cdromimgval);
        mCDLayout = (LinearLayout) findViewById(R.id.cdromimgl);

        mFDA = (Spinner) findViewById(R.id.floppyimgval);
        mFDALayout = (LinearLayout) findViewById(R.id.floppyimgl);

        mFDB = (Spinner) findViewById(R.id.floppybimgval);
        mFDBLayout = (LinearLayout) findViewById(R.id.floppybimgl);

        mSD = (Spinner) findViewById(R.id.sdimgval);
        mSDLayout = (LinearLayout) findViewById(R.id.sdimgl);

    }

    private void setupListeners() {

        mCD.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String cd = (String) ((ArrayAdapter<?>) mCD.getAdapter()).getItem(position);

                if (position == 0) {
                    update(LimboActivity.currMachine, MachineOpenHelper.CDROM, "");
                    LimboActivity.currMachine.cd_iso_path = "";
                } else if (position == 1) {
                    filetype = LimboActivity.FileType.CD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mCD.setSelection(0);
                } else if (position > 1) {
                    update(LimboActivity.currMachine, MachineOpenHelper.CDROM, cd);
                    LimboActivity.currMachine.cd_iso_path = cd;
                    // TODO: If Machine is running eject and set
                    // floppy img
                }
                if (LimboActivity.vmexecutor != null && position > 1) {
                    mCD.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("ide1-cd0", LimboActivity.currMachine.cd_iso_path);
                    mCD.setEnabled(true);
                } else if (LimboActivity.vmexecutor != null && position == 0) {
                    mCD.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("ide1-cd0", null); // Eject
                    mCD.setEnabled(true);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
                // Log.v(TAG, "Nothing selected");
            }
        });

        mFDA.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String fda = (String) ((ArrayAdapter<?>) mFDA.getAdapter()).getItem(position);
                if (position == 0) {
                    update(LimboActivity.currMachine, MachineOpenHelper.FDA, "");
                    LimboActivity.currMachine.fda_img_path = "";
                } else if (position == 1) {
                    filetype = LimboActivity.FileType.FDA;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mFDA.setSelection(0);
                } else if (position > 1) {
                    update(LimboActivity.currMachine, MachineOpenHelper.FDA, fda);
                    LimboActivity.currMachine.fda_img_path = fda;
                    // TODO: If Machine is running eject and set
                    // floppy img
                }
                if (LimboActivity.vmexecutor != null && position > 1) {
                    mFDA.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("floppy0", LimboActivity.currMachine.fda_img_path);
                    mFDA.setEnabled(true);
                } else if (LimboActivity.vmexecutor != null && position == 0) {
                    mFDA.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("floppy0", null); // Eject
                    mFDA.setEnabled(true);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
                // Log.v(TAG, "Nothing selected");
            }
        });

        mFDB.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String fdb = (String) ((ArrayAdapter<?>) mFDB.getAdapter()).getItem(position);
                if (position == 0) {
                    update(LimboActivity.currMachine, MachineOpenHelper.FDB, "");
                    LimboActivity.currMachine.fdb_img_path = "";
                } else if (position == 1) {
                    filetype = LimboActivity.FileType.FDB;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mFDB.setSelection(0);
                } else if (position > 1) {
                    update(LimboActivity.currMachine, MachineOpenHelper.FDB, fdb);
                    LimboActivity.currMachine.fdb_img_path = fdb;
                    // TODO: If Machine is running eject and set
                    // floppy img
                }
                if (LimboActivity.vmexecutor != null && position > 1) {
                    mFDB.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("floppy1", LimboActivity.currMachine.fdb_img_path);
                    mFDB.setEnabled(true);
                } else if (LimboActivity.vmexecutor != null && position == 0) {
                    mFDB.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("floppy1", null); // Eject
                    mFDB.setEnabled(true);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
                // Log.v(TAG, "Nothing selected");
            }
        });

        mSD.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String sd = (String) ((ArrayAdapter<?>) mSD.getAdapter()).getItem(position);
                if (position == 0) {
                    update(LimboActivity.currMachine, MachineOpenHelper.SD, "");
                    LimboActivity.currMachine.sd_img_path = "";
                } else if (position == 1) {
                    filetype = LimboActivity.FileType.SD;
                    FileManager.browse(activity, filetype, Config.OPEN_IMAGE_FILE_REQUEST_CODE);
                    mSD.setSelection(0);
                } else if (position > 1) {
                    update(LimboActivity.currMachine, MachineOpenHelper.SD, sd);
                    LimboActivity.currMachine.sd_img_path = sd;
                    // TODO: If Machine is running eject and set
                    // floppy img
                }
                if (LimboActivity.vmexecutor != null && position > 1) {
                    mSD.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("sd0", LimboActivity.currMachine.sd_img_path);
                    mSD.setEnabled(true);
                } else if (LimboActivity.vmexecutor != null && position == 0) {
                    mSD.setEnabled(false);
                    LimboActivity.vmexecutor.change_dev("sd0", null); // Eject
                    mSD.setEnabled(true);
                }
            }

            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
                // Log.v(TAG, "Nothing selected");
            }
        });

    }


    // Set File Adapters
    public void populateDiskAdapter(final Spinner spinner, final String fileType, final boolean createOption,
                                    final String value) {

        Thread t = new Thread(new Runnable() {
            public void run() {

                ArrayList<String> oldHDs = FavOpenHelper.getInstance(activity).getFav(fileType);
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
                        UIUtils.LimboFileSpinnerAdapter adapter = new UIUtils.LimboFileSpinnerAdapter(activity, R.layout.custom_spinner_item, arraySpinner, index);
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
        // Set spinners to values from currmachine

        Thread thread = new Thread(new Runnable() {
            public void run() {

                if (currMachine.enableCDROM) {
                    populateDiskAdapter(mCD, "cd", false, currMachine.cd_iso_path);
                } else {
                    mCDLayout.setVisibility(View.GONE);
                }
                if (currMachine.enableFDA) {
                    populateDiskAdapter(mFDA, "fda", false, currMachine.fda_img_path);
                } else {
                    mFDALayout.setVisibility(View.GONE);
                }
                if (currMachine.enableFDB) {

                    populateDiskAdapter(mFDB, "fdb", false, currMachine.fdb_img_path);
                } else {
                    mFDBLayout.setVisibility(View.GONE);
                }
                if (currMachine.enableSD) {
                    populateDiskAdapter(mSD, "sd", false, currMachine.cd_iso_path);
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


    @SuppressWarnings("unchecked")
    public void setDriveAttr(LimboActivity.FileType fileType, final String file) {

        addDriveToList(file, fileType);

        if (fileType != null && fileType == LimboActivity.FileType.CD && file != null && !file.trim().equals("")) {
            update(LimboActivity.currMachine, MachineOpenHelper.CDROM, file);

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    if (((ArrayAdapter<String>) mCD.getAdapter()).getPosition(file) < 0) {
                        ((ArrayAdapter<String>) mCD.getAdapter()).add(file);
                    }
                    setDiskValue(mCD, file);
                    int res = mCD.getSelectedItemPosition();
                    if (res == 1) {
                        mCD.setSelection(0);
                    }
                }
            });

        } else if (fileType != null && fileType == LimboActivity.FileType.SD && file != null && !file.trim().equals("")) {
            update(LimboActivity.currMachine, MachineOpenHelper.SD, file);

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    if (((ArrayAdapter<String>) mSD.getAdapter()).getPosition(file) < 0) {
                        ((ArrayAdapter<String>) mSD.getAdapter()).add(file);
                    }
                    setDiskValue(mSD, file);
                    int res = mSD.getSelectedItemPosition();
                    if (res == 1) {
                        mSD.setSelection(0);

                    }
                }
            });

        } else if (file != null && !file.trim().equals("") && fileType == LimboActivity.FileType.FDA) {
            update(LimboActivity.currMachine, MachineOpenHelper.FDA, file);

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    if (((ArrayAdapter<String>) mFDA.getAdapter()).getPosition(file) < 0) {
                        ((ArrayAdapter<String>) mFDA.getAdapter()).add(file);
                    }
                    setDiskValue(mFDA, file);
                    int res = mFDA.getSelectedItemPosition();
                    if (res == 1) {
                        mFDA.setSelection(0);
                    }
                }
            });

        } else if (file != null && !file.trim().equals("") && fileType == LimboActivity.FileType.FDB) {
            update(LimboActivity.currMachine, MachineOpenHelper.FDB, file);

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    if (((ArrayAdapter<String>) mFDB.getAdapter()).getPosition(file) < 0) {
                        ((ArrayAdapter<String>) mFDB.getAdapter()).add(file);
                    }
                    setDiskValue(mFDB, file);
                    int res = mFDB.getSelectedItemPosition();
                    if (res == 1) {
                        mFDB.setSelection(0);

                    }
                }
            });

        }
    }


    private void setDiskValue(Spinner spinner, String value) {
        if (spinner != null) {
            int pos = ((ArrayAdapter<String>) spinner.getAdapter()).getPosition(value);
            // Log.v("DB", "Got pos: " + pos + " for CDROM=" + cdrom);
            if (pos > 1) {
                spinner.setSelection(pos);
            } else {
                spinner.setSelection(0);
            }
        } else {
            spinner.setSelection(0);
        }
    }

    private void addDriveToList(String file, LimboActivity.FileType type) {
        // Check if exists
        // Log.v(TAG, "Adding To list: " + type + ":" + file);
        int res = FavOpenHelper.getInstance(activity).getFavSeq(file, type.toString().toLowerCase());
        if (res == -1) {
            if (type == LimboActivity.FileType.CD) {
                mCD.getAdapter().getCount();
            } else if (type == LimboActivity.FileType.FDA) {
                mFDA.getAdapter().getCount();
            } else if (type == LimboActivity.FileType.FDB) {
                mFDB.getAdapter().getCount();
            }
            FavOpenHelper.getInstance(activity).insertFav(file, type.toString().toLowerCase());
        }

    }


    public void update(final Machine myMachine, final String colname, final String value) {

        Thread thread = new Thread(new Runnable() {
            public void run() {
                MachineOpenHelper.getInstance(activity).update(myMachine, colname, value);
            }
        });
        thread.setPriority(Thread.MIN_PRIORITY);
        thread.start();

    }

}
