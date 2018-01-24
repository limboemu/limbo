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

import android.androidVNC.AbstractScaling;
import android.app.Activity;
import android.app.AlertDialog;
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
import android.os.SystemClock;
import android.support.v4.provider.DocumentFile;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.ActionBar;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

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

		UIUtils.setOrientation(this);

		if (LimboSettingsManager.getFullscreen(this))
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		super.onCreate(b);

		this.vncCanvas.setFocusableInTouchMode(true);

		UIUtils.setupToolBar(this);

		UIUtils.showHints(this);

		setDefaulViewMode();

		onMouse();



	}

	private void setDefaulViewMode() {
		

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

        this.supportInvalidateOptionsMenu();
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
		LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Running in Background");
		super.onPause();
	}

	public void onResume() {
		LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Running");
		super.onResume();
	}

	public void checkStatus() {
		while (timeQuit != true) {
			String status = checkCompletion();
			Log.v("Inside", "Status: " + status);
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

	public void startSaveVMListener() {
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
//		new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
//			@Override
//			public void run() {
//				Toast.makeText(getApplicationContext(), "VM State saved", Toast.LENGTH_LONG).show();
//			}
//		}, 1000);

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

    public void pausedErrorVM() {


        new AlertDialog.Builder(this).setTitle("Error").setMessage("Could not pause VM. View log for details")
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        Thread t = new Thread(new Runnable() {
                            public void run() {
                                try {
                                    Thread.sleep(1000);
                                } catch (InterruptedException ex) {
                                    Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
                                }
                                onHMP();
                                String commandStop = "cont\n";
                                for (int i = 0; i < commandStop.length(); i++)
                                    vncCanvas.sendText(commandStop.charAt(i) + "");
                                try {
                                    Thread.sleep(1000);
                                } catch (InterruptedException ex) {
                                    Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
                                }
                                onVNC();
                            }
                        });
                        t.start();


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
			if (drives !=null && fileType != null && file != null) {
				drives.setDriveAttr(fileType, file, true);
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

				if (drives!=null && drives.filetype != null && file != null) {
					drives.setDriveAttr(drives.filetype, file, true);
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
			if (LimboActivity.currMachine.hasRemovableDevices()) {
				drives = new DrivesDialogBox(activity, R.style.Transparent, this, LimboActivity.currMachine);
				drives.show();
			} else {
				UIUtils.toastShort(activity, "No removable devices attached");
			}

		} else if (item.getItemId() == R.id.itemMonitor) {
			if (this.monitorMode) {
				this.onVNC();
			} else {
				this.onHMP();
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
		} else if (item.getItemId() == R.id.itemHideToolbar) {
            this.onHideToolbar();
        } else if (item.getItemId() == R.id.itemViewLog) {
            this.onViewLog();
        }
        // this.vncCanvas.requestFocus();

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

	public void setContentView() {
		
		setContentView(R.layout.canvas);

	}

	private boolean toggleFullScreen() {
		
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
		inputHandler = getInputHandlerById(R.id.itemInputTouchpad);
		connection.setInputMode(inputHandler.getName());
		connection.setFollowMouse(true);
		mouseOn = true;
		showPanningState();
		return true;

	}

	private boolean onMouse() {

		// Limbo: For now we disable other modes
		mouseOn = false;

		
		if (mouseOn == false) {
			inputHandler = getInputHandlerById(R.id.itemInputTouchpad);
			connection.setInputMode(inputHandler.getName());
			connection.setFollowMouse(true);
			mouseOn = true;
		} else {
			// XXX: Limbo
			// we disable panning for now
			// input1 = getInputHandlerById(R.id.itemFitToScreen);
			// input1 = getInputHandlerById(R.id.itemInputTouchPanZoomMouse);
			// connection.setFollowMouse(false);
			// mouseOn = false;
		}

        Toast.makeText(this.getApplicationContext(), "Mouse Trackpad Mode enabled", Toast.LENGTH_SHORT).show();
		//Start calibration
        calibration();

		return true;
	}

	//XXX: We need to adjust the mouse inside the Guest
    // This is a known issue with QEMU under VNC mode
    // this only fixes things temporarily but it's better
    //  than nothing
    public void calibration() {
        Thread t = new Thread(new Runnable() {
            public void run() {
            try {

                int origX = vncCanvas.mouseX;
                int origY = vncCanvas.mouseY;
                MotionEvent event = null;

                for(int i = 0; i< 4*20; i++) {
                    int x = 0+i*50;
                    int y = 0+i*50;
                    if(i%4==1){
                        x=vncCanvas.rfb.framebufferWidth;
                    }else if (i%4==2) {
                        y=vncCanvas.rfb.framebufferHeight;
                    }else if (i%4==3) {
                        x=0;
                    }

                    event = MotionEvent.obtain(SystemClock.uptimeMillis(),
                            SystemClock.uptimeMillis(), MotionEvent.ACTION_MOVE,
                            x,y, 0);
                    Thread.sleep(50);
                    vncCanvas.processPointerEvent(event, false, false);


                }

                Thread.sleep(50);
                event = MotionEvent.obtain(SystemClock.uptimeMillis(),
                        SystemClock.uptimeMillis(), MotionEvent.ACTION_MOVE,
                        origX,origY, 0);
                vncCanvas.processPointerEvent(event, false, false);

            }catch(Exception ex) {

            }
            }
        });
        t.start();
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
		getMenuInflater().inflate(R.menu.vnccanvasactivitymenu, menu);

		if (vncCanvas.scaling != null) {
			menu.findItem(vncCanvas.scaling.getId()).setChecked(true);
		}

		if (this.monitorMode) {
			menu.findItem(R.id.itemMonitor).setTitle("VM Display");
            menu.findItem(R.id.itemMonitor).setIcon(R.drawable.ui);

		} else {
			menu.findItem(R.id.itemMonitor).setTitle("QEMU Monitor");
            menu.findItem(R.id.itemMonitor).setIcon(R.drawable.terminal);

		}

		//XXX: We don't need these for now
		menu.removeItem(menu.findItem(R.id.itemEnterText).getItemId());
		menu.removeItem(menu.findItem(R.id.itemSendKeyAgain).getItemId());
		menu.removeItem(menu.findItem(R.id.itemSpecialKeys).getItemId());
		menu.removeItem(menu.findItem(R.id.itemInputMode).getItemId());
        menu.removeItem(menu.findItem(R.id.itemScaling).getItemId());
        menu.removeItem(menu.findItem(R.id.itemCtrlAltDel).getItemId());
        menu.removeItem(menu.findItem(R.id.itemCtrlC).getItemId());

        if (LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
            menu.removeItem(menu.findItem(R.id.itemHideToolbar).getItemId());
        }

		// Menu inputMenu = menu.findItem(R.id.itemInputMode).getSubMenu();
		//
		// inputModeMenuItems = new MenuItem[inputModeIds.length];
		// for (int i = 0; i < inputModeIds.length; i++) {
		// inputModeMenuItems[i] = inputMenu.findItem(inputModeIds[i]);
		// }
		// updateInputMenu();
		// menu.removeItem(menu.findItem(R.id.itemCenterMouse).getItemId());

		// Limbo: Disable Panning for now
		// if (this.mouseOn) {
		// menu.findItem(R.id.itemCenterMouse).setTitle("Pan (Mouse Off)");
		// menu.findItem(R.id.itemCenterMouse).setIcon(R.drawable.pan);
		// } else {
		menu.findItem(R.id.itemCenterMouse).setTitle("Mouse");
		menu.findItem(R.id.itemCenterMouse).setIcon(R.drawable.mouse);
		//
		// }

        int maxMenuItemsShown = 4;
        int actionShow = MenuItemCompat.SHOW_AS_ACTION_IF_ROOM;
        if(UIUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 8;
            actionShow = MenuItemCompat.SHOW_AS_ACTION_ALWAYS;
        }
		for (int i = 0; i < menu.size() && i < maxMenuItemsShown; i++) {
			MenuItemCompat.setShowAsAction(menu.getItem(i), actionShow);
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
		// Prevent crashes from activating mouse when machine is paused
		if (LimboActivity.vmexecutor.paused == 1)
			return;

		InputMethodManager inputMgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		// inputMgr.toggleSoftInput(0, 0);
		inputMgr.showSoftInput(this.vncCanvas, InputMethodManager.SHOW_FORCED);
	}

	private void onHMP() {
		monitorMode = true;
		vncCanvas.sendMetaKey1(50, 6);

	}

	private void onVNC() {
		monitorMode = false;
		vncCanvas.sendMetaKey1(49, 6);
	}

	// FIXME: We need this to able to catch complex characters strings like
	// grave and send it as text
	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		if (event.getAction() == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
			vncCanvas.sendText(event.getCharacters().toString());
			return true;
		} else
			return super.dispatchKeyEvent(event);

	}

	private void onSaveSnapshot(final String stateName) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				((LimboActivity) LimboActivity.activity).saveSnapshotDB(stateName);
				onHMP();
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
						UIUtils.toastShort(LimboVNCActivity.this, "Please wait while saving HD Snapshot");
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

	private void resumeVMMonitor() {
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
					if(vncCanvas == null)
					    return;
					onHMP();
					// new Handler(Looper.getMainLooper()).postDelayed(new
					// Runnable() {
					// @Override
					// public void run() {
					// Toast.makeText(getApplicationContext(), "Please wait
					// while resuming VM State",
					// Toast.LENGTH_LONG).show();
					// }
					// }, 500);
					try {
						Thread.sleep(2000);
					} catch (InterruptedException ex) {
						Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
					}

					String commandStop = "cont\n";
					for (int i = 0; i < commandStop.length(); i++)
						vncCanvas.sendText(commandStop.charAt(i) + "");
					try {
						Thread.sleep(1000);
					} catch (InterruptedException ex) {
						Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
					}
					onVNC();
					LimboActivity.vmexecutor.paused = 0;
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

	private void onPauseVMMonitor() {
		Thread t = new Thread(new Runnable() {
			public void run() {
				// Delete any previous state file
				if (LimboActivity.vmexecutor.save_state_name != null) {
					File file = new File(LimboActivity.vmexecutor.save_state_name);
					if (file.exists()) {
						file.delete();
					}
				}

				onHMP();
				new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
					@Override
					public void run() {
                        UIUtils.toastShort(LimboVNCActivity.this, "Please wait while saving VM State");
					}
				}, 500);
				try {
					Thread.sleep(500);
				} catch (InterruptedException ex) {
					Logger.getLogger(LimboVNCActivity.class.getName()).log(Level.SEVERE, null, ex);
				}

				String commandStop = "stop\n";
				for (int i = 0; i < commandStop.length(); i++)
					vncCanvas.sendText(commandStop.charAt(i) + "");

				String commandMigrate = "migrate fd:"
						+ LimboActivity.vmexecutor.get_fd(LimboActivity.vmexecutor.save_state_name) + "\n";
				for (int i = 0; i < commandMigrate.length(); i++)
					vncCanvas.sendText(commandMigrate.charAt(i) + "");

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

	private void onPauseVMQMP() {
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
				String command = QmpClient.stop();
				String msg = QmpClient.sendCommand(command);
				if (msg != null)
					Log.i(TAG, msg);

				command = QmpClient.migrate(false, false, uri);
				msg = QmpClient.sendCommand(command);
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
		stateView.setPadding(20, 20, 20, 20);
		alertDialog.setView(stateView);

		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Pause", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {


				if (LimboActivity.vmexecutor.enableqmp == 1)
					onPauseVMQMP();
				else
					onPauseVMMonitor();

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
            UIUtils.toastLong(LimboVNCActivity.this, "No HDD image found, please create a qcow2 image from Limbo console");
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

				if (Config.enableSaveVMmonitor) {
					// XXX: Safer for now via the Monitor console
					onSaveSnapshot(stateView.getText().toString());
				} else {
					// FIXME: This saves the vm natively but cannot resume the
					// vm
					LimboActivity.vmexecutor.save(LimboActivity.vmexecutor.snapshot_name);

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
			// Get the state of saving full disk snapshot
			save_state = LimboActivity.vmexecutor.get_save_state();

			// Get the state of saving the VM memory only
			pause_state = LimboActivity.vmexecutor.get_pause_state();
			//Log.d(TAG, "save_state = " + save_state);
			//Log.d(TAG, "pause_state = " + pause_state);
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
                    pausedErrorVM();
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

		// super.onBackPressed();
		if (!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
			ActionBar bar = this.getSupportActionBar();
			if (bar != null) {
				if (bar.isShowing()) {
					bar.hide();
				} else
					bar.show();
			}
		} else
			super.onBackPressed();

	}

	public void onHideToolbar(){
			ActionBar bar = this.getSupportActionBar();
			if (bar != null) {
					bar.hide();
			}
    }

	@Override
	public void onConnected() {
        this.resumeVMMonitor();
    }

}
