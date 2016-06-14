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

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.limbo.emu.main.R;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.QmpClient;

import android.androidVNC.AbstractScaling;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.provider.DocumentFile;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

//import com.max2idea.android.limbo.main.R;

/**
 * 
 * @author Dev
 */
public class LimboVNCActivity extends android.androidVNC.VncCanvasActivity {

	public static final int KEYBOARD = 10000;
	public static final int QUIT = 10001;
	public static final int HELP = 10002;
	private boolean monitorMode = false;
	private boolean mouseOn = false;
	private Object lockTime = new Object();
	private boolean timeQuit = false;
	private Thread timeListenerThread;
	private ProgressDialog progDialog;

	@Override
	public void onCreate(Bundle b) {
		if (Config.disableTitleBar || android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.LOLLIPOP) {
			requestWindowFeature(Window.FEATURE_NO_TITLE);
		}
		super.onCreate(b);

		if (LimboSettingsManager.getOrientationReverse(this))
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);

		if (Config.enable_qemu_fullScreen)
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		Toast toast = Toast.makeText(activity, "Press Volume Down for Right Click", Toast.LENGTH_SHORT);
		toast.setGravity(Gravity.TOP | Gravity.CENTER, 0, 0);
		toast.show();
		this.vncCanvas.setFocusableInTouchMode(true);
		onMouse();

		setDefaulViewMode();

	}

	private void setDefaulViewMode() {
		// TODO Auto-generated method stub

		// Fit to Screen
		AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(this);
		showPanningState();

		// Full Screen
		this.toggleFullScreen();

		setLayout(getResources().getConfiguration());
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		setLayout(newConfig);
	}

	private void setLayout(Configuration newConfig) {
		// TODO Auto-generated method stub
		View vnc_canvas = (View) this.findViewById(R.id.vnc_canvas_layout);
		View zoom = (View) this.findViewById(R.id.zoom_layout);

		LinearLayout.LayoutParams vnc_params = (LinearLayout.LayoutParams) vnc_canvas.getLayoutParams();
		LinearLayout.LayoutParams zoom_params = (LinearLayout.LayoutParams) zoom.getLayoutParams();

		if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {

			vnc_params.weight = 0.5f;
			zoom_params.weight = 0.5f;

		} else if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {

			vnc_params.weight = 1f;
			zoom_params.weight = 0f;

		}
		vnc_canvas.setLayoutParams(vnc_params);
		zoom.setLayoutParams(zoom_params);
	}

	public void stopTimeListener() {
		Log.v(TAG, "Stopping Listener");
		synchronized (this.lockTime) {
			this.timeQuit = true;
			this.lockTime.notifyAll();
		}
	}

	public void onDestroy() {
		super.onDestroy();
		this.stopTimeListener();

	}

	public void onPause() {
		super.onPause();
	}

	public void onResume() {
		super.onResume();
	}

	public void timeListener() {
		while (timeQuit != true) {
			String status = checkCompletion();
			Log.v("Inside", "Status: " + status);
			if (status == null || status.equals("") || status.equals("DONE")) {
				Log.v("Inside", "Saving state is done: " + status);
				stopTimeListener();
				return;
			}
			try {
				Thread.sleep(1000);
			} catch (InterruptedException ex) {
				Log.v("SaveVM", "Could not sleep");
			}
		}
		Log.v("SaveVM", "Save state complete");

	}

	public void startSaveVMListener() {
		this.stopTimeListener();
		timeQuit = false;
		try {
			Log.v("Listener", "Time Listener Started...");
			timeListener();
			synchronized (lockTime) {
				while (timeQuit == false) {
					lockTime.wait();
				}
				lockTime.notifyAll();
			}
		} catch (Exception ex) {
			ex.printStackTrace();
			Log.v("SaveVM", "Time listener thread error: " + ex.getMessage());
		}
		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				Toast.makeText(getApplicationContext(), "VM State saved", Toast.LENGTH_LONG).show();
			}
		}, 1000);

		Log.v("Listener", "Time listener thread exited...");

	}

	String TAG = "LimboVNCActivity";

	public void stopVM(boolean exit) {

		new AlertDialog.Builder(this).setTitle("Shutdown VM")
				.setMessage("To avoid any corrupt data make sure you "
						+ "have already shutdown the Operating system from within the VM. Continue?")
				.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						if (LimboActivity.vmexecutor != null) {
							LimboService.stopService();
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

	public void pausedVM() {

		LimboActivity.vmexecutor.paused = 1;
		((LimboActivity) LimboActivity.activity).saveStateVMDB();

		new AlertDialog.Builder(this).setTitle("Paused").setMessage("VM is now Paused tap OK to exit")
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						if (LimboActivity.vmexecutor != null) {
							LimboService.stopService();
						} else if (activity.getParent() != null) {
							activity.getParent().finish();
						} else {
							activity.finish();
						}
					}
				}).show();
	}

	DrivesDialogBox drives = null;

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		// Log.v(TAG, "RET CODE: " + resultCode);
		if (resultCode == Config.FILEMAN_RETURN_CODE) {
			// Read from activity
			String currDir = LimboSettingsManager.getLastDir(this);
			String file = "";
			String fileType = "";
			Bundle b = data.getExtras();
			fileType = b.getString("fileType");
			file = b.getString("file");
			currDir = b.getString("currDir");
			// Log.v(TAG, "Got New Dir: " + currDir);
			// Log.v(TAG, "Got File Type: " + fileType);
			// Log.v(TAG, "Got New File: " + file);
			if (currDir != null && !currDir.trim().equals("")) {
				LimboSettingsManager.setLastDir(this, currDir);
			}
			if (fileType != null && file != null) {
				DrivesDialogBox.setDriveAttr(fileType, file, true);
			}

		} else if (requestCode == Config.REQUEST_SDCARD_CODE) {
			if (data != null) {
				Uri uri = data.getData();
				DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
				String file = uri.toString();
				final int takeFlags = data.getFlags()
						& (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
				getContentResolver().takePersistableUriPermission(uri, takeFlags);

				// Protect from qemu thinking it's a protocol
				file = ("/" + file).replace(":", "");

				if (DrivesDialogBox.filetype != null && file != null) {
					DrivesDialogBox.setDriveAttr(DrivesDialogBox.filetype, file, true);
				}
			}

		}

		// Check if says open

	}

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {
		super.onOptionsItemSelected(item);
		if (item.getItemId() == this.KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
			this.onKeyboard();
		} else if (item.getItemId() == R.id.itemReset) {
			onRestartVM();
		} else if (item.getItemId() == R.id.itemShutdown) {
			stopVM(false);
		} else if (item.getItemId() == R.id.itemDrives) {
			// Show up removable devices dialog
			drives = new DrivesDialogBox(activity, R.style.Transparent, this, LimboActivity.enableCDROM,
					LimboActivity.enableFDA, LimboActivity.enableFDB, LimboActivity.enableSD);
			drives.show();
		} else if (item.getItemId() == R.id.itemMonitor) {
			if (this.monitorMode) {
				this.onVNC();
			} else {
				this.onQMP();
			}
		} else if (item.getItemId() == R.id.itemSaveState) {
			this.promptPause(activity);
		} else if (item.getItemId() == R.id.itemSaveSnapshot) {
			this.promptStateName(activity);
		} else if (item.getItemId() == R.id.itemFitToScreen) {
			return onFitToScreen();
		} else if (item.getItemId() == R.id.itemFullScreen) {
			return toggleFullScreen();
		} else if (item.getItemId() == this.QUIT) {
		} else if (item.getItemId() == R.id.itemCenterMouse) {
			return onMouse();
		} else if (item.getItemId() == R.id.itemHelp) {
			this.onMenuHelp();
		}
		// this.vncCanvas.requestFocus();
		return true;
	}

	private boolean toggleFullScreen() {
		// TODO Auto-generated method stub
		View vnc_canvas = (View) this.findViewById(R.id.vnc_canvas_layout);
		View zoom = (View) this.findViewById(R.id.zoom_layout);

		LinearLayout.LayoutParams vnc_params = (LinearLayout.LayoutParams) vnc_canvas.getLayoutParams();
		LinearLayout.LayoutParams zoom_params = (LinearLayout.LayoutParams) zoom.getLayoutParams();

		if (vnc_params.weight == 1f) {
			vnc_params.weight = 0.5f;
			zoom_params.weight = 0.5f;
		} else {
			vnc_params.weight = 1f;
			zoom_params.weight = 0f;
		}
		vnc_canvas.setLayoutParams(vnc_params);
		zoom.setLayoutParams(zoom_params);

		return false;
	}

	private boolean onFitToScreen() {
		// TODO Auto-generated method stub
		input1 = getInputHandlerById(R.id.itemInputTouchpad);
		if (input1 != null) {
			inputHandler = input1;
			connection.setInputMode(input1.getName());
			connection.setFollowMouse(true);
			mouseOn = true;
			showPanningState();
			return true;
		}
		return true;
	}

	private boolean onMouse() {
		// TODO Auto-generated method stub
		if (mouseOn == false) {
			input1 = getInputHandlerById(R.id.itemInputTouchpad);
		} else {
			input1 = getInputHandlerById(R.id.itemInputTouchPanZoomMouse);
		}
		if (input1 != null) {
			inputHandler = input1;
			connection.setInputMode(input1.getName());
			if (mouseOn == false) {
				connection.setFollowMouse(true);
				mouseOn = true;
			} else {
				connection.setFollowMouse(false);
				mouseOn = false;
			}
			showPanningState();
			return true;
		}
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		menu.clear();
		getMenuInflater().inflate(R.menu.vnccanvasactivitymenu, menu);

		if (vncCanvas.scaling != null) {
			menu.findItem(vncCanvas.scaling.getId()).setChecked(true);
		}

		if (this.monitorMode) {
			menu.findItem(R.id.itemMonitor).setTitle("VM Display");

		} else {
			menu.findItem(R.id.itemMonitor).setTitle("QEMU Monitor");

		}
		menu.removeItem(menu.findItem(R.id.itemEnterText).getItemId());
		menu.removeItem(menu.findItem(R.id.itemSendKeyAgain).getItemId());
		menu.removeItem(menu.findItem(R.id.itemSpecialKeys).getItemId());
		menu.removeItem(menu.findItem(R.id.itemInputMode).getItemId());

		// Menu inputMenu = menu.findItem(R.id.itemInputMode).getSubMenu();
		//
		// inputModeMenuItems = new MenuItem[inputModeIds.length];
		// for (int i = 0; i < inputModeIds.length; i++) {
		// inputModeMenuItems[i] = inputMenu.findItem(inputModeIds[i]);
		// }
		// updateInputMenu();
		// menu.removeItem(menu.findItem(R.id.itemCenterMouse).getItemId());

		if (this.mouseOn) {
			menu.findItem(R.id.itemCenterMouse).setTitle("Pan (Mouse Off)");
			menu.findItem(R.id.itemCenterMouse).setIcon(R.drawable.pan);
		} else {
			menu.findItem(R.id.itemCenterMouse).setTitle("Mouse (Pan Off)");
			menu.findItem(R.id.itemCenterMouse).setIcon(R.drawable.mouse);

		}

		return true;

	}

	public void onRestartVM() {
		Thread t = new Thread(new Runnable() {
			public void run() {
				if (LimboActivity.vmexecutor != null) {
					Log.v(TAG, "Restarting the VM...");
					LimboActivity.vmexecutor.stopvm(1);

				} else {
					Log.v(TAG, "Not running VM...");
				}
			}
		});
		t.start();
	}

	private void onKeyboard() {
		InputMethodManager inputMgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		inputMgr.toggleSoftInput(0, 0);
	}

	private void onQMP() {
		monitorMode = true;
		vncCanvas.sendMetaKey1(50, 6);

	}

	private void onVNC() {
		monitorMode = false;
		vncCanvas.sendMetaKey1(49, 6);
	}

	private void onSaveSnapshot(final String stateName) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				((LimboActivity) LimboActivity.activity).saveSnapshotDB(stateName);
				onQMP();
				try {
					Thread.sleep(500);
				} catch (InterruptedException ex) {
					Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
				}

				String command = "savevm " + stateName + "\n";
				for (int i = 0; i < command.length(); i++)
					vncCanvas.sendText(command.charAt(i) + "");

				try {
					Thread.sleep(2000);
				} catch (InterruptedException ex) {
					Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
				}

				onVNC();

				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(getApplicationContext(), "Please wait while saving HD Snapshot",
								Toast.LENGTH_LONG).show();
						// progDialog = ProgressDialog.show(activity, "Please
						// Wait", "Saving VM
						// State...", true);
						VMListener a = new VMListener();
						a.execute();
					}
				}, 3000);
			}
		});
		t.start();

	}

	// private void onPauseVM() {
	// Thread t = new Thread(new Runnable() {
	// public void run() {
	// // Delete any previous state file
	// if (LimboActivity.vmexecutor.save_state_name != null) {
	// File file = new File(LimboActivity.vmexecutor.save_state_name);
	// if (file.exists()) {
	// file.delete();
	// }
	// }
	//
	// LimboActivity.vmexecutor.paused = 1;
	// ((LimboActivity) LimboActivity.activity).saveStateVMDB();
	//
	// onQMP();
	// new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
	// @Override
	// public void run() {
	// Toast.makeText(getApplicationContext(), "Please wait while saving VM
	// State", Toast.LENGTH_LONG)
	// .show();
	// }
	// }, 500);
	// try {
	// Thread.sleep(500);
	// } catch (InterruptedException ex) {
	// Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE,
	// null, ex);
	// }
	//
	// String commandStop = "stop\n";
	// for (int i = 0; i < commandStop.length(); i++)
	// vncCanvas.sendText(commandStop.charAt(i) + "");
	//
	// String commandMigrate = "migrate fd:"
	// +
	// LimboActivity.vmexecutor.get_fd(LimboActivity.vmexecutor.save_state_name)
	// + "\n";
	// for (int i = 0; i < commandMigrate.length(); i++)
	// vncCanvas.sendText(commandMigrate.charAt(i) + "");
	//
	// new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
	// @Override
	// public void run() {
	// VMListener a = new VMListener();
	// a.execute();
	// }
	// }, 0);
	// }
	// });
	// t.start();
	//
	// }

	private void onPauseVM() {
		Thread t = new Thread(new Runnable() {
			public void run() {
				// Delete any previous state file
				if (LimboActivity.vmexecutor.save_state_name != null) {
					File file = new File(LimboActivity.vmexecutor.save_state_name);
					if (file.exists()) {
						file.delete();
					}
				}

				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(getApplicationContext(), "Please wait while saving VM State", Toast.LENGTH_SHORT)
								.show();
					}
				}, 0);

				String uri = "fd:" + LimboActivity.vmexecutor.get_fd(LimboActivity.vmexecutor.save_state_name);
				// String uri = "exec:cat>" +
				// LimboActivity.vmexecutor.save_state_name;
				// final String msg = LimboActivity.vmexecutor.pausevm(uri);

				String command = QmpClient.migrate(true, false, false, uri);
				final String msg = QmpClient.sendCommand(command);
				if (msg != null)
					Log.i(TAG, msg);

				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {
						pausedVM();
					}
				}, 1000);
				
				// new Handler(Looper.getMainLooper()).postDelayed(new
				// Runnable() {
				// @Override
				// public void run() {
				// Toast.makeText(getApplicationContext(), msg,
				// Toast.LENGTH_SHORT).show();
				// VMListener a = new VMListener();
				// a.execute();
				// }
				// }, 0);
			}
		});
		t.start();

	}

	private class VMListener extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {
			startSaveVMListener();
			return null;
		}

		@Override
		protected void onPostExecute(Void test) {
			// if (progDialog.isShowing()) {
			// progDialog.dismiss();
			// }

		}
	}

	private void fullScreen() {
		AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(this);
		showPanningState();
	}

	public void promptPause(final Activity activity) {
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Pause VM");
		TextView stateView = new TextView(activity);
		stateView.setText("This make take a while depending on the RAM size used");
		stateView.setId(201012011);
		stateView.setPadding(5, 5, 5, 5);
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Pause", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				// UIUtils.log("Searching...");
				EditText a = (EditText) alertDialog.findViewById(201012010);

				onPauseVM();

				return;
			}
		});
		alertDialog.show();

	}

	public void promptStateName(final Activity activity) {
		if (//
		(LimboActivity.currMachine.hda_img_path == null || !LimboActivity.currMachine.hda_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdb_img_path == null
						|| !LimboActivity.currMachine.hdb_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdc_img_path == null
						|| !LimboActivity.currMachine.hdc_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdd_img_path == null
						|| !LimboActivity.currMachine.hdd_img_path.contains(".qcow2")))

		{
			Toast.makeText(activity.getApplicationContext(),
					"No HDD image found, please create a qcow2 image from Limbo console", Toast.LENGTH_LONG).show();
			return;
		}
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Snapshot/State Name");
		EditText stateView = new EditText(activity);
		if (LimboActivity.currMachine.snapshot_name != null) {
			stateView.setText(LimboActivity.currMachine.snapshot_name);
		}
		stateView.setEnabled(true);
		stateView.setVisibility(View.VISIBLE);
		stateView.setId(201012010);
		stateView.setSingleLine();
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				// UIUtils.log("Searching...");
				EditText a = (EditText) alertDialog.findViewById(201012010);

				if (Config.enableSaveVMmonitor) {
					// XXX: Safer for now via the Monitor console
					onSaveSnapshot(a.getText().toString());
				} else {
					// FIXME: This saves the vm natively but cannot resume the
					// vm
					LimboActivity.vmexecutor.save();

				}

				return;
			}
		});
		alertDialog.show();

	}

	private String checkCompletion() {
		String save_state = "";
		String pause_state = "";
		if (LimboActivity.vmexecutor != null) {
			save_state = LimboActivity.vmexecutor.get_save_state();
			pause_state = LimboActivity.vmexecutor.get_pause_state();
			Log.d(TAG, "save_state = " + save_state);
			Log.d(TAG, "pause_state = " + pause_state);
		}
		if (pause_state.equals("SAVING")) {
			return pause_state;
		} else if (pause_state.equals("DONE")) {
			// FIXME: We wait for 5 secs to complete the state save not ideal
			// for large OSes
			// we should find a way to detect when QMP is really done so we
			// don't get corrupt file states
			new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
				@Override
				public void run() {
					pausedVM();
				}
			}, 100);
			return pause_state;

		}
		return save_state;
	}

	private static void onMenuHelp() {
		String url = "https://github.com/limboemu/limbo";
		Intent i = new Intent(Intent.ACTION_VIEW);
		i.setData(Uri.parse(url));
		LimboActivity.activity.startActivity(i);

	}

	public void onBackPressed() {
		if (vncCanvas != null)
			vncCanvas.closeConnection();
		super.onBackPressed();
		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				Toast toast = Toast.makeText(activity, "Disconnected from VM", Toast.LENGTH_LONG);
				toast.setGravity(Gravity.TOP | Gravity.CENTER, 0, 0);
				toast.show();
			}
		}, 100);

	}

}
