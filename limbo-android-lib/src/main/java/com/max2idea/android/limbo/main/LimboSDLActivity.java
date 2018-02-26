package com.max2idea.android.limbo.main;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.provider.DocumentFile;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.ActionBar;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;

import org.json.JSONObject;
import org.libsdl.app.ClearRenderer;
import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

/**
 * SDL Activity
 */
public class LimboSDLActivity extends SDLActivity {

	public static final int KEYBOARD = 10000;
	public static final int QUIT = 10001;
	public static final int HELP = 10002;
	private boolean monitorMode = false;
	private boolean mouseOn = false;
	private Object lockTime = new Object();
	private boolean timeQuit = false;
	private Thread timeListenerThread;
	private ProgressDialog progDialog;
	public static Activity activity ;

	public String cd_iso_path = null;

	// HDD
	public String hda_img_path = null;
	public String hdb_img_path = null;
	public String hdc_img_path = null;
	public String hdd_img_path = null;

	public String fda_img_path = null;
	public String fdb_img_path = null;
	public String cpu = null;
	public String TAG = "VMExecutor";

	public int aiomaxthreads = 1;
	// Default Settings
	public int memory = 128;
	public String bootdevice = null;
	// net
	public String net_cfg = "None";
	public int nic_num = 1;
	public String vga_type = "std";
	public String hd_cache = "default";
	public String nic_driver = null;
    public String soundcard = null;
	public String lib = "liblimbo.so";
	public String lib_path = null;
	public int restart = 0;
	public String snapshot_name = "limbo";
	public int disableacpi = 0;
	public int disablehpet = 0;
	public int disabletsc = 0;
	public static int enablebluetoothmouse = 0;
	public int enableqmp = 0;
	public int enablevnc = 0;
	public String vnc_passwd = null;
	public int vnc_allow_external = 0;
	public String qemu_dev = null;
	public String qemu_dev_value = null;
	public String base_dir = Config.basefiledir;
	public String dns_addr = null;
	private boolean once = true;
	private boolean zoomable = false;
	private String status = null;

	public static Handler handler;

	// This is what SDL runs in. It invokes SDL_main(), eventually
	private static Thread mSDLThread;

	// EGL private objects
	private static EGLContext mEGLContext;
	private static EGLSurface mEGLSurface;
	private static EGLDisplay mEGLDisplay;
	private static EGLConfig mEGLConfig;
	private static int mGLMajor, mGLMinor;

	public static int width;
	public static int height;
	public static int screen_width;
	public static int screen_height;

	private static Activity activity1;

	// public static void showTextInput(int x, int y, int w, int h) {
	// // Transfer the task to the main thread as a Runnable
	// // mSingleton.commandHandler.post(new ShowTextInputHandler(x, y, w, h));
	// }

	public static void singleClick(final MotionEvent event, final int pointer_id) {
		
		Thread t = new Thread(new Runnable() {
			public void run() {
				// Log.d("SDL", "Mouse Single Click");
				SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 0, 0, 0);
				try {
					Thread.sleep(100);
				} catch (InterruptedException ex) {
					// Log.v("singletap", "Could not sleep");
				}
				SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, 0, 0);
			}
		});
		t.start();
	}

	private void promptBluetoothMouse(final Activity activity) {
		

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Enable Bluetooth Mouse");

		LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20,20,20,20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

		TextView textView = new TextView(activity);
		textView.setVisibility(View.VISIBLE);
		textView.setText(
				"Step 1: Disable Mouse Acceleration inside the Guest OS.\n\tFor DSL use command: dsl@box:/>xset m 1\n"
						+ "Step 2: Pair your Bluetooth Mouse and press OK!\n"
						+ "Step 3: Move your mouse pointer to all desktop corners to calibrate.\n");

        LinearLayout.LayoutParams textViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
		mLayout.addView(textView, textViewParams);
		alertDialog.setView(mLayout);

		final Handler handler = this.handler;

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);
				LimboSDLActivity.singleClick(a, 0);
				// SDLActivityCommon.onNativeMouseReset(0, 0,
				// MotionEvent.ACTION_MOVE, vm_width / 2, vm_height / 2, 0);
				LimboSDLActivity.enablebluetoothmouse = 1;

			}
		});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				LimboSDLActivity.enablebluetoothmouse = 0;
				return;
			}
		});
		alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				LimboSDLActivity.enablebluetoothmouse = 0;
				return;

			}
		});
		alertDialog.show();

	}

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
			if (fileType != null && file != null && drives!=null) {
				drives.setDriveAttr(fileType, file);
			}

		} else if (requestCode == Config.REQUEST_SDCARD_CODE) {
			if (data != null) {
				Uri uri = data.getData();
				DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
				String file = uri.toString();

				activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

                if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                    final int takeFlags = Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
                    getContentResolver().takePersistableUriPermission(uri, takeFlags);
                }

				// Protect from qemu thinking it's a protocol
				file = ("/" + file).replace(":", "");

				if (drives != null && drives.filetype != null && file != null) {

					final String fileTmp = file;
					Thread thread = new Thread(new Runnable() {
						public void run() {

							drives.setDriveAttr(drives.filetype, fileTmp);
						}
					});
					thread.setPriority(Thread.MIN_PRIORITY);
					thread.start();
				}
			}

		}

		// Check if says open

	}

	public void setParams(Machine machine) {

		if (machine == null) {
			return;
		}
		memory = machine.memory;
		vga_type = machine.vga_type;
		hd_cache = machine.hd_cache;
		snapshot_name = machine.snapshot_name;
		disableacpi = machine.disableacpi;
		disablehpet = machine.disablehpet;
		disabletsc = machine.disablefdbootchk;
		// enablebluetoothmouse = machine.bluetoothmouse;
		enableqmp = machine.enableqmp;
		enablevnc = machine.enablevnc;

		if (machine.cpu.endsWith("(64Bit)")) {
			// lib_path = FileUtils.getDataDir() +
			// "/lib/libqemu-system-x86_64.so";
			cpu = machine.cpu.split(" ")[0];
		} else {
			cpu = machine.cpu;
			// x86_64 can run 32bit as well as no need for the extra lib
			// lib_path = FileUtils.getDataDir() +
			// "/lib/libqemu-system-x86_64.so";
		}
		// Add other archs??

		// Load VM library
		// loadNativeLibs("libSDL.so");
		// loadNativeLibs("libSDL_image.so");
		// loadNativeLibs("libmikmod.so");
		// loadNativeLibs("libSDL_mixer.so");
		// loadNativeLibs("libSDL_ttf.so");
		// loadNativeLibs(lib);

		if (machine.cd_iso_path == null || machine.cd_iso_path.equals("None")) {
			cd_iso_path = null;
		} else {
			cd_iso_path = machine.cd_iso_path;
		}
		if (machine.hda_img_path == null || machine.hda_img_path.equals("None")) {
			hda_img_path = null;
		} else {
			hda_img_path = machine.hda_img_path;
		}

		if (machine.hdb_img_path == null || machine.hdb_img_path.equals("None")) {
			hdb_img_path = null;
		} else {
			hdb_img_path = machine.hdb_img_path;
		}

		if (machine.hdc_img_path == null || machine.hdc_img_path.equals("None")) {
			hdc_img_path = null;
		} else {
			hdc_img_path = machine.hdc_img_path;
		}

		if (machine.hdd_img_path == null || machine.hdd_img_path.equals("None")) {
			hdd_img_path = null;
		} else {
			hdd_img_path = machine.hdd_img_path;
		}

		if (machine.fda_img_path == null || machine.fda_img_path.equals("None")) {
			fda_img_path = null;
		} else {
			fda_img_path = machine.fda_img_path;
		}

		if (machine.fdb_img_path == null || machine.fdb_img_path.equals("None")) {
			fdb_img_path = null;
		} else {
			fdb_img_path = machine.fdb_img_path;
		}
		if (machine.bootdevice == null) {
			bootdevice = null;
		} else if (machine.bootdevice.equals("Default")) {
			bootdevice = null;
		} else if (machine.bootdevice.equals("CD Rom")) {
			bootdevice = "d";
		} else if (machine.bootdevice.equals("Floppy")) {
			bootdevice = "a";
		} else if (machine.bootdevice.equals("Hard Disk")) {
			bootdevice = "c";
		}

		if (machine.net_cfg == null || machine.net_cfg.equals("None")) {
			net_cfg = "none";
			nic_driver = null;
		} else if (machine.net_cfg.equals("User")) {
			net_cfg = "user";
			nic_driver = machine.nic_driver;
		}

		soundcard = machine.soundcard;

	}

	public static void sendCtrlAtlKey(int code) {

		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_LEFT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_ALT_LEFT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyDown(code);
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_LEFT);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_ALT_LEFT);
		SDLActivity.onNativeKeyUp(code);
	}

	public void stopTimeListener() {
		Log.v("SaveVM", "Stopping Listener");
		synchronized (this.lockTime) {
			this.timeQuit = true;
			this.lockTime.notifyAll();
		}
	}

	public void onDestroy() {

		// Now wait for the SDL thread to quit
		Log.v("LimboSDL", "Waiting for SDL thread to quit");
		if (mSDLThread != null) {
			try {
				mSDLThread.join();
			} catch (Exception e) {
				Log.v("SDL", "Problem stopping thread: " + e);
			}
			mSDLThread = null;

			Log.v("SDL", "Finished waiting for SDL thread");
		}
		this.stopTimeListener();
		super.onDestroy();
	}

	public void timeListener() {
		while (timeQuit != true) {
			status = checkCompletion();
			// Log.v("timeListener", "Status: " + status);
			if (status == null
                    || status.equals("")
                    || status.equals("DONE")
                    || status.equals("ERROR")
                    ) {
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

	public void startTimeListener() {
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
		Log.v("Listener", "Time listener thread exited...");

	}

	public DrivesDialogBox drives = null;

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {
		// Log.v("Limbo", "Inside Options Check");
		super.onOptionsItemSelected(item);
		if (item.getItemId() == R.id.itemDrives) {
			// Show up removable devices dialog
			if (LimboActivity.currMachine.hasRemovableDevices()) {
				drives = new DrivesDialogBox(activity, R.style.Transparent, this, LimboActivity.currMachine);
				drives.show();
			} else {
				UIUtils.toastShort(activity, "No removable devices attached");
			}
		} else if (item.getItemId() == R.id.itemReset) {
			resetVM();
		} else if (item.getItemId() == R.id.itemShutdown) {
			stopVM(false);
		} else if (item.getItemId() == R.id.itemMouse) {
			onMouse();
		} else if (item.getItemId() == this.KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
			this.onKeyboard();
		} else if (item.getItemId() == R.id.itemMonitor) {
			if (this.monitorMode) {
				this.onVMConsole();
			} else {
				this.onMonitor();
			}
		} else if (item.getItemId() == R.id.itemVolume) {
			this.onSelectMenuVol();
		} else if (item.getItemId() == R.id.itemExternalMouse) {
			if (android.os.Build.VERSION.SDK_INT == android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				this.promptBluetoothMouse(activity);
			} else {
				UIUtils.toastLong(this, "External Mouse support only for ICS and above");
			}

		} else if (item.getItemId() == R.id.itemSaveState) {
			this.promptPause(activity);
		} else if (item.getItemId() == R.id.itemSaveSnapshot) {
			this.promptStateName(activity);
		} else if (item.getItemId() == R.id.itemFitToScreen) {
			setFitToScreen();
		} else if (item.getItemId() == R.id.itemStretchToScreen) {
			setStretchToScreen();
		} else if (item.getItemId() == R.id.itemZoomIn) {
			this.setZoomIn();
		} else if (item.getItemId() == R.id.itemZoomOut) {
			this.setZoomOut();
		} else if (item.getItemId() == R.id.itemCtrlAltDel) {
			this.onCtrlAltDel();
		} else if (item.getItemId() == R.id.itemCtrlC) {
			this.onCtrlC();
		} else if (item.getItemId() == R.id.itemOneToOne) {
			this.setOneToOne();
		} else if (item.getItemId() == R.id.itemZoomable) {
			this.setZoomable();
		} else if (item.getItemId() == this.QUIT) {
		} else if (item.getItemId() == R.id.itemHelp) {
			UIUtils.onHelp(this);
		}  else if (item.getItemId() == R.id.itemHideToolbar) {
            this.onHideToolbar();
        } else if (item.getItemId() == R.id.itemDisplayRefreshRate) {
            this.onSelectMenuGUIRefreshRate();
		} else if (item.getItemId() == R.id.itemViewLog) {
            this.onViewLog();
        }
		// this.canvas.requestFocus();

//		if (!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
//			ActionBar bar = this.getSupportActionBar();
//			if (bar != null) {
//				if (bar.isShowing()) {
//					bar.hide();
//				} else
//					bar.show();
//			}
//		}

        this.supportInvalidateOptionsMenu();
		return true;
	}

    public void onViewLog() {
        FileUtils.viewLimboLog(this);
    }

    public void onHideToolbar(){
        ActionBar bar = this.getSupportActionBar();
        if (bar != null) {
            bar.hide();
        }
    }


	private void onMouse() {

		MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);
		LimboSDLActivity.singleClick(a, 0);
		// SDLActivityCommon.onNativeMouseReset(0, 0, MotionEvent.ACTION_MOVE,
		// vm_width / 2, vm_height / 2, 0);
		Toast.makeText(this.getApplicationContext(), "Mouse Trackpad Mode enabled", Toast.LENGTH_SHORT).show();
	}

	private void onCtrlAltDel() {
		
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_ALT_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_FORWARD_DEL);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_FORWARD_DEL);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_ALT_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	private void onCtrlC() {
		
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_C);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_C);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_RIGHT);
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	public void resetVM() {

		new AlertDialog.Builder(this).setTitle("Reset VM")
				.setMessage("To avoid any corrupt data make sure you "
						+ "have already shutdown the Operating system from within the VM. Continue?")
				.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						new Thread(new Runnable() {
							public void run() {
								Log.v("SDL", "VM is reset");
								onRestartVM();
							}
						}).start();

					}
				}).setNegativeButton("No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
					}
				}).show();
	}

	public void stopVM(boolean exit) {

		new AlertDialog.Builder(this).setTitle("Shutdown VM")
				.setMessage("To avoid any corrupt data make sure you "
						+ "have already shutdown the Operating system from within the VM. Continue?")
				.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						new Thread(new Runnable() {
							public void run() {
								Log.v("SDL", "VM is stopped");
								nativeQuit();
							}
						}).start();

					}
				}).setNegativeButton("No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
					}
				}).show();
	}

	private static void setStretchToScreen() {
		

		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = true;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAtlKey(KeyEvent.KEYCODE_6);
			}
		}).start();

	}

	private static void setFitToScreen() {
		

		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = true;
				sendCtrlAtlKey(KeyEvent.KEYCODE_5);

			}
		}).start();

	}

	private void setOneToOne() {
		
		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAtlKey(KeyEvent.KEYCODE_U);
			}
		}).start();

	}

	private void setFullScreen() {
		

		new Thread(new Runnable() {
			public void run() {
				sendCtrlAtlKey(KeyEvent.KEYCODE_F);
			}
		}).start();

	}

	private void setZoomIn() {
		
		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAtlKey(KeyEvent.KEYCODE_4);
			}
		}).start();

	}

	private void setZoomOut() {
		

		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAtlKey(KeyEvent.KEYCODE_3);

			}
		}).start();

	}

	private void setZoomable() {
		
		zoomable = true;

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
		// Log.v("Limbo", "Inside Options Created");
		getMenuInflater().inflate(R.menu.sdlactivitymenu, menu);

		// if (vncCanvas.scaling != null) {
		// menu.findItem(vncCanvas.scaling.getId()).setChecked(true);
		// }

		// Remove snapshots for now
		menu.removeItem(menu.findItem(R.id.itemSaveSnapshot).getItemId());

		// Remove Monitor console for SDL2 it creates 2 SDL windows and SDL for
		// android supports only 1
		menu.removeItem(menu.findItem(R.id.itemMonitor).getItemId());

		// Remove scaling for now
		menu.removeItem(menu.findItem(R.id.itemScaling).getItemId());

		// Remove external mouse for now
		menu.removeItem(menu.findItem(R.id.itemExternalMouse).getItemId());

        menu.removeItem(menu.findItem(R.id.itemCtrlAltDel).getItemId());
        menu.removeItem(menu.findItem(R.id.itemCtrlC).getItemId());

        if (LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
            menu.removeItem(menu.findItem(R.id.itemHideToolbar).getItemId());
        }

        if (soundcard==null || soundcard.equals("None")) {
            menu.removeItem(menu.findItem(R.id.itemVolume).getItemId());
        }

		// if (this.monitorMode) {
		// menu.findItem(R.id.itemMonitor).setTitle("VM Console");
		//
		// } else {
		// menu.findItem(R.id.itemMonitor).setTitle("Monitor Console");
		//
		// }
		//
		// // Menu inputMenu = menu.findItem(R.id.itemInputMode).getSubMenu();
		//
		// if (this.mouseOn) { // Panning is disable for now
		// menu.findItem(R.id.itemMouse).setTitle("Pan (Mouse Off)");
		// menu.findItem(R.id.itemMouse).setIcon(R.drawable.pan);
		// } else {
		// menu.findItem(R.id.itemMouse).setTitle("Enable Mouse");
		// menu.findItem(R.id.itemMouse).setIcon(R.drawable.mouse);
		//
		// }

        int maxMenuItemsShown = 4;
        int actionShow = MenuItemCompat.SHOW_AS_ACTION_IF_ROOM;
        if(UIUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 6;
            actionShow = MenuItemCompat.SHOW_AS_ACTION_ALWAYS;
        }
        for (int i = 0; i < menu.size() && i < maxMenuItemsShown; i++) {
            MenuItemCompat.setShowAsAction(menu.getItem(i), actionShow);
        }

		return true;

	}

	private void onMonitor() {
		new Thread(new Runnable() {
			public void run() {
				monitorMode = true;
				// final KeyEvent altDown = new KeyEvent(downTime, eventTime,
				// KeyEvent.ACTION_DOWN,
				// KeyEvent.KEYCODE_2, 1, KeyEvent.META_ALT_LEFT_ON);
				sendCtrlAtlKey(KeyEvent.KEYCODE_2);
				// sendCtrlAtlKey(altDown);
				Log.v("Limbo", "Monitor On");
			}
		}).start();

	}

	private void onVMConsole() {
		monitorMode = false;
		sendCtrlAtlKey(KeyEvent.KEYCODE_1);
	}

	private void onSaveState(final String stateName) {
		// onMonitor();
		// try {
		// Thread.sleep(1000);
		// } catch (InterruptedException ex) {
		// Logger.getLogger(LimboVNCActivity.class.getName()).log(
		// Level.SEVERE, null, ex);
		// }
		// vncCanvas.sendText("savevm " + stateName + "\n");
		// Toast.makeText(this.getApplicationContext(),
		// "Please wait while saving VM State", Toast.LENGTH_LONG).show();
		new Thread(new Runnable() {
			public void run() {
				Log.v("SDL", "Saving VM1");
				nativePause();
				// LimboActivity.vmexecutor.saveVM1(stateName);

				nativeResume();

			}
		}).start();

		// try {
		// Thread.sleep(1000);
		// } catch (InterruptedException ex) {
		// Logger.getLogger(LimboVNCActivity.class.getName()).log(
		// Level.SEVERE, null, ex);
		// }
		// onSDL();
		((LimboActivity) LimboActivity.activity).saveSnapshotDB(stateName);

		progDialog = ProgressDialog.show(activity, "Please Wait", "Saving VM State...", true);
		SaveVM a = new SaveVM();
		a.execute();

	}

	public void saveStateDB(String snapshot_name) {
		LimboActivity.currMachine.snapshot_name = snapshot_name;
		int ret = MachineOpenHelper.getInstance(activity).deleteMachine(LimboActivity.currMachine);
		ret = MachineOpenHelper.getInstance(activity).insertMachine(LimboActivity.currMachine);

	}

	private void onSaveState1(String stateName) {
		// Log.v("onSaveState1", stateName);
		monitorMode = true;
		sendCtrlAtlKey(KeyEvent.KEYCODE_2);
		try {
			Thread.sleep(3000);
		} catch (InterruptedException ex) {
			Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
		}
		sendText("savevm " + stateName + "\n");
		saveStateDB(stateName);
		try {
			Thread.sleep(3000);
		} catch (InterruptedException ex) {
			Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
		}
		sendCommand(COMMAND_SAVEVM, "vm");

	}

	// FIXME: We need this to able to catch complex characters strings like
	// grave and send it as text
	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		if (event.getAction() == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
			sendText(event.getCharacters().toString());
			return true;
		} else
			return super.dispatchKeyEvent(event);

	}

	private static void sendText(String string) {
		
		// Log.v("sendText", string);
		KeyCharacterMap keyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
		KeyEvent[] keyEvents = keyCharacterMap.getEvents(string.toCharArray());
		if (keyEvents != null)
			for (int i = 0; i < keyEvents.length; i++) {

				if (keyEvents[i].getAction() == KeyEvent.ACTION_DOWN) {
					// Log.v("sendText", "Up: " + keyEvents[i].getKeyCode());
					SDLActivity.onNativeKeyDown(keyEvents[i].getKeyCode());
				} else if (keyEvents[i].getAction() == KeyEvent.ACTION_UP) {
					// Log.v("sendText", "Down: " + keyEvents[i].getKeyCode());
					SDLActivity.onNativeKeyUp(keyEvents[i].getKeyCode());
				}
			}
	}

	private class SaveVM extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... arg0) {
			// Log.v("handler", "Save VM");
			startTimeListener();
			return null;
		}

		@Override
		protected void onPostExecute(Void test) {
			try {
				if (progDialog.isShowing()) {
					progDialog.dismiss();
				}
				monitorMode = false;
				sendCtrlAtlKey(KeyEvent.KEYCODE_1);
			} catch (Exception e) {
				e.printStackTrace();
			}

		}
	}

	private void fullScreen() {
		// AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(
		// this);
		// showPanningState();
	}

	public void promptStateName(final Activity activity) {
		// Log.v("promptStateName", "ask");
		if ((LimboActivity.currMachine.hda_img_path == null
				|| !LimboActivity.currMachine.hda_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdb_img_path == null
						|| !LimboActivity.currMachine.hdb_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdc_img_path == null
						|| !LimboActivity.currMachine.hdc_img_path.contains(".qcow2"))
				&& (LimboActivity.currMachine.hdd_img_path == null
						|| !LimboActivity.currMachine.hdd_img_path.contains(".qcow2")))

		{
            UIUtils.toastLong(this, "No HDD image found, please create a qcow2 image from Limbo console");
			return;
		}
		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Snapshot/State Name");
		final EditText stateView = new EditText(activity);
		if (LimboActivity.currMachine.snapshot_name != null) {
			stateView.setText(LimboActivity.currMachine.snapshot_name);
		}
		stateView.setEnabled(true);
		stateView.setVisibility(View.VISIBLE);
		stateView.setSingleLine();
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {

				progDialog = ProgressDialog.show(activity, "Please Wait", "Saving VM State...", true);
				new Thread(new Runnable() {
					public void run() {
						// Log.v("promptStateName", a.getText().toString());
						onSaveState1(stateView.getText().toString());
					}
				}).start();

				return;
			}
		});
		alertDialog.show();

	}

	public void pausedVM() {

		LimboActivity.vmexecutor.paused = 1;
		((LimboActivity) LimboActivity.activity).saveStateVMDB();

		new AlertDialog.Builder(this).setTitle("Paused").setMessage("VM is now Paused tap OK to exit")
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						if (LimboActivity.vmexecutor != null) {
                            LimboActivity.vmexecutor.stopvm(0);
						} else if (activity.getParent() != null) {
							activity.getParent().finish();
						} else {
							activity.finish();
						}
					}
				}).show();
	}


    public void pausedErrorVM(String errStr) {


        new AlertDialog.Builder(this).setTitle("Error").setMessage(errStr)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        Thread t = new Thread(new Runnable() {
                            public void run() {
                                String command = QmpClient.cont();
                                String msg = QmpClient.sendCommand(command);
                            }
                        });
                        t.start();
                    }
                }).show();
    }

	private String checkCompletion() {
		String save_state = "";
		String pause_state = "";
		if (LimboActivity.vmexecutor != null) {
			// Get the state of saving full disk snapshot
			save_state = LimboActivity.vmexecutor.get_save_state();

			// Get the state of saving the VM memory only
			pause_state = LimboActivity.vmexecutor.get_pause_state();
//			Log.d(TAG, "save_state = " + save_state);
//			Log.d(TAG, "pause_state = " + pause_state);
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

		} else if (pause_state.equals("ERROR")) {
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    pausedErrorVM("Could not pause VM. View log file for details");
                }
            }, 100);
            return pause_state;
        }
		return save_state;
	}

	private static boolean fitToScreen = Config.enable_qemu_fullScreen;
	private static boolean stretchToScreen = false; // Start with fitToScreen

	// Setup
	protected void onCreate(Bundle savedInstanceState) {
		// Log.v("SDL", "onCreate()");
        activity = this;
		UIUtils.setOrientation(this);

		if (LimboSettingsManager.getFullscreen(this))
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		super.onCreate(savedInstanceState);

		Log.v("SDL", "Max Mem = " + Runtime.getRuntime().maxMemory());
		this.handler = commandHandler;
		this.activity1 = this;

		if (LimboActivity.currMachine == null) {
			Log.v("SDLAcivity", "No VM selected!");
		}else 
			setParams(LimboActivity.currMachine);

		// So we can call stuff from static callbacks
		mSingleton = this;

		createUI(0, 0);

		Toast toast = Toast.makeText(activity, "Press Volume Down for Right Click", Toast.LENGTH_SHORT);
		toast.setGravity(Gravity.TOP | Gravity.CENTER, 0, 0);
		toast.show();
		// new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
		// @Override
		// public void run() {
		// UIUtils.setOrientation(activity);
		// }
		// }, 2000);

		UIUtils.setupToolBar(this);

		UIUtils.showHints(this);

		this.resumeVM();

	}

	public SDLSurface getSDLSurface() {
		
		if (mSurface == null)
			mSurface = new SDLSurface(activity);
		return mSurface;
	}

	private void setScreenSize() {
		
		// WindowManager wm = (WindowManager) this
		// .getSystemService(Context.WINDOW_SERVICE);
		// Display display = wm.getDefaultDisplay();
		// this.screen_width = display.getWidth();
		// this.screen_height = display.getHeight();

	}

	private void createUI(int w, int h) {
		
		// Set up the surface
		mSurface = getSDLSurface();
		mSurface.setRenderer(new ClearRenderer());

		// mSurface.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
		// setContentView(mSurface);

		int width = w;
		int height = h;
		if (width == 0) {
			width = RelativeLayout.LayoutParams.WRAP_CONTENT;
		}
		if (height == 0) {
			height = RelativeLayout.LayoutParams.WRAP_CONTENT;
		}

		setContentView(R.layout.main_sdl);

		RelativeLayout mLayout = (RelativeLayout) findViewById(R.id.sdl);
		RelativeLayout.LayoutParams surfaceParams = new RelativeLayout.LayoutParams(width, height);
		surfaceParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		surfaceParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);

		mLayout.addView(mSurface, surfaceParams);

		SurfaceHolder holder = mSurface.getHolder();
		setScreenSize();
	}

	protected void onPause() {
		Log.v("SDL", "onPause()");
		LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Suspended");
		super.onPause();

	}

	private void onKeyboard() {
		InputMethodManager inputMgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		// inputMgr.toggleSoftInput(0, 0);
		inputMgr.showSoftInput(this.mSurface, InputMethodManager.SHOW_FORCED);
	}

	public void onSelectMenuVol() {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Volume");

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

		LinearLayout t = createVolumePanel();
		t.setLayoutParams(volParams);

		ScrollView s = new ScrollView(activity);
		s.addView(t);
		alertDialog.setView(s);
		alertDialog.setButton(Dialog.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {

			public void onClick(DialogInterface dialog, int which) {
				alertDialog.cancel();
			}
		});
		alertDialog.show();

	}

	public LinearLayout createVolumePanel() {
		LinearLayout layout = new LinearLayout (this);
		layout.setPadding(20, 20, 20, 20);

        LinearLayout.LayoutParams volparams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);

		SeekBar vol = new SeekBar(this);
		vol.setMax(maxVolume);
		int volume = getCurrentVolume();
		vol.setProgress(volume);
		vol.setLayoutParams(volparams);

		((SeekBar) vol).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar s, int progress, boolean touch) {
                setVolume(progress);
			}

			public void onStartTrackingTouch(SeekBar arg0) {

			}

			public void onStopTrackingTouch(SeekBar arg0) {

			}
		});

		layout.addView(vol);

		return layout;

	}

	protected void onResume() {
		Log.v("SDL", "onResume()");

		LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Running");

		// if (status == null || status.equals("") || status.equals("DONE"))
		// SDLActivity.nativeResume();

		// mSurface.reSize();
		// new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
		// @Override
		// public void run() {
		// UIUtils.setOrientation(activity);
		// }
		// }, 1000);

		super.onResume();
		onMouse();
	}

	// static void resume() {
	// Log.v("Resume", "Resuming -> Full Screeen");
	// if (SDLActivityCommon.fitToScreen)
	// SDLActivityCommon.setFitToScreen();
	// if (SDLActivityCommon.stretchToScreen)
	// SDLActivityCommon.setStretchToScreen();
	// else
	// LimboActivity.vmexecutor.toggleFullScreen();
	// }

	// Messages from the SDLMain thread
	static int COMMAND_CHANGE_TITLE = 1;
	static int COMMAND_SAVEVM = 2;

	public void loadLibraries() {
		// No loading of .so we do it outside
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

	public void promptPause(final Activity activity) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Pause VM");
		TextView stateView = new TextView(activity);
		stateView.setText("This make take a while depending on the RAM size used");
		stateView.setPadding(10, 10, 10, 10);
		alertDialog.setView(stateView);

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Pause", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				onPauseVM();
				return;
			}
		});
		alertDialog.show();
	}

	public void startSaveVMListener() {
		stopTimeListener();
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
//		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
//			@Override
//			public void run() {
//				Toast.makeText(getApplicationContext(), "VM State saved", Toast.LENGTH_LONG).show();
//			}
//		}, 1000);

		Log.v("Listener", "Time listener thread exited...");

	}

	// Currently not working due to SDL can only support 1 window for Android
	private void onHMP() {
		monitorMode = true;
		sendCtrlAtlKey(KeyEvent.KEYCODE_2);

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
	// onHMP();
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
	// sendText(commandStop.charAt(i) + "");
	//
	// String commandMigrate = "migrate fd:"
	// +
	// LimboActivity.vmexecutor.get_fd(LimboActivity.vmexecutor.save_state_name)
	// + "\n";
	// for (int i = 0; i < commandMigrate.length(); i++)
	// sendText(commandMigrate.charAt(i) + "");
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
				String command = QmpClient.stop();
				String msg = QmpClient.sendCommand(command);
				if (msg != null)
					Log.i(TAG, msg);
				command = QmpClient.migrate(false, false, uri);
				msg = QmpClient.sendCommand(command);
				if (msg != null) {
                    Log.i(TAG, msg);
                    processMigrationResponse(msg);
                }

				// XXX: We cant be sure that the machine state is completed
				// saving
				// new Handler(Looper.getMainLooper()).postDelayed(new
				// Runnable() {
				// @Override
				// public void run() {
				// pausedVM();
				// }
				// }, 1000);

				// XXX: Instead we poll to see if migration is complete
				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {
						VMListener a = new VMListener();
						a.execute();
					}
				}, 0);
			}
		});
		t.start();

	}

    private void processMigrationResponse(String response) {
        String errorStr = null;
        try {
            JSONObject object = new JSONObject(response);
            errorStr = object.getString("error");
        }catch (Exception ex) {

        }
            if (errorStr != null) {
                String descStr = null;

                try {
                    JSONObject descObj = new JSONObject(errorStr);
                    descStr = descObj.getString("desc");
                }catch (Exception ex) {

                }
                final String descStr1 = descStr;

                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        pausedErrorVM(descStr1!=null?descStr1:"Could not pause VM. View log for details");
                    }
                }, 100);

            }

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

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		boolean res = this.mSurface.onTouchProcess(this.mSurface, event);
		res = this.mSurface.onTouchEventProcess(event);
		return false;
	}

	private void resumeVM() {
		Thread t = new Thread(new Runnable() {
			public void run() {
				if (LimboActivity.vmexecutor.paused == 1) {

					try {
						Thread.sleep(3000);
					} catch (InterruptedException ex) {
						Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
					}
					LimboActivity.vmexecutor.paused = 0;
					// new Handler(Looper.getMainLooper()).postDelayed(new
					// Runnable() {
					// @Override
					// public void run() {
					// Toast.makeText(getApplicationContext(), "Please wait
					// while resuming VM State",
					// Toast.LENGTH_SHORT).show();
					// }
					// }, 500);

					String command = QmpClient.cont();
					String msg = QmpClient.sendCommand(command);
					if (msg != null)
						Log.i(TAG, msg);
					new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
						@Override
						public void run() {
							onMouse();
						}
					}, 500);
				}
			}
		});
		t.start();

	}

	public void onBackPressed() {
		// Log.d(TAG, "Pressed Back");

		// super.onBackPressed();
		if (!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
			ActionBar bar = this.getSupportActionBar();
			if (bar != null) {
				if (bar.isShowing()) {
					bar.hide();
				} else
					bar.show();
			}
		} else {
			stopVM(false);
		}

	}

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        this.supportInvalidateOptionsMenu();
    }

    public void onSelectMenuGUIRefreshRate() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Display Refresh Rate (Hz)");

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

        LinearLayout t = createGUIRefreshRatePanel();
        t.setLayoutParams(volParams);

        ScrollView s = new ScrollView(activity);
        s.addView(t);
        alertDialog.setView(s);
        alertDialog.setButton(Dialog.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {

            public void onClick(DialogInterface dialog, int which) {
                alertDialog.cancel();
            }
        });
        alertDialog.show();

    }


    public LinearLayout createGUIRefreshRatePanel() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        int currRate = getCurrentGUIRefreshRate();

        final TextView value = new TextView(this);
        value.setText(currRate+" Hz");
        layout.addView(value);
        value.setLayoutParams(params);

        SeekBar rate = new SeekBar(this);
        rate.setMax(Config.MAX_VNC_REFRESH_RATE);

        rate.setProgress(currRate);
        rate.setLayoutParams(params);

        ((SeekBar) rate).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar s, int progress, boolean touch) {
                value.setText(progress+1+" Hz");
            }

            public void onStartTrackingTouch(SeekBar arg0) {

            }

            public void onStopTrackingTouch(SeekBar arg0) {
                int progress = arg0.getProgress()+1;
                LimboActivity.vmexecutor.changevar("gui_refresh_interval_default", 1000 / progress);
            }
        });


        layout.addView(rate);

        return layout;

    }
    public int getCurrentGUIRefreshRate() {
        return 1000 / LimboActivity.vmexecutor.getvar("gui_refresh_interval_default");
    }
}
