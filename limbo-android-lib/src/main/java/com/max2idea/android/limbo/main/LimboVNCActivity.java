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
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.support.v4.provider.DocumentFile;
import android.support.v4.view.MenuItemCompat;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.json.JSONException;
import org.json.JSONObject;

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

		if (LimboSettingsManager.getFullscreen(this))
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		super.onCreate(b);

		this.vncCanvas.setFocusableInTouchMode(true);

		UIUtils.setupToolBar(this);

		setDefaulViewMode();

        setUIModeMobile();



	}

	private void setDefaulViewMode() {
		

		// Fit to Screen
		AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(this);
		showPanningState();

		// Full Screen
		this.toggleFullScreen();

		setLayout(getResources().getConfiguration());

        UIUtils.setOrientation(this);
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		setLayout(newConfig);
	}

	private void setLayout(Configuration newConfig) {
		
		View vnc_canvas = (View) this.findViewById(R.id.vnc_canvas_layout);
		View zoom = (View) this.findViewById(R.id.zoom_layout);

		LinearLayout.LayoutParams vnc_layout_params = (LinearLayout.LayoutParams) vnc_canvas.getLayoutParams();
        LinearLayout.LayoutParams vnc_params = (LinearLayout.LayoutParams) vnc_canvas.getLayoutParams();
		LinearLayout.LayoutParams zoom_params = (LinearLayout.LayoutParams) zoom.getLayoutParams();

		if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {

            vnc_layout_params.weight = 0.2f;
			zoom_params.weight = 0.8f;
            vnc_layout_params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
            vnc_layout_params.width = LinearLayout.LayoutParams.MATCH_PARENT;


        } else if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {

            vnc_layout_params.weight = 1f;
			zoom_params.weight = 0f;
            vnc_layout_params.height = LinearLayout.LayoutParams.MATCH_PARENT;
            vnc_layout_params.width = LinearLayout.LayoutParams.MATCH_PARENT;

		}
		vnc_canvas.setLayoutParams(vnc_layout_params);
		zoom.setLayoutParams(zoom_params);

        this.invalidateOptionsMenu();
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
			Log.v("TAG", "Status: " + status);
			if (status == null
                    //|| status.equals("")
                    || status.toUpperCase().equals("COMPLETED")
                    || status.toUpperCase().equals("FAILED")
                    ) {
				//Log.v(TAG, "Saving state is done: " + status);
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

                        Log.i(TAG, "VM Paused, Shutting Down");
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
								//XXX: we now use QMP
								Thread t = new Thread(new Runnable() {
									public void run() {
										String command = QmpClient.cont();
										String msg = QmpClient.sendCommand(command);
									}
								});
								t.start();
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

				if (drives!=null && drives.filetype != null && file != null) {
					drives.setDriveAttr(drives.filetype, file);
				}
			}

		}

		// Check if says open

	}

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {
		super.onOptionsItemSelected(item);
		if (item.getItemId() == this.KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
			toggleKeyboardFlag = UIUtils.onKeyboard(this, toggleKeyboardFlag);
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
				this.onMonitor();
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
            onMouseMode();
		}
        else if (item.getItemId() == R.id.itemHelp) {
			UIUtils.onHelp(this);
		} else if (item.getItemId() == R.id.itemHideToolbar) {
            this.onHideToolbar();
        } else if (item.getItemId() == R.id.itemDisplay) {
            this.onSelectMenuVNCDisplay();
        } else if (item.getItemId() == R.id.itemViewLog) {
            this.onViewLog();
        }

        this.invalidateOptionsMenu();

		return true;
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
                        setUIModeDesktop(LimboVNCActivity.this, false);
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
		if(vncCanvas.rfb.framebufferWidth < vncCanvas.getWidth()
				&& vncCanvas.rfb.framebufferHeight < vncCanvas.getHeight())
			return true;

		return false;
	}
    private void onDisplayMode() {

        String [] items = {
                "Normal (One-To-One)",
                "Fit To Screen"
                //"Full Screen" //Stretched
        };
        int currentScaleType = vncCanvas.getScaleType() == ImageView.ScaleType.FIT_CENTER? 1 : 0;

        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle("Display Mode");
        mBuilder.setSingleChoiceItems(items, currentScaleType, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch(i){
                    case 0:
                        onNormalScreen();
                        onMouse();
                        break;
                    case 1:
                        if(Config.mouseMode == Config.MouseMode.External){
                            UIUtils.toastShort(LimboVNCActivity.this, "Fit to Screen disabled under Desktop mode");
                            dialog.dismiss();
                            return;
                        }
                        onFitToScreen();
                        onMouse();
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

    private void setUIModeMobile(){


        MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);

        Config.mouseMode = Config.MouseMode.Trackpad;
        onFitToScreen();
        onMouse();

        //UIUtils.toastShort(LimboVNCActivity.this, "Trackpad Calibrating");
        invalidateOptionsMenu();
    }

    private void setUIModeDesktop(final Activity activity, final boolean mouseMethodAlt) {


        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Desktop mode");

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20,20,20,20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        TextView textView = new TextView(activity);
        textView.setVisibility(View.VISIBLE);

        String desktopInstructions = this.getString(R.string.desktopInstructions);
        if(!checkVMResolutionFits()){
			String resolutionWarning = "Warning: Machine resolution "
					+ vncCanvas.rfb.framebufferWidth + "x" + vncCanvas.rfb.framebufferHeight +
					" is too high for Desktop Mode. " +
					"Scaling will be used and Mouse Alignment will not be accurate. " +
					"Reduce display resolution for better experience\n\n";
			desktopInstructions = resolutionWarning + desktopInstructions;
		}
        textView.setText(desktopInstructions);

        LinearLayout.LayoutParams textViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(textView, textViewParams);
        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

                MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);
                Config.mouseMode = Config.MouseMode.External;
                Toast.makeText(LimboVNCActivity.this, "External Mouse Enabled", Toast.LENGTH_SHORT).show();
                onNormalScreen();
                AbstractScaling.getById(R.id.itemOneToOne).setScaleTypeForActivity(LimboVNCActivity.this);
                showPanningState();

                onMouse();

                //vncCanvas.reSize(false);
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

		UIUtils.setOrientation(this);
        ActionBar bar = this.getActionBar();
        if (bar != null && !LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
            bar.hide();
        }

		inputHandler = getInputHandlerById(R.id.itemInputTouchpad);
		connection.setInputMode(inputHandler.getName());
		connection.setFollowMouse(true);
		mouseOn = true;
        AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(this);
		showPanningState();


        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        params.gravity = Gravity.CENTER;
        this.vncCanvas.setLayoutParams(params);
		return true;

	}

    private boolean onNormalScreen() {

		//Force only landscape
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        ActionBar bar = LimboVNCActivity.this.getActionBar();
        if (bar != null) {
            bar.show();
        }

        inputHandler = getInputHandlerById(R.id.itemInputTouchpad);
        connection.setInputMode(inputHandler.getName());
        connection.setFollowMouse(true);
        mouseOn = true;
        AbstractScaling.getById(R.id.itemOneToOne).setScaleTypeForActivity(this);
        showPanningState();

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );
        params.gravity = Gravity.TOP;
        this.vncCanvas.setLayoutParams(params);
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

        int maxMenuItemsShown = 4;
        int actionShow = MenuItemCompat.SHOW_AS_ACTION_IF_ROOM;
        if(UIUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 6;
            actionShow = MenuItemCompat.SHOW_AS_ACTION_ALWAYS;
        }

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
        menu.removeItem(menu.findItem(R.id.itemColorMode).getItemId());
        menu.removeItem(menu.findItem(R.id.itemFullScreen).getItemId());

        if (LimboSettingsManager.getAlwaysShowMenuToolbar(activity) || Config.mouseMode == Config.MouseMode.External) {
            menu.removeItem(menu.findItem(R.id.itemHideToolbar).getItemId());
            maxMenuItemsShown--;
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

	public static boolean toggleKeyboardFlag = true;

	private void onMonitor() {
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

	//FIXME: need to use QMP instead
	private void onSaveSnapshot(final String stateName) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				((LimboActivity) LimboActivity.activity).saveSnapshotDB(stateName);
				onMonitor();
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
					if(vncCanvas == null)
					    return;

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
//				if (msg != null)
//					Log.i(TAG, msg);
				command = QmpClient.migrate(false, false, uri);
				msg = QmpClient.sendCommand(command);
				if (msg != null) {
//					Log.i(TAG, msg);
					processMigrationResponse(msg);
				}

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
					onPauseVM();
				return;
			}
		});
		alertDialog.show();

	}

	//TODO: Snapshot is not supported right now
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
					QmpClient.sendCommand(QmpClient.save_snapshot(LimboActivity.vmexecutor.snapshot_name));
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
            // FIXME: We wait to complete the state
			new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
				@Override
				public void run() {
					pausedVM();
				}
			}, 4000);
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


	public void onBackPressed() {

		// super.onBackPressed();
		if (!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
			ActionBar bar = this.getActionBar();
			if (bar != null) {
				if (bar.isShowing() && Config.mouseMode == Config.MouseMode.Trackpad) {
					bar.hide();
				} else
					bar.show();
			}
		} else
			super.onBackPressed();

	}

	public void onHideToolbar(){
			ActionBar bar = this.getActionBar();
			if (bar != null) {
					bar.hide();
			}
    }

	@Override
	public void onConnected() {
        this.resumeVM();
        LimboActivity.currMachine.paused = 0;
        MachineOpenHelper.getInstance(activity).update(LimboActivity.currMachine,
                MachineOpenHelper.getInstance(activity).PAUSED, 0 + "");
		UIUtils.showHints(this);
    }

    public void onSelectMenuVNCDisplay() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle("Display");

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

        LinearLayout t = createVNCDisplayPanel();
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


    public LinearLayout createVNCDisplayPanel() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        int currRate = getCurrentVNCRefreshRate();

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


        Button colors = new Button(this);
        colors.setText("Color Mode");
        colors.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                selectColorModel();

            }
        });
        buttonsLayout.addView(colors);

        layout.addView(buttonsLayout);

        final TextView value = new TextView(this);
        value.setText("Display Refresh Rate: " + currRate+" Hz");
        layout.addView(value);
        value.setLayoutParams(params);

        SeekBar rate = new SeekBar(this);
        rate.setMax(Config.MAX_DISPLAY_REFRESH_RATE);

        rate.setProgress(currRate);
        rate.setLayoutParams(params);

        ((SeekBar) rate).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar s, int progress, boolean touch) {
                value.setText("Refresh Rate: " + (progress+1)+" Hz");
            }

            public void onStartTrackingTouch(SeekBar arg0) {

            }

            public void onStopTrackingTouch(SeekBar arg0) {
                int progress = arg0.getProgress()+1;
				int refreshMs = 1000 / progress;
				Log.v(TAG, "Changing display refresh rate (ms): " + refreshMs);
                LimboActivity.vmexecutor.setvncrefreshrate(refreshMs);

            }
        });


        layout.addView(rate);

        return layout;

    }
    public int getCurrentVNCRefreshRate() {
        return 1000 / LimboActivity.vmexecutor.getvncrefreshrate();
    }

}
