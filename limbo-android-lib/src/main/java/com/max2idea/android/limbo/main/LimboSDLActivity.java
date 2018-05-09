package com.max2idea.android.limbo.main;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Point;
import android.media.AudioManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Vibrator;
import android.support.v4.provider.DocumentFile;
import android.support.v4.view.MenuItemCompat;

import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.jni.VMExecutor;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;

import org.json.JSONException;
import org.json.JSONObject;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLControllerManager;

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
	public static final String TAG = "LimboSDLActivity";
    protected static ViewGroup mMainLayout;
	public static final int KEYBOARD = 10000;
	public static final int QUIT = 10001;
	public static final int HELP = 10002;
	private boolean monitorMode = false;
	private boolean mouseOn = false;
	private Object lockTime = new Object();
	private boolean timeQuit = false;
	private Thread timeListenerThread;
	private ProgressDialog progDialog;
	public static LimboSDLActivity activity ;

	public String cd_iso_path = null;

	// HDD
	public String hda_img_path = null;
	public String hdb_img_path = null;
	public String hdc_img_path = null;
	public String hdd_img_path = null;

	public String fda_img_path = null;
	public String fdb_img_path = null;
	public String cpu = null;

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
	public int enableqmp = 0;
	public int enablevnc = 0;
	public String vnc_passwd = null;
	public int vnc_allow_external = 0;
	public String qemu_dev = null;
	public String qemu_dev_value = null;
	public String base_dir = null;
	public String dns_addr = null;
	private boolean once = true;
	private boolean zoomable = false;
	private String status = null;


	// This is what SDL runs in. It invokes SDL_main(), eventually
	private static Thread mSDLThread;

	// EGL private objects
	private static EGLContext mEGLContext;
	private static EGLSurface mEGLSurface;
	private static EGLDisplay mEGLDisplay;
	private static EGLConfig mEGLConfig;
	private static int mGLMajor, mGLMinor;


	private static Activity activity1;

	// public static void showTextInput(int x, int y, int w, int h) {
	// // Transfer the task to the main thread as a Runnable
	// // mSingleton.commandHandler.post(new ShowTextInputHandler(x, y, w, h));
	// }

	public static void singleClick(final MotionEvent event, final int pointer_id) {
		
		Thread t = new Thread(new Runnable() {
			public void run() {
				// Log.d("SDL", "Mouse Single Click");
				try {
					Thread.sleep(200);
				} catch (InterruptedException ex) {
					// Log.v("singletap", "Could not sleep");
				}
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 1,0, 0);
				try {
					Thread.sleep(200);
				} catch (InterruptedException ex) {
					// Log.v("singletap", "Could not sleep");
				}
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 1, 0, 0);
			}
		});
		t.start();
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

				if(!file.contains("com.android.externalstorage.documents"))
				{
					UIUtils.showFileNotSupported(this);
					return;
				}

				activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
				activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
				activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

				final int takeFlags = data.getFlags()
						& (
						Intent.FLAG_GRANT_READ_URI_PERMISSION |
								Intent.FLAG_GRANT_WRITE_URI_PERMISSION
				);
                getContentResolver().takePersistableUriPermission(uri, takeFlags);


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

	public static void delayKey(int ms) {
		try {
			Thread.sleep(ms);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	public static void sendCtrlAltKey(int code) {
		delayKey(100);
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_LEFT);
		delayKey(100);
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_ALT_LEFT);
		delayKey(100);
		if(code>=0) {
			SDLActivity.onNativeKeyDown(code);
			delayKey(100);
			SDLActivity.onNativeKeyUp(code);
			delayKey(100);
		}
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_ALT_LEFT);
		delayKey(100);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_LEFT);
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

		LimboActivity.vmexecutor.doStopVM(0);
		super.onDestroy();
	}


	public void checkStatus() {
		while (timeQuit != true) {
			String status = checkCompletion();
			Log.v(TAG, "Status: " + status);
			if (status == null
					//|| status.equals("")
					|| status.toUpperCase().equals("COMPLETED")
					|| status.toUpperCase().equals("FAILED")
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
			checkStatus();
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
	public static boolean toggleKeyboardFlag = true;

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
            onMouseMode();
		} else if (item.getItemId() == this.KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
			toggleKeyboardFlag = UIUtils.onKeyboard(this, toggleKeyboardFlag);
		}
        else if (item.getItemId() == R.id.itemMonitor) {
			if (this.monitorMode) {
				this.onVMConsole();
			} else {
				this.onMonitor();
			}
		} else if (item.getItemId() == R.id.itemVolume) {
			this.onSelectMenuVol();
		} else if (item.getItemId() == R.id.itemSaveState) {
			this.promptPause(activity);
		} else if (item.getItemId() == R.id.itemSaveSnapshot) {
			//TODO:
			//this.promptStateName(activity);
		} else if (item.getItemId() == R.id.itemFitToScreen) {
            onFitToScreen();
		} else if (item.getItemId() == R.id.itemStretchToScreen) {
			onStretchToScreen();
		} else if (item.getItemId() == R.id.itemZoomIn) {
			this.setZoomIn();
		} else if (item.getItemId() == R.id.itemZoomOut) {
			this.setZoomOut();
		} else if (item.getItemId() == R.id.itemCtrlAltDel) {
			this.onCtrlAltDel();
		} else if (item.getItemId() == R.id.itemCtrlC) {
			this.onCtrlC();
		} else if (item.getItemId() == R.id.itemOneToOne) {
			this.onNormalScreen();
		} else if (item.getItemId() == R.id.itemZoomable) {
			this.setZoomable();
		} else if (item.getItemId() == this.QUIT) {
		} else if (item.getItemId() == R.id.itemHelp) {
			UIUtils.onHelp(this);
		}  else if (item.getItemId() == R.id.itemHideToolbar) {
            this.onHideToolbar();
        } else if (item.getItemId() == R.id.itemDisplay) {
            this.onSelectMenuSDLDisplay();
		} else if (item.getItemId() == R.id.itemViewLog) {
            this.onViewLog();
        }
		// this.canvas.requestFocus();


        this.invalidateOptionsMenu();
		return true;
	}

    public void onViewLog() {
        FileUtils.viewLimboLog(this);
    }

    public void onHideToolbar(){
        ActionBar bar = this.getActionBar();
        if (bar != null) {
            bar.hide();
        }
    }


    private void onMouseMode() {

        String [] items = {"Trackpad Mouse (Phone)",
                "Bluetooth/USB Mouse (Desktop mode)", //Physical mouse for Chromebook, Android x86 PC, or Bluetooth Mouse
        };
        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle("Mouse");
        mBuilder.setSingleChoiceItems(items, Config.mouseMode.ordinal(), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch(i){
                    case 0:
                        setUIModeMobile();
                        break;
                    case 1:
                    	setUIModeDesktop(LimboSDLActivity.this, false);
                        break;
                    default:
                        break;
                }
                dialog.dismiss();
            }
        });
        final AlertDialog alertDialog = mBuilder.create();
        alertDialog.show();

    }

	public boolean checkVMResolutionFits() {
		int width = mLayout.getWidth();
		int height = mLayout.getHeight();
		ActionBar bar = activity.getActionBar();

		if (!LimboSettingsManager.getAlwaysShowMenuToolbar(LimboSDLActivity.this)
				&& bar != null && bar.isShowing()) {
			height += bar.getHeight();
		}

		if(vm_width < width && vm_height < height)
			return true;

		return false;
	}

    public void calibration() {
		//XXX: No need to calibrate for SDL trackpad.
    }

    private void setUIModeMobile(){

        UIUtils.setOrientation(this);
        MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);

		//TODO: needed?
        //LimboSDLActivity.singleClick(a, 0);
        Config.mouseMode = Config.MouseMode.Trackpad;
		LimboActivity.vmexecutor.setRelativeMouseMode(1);
        Toast.makeText(this.getApplicationContext(), "Trackpad Enabled", Toast.LENGTH_SHORT).show();
        onFitToScreen();
        calibration();
        invalidateOptionsMenu();
    }

    private void setUIModeDesktop(final Activity activity, final boolean mouseMethodAlt) {


        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Desktop Mode");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20,20,20,20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        TextView textView = new TextView(activity);
        textView.setVisibility(View.VISIBLE);
		String desktopInstructions = this.getString(R.string.desktopInstructions);
		if(!checkVMResolutionFits()){
			String resolutionWarning = "Warning: Machine resolution "
					+ vm_width+ "x" + vm_height +
					" is too high for Desktop Mode. " +
					"Scaling will be used and Mouse Alignment will not be accurate. " +
					"Reduce display resolution within the Guest OS for better experience.\n\n";
			desktopInstructions = resolutionWarning + desktopInstructions;
		}
		textView.setText(desktopInstructions);

		ScrollView scrollView = new ScrollView(this);
		scrollView.addView(textView);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(scrollView, params);

        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

                MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);

                //TODO: needed?
                //LimboSDLActivity.singleClick(a, 0);

                //TODO: not needed?
                //SDLActivity.onNativeMouseReset(0, 0, MotionEvent.ACTION_MOVE, 0, 0, 0);
                //SDLActivity.onNativeMouseReset(0, 0, MotionEvent.ACTION_MOVE, vm_width, vm_height, 0);

                Config.mouseMode = Config.MouseMode.External;
                LimboActivity.vmexecutor.setRelativeMouseMode(0);
                Toast.makeText(LimboSDLActivity.this, "External Mouse Enabled", Toast.LENGTH_SHORT).show();
                onNormalScreen();
                calibration();
                invalidateOptionsMenu();
                alertDialog.dismiss();
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                alertDialog.dismiss();
            }
        });
        alertDialog.show();

    }

	private void onCtrlAltDel() {

		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_RIGHT);
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_ALT_RIGHT);
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_FORWARD_DEL);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_FORWARD_DEL);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_ALT_RIGHT);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_RIGHT);
	}

	private void onCtrlC() {

		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_CTRL_RIGHT);
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_C);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_C);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_CTRL_RIGHT);
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
						LimboActivity.vmexecutor.stopvm(0);
//						new Thread(new Runnable() {
//							public void run() {
//								Log.v("SDL", "VM is stopped");
//								nativeQuit();
//							}
//						}).start();

					}
				}).setNegativeButton("No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
					}
				}).show();
	}

	//TODO: not working
	private void onStretchToScreen() {


		new Thread(new Runnable() {
			public void run() {
				Log.d(TAG, "onStretchToScreen");
				LimboSDLActivity.stretchToScreen = true;
				LimboSDLActivity.fitToScreen = false;
                sendCtrlAltKey(KeyEvent.KEYCODE_F); // not working
				resize();

			}
		}).start();

	}

	private void onFitToScreen() {
		UIUtils.setOrientation(this);
        ActionBar bar = LimboSDLActivity.this.getActionBar();
        if (bar != null && !LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
            bar.hide();
        }
		new Thread(new Runnable() {
			public void run() {
				Log.d(TAG, "onFitToScreen");
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = true;
				resize();

			}
		}).start();

	}

	private void onNormalScreen() {
		ActionBar bar = LimboSDLActivity.this.getActionBar();
		if (bar != null && !LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
			bar.hide();
		}
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
		new Thread(new Runnable() {
			public void run() {
				Log.d(TAG, "onNormalScreen");
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				resize();

			}
		}).start();

	}

	public void resize() {

		//XXX: flag so no mouse events are processed
		isResizing = true;

		//XXX: This is needed so Nougat+ devices will update their layout
		new Handler(Looper.getMainLooper()).post(new Runnable() {
			@Override
			public void run() {
				((LimboSDLSurface) mSurface).getHolder().setFixedSize(0, 0);
			}
		});

		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
			@Override
			public void run() {
				((LimboSDLSurface) mSurface).doResize(false);
			}
		}, 1500);
	}

	private void setZoomIn() {

		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAltKey(KeyEvent.KEYCODE_4);
			}
		}).start();

	}

	private void setZoomOut() {


		new Thread(new Runnable() {
			public void run() {
				LimboSDLActivity.stretchToScreen = false;
				LimboSDLActivity.fitToScreen = false;
				sendCtrlAltKey(KeyEvent.KEYCODE_3);

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

        int maxMenuItemsShown = 4;
        int actionShow = MenuItemCompat.SHOW_AS_ACTION_IF_ROOM;
        if(UIUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 6;
            actionShow = MenuItemCompat.SHOW_AS_ACTION_ALWAYS;
        }

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
        //menu.removeItem(menu.findItem(R.id.itemUIMode).getItemId());

        menu.removeItem(menu.findItem(R.id.itemCtrlAltDel).getItemId());
        menu.removeItem(menu.findItem(R.id.itemCtrlC).getItemId());

        if (LimboSettingsManager.getAlwaysShowMenuToolbar(activity) || Config.mouseMode == Config.MouseMode.External) {
            menu.removeItem(menu.findItem(R.id.itemHideToolbar).getItemId());
            maxMenuItemsShown--;
        }

        if (soundcard==null || soundcard.equals("None")) {
            menu.removeItem(menu.findItem(R.id.itemVolume).getItemId());
            maxMenuItemsShown--;
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
				sendCtrlAltKey(KeyEvent.KEYCODE_2);
				// sendCtrlAltKey(altDown);
				Log.v("Limbo", "Monitor On");
			}
		}).start();

	}

	private void onVMConsole() {
		monitorMode = false;
		sendCtrlAltKey(KeyEvent.KEYCODE_1);
	}


	// FIXME: We need this to able to catch complex characters strings like
	// grave and send it as text
	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		if (event.getAction() == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
			sendText(event.getCharacters().toString());
			return true;
		}  else if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
			this.onBackPressed();
			return true;
		} if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) {
			// We emulate right click with volume down
			if(event.getAction() == KeyEvent.ACTION_DOWN) {
				MotionEvent e = MotionEvent.obtain(1000, 1000, MotionEvent.ACTION_DOWN, 0, 0, 0, 0, 0, 0, 0,
						InputDevice.SOURCE_TOUCHSCREEN, 0);
				rightClick(e, 0);
			}
			return true;
		} else if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP) {
			// We emulate middle click with volume up
			if(event.getAction() == KeyEvent.ACTION_DOWN) {
				MotionEvent e = MotionEvent.obtain(1000, 1000, MotionEvent.ACTION_DOWN, 0, 0, 0, 0, 0, 0, 0,
						InputDevice.SOURCE_TOUCHSCREEN, 0);
				middleClick(e, 0);
			}
			return true;
		} else {
			return super.dispatchKeyEvent(event);
		}

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


	public void pausedVM() {

		LimboActivity.vmexecutor.paused = 1;
		((LimboActivity) LimboActivity.activity).saveStateVMDB();

		new AlertDialog.Builder(this).setTitle("Paused").setMessage("VM is now Paused tap OK to exit")
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						//close fd
						//XXX: This is probably not needed since the qemu backend will close the fd
//						if(LimboActivity.vmexecutor.current_fd >0)
//							FileUtils.close_fd(LimboActivity.vmexecutor.current_fd);

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
//			save_state = LimboActivity.vmexecutor.get_save_state();
//
//			// Get the state of saving the VM memory only
//			pause_state = LimboActivity.vmexecutor.get_pause_state();
			//Log.d(TAG, "save_state = " + save_state);
			//Log.d(TAG, "pause_state = " + pause_state);

			String command = QmpClient.query_migrate();
			String res = QmpClient.sendCommand(command);


			if(res!=null && !res.equals("")) {
				//Log.d(TAG, "Migrate status: " + res);
				try {
					JSONObject resObj = new JSONObject(res);
					String resInfo = resObj.getString("return");
					JSONObject resInfoObj = new JSONObject(resInfo);
					pause_state = resInfoObj.getString("status");
				} catch (JSONException e) {
					if(Config.debug)
						e.printStackTrace();
				}
				if(pause_state!=null && pause_state.toUpperCase().equals("FAILED")){
					Log.e(TAG, "Error: " + res);
				}
			}
		}


		if (pause_state.toUpperCase().equals("ACTIVE")) {
			return pause_state;
		} else if (pause_state.toUpperCase().equals("COMPLETED")) {
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

		} else if (pause_state.toUpperCase().equals("FAILED")) {
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

	public static boolean fitToScreen = true;
    public static boolean stretchToScreen = false;

	// Setup
	protected void onCreate(Bundle savedInstanceState) {
		// Log.v("SDL", "onCreate()");
        activity = this;
		base_dir = Config.getBasefileDir(this);

		if (LimboSettingsManager.getFullscreen(this))
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		super.onCreate(savedInstanceState);
        setupVolume();

		mSingleton = this;

		Log.v("SDL", "Max Mem = " + Runtime.getRuntime().maxMemory());

		this.activity1 = this;

		if (LimboActivity.currMachine == null) {
			Log.v("SDLAcivity", "No VM selected!");
		}else
			setParams(LimboActivity.currMachine);

		// So we can call stuff from static callbacks
		mSingleton = this;

		createUI(0, 0);

		UIUtils.toastShortTop(activity, "Press Volume Down for Right Click");

		UIUtils.setupToolBar(this);

		UIUtils.showHints(this);

		this.resumeVM();

        UIUtils.setOrientation(this);


	}

	//TODO: not needed?
//	public SDLSurface getSDLSurface() {
//
//		if (mSurface == null)
//			mSurface = new SDLSurface(activity);
//		return mSurface;
//	}

	private void setScreenSize() {

		// WindowManager wm = (WindowManager) this
		// .getSystemService(Context.WINDOW_SERVICE);
		// Display display = wm.getDefaultDisplay();
		// this.screen_width = display.getWidth();
		// this.screen_height = display.getHeight();

	}


    private void createUI(int w, int h) {

		// Set up the surface
		//TODO:
		mSurface = new LimboSDLSurface(this);

		//TODO: needed?
		//mSurface.setRenderer(new ClearRenderer());


		int width = w;
		int height = h;
		if (width == 0) {
			width = RelativeLayout.LayoutParams.WRAP_CONTENT;
		}
		if (height == 0) {
			height = RelativeLayout.LayoutParams.WRAP_CONTENT;
		}

		setContentView(R.layout.main_sdl);

		//TODO:
        mLayout = (LinearLayout) activity.findViewById(R.id.sdl_layout);
		mMainLayout = (LinearLayout) activity.findViewById(R.id.main_layout);

		RelativeLayout mLayout = (RelativeLayout) findViewById(R.id.sdl);
		RelativeLayout.LayoutParams surfaceParams = new RelativeLayout.LayoutParams(width, height);
		surfaceParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		surfaceParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);

		//TODO: reassign?
		mLayout.addView(mSurface, surfaceParams);

		//TODO: not needed?
		//SurfaceHolder holder = mSurface.getHolder();

		setScreenSize();
	}

	protected void onPause() {
		Log.v("SDL", "onPause()");
		LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Suspended");
		super.onPause();

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

		int volume = 0;

		//TODO:
		vol.setMax(maxVolume);
		volume = getCurrentVolume();

		vol.setProgress(volume);
		vol.setLayoutParams(volparams);

		((SeekBar) vol).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar s, int progress, boolean touch) {
				//TODO:
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



		super.onResume();
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
		stateView.setPadding(20, 20, 20, 20);
		alertDialog.setView(stateView);

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Pause", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				onPauseVM();
				return;
			}
		});
		alertDialog.show();
	}

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

				LimboActivity.vmexecutor.current_fd = LimboActivity.vmexecutor.get_fd(LimboActivity.vmexecutor.save_state_name);

				String uri = "fd:" + LimboActivity.vmexecutor.current_fd;
				String command = QmpClient.stop();
				String msg = QmpClient.sendCommand(command);
//				if (msg != null)
//					Log.i(TAG, msg);
				command = QmpClient.migrate(false, false, uri);
				msg = QmpClient.sendCommand(command);
				if (msg != null) {
//                    Log.i(TAG, msg);
                    processMigrationResponse(msg);
                }

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
			if(Config.debug)
				ex.printStackTrace();
        }
            if (errorStr != null) {
                String descStr = null;

                try {
                    JSONObject descObj = new JSONObject(errorStr);
                    descStr = descObj.getString("desc");
                }catch (Exception ex) {
					if(Config.debug)
						ex.printStackTrace();
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
			startTimeListener();
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
        boolean res = false;
        if(Config.mouseMode == Config.MouseMode.External){
            return res;
        }
        //TODO:
		res = ((LimboSDLSurface) this.mSurface).onTouchProcess(this.mSurface, event);
		res = ((LimboSDLSurface) this.mSurface).onTouchEventProcess(event);
		return true;
	}

	private void resumeVM() {
		if(LimboActivity.vmexecutor == null){
			return;
		}
		Thread t = new Thread(new Runnable() {
			public void run() {
				if (LimboActivity.vmexecutor.paused == 1) {

					try {
						Thread.sleep(4000);
					} catch (InterruptedException ex) {
						Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
					}
					LimboActivity.vmexecutor.paused = 0;

					String command = QmpClient.cont();
					String msg = QmpClient.sendCommand(command);
//					if (msg != null)
//						Log.i(TAG, msg);

					new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
						@Override
						public void run() {
							setUIModeMobile();
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
			ActionBar bar = this.getActionBar();
			if (bar != null) {
				if (bar.isShowing())
					bar.hide();
				else
					bar.show();
			}
		} else {
			stopVM(false);
		}

	}

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        this.invalidateOptionsMenu();
    }

    public void onSelectMenuSDLDisplay() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Display");

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

        LinearLayout t = createSDLDisplayPanel();
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


    public LinearLayout createSDLDisplayPanel() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        int currRate = getCurrentSDLRefreshRate();

        LinearLayout buttonsLayout = new LinearLayout(this);
        buttonsLayout.setOrientation(LinearLayout.HORIZONTAL);
        buttonsLayout.setGravity(Gravity.CENTER_HORIZONTAL);
        Button displayMode = new Button (this);

        displayMode.setText("Display Mode");
        displayMode.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                onDisplayMode();
            }
        });
        buttonsLayout.addView(displayMode);
        layout.addView(buttonsLayout);

        final TextView value = new TextView(this);
        value.setText("Idle Refresh Rate: " + currRate+" Hz");
        layout.addView(value);
        value.setLayoutParams(params);

        SeekBar rate = new SeekBar(this);
        rate.setMax(Config.MAX_DISPLAY_REFRESH_RATE);

        rate.setProgress(currRate);
        rate.setLayoutParams(params);

        ((SeekBar) rate).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar s, int progress, boolean touch) {
                value.setText("Idle Refresh Rate: " + (progress+1)+" Hz");
            }

            public void onStartTrackingTouch(SeekBar arg0) {

            }

            public void onStopTrackingTouch(SeekBar arg0) {
                int progress = arg0.getProgress()+1;
                int refreshMs = 1000 / progress;
                Log.v(TAG, "Changing idle refresh rate: (ms)" + refreshMs);
                LimboActivity.vmexecutor.setsdlrefreshrate(refreshMs);
            }
        });


        layout.addView(rate);

        return layout;

    }

    public int getCurrentSDLRefreshRate() {
        return 1000 / LimboActivity.vmexecutor.getsdlrefreshrate();
    }



    private void onDisplayMode() {

        String [] items = {
                "Normal (One-To-One)",
                "Fit To Screen"
//                ,"Stretch To Screen" //Stretched
        };
        int currentScaleType = 0;
        if(fitToScreen){
        	currentScaleType = 1;
		} else if(stretchToScreen)
			currentScaleType = 2;

        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle("Display Mode");
        mBuilder.setSingleChoiceItems(items, currentScaleType, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch(i){
                    case 0:
                        onNormalScreen();
                        break;
                    case 1:
                        if(Config.mouseMode == Config.MouseMode.External){
                            UIUtils.toastShort(LimboSDLActivity.this, "Fit to Screen Disabled under Desktop Mode");
                            dialog.dismiss();
                            return;
                        }
                        onFitToScreen();
                        break;
                    case 2:
						if(Config.mouseMode == Config.MouseMode.External){
							UIUtils.toastShort(LimboSDLActivity.this, "Stretch Screen Disabled under Desktop Mode");
							dialog.dismiss();
							return;
						}
						onStretchToScreen();
                        break;
                    default:
                        break;
                }
                dialog.dismiss();
            }
        });
        final AlertDialog alertDialog = mBuilder.create();
        alertDialog.show();

    }


    @Override
	protected synchronized void runSDLMain(){

		//We go through the vm executor
		LimboActivity.startvm(LimboActivity.activity, Config.UI_SDL);

		//XXX: we hold the thread because SDLActivity will exit
		try {
			wait();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	public static int vm_width;
	public static int vm_height;

	public static void onVMResolutionChanged(int w, int h)
	{
		boolean refreshDisplay = false;

		if(w!=vm_width || h!=vm_height)
			refreshDisplay = true;
		vm_width = w;
		vm_height = h;

		Log.v(TAG, "VM resolution changed to " + vm_width + "x" + vm_height);


		if(refreshDisplay) {
			activity.resize();
		}

	}

	public static boolean isResizing = false;

	public class LimboSDLSurface extends ExSDLSurface implements View.OnKeyListener, View.OnTouchListener {

		public boolean initialized = false;

		public LimboSDLSurface(Context context) {
			super(context);
			setOnKeyListener(this);
			setOnTouchListener(this);
			gestureDetector = new GestureDetector(activity, new GestureListener());
			setOnGenericMotionListener(new SDLGenericMotionListener_API12());
		}

		public void surfaceChanged(SurfaceHolder holder,
								   int format, int width, int height) {
			super.surfaceChanged(holder, format, width, height);
		}

		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			super.surfaceCreated(holder);
		}

		@Override
		public void onConfigurationChanged(Configuration newConfig) {
			super.onConfigurationChanged(newConfig);
			resize();
		}



		public synchronized void doResize(boolean reverse) {
			//XXX: notify the UI not to process mouse motion
			isResizing = true;
			Log.v(TAG, "doResize");

			Display display = SDLActivity.mSingleton.getWindowManager().getDefaultDisplay();
			int height = 0;
			int width = 0;

			Point size = new Point();
			display.getSize(size);
			int screen_width = size.x;
			int screen_height = size.y;

			final ActionBar bar = ((SDLActivity) activity).getActionBar();

			if(LimboSDLActivity.mLayout != null) {
				width = LimboSDLActivity.mLayout.getWidth();
				height = LimboSDLActivity.mLayout.getHeight();
			}

			//native resolution for use with external mouse
			if(!LimboSDLActivity.stretchToScreen && !LimboSDLActivity.fitToScreen) {
				width = LimboSDLActivity.vm_width;
				height = LimboSDLActivity.vm_height;
			}

			if(reverse){
				int temp = width;
				width = height;
				height = temp;
			}

			boolean portrait = SDLActivity.mSingleton.getResources()
					.getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT;

			if (portrait) {
				if(Config.mouseMode != Config.MouseMode.External) {
					getHolder().setFixedSize(width,(int) (width / (LimboSDLActivity.vm_width / (float) LimboSDLActivity.vm_height)));
				}
			} else {
				if ( (stretchToScreen || fitToScreen)
						&& !LimboSettingsManager.getAlwaysShowMenuToolbar(LimboSDLActivity.this)
						&& bar != null && bar.isShowing()) {
					height += bar.getHeight();
				}
				getHolder().setFixedSize(width, height);
			}
			initialized = true;
			isResizing = false;


		}

		// XXX: SDL is missing some key codes in sdl2-keymap.h
		// So we create them with a Shift Modifier
		private boolean handleMissingKeys(int keyCode, int action) {

			int keyCodeTmp = keyCode;
			switch (keyCode) {
				case 77:
					keyCodeTmp = 9;
					break;
				case 81:
					keyCodeTmp = 70;
					break;
				case 17:
					keyCodeTmp = 15;
					break;
				case 18:
					keyCodeTmp = 10;
					break;
				default:
					return false;

			}
			if (action == KeyEvent.ACTION_DOWN) {
				SDLActivity.onNativeKeyDown(59);
				SDLActivity.onNativeKeyDown(keyCodeTmp);
			} else {
				SDLActivity.onNativeKeyUp(59);
				SDLActivity.onNativeKeyUp(keyCodeTmp);
			}
			return true;

		}

		@Override
		public boolean onKey(View v, int keyCode, KeyEvent event) {

			if ((keyCode == KeyEvent.KEYCODE_BACK) || (keyCode == KeyEvent.KEYCODE_FORWARD)) {
				// dismiss android back and forward keys
				return true;
			} else if (event.getKeyCode() == KeyEvent.KEYCODE_MENU) {
				return false;
			} else if (event.getAction() == KeyEvent.ACTION_DOWN) {
				Log.v("SDL", "key down: " + keyCode);
				if (!handleMissingKeys(keyCode, event.getAction()))
					SDLActivity.onNativeKeyDown(keyCode);
				return true;
			} else if (event.getAction() == KeyEvent.ACTION_UP) {
				Log.v("SDL", "key up: " + keyCode);
				if (!handleMissingKeys(keyCode, event.getAction()))
					SDLActivity.onNativeKeyUp(keyCode);
				return true;
			} else {
				return super.onKey(v, keyCode, event);
			}

		}

		// Touch events
		public boolean onTouchProcess(View v, MotionEvent event) {

			if(Config.mouseMode == Config.MouseMode.External
				&& MotionEvent.TOOL_TYPE_FINGER == event.getToolType(0))
				return true;

			if(Config.mouseMode == Config.MouseMode.Trackpad
					&& MotionEvent.TOOL_TYPE_MOUSE == event.getToolType(0))
				return true;

			// Log.v("onTouch",
			// "Action=" + event.getAction() + ", X,Y=" + event.getX() + ","
			// + event.getY() + " P=" + event.getPressure());
			int action = event.getAction();
			float x = event.getX(0);
			float y = event.getY(0);
			float p = event.getPressure(0);

			int relative = Config.mouseMode == Config.MouseMode.External? 0: 1;

			int sdlMouseButton = 0;
			if(event.getButtonState() == MotionEvent.BUTTON_PRIMARY)
				sdlMouseButton = Config.SDL_MOUSE_LEFT;
			else if(event.getButtonState() == MotionEvent.BUTTON_SECONDARY)
				sdlMouseButton = Config.SDL_MOUSE_RIGHT;
			else if(event.getButtonState() == MotionEvent.BUTTON_TERTIARY)
				sdlMouseButton = Config.SDL_MOUSE_MIDDLE;


			if (event.getAction() == MotionEvent.ACTION_MOVE) {

				if (mouseUp) {
					old_x = x;
					old_y = y;
					mouseUp = false;
				}
				if (action == MotionEvent.ACTION_MOVE) {
					if(Config.mouseMode == Config.MouseMode.External) {
						//Log.d("SDL", "onTouch Absolute Move by=" + action + ", X,Y=" + (x) + "," + (y) + " P=" + p);
						LimboActivity.vmexecutor.onLimboMouse(0, MotionEvent.ACTION_MOVE,0, x , y );
					}else {
						//Log.d("SDL", "onTouch Relative Moving by=" + action + ", X,Y=" + (x -
//                            old_x) + "," + (y - old_y) + " P=" + p);
						LimboActivity.vmexecutor.onLimboMouse(0, MotionEvent.ACTION_MOVE,1, (x - old_x)  * sensitivity_mult, (y - old_y) * sensitivity_mult);
					}

				}
				// save current
				old_x = x;
				old_y = y;

			}
			else if (event.getAction() == event.ACTION_UP ) {
				//Log.d("SDL", "onTouch Up: " + sdlMouseButton);
				//XXX: it seems that the Button state is not available when Button up so
				//  we should release all mouse buttons to be safe since we don't know which one fired the event
				if(sdlMouseButton == Config.SDL_MOUSE_MIDDLE
						||sdlMouseButton == Config.SDL_MOUSE_RIGHT
						) {
					LimboActivity.vmexecutor.onLimboMouse(sdlMouseButton, MotionEvent.ACTION_UP, relative, x, y);
				} else if (sdlMouseButton != 0) {
					LimboActivity.vmexecutor.onLimboMouse(sdlMouseButton, MotionEvent.ACTION_UP, relative, x, y);
				} else { // if we don't have inforamtion about which button we can make some guesses

					//Or only the last one pressed
					if (lastMouseButtonDown > 0) {
						if(lastMouseButtonDown == Config.SDL_MOUSE_MIDDLE
								||lastMouseButtonDown == Config.SDL_MOUSE_RIGHT
								) {
							LimboActivity.vmexecutor.onLimboMouse(lastMouseButtonDown, MotionEvent.ACTION_UP, relative,x, y);
						}else
							LimboActivity.vmexecutor.onLimboMouse(lastMouseButtonDown, MotionEvent.ACTION_UP, relative, x, y);
					} else {
						//ALl buttons
						if (Config.mouseMode == Config.MouseMode.Trackpad) {
							LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 1, 0, 0);
						} else if (Config.mouseMode == Config.MouseMode.External) {
							LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, x, y);
							LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, 0, x, y);
							LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, 0, x, y);
						}
					}
				}
				lastMouseButtonDown = -1;
				mouseUp = true;
			}
			else if (event.getAction() == event.ACTION_DOWN
					&& Config.mouseMode == Config.MouseMode.External
					) {

				//XXX: Some touch events for touchscreen mode are primary so we force left mouse button
				if(sdlMouseButton == 0 && MotionEvent.TOOL_TYPE_FINGER == event.getToolType(0)) {
					sdlMouseButton = Config.SDL_MOUSE_LEFT;
				}

				LimboActivity.vmexecutor.onLimboMouse(sdlMouseButton, MotionEvent.ACTION_DOWN, relative, x, y);
				lastMouseButtonDown = sdlMouseButton;
			}
			return true;
		}

		public boolean onTouch(View v, MotionEvent event) {
			boolean res = false;
			if(Config.mouseMode == Config.MouseMode.External){
				res = onTouchProcess(v,event);
				res = onTouchEventProcess(event);
			}
			return res;
		}

		public boolean onTouchEvent(MotionEvent event) {
			return false;
		}

		public boolean onTouchEventProcess(MotionEvent event) {
			// Log.v("onTouchEvent",
			// "Action=" + event.getAction() + ", X,Y=" + event.getX() + ","
			// + event.getY() + " P=" + event.getPressure());
			// MK
			if (event.getAction() == MotionEvent.ACTION_CANCEL)
				return true;

			if (!firstTouch) {
				firstTouch = true;
			}
			if (event.getPointerCount() > 1) {

				// XXX: Limbo Legacy enable Right Click with 2 finger touch
				// Log.v("Right Click",
				// "Action=" + event.getAction() + ", X,Y=" + event.getX()
				// + "," + event.getY() + " P=" + event.getPressure());
				// rightClick(event);
				return true;
			} else
				return gestureDetector.onTouchEvent(event);
		}
	}

	public AudioManager am;
	protected int maxVolume;

	protected void setupVolume() {
		if (am == null) {
			am = (AudioManager) mSingleton.getSystemService(Context.AUDIO_SERVICE);
			maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}
	}

	public void setVolume(int volume) {
		if(am!=null)
			am.setStreamVolume(AudioManager.STREAM_MUSIC, volume, 0);
	}

	protected int getCurrentVolume() {
		int volumeTmp = 0;
		if(am!=null)
			volumeTmp = am.getStreamVolume(AudioManager.STREAM_MUSIC);
		return volumeTmp;
	}


	//XXX: We want to suspend only when app is calling onPause()
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {

	}

	public boolean rightClick(final MotionEvent e, final int i) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				Log.d("SDL", "Mouse Right Click");
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_DOWN, 1, -1, -1);
				try {
					Thread.sleep(100);
				} catch (InterruptedException ex) {
//					Log.v("SDLSurface", "Interrupted: " + ex);
				}
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, 1, -1, -1);
			}
		});
		t.start();
		return true;

	}

	public boolean middleClick(final MotionEvent e, final int i) {
		Thread t = new Thread(new Runnable() {
			public void run() {
                Log.d("SDL", "Mouse Middle Click");
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_DOWN, 1,-1, -1);
				try {
					Thread.sleep(100);
				} catch (InterruptedException ex) {
//                    Log.v("SDLSurface", "Interrupted: " + ex);
				}
				LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, 1,-1, -1);
			}
		});
		t.start();
		return true;

	}

	private void doubleClick(final MotionEvent event, final int pointer_id) {

		Thread t = new Thread(new Runnable() {
			public void run() {
				//Log.d("SDL", "Mouse Double Click");
				for (int i = 0; i < 2; i++) {
					LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 1, 0, 0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
						// Log.v("doubletap", "Could not sleep");
					}
					LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 1,0, 0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
						// Log.v("doubletap", "Could not sleep");
					}
				}
			}
		});
		t.start();
	}


	int lastMouseButtonDown = -1;
	public float old_x = 0;
	public float old_y = 0;
	private boolean mouseUp = true;
	private float sensitivity_mult = (float) 1.0;
	private boolean firstTouch = false;



	private class GestureListener extends GestureDetector.SimpleOnGestureListener {

		@Override
		public boolean onDown(MotionEvent event) {
			// Log.v("onDown", "Action=" + event.getAction() + ", X,Y=" + event.getX()
			// + "," + event.getY() + " P=" + event.getPressure());
			return true;
		}

		@Override
		public void onLongPress(MotionEvent event) {
			// Log.d("SDL", "Long Press Action=" + event.getAction() + ", X,Y="
			// + event.getX() + "," + event.getY() + " P="
			// + event.getPressure());
			if(Config.mouseMode == Config.MouseMode.External)
				return;

			LimboActivity.vmexecutor.onLimboMouse(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 1, 0, 0);
			Vibrator v = (Vibrator) activity.getSystemService(Context.VIBRATOR_SERVICE);
			if (v.hasVibrator()) {
				v.vibrate(100);
			}
		}

		public boolean onSingleTapConfirmed(MotionEvent event) {
			float x1 = event.getX();
			float y1 = event.getY();

			if(Config.mouseMode == Config.MouseMode.External)
				return true;

//			 Log.d("onSingleTapConfirmed", "Tapped at: (" + x1 + "," + y1 +
//			 ")");

			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				//Log.v("onSingleTapConfirmed", "Action=" + action + ", X,Y=" + x + "," + y + " P=" + p);
				if (event.getAction() == event.ACTION_DOWN
						&& MotionEvent.TOOL_TYPE_FINGER == event.getToolType(0)) {
					//Log.d("SDL", "onTouch Down: " + event.getButtonState());
					LimboSDLActivity.singleClick(event, i);
				}
			}
			return true;

		}

		// event when double tap occurs
		@Override
		public boolean onDoubleTap(MotionEvent event) {
//			Log.d("onDoubleTap", "Tapped at: (" + event.getX() + "," + event.getY() + ")");

			if(Config.mouseMode == Config.MouseMode.External
				//&& MotionEvent.TOOL_TYPE_MOUSE == event.getToolType(0)
					)
				return true;

			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				// Log.v("onDoubleTap", "Action=" + action + ", X,Y=" + x + "," + y + " P=" + p);
				doubleClick(event, i);
			}

			return true;
		}
	}

	class SDLGenericMotionListener_API12 implements View.OnGenericMotionListener {
		private LimboSDLSurface mSurface;

		@Override
		public boolean onGenericMotion(View v, MotionEvent event) {
			float x, y;
			int action;

			switch (event.getSource()) {
				case InputDevice.SOURCE_JOYSTICK:
				case InputDevice.SOURCE_GAMEPAD:
				case InputDevice.SOURCE_DPAD:
					SDLControllerManager.handleJoystickMotionEvent(event);
					return true;

				case InputDevice.SOURCE_MOUSE:
					if(Config.mouseMode == Config.MouseMode.Trackpad)
						break;

					action = event.getActionMasked();
//                    Log.d("SDL", "onGenericMotion, action = " + action + "," + event.getX() + ", " + event.getY());
					switch (action) {
						case MotionEvent.ACTION_SCROLL:
							x = event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0);
							y = event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0);
//                            Log.d("SDL", "Mouse Scroll: " + x + "," + y);
							LimboActivity.vmexecutor.onLimboMouse(0, action, 0, x, y);
							return true;

						case MotionEvent.ACTION_HOVER_MOVE:
							if(Config.processMouseHistoricalEvents) {
								final int historySize = event.getHistorySize();
								for (int h = 0; h < historySize; h++) {
									float ex = event.getHistoricalX(h);
									float ey = event.getHistoricalY(h);
									float ep = event.getHistoricalPressure(h);
									processHoverMouse(ex, ey, ep, action);
								}
							}

							float ex = event.getX();
							float ey = event.getY();
							float ep = event.getPressure();
							processHoverMouse(ex, ey, ep, action);
							return true;

						case MotionEvent.ACTION_UP:

						default:
							break;
					}
					break;

				default:
					break;
			}

			// Event was not managed
			return false;
		}

		private void processHoverMouse(float x,float y,float p, int action) {



			if(Config.mouseMode == Config.MouseMode.External) {
				//Log.d("SDL", "Mouse Hover: " + x + "," + y);
				LimboActivity.vmexecutor.onLimboMouse(0, action, 0, x, y);
			}
		}

	}

	GestureDetector gestureDetector;

}
