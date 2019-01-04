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
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
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
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.utils.DrivesDialogBox;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.Machine;
import com.max2idea.android.limbo.utils.MachineOpenHelper;
import com.max2idea.android.limbo.utils.QmpClient;
import com.max2idea.android.limbo.utils.UIUtils;

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.json.JSONObject;


/**
 * 
 * @author Dev
 */
public class LimboVNCActivity extends android.androidVNC.VncCanvasActivity {

	public static final int KEYBOARD = 10000;
	public static final int QUIT = 10001;
	public static final int HELP = 10002;
	private static boolean monitorMode = false;
	private boolean mouseOn = false;
	private Object lockTime = new Object();
	private boolean timeQuit = false;
	private Thread timeListenerThread;
	private ProgressDialog progDialog;
	private static boolean firstConnection;

	@Override
	public void onCreate(Bundle b) {

		if (LimboSettingsManager.getFullscreen(this))
			getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);

		super.onCreate(b);

		this.vncCanvas.setFocusableInTouchMode(true);

		UIUtils.setupToolBar(this);

		setDefaulViewMode();

//        setUIModeMobile();


	}

	private void setDefaulViewMode() {
		

		// Fit to Screen
		AbstractScaling.getById(R.id.itemFitToScreen).setScaleTypeForActivity(this);
		showPanningState();

//        screenMode = VNCScreenMode.FitToScreen;
		setLayout(getResources().getConfiguration());

        UIUtils.setOrientation(this);
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		setLayout(newConfig);
	}

	public enum VNCScreenMode {
	    Normal,
        FitToScreen,
        Fullscreen //fullscreen not implemented yet
    }

    public static VNCScreenMode screenMode = VNCScreenMode.FitToScreen;

	private void setLayout(Configuration newConfig) {

        boolean isLanscape =
                (newConfig!=null && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)
                || UIUtils.isLandscapeOrientation(this);

        View vnc_canvas_layout = (View) this.findViewById(R.id.vnc_canvas_layout);
        RelativeLayout.LayoutParams vnc_canvas_layout_params = null;
        RelativeLayout.LayoutParams vnc_params = null;
        //normal 1-1
        if(screenMode == VNCScreenMode.Normal) {
            if (isLanscape) {
                vnc_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_params.addRule(RelativeLayout.CENTER_IN_PARENT);
                vnc_canvas_layout_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_canvas_layout_params.addRule(RelativeLayout.CENTER_IN_PARENT);
            } else {
                vnc_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_params.addRule(RelativeLayout.CENTER_HORIZONTAL);

                vnc_canvas_layout_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_canvas_layout_params.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                vnc_canvas_layout_params.addRule(RelativeLayout.CENTER_HORIZONTAL);
            }
        } else {
            //fittoscreen
            if (isLanscape) {
                vnc_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                );
                vnc_params.addRule(RelativeLayout.CENTER_IN_PARENT);
                vnc_canvas_layout_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                );
                vnc_canvas_layout_params.addRule(RelativeLayout.CENTER_IN_PARENT);
            } else {
                vnc_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_params.addRule(RelativeLayout.CENTER_HORIZONTAL);

                vnc_canvas_layout_params = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                vnc_canvas_layout_params.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                vnc_canvas_layout_params.addRule(RelativeLayout.CENTER_HORIZONTAL);
            }
        }
        this.vncCanvas.setLayoutParams(vnc_params);
        vnc_canvas_layout.setLayoutParams(vnc_canvas_layout_params);

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
	    if(LimboActivity.currMachine!=null)
		    LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Running in Background");
		super.onPause();
	}

	public void onResume() {
	    if(LimboActivity.currMachine!=null)
		    LimboService.notifyNotification(LimboActivity.currMachine.machinename + ": VM Running");
		super.onResume();
	}

	public void checkStatus() {
		while (timeQuit != true) {
			LimboActivity.VMStatus status = Machine.checkSaveVMStatus(activity);
			Log.v(TAG, "Status: " + status);
			if (status == LimboActivity.VMStatus.Unknown
                    || status == LimboActivity.VMStatus.Completed
                    || status == LimboActivity.VMStatus.Failed
                    ) {
				//Log.v(TAG, "Saving state is done: " + status);
				stopTimeListener();
				return;
			}
			try {
				Thread.sleep(1000);
			} catch (InterruptedException ex) {
				Log.w("SaveVM", "Interrupted");
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
		Log.v("Listener", "Time listener thread exited...");

	}

	String TAG = "LimboVNCActivity";


	DrivesDialogBox drives = null;

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		// Log.v(TAG, "RET CODE: " + resultCode);
        if (requestCode == Config.OPEN_IMAGE_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
            String file = null;
            if(requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                DrivesDialogBox.filetype = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getFilePathFromIntent(activity, data);
            }
            if(drives !=null && file!=null)
                drives.setDriveAttr(DrivesDialogBox.filetype, file);
        }else if (requestCode == Config.OPEN_LOG_FILE_DIR_REQUEST_CODE|| requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
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

	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {
		super.onOptionsItemSelected(item);
		if (item.getItemId() == this.KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
			toggleKeyboardFlag = UIUtils.onKeyboard(this, toggleKeyboardFlag);
		} else if (item.getItemId() == R.id.itemReset) {
			Machine.resetVM(activity);
		} else if (item.getItemId() == R.id.itemShutdown) {
			Machine.stopVM(activity);
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
		} else if (item.getItemId() == R.id.itemCalibrateMouse) {
            calibration();
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
                        promptSetUIModeDesktop(LimboVNCActivity.this, false);
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

        try {
            MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);

            Config.mouseMode = Config.MouseMode.Trackpad;
            LimboSettingsManager.setDesktopMode(this, false);
            onFitToScreen();
            onMouse();

            //UIUtils.toastShort(LimboVNCActivity.this, "Trackpad Calibrating");
            invalidateOptionsMenu();
        } catch (Exception ex) {
            if(Config.debug)
                ex.printStackTrace();
        }
        }

    private void promptSetUIModeDesktop(final Activity activity, final boolean mouseMethodAlt) {


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

                setUIModeDesktop();
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

    private void setUIModeDesktop() {

	    try {
            MotionEvent a = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);
            Config.mouseMode = Config.MouseMode.External;
            LimboSettingsManager.setDesktopMode(this, true);
            UIUtils.toastShort(LimboVNCActivity.this, "External Mouse Enabled");
            onNormalScreen();
            AbstractScaling.getById(R.id.itemOneToOne).setScaleTypeForActivity(LimboVNCActivity.this);
            showPanningState();

            onMouse();
        } catch (Exception e) {
	        if(Config.debug)
	            e.printStackTrace();
        }
        //vncCanvas.reSize(false);
        invalidateOptionsMenu();
	}


    public void onViewLog() {
        FileUtils.viewLimboLog(this);
    }

	public void setContentView() {
		
		setContentView(R.layout.limbo_vnc);

	}

	private boolean toggleFullScreen() {
		
        UIUtils.toastShort(this, "VNC Fullscreen not supported");

		return false;
	}

	private boolean onFitToScreen() {

	    try {
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
            screenMode = VNCScreenMode.FitToScreen;
            setLayout(null);

            return true;
        } catch (Exception ex) {
	        if(Config.debug)
	            ex.printStackTrace();
        }
        return false;
	}

    private boolean onNormalScreen() {

	    try {
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
            screenMode = VNCScreenMode.Normal;
            setLayout(null);

            return true;
        } catch (Exception ex) {
	        if(Config.debug)
	            ex.printStackTrace();
        } finally {

        }
        return false;
    }

	private boolean onMouse() {

		// Limbo: For now we disable other modes
        if(Config.disableMouseModes)
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
    // this only fixes things temporarily.
    // There is a workaround to choose USB Tablet for mouse emulation
    // though it might not work for all Guest OSes
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
                    Thread.sleep(10);
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





	public static boolean toggleKeyboardFlag = true;

	private void onMonitor() {
        UIUtils.toastShort(this, "Connecting to QEMU Monitor, please wait");

        Thread t = new Thread(new Runnable() {
            public void run() {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                monitorMode = true;
                vncCanvas.sendMetaKey1(50, 6);

            }
        });
        t.start();
	}

	private void onVNC() {
        UIUtils.toastShort(this, "Connecting to VM, please wait");

        Thread t = new Thread(new Runnable() {
            public void run() {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                monitorMode = false;
                vncCanvas.sendMetaKey1(49, 6);
            }
        });
        t.start();


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
			    if(LimboActivity.getInstance()!=null)
				    LimboActivity.getInstance().saveSnapshotDB(stateName);

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

				UIUtils.toastShort(getApplicationContext(), "Please wait while saving VM State");

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

		if(response.contains("error")) {
            try {
                JSONObject object = new JSONObject(response);
                errorStr = object.getString("error");
            } catch (Exception ex) {
                if (Config.debug)
                    ex.printStackTrace();
            }
        }
		if (errorStr != null && errorStr.contains("desc")) {
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
					Machine.pausedErrorVM(activity, descStr1);
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
        if(!firstConnection)
			UIUtils.showHints(this);
        firstConnection = true;

        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {

                if(Config.mouseMode == Config.MouseMode.External)
                    setUIModeDesktop();
                else
                    setUIModeMobile();
            }
        },1000);

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
