package com.max2idea.android.limbo.utils;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import java.util.ArrayList;
import java.util.Iterator;

public class DrivesDialogBox extends Dialog {
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

	public Spinner mCD;
	public LinearLayout mCDLayout;
	public LinearLayout mFDALayout;
	public LinearLayout mFDBLayout;
	public LinearLayout mSDLayout;
	public Spinner mFDA;
	public Spinner mSD;
	public Spinner mFDB;
	public Button mOK;

	public String filetype;

	private void getWidgets() {
		mCD = (Spinner) findViewById(R.id.cdromimgval);
		mCDLayout = (LinearLayout) findViewById(R.id.cdromimgl);

		mFDA = (Spinner) findViewById(R.id.floppyimgval);
		mFDALayout = (LinearLayout) findViewById(R.id.floppyimgl);

		mFDB = (Spinner) findViewById(R.id.floppybimgval);
		mFDBLayout = (LinearLayout) findViewById(R.id.floppybimgl);

		mSD = (Spinner) findViewById(R.id.sdimgval);
		mSDLayout = (LinearLayout) findViewById(R.id.sdimgl);

		mOK = (Button) findViewById(R.id.okButton);
	}

	private void setupListeners() {
		mOK.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				dismiss();
			}
		});
		mCD.setOnItemSelectedListener(new OnItemSelectedListener() {

			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cd = (String) ((ArrayAdapter<?>) mCD.getAdapter()).getItem(position);

				if (position == 0) {
					update(LimboActivity.currMachine, MachineOpenHelper.CDROM, "");
					LimboActivity.currMachine.cd_iso_path = "";
				} else if (position == 1) {
					browse("cd");
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
					browse("fda");
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
					browse("fdb");
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
					browse("sd");
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

	private void initUI() {
		// Set spinners to values from currmachine

		Thread thread = new Thread(new Runnable() {
			public void run() {

				if (currMachine.enableCDROM) {
					populateCDRom("cd");
				} else {
					mCDLayout.setVisibility(View.GONE);
				}
				if (currMachine.enableFDA) {
					populateFloppy("fda");
				} else {
					mFDALayout.setVisibility(View.GONE);
				}
				if (currMachine.enableFDB) {
					populateFloppy("fdb");
				} else {
					mFDBLayout.setVisibility(View.GONE);
				}
				if (currMachine.enableSD) {
					populateSD("sd");
				} else {
					mSDLayout.setVisibility(View.GONE);
				}
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        setCDROM(LimboActivity.currMachine.cd_iso_path);
                        setFDA(LimboActivity.currMachine.fda_img_path);
                        setFDB(LimboActivity.currMachine.fdb_img_path);
                        setSD(LimboActivity.currMachine.sd_img_path);

                    }
                });
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    public void run() {
                        setupListeners();
                    }
                },500);
			}
		});
		thread.setPriority(Thread.MIN_PRIORITY);
		thread.start();
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
		ArrayAdapter<String> cdromAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item,
				arraySpinner);
		cdromAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		mCD.setAdapter(cdromAdapter);
		mCD.invalidate();
	}

	private void populateSD(String fileType) {
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
		ArrayAdapter<String> sdAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
		sdAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		mSD.setAdapter(sdAdapter);
		mSD.invalidate();
	}

	// Set Floppy
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
			ArrayAdapter<String> fdaAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item,
					arraySpinner);
			fdaAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			mFDA.setAdapter(fdaAdapter);
			mFDA.invalidate();
		} else if (fileType.equals("fdb")) {
			ArrayAdapter<String> fdbAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item,
					arraySpinner);
			fdbAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
			mFDB.setAdapter(fdbAdapter);
			mFDB.invalidate();
		}
	}

	@SuppressWarnings("unchecked")
	public void setDriveAttr(String fileType, final String file) {

        addDriveToList(file, fileType);

		if (fileType != null && fileType.startsWith("cd") && file != null && !file.trim().equals("")) {
			update(LimboActivity.currMachine, MachineOpenHelper.CDROM, file);
			if (((ArrayAdapter<String>) mCD.getAdapter()).getPosition(file) < 0) {
				((ArrayAdapter<String>) mCD.getAdapter()).add(file);
			}
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    setCDROM(file);
                    int res = mCD.getSelectedItemPosition();
                    if (res == 1) {
                        mCD.setSelection(0);
                    }
                }
            });

		} else if (fileType != null && fileType.startsWith("sd") && file != null && !file.trim().equals("")) {
			update(LimboActivity.currMachine, MachineOpenHelper.SD, file);
			if (((ArrayAdapter<String>) mSD.getAdapter()).getPosition(file) < 0) {
				((ArrayAdapter<String>) mSD.getAdapter()).add(file);
			}
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                public void run() {
                    setSD(file);
                    int res = mFDB.getSelectedItemPosition();
                    if (res == 1) {
                        mSD.setSelection(0);

                    }
                }
            });

		} else if (fileType != null && fileType.startsWith("fd") && file != null && !file.trim().equals("")) {
			if (fileType.startsWith("fda")) {
				update(LimboActivity.currMachine, MachineOpenHelper.FDA, file);
				if (((ArrayAdapter<String>) mFDA.getAdapter()).getPosition(file) < 0) {
					((ArrayAdapter<String>) mFDA.getAdapter()).add(file);
				}
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        setFDA(file);
                        int res = mFDA.getSelectedItemPosition();
                        if (res == 1) {
                            mFDA.setSelection(0);
                        }
                    }
                });

			} else if (fileType.startsWith("fdb")) {
				update(LimboActivity.currMachine, MachineOpenHelper.FDB, file);
				if (((ArrayAdapter<String>) mFDB.getAdapter()).getPosition(file) < 0) {
					((ArrayAdapter<String>) mFDB.getAdapter()).add(file);
				}
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    public void run() {
                        setFDB(file);
                        int res = mFDB.getSelectedItemPosition();
                        if (res == 1) {
                            mFDB.setSelection(0);

                        }
                    }
                });

			}
		}

	}

	@SuppressWarnings("unchecked")
	private void setCDROM(final String cdrom) {
		LimboActivity.currMachine.cd_iso_path = cdrom;


				if (cdrom != null) {
					int pos = ((ArrayAdapter<String>) mCD.getAdapter()).getPosition(cdrom);
					// Log.v("DB", "Got pos: " + pos + " for CDROM=" + cdrom);
					if (pos > 1) {
						mCD.setSelection(pos);
					} else {
						mCD.setSelection(0);
					}
				} else {
					mCD.setSelection(0);
				}

	}

	@SuppressWarnings("unchecked")
	private void setSD(final String sd) {
		LimboActivity.currMachine.sd_img_path = sd;

				if (sd != null) {
					int pos = ((ArrayAdapter<String>) mSD.getAdapter()).getPosition(sd);
					// Log.v("DB", "Got pos: " + pos + " for CDROM=" + cdrom);
					if (pos > 1) {
						mSD.setSelection(pos);
					} else {
						mSD.setSelection(0);
					}
				} else {
					mSD.setSelection(0);
				}


	}

	@SuppressWarnings("unchecked")
	private void setFDA(final String fda) {
		LimboActivity.currMachine.fda_img_path = fda;

				if (fda != null) {
					int pos = ((ArrayAdapter<String>) mFDA.getAdapter()).getPosition(fda);
					// Log.v("DB", "Got pos: " + pos + " for FDA=" + fda);
					if (pos >= 0) {
						mFDA.setSelection(pos);
					} else {
						mFDA.setSelection(0);
					}
				} else {
					mFDA.setSelection(0);
				}


	}

	@SuppressWarnings("unchecked")
	private void setFDB(final String fdb) {
		LimboActivity.currMachine.fdb_img_path = fdb;


				if (fdb != null) {
					int pos = ((ArrayAdapter<String>) mFDA.getAdapter()).getPosition(fdb);
					// Log.v("DB", "Got pos: " + pos + " for FDB=" + fdb);
					if (pos >= 0) {
						mFDB.setSelection(pos);
					} else {
						mFDB.setSelection(0);
					}
				} else {
					mFDB.setSelection(0);
				}

	}

	private void addDriveToList(String file, String type) {
		// Check if exists
		// Log.v(TAG, "Adding To list: " + type + ":" + file);
		int res = FavOpenHelper.getInstance(activity).getFavUrlSeq(file, type);
		if (res == -1) {
			if (type.equals("cd")) {
				mCD.getAdapter().getCount();
			} else if (type.equals("fda")) {
				mFDA.getAdapter().getCount();
			} else if (type.equals("fdb")) {
				mFDB.getAdapter().getCount();
			}
			FavOpenHelper.getInstance(activity).insertFavURL(file, type);
		}

	}

	public void browse(String fileType) {
		// Check if SD card is mounted
		// Log.v(TAG, "Browsing: " + fileType);
		String state = Environment.getExternalStorageState();
		if (!Environment.MEDIA_MOUNTED.equals(state)) {
			Toast.makeText(activity.getApplicationContext(), "Error: SD card is not mounted", Toast.LENGTH_LONG).show();
            UIUtils.toastShort(activity, "Error: SD card is not mounted");
			return;
		}

		if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP || !Config.enableExternalSD) {
			String dir = null;
			// GET THE LAST ACCESSED DIR FROM THE REG
			String lastDir = LimboSettingsManager.getLastDir(activity);
			try {
				Intent i = null;
				i = getFileManIntent();
				Bundle b = new Bundle();
				b.putString("lastDir", lastDir);
				b.putString("fileType", fileType);
				i.putExtras(b);
				// Log.v("**PASS** ", lastDir);
				activity.startActivityForResult(i, Config.FILEMAN_REQUEST_CODE);

			} catch (Exception e) {
				// Log.v(TAG, "Error while starting Filemanager: " +
				// e.getMessage());
			}
		} else {
			filetype = fileType;
			LimboFileManager.promptSDCardAccess(activity, fileType);
		}
	}

	public Intent getFileManIntent() {
		return new Intent(activity, com.max2idea.android.limbo.main.LimboFileManager.class);
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
