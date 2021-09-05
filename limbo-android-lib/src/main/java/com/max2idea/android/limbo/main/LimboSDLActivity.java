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
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.help.Help;
import com.max2idea.android.limbo.keyboard.KeyboardUtils;
import com.max2idea.android.limbo.keymapper.KeyMapManager;
import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.machine.Machine;
import com.max2idea.android.limbo.machine.MachineAction;
import com.max2idea.android.limbo.machine.MachineController;
import com.max2idea.android.limbo.machine.MachineProperty;
import com.max2idea.android.limbo.machine.Presenter;
import com.max2idea.android.limbo.qmp.QmpClient;
import com.max2idea.android.limbo.screen.ScreenUtils;
import com.max2idea.android.limbo.toast.ToastUtils;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLAudioManager;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Our overloaded SDLActivity with compatibility reroutes for the mouse and other extra functionality
 * for better usability. In general SDL is slower than VNC but offers Audio support for QEMU.
 */
public class LimboSDLActivity extends SDLActivity
        implements KeyMapManager.OnSendKeyEventListener, KeyMapManager.OnSendMouseEventListener,
        KeyMapManager.OnUnhandledTouchEventListener, MachineController.OnMachineStatusChangeListener {
    public static final int KEYBOARD = 10000;
    private static final String TAG = "LimboSDLActivity";
    private final static int keyDelay = 100;
    private final static int mouseButtonDelay = 100;

    public static boolean toggleKeyboardFlag = true;
    public static boolean isResizing = false;
    public static boolean pendingPause;
    public static boolean pendingStop;
    public static int vm_width;
    public static int vm_height;
    private final ExecutorService mouseEventsExecutor = Executors.newFixedThreadPool(1);
    private final ExecutorService keyEventsExecutor = Executors.newFixedThreadPool(1);
    public DrivesDialogBox drives = null;
    public SDLScreenMode screenMode = SDLScreenMode.FitToScreen;
    public AudioManager am;
    protected int maxVolume;
    // store state
    private AudioTrack audioTrack;
    private AudioRecord mAudioRecord;
    private boolean monitorMode = false;
    private KeyMapManager mKeyMapManager;
    private ViewListener viewListener;
    public static MouseMode mouseMode = MouseMode.Trackpad;

    public static LimboSDLActivity getSingleton() {
        return (LimboSDLActivity) mSingleton;
    }

    public static void showHints(Activity activity) {
        ToastUtils.toastShortTop(activity, activity.getString(R.string.PressVolumeDownForRightClick));
    }

    public static void setupToolBar(AppCompatActivity activity) {
        Toolbar tb = activity.findViewById(R.id.toolbar);
        activity.setSupportActionBar(tb);

        // Get the ActionBar here to configure the way it behaves.
        ActionBar ab = activity.getSupportActionBar();
        if (ab != null) {
            ab.setHomeAsUpIndicator(R.drawable.limbo); // set a custom icon
            ab.setDisplayShowHomeEnabled(true); // show or hide the default home
            ab.setDisplayHomeAsUpEnabled(true);
            ab.setDisplayShowCustomEnabled(true); // enable overriding the
            ab.setDisplayShowTitleEnabled(true); // disable the default title
            ab.setTitle(R.string.app_name);
            if (!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)) {
                ab.hide();
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == Config.OPEN_IMAGE_FILE_REQUEST_CODE || requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                drives.fileType = FileUtils.getFileTypeFromIntent(this, data);
                file = FileUtils.getFilePathFromIntent(this, data);
            }
            if (drives != null && file != null)
                drives.setDriveAttr(drives.fileType, file);
        } else if (requestCode == Config.OPEN_LOG_FILE_DIR_REQUEST_CODE || requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
            String file;
            if (requestCode == Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE) {
                file = FileUtils.getFileUriFromIntent(this, data, true);
            } else {
                file = FileUtils.getDirPathFromIntent(this, data);
            }
            if (file != null) {
                FileUtils.saveLogToFile(this, file);
            }
        }
    }

    public void sendCtrlAltKey(int code) {
        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_LEFT, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_ALT_LEFT, true);
        if (code >= 0) {
            sendKeyEvent(null, code, true);
            sendKeyEvent(null, code, false);
        }
        sendKeyEvent(null, KeyEvent.KEYCODE_ALT_LEFT, false);
        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_LEFT, false);
    }

    public void onDestroy() {
        mNextNativeState = NativeState.PAUSED;
        mIsResumedCalled = false;

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        SDLActivity.handleNativeState();
        SDLActivity.mSuspendOnly = true;
        removeListeners();

        super.onDestroy();
    }

    private void removeListeners() {
        MachineController.getInstance().removeOnStatusChangeListener(this);
        mKeyMapManager.setOnSendKeyEventListener(this);
        mKeyMapManager.setOnSendMouseEventListener(this);
        mKeyMapManager.setOnUnhandledTouchEventListener(this);
        setViewListener(null);
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {
        super.onOptionsItemSelected(item);
        if (item.getItemId() == R.id.itemDrives) {
            // Show up removable devices dialog
            if (MachineController.getInstance().getMachine().hasRemovableDevices()) {
                drives = new DrivesDialogBox(LimboSDLActivity.this, R.style.Transparent, MachineController.getInstance().getMachine());
                drives.show();
            } else {
                ToastUtils.toastShort(this, getString(R.string.NoRemovableDevicesAttached));
            }
        } else if (item.getItemId() == R.id.itemReset) {
            LimboActivityCommon.promptResetVM(this, viewListener);
        } else if (item.getItemId() == R.id.itemShutdown) {
            KeyboardUtils.hideKeyboard(this, mSurface);
            LimboActivityCommon.promptStopVM(this, viewListener);
        } else if (item.getItemId() == R.id.itemDisconnet) {
            finish();
        } else if (item.getItemId() == R.id.itemMouse) {
            onMouseMode();
        } else if (item.getItemId() == KEYBOARD || item.getItemId() == R.id.itemKeyboard) {
            //XXX: need to delay to work properly
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    toggleKeyboardFlag = KeyboardUtils.onKeyboard(LimboSDLActivity.this, toggleKeyboardFlag, mSurface);
                }
            }, 200);
        } else if (item.getItemId() == R.id.itemMonitor) {
            if (monitorMode) {
                onVMConsole();
            } else {
                onMonitor();
            }
        } else if (item.getItemId() == R.id.itemVolume) {
            onSelectMenuVol();
        } else if (item.getItemId() == R.id.itemSaveState) {
            LimboActivityCommon.promptPause(this, viewListener);
        } else if (item.getItemId() == R.id.itemFitToScreen) {
            onFitToScreen();
        } else if (item.getItemId() == R.id.itemStretchToScreen) {
            onStretchToScreen();
        } else if (item.getItemId() == R.id.itemCtrlAltDel) {
            onCtrlAltDel();
        } else if (item.getItemId() == R.id.itemCtrlC) {
            onCtrlC();
        } else if (item.getItemId() == R.id.itemOneToOne) {
            onNormalScreen();
        } else if (item.getItemId() == R.id.itemHelp) {
            Help.onHelp(this);
        } else if (item.getItemId() == R.id.itemHideToolbar) {
            onHideToolbar();
        } else if (item.getItemId() == R.id.itemDisplay) {
            onSelectMenuSDLDisplay();
        } else if (item.getItemId() == R.id.itemViewLog) {
            onViewLog();
        } else if (item.getItemId() == R.id.itemKeyboardMapper) {
            toggleKeyMapper();
        } else if (item.getItemId() == R.id.itemSendText) {
            promptSendText();
        }

        invalidateOptionsMenu();
        return true;
    }

    private void promptSendText() {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getString(R.string.SendText));
        final EditText text = new EditText(this);
        text.setText("");
        text.setEnabled(true);
        text.setVisibility(View.VISIBLE);
        text.setSingleLine();
        alertDialog.setView(text);

        ClipboardManager clipboardManager = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        ClipData clipData = clipboardManager.getPrimaryClip();
        if (clipData != null) {
            if (clipData.getItemCount() > 0)
                text.setText(clipData.getItemAt(0).getText());
        }

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Send), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                String textStr = text.getText().toString();
                sendText(textStr);
            }
        });
        alertDialog.show();
    }

    public void onViewLog() {
        Logger.viewLimboLog(this);
    }

    public void onHideToolbar() {
        ActionBar bar = getSupportActionBar();
        if (bar != null) {
            bar.hide();
        }
    }

    private void onMouseMode() {

        String[] items = {getString(R.string.TrackpadDescr),
                getString(R.string.ExternalMouseDescr), //Physical mouse for Chromebook, Android x86 PC, or Bluetooth Mouse
        };
        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle(R.string.Mouse);
        mBuilder.setSingleChoiceItems(items, mouseMode.ordinal(), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch (i) {
                    case 0:
                        setTrackpadMode(true);
                        break;
                    case 1:
                        promptSetDesktopMode();
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
        ActionBar bar = getSupportActionBar();

        if (!LimboSettingsManager.getAlwaysShowMenuToolbar(LimboSDLActivity.this)
                && bar != null && bar.isShowing()) {
            height += bar.getHeight();
        }

        return vm_width < width && vm_height < height;
    }

    public void calibration() {
        //XXX: Legacy: No need to calibrate for SDL trackpad.
    }

    protected void setTrackpadMode(boolean fitToScreen) {
        try {
            ScreenUtils.setOrientation(this);
            mouseMode = MouseMode.Trackpad;
            LimboSettingsManager.setDesktopMode(this, false);
            if (Config.showToast)
                ToastUtils.toastShort(getApplicationContext(), getString(R.string.TrackpadEnabled));
            if (fitToScreen)
                onFitToScreen();
            else
                onNormalScreen();
            calibration();
            invalidateOptionsMenu();
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }

    }

    private void promptSetDesktopMode() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getString(R.string.desktopMode));

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        TextView textView = new TextView(this);
        textView.setVisibility(View.VISIBLE);
        String desktopInstructions = getString(R.string.desktopInstructions);
        if (!checkVMResolutionFits()) {
            String resolutionWarning = getString(R.string.Warning) + ": " + getString(R.string.MachineResolution) + ": "
                    + vm_width + "x" + vm_height +
                    " " + getString(R.string.MachineResoultionWarning);
            desktopInstructions = resolutionWarning + desktopInstructions;
        }
        textView.setText(desktopInstructions);
        ScrollView scrollView = new ScrollView(this);
        scrollView.addView(textView);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(scrollView, params);
        alertDialog.setView(mLayout);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                setUIModeDesktop();
                alertDialog.dismiss();
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, getString(R.string.Cancel), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                alertDialog.dismiss();
            }
        });
        alertDialog.show();

    }

    protected void setUIModeDesktop() {
        try {
            mouseMode = MouseMode.External;
            LimboSettingsManager.setDesktopMode(this, true);
            if (Config.showToast)
                ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.ExternalMouseEnabled));
            onNormalScreen();
            calibration();
            invalidateOptionsMenu();
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }
    }

    private void onCtrlAltDel() {

        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_RIGHT, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_ALT_RIGHT, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_FORWARD_DEL, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_FORWARD_DEL, false);
        sendKeyEvent(null, KeyEvent.KEYCODE_ALT_RIGHT, false);
        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_RIGHT, false);
    }

    private void onCtrlC() {

        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_RIGHT, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_C, true);
        sendKeyEvent(null, KeyEvent.KEYCODE_C, false);
        sendKeyEvent(null, KeyEvent.KEYCODE_CTRL_RIGHT, false);
    }

    //FIXME: not working also scaling up has performance impact on libSDL
    private void onStretchToScreen() {
        Log.d(TAG, "onStretchToScreen");
        screenMode = SDLScreenMode.Fullscreen;
        sendCtrlAltKey(KeyEvent.KEYCODE_F); // not working
        if (Config.showToast)
            ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.ResizingPleaseWait));
        resize(null, 2000);
    }

    private void onFitToScreen() {
        try {
            ScreenUtils.setOrientation(this);
            ActionBar bar = getSupportActionBar();
            if (bar != null && !LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
                bar.hide();
            }
            Log.d(TAG, "onFitToScreen");
            screenMode = SDLScreenMode.FitToScreen;
            if (Config.showToast)
                ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.ResizingPleaseWait));
            resize(null, 2000);
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }

    }

    private void onNormalScreen() {
        try {
            ActionBar bar = getSupportActionBar();
            if (bar != null && !LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
                bar.hide();
            }
            //XXX: force Landscape on normal screen
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            Log.d(TAG, "onNormalScreen");
            screenMode = SDLScreenMode.Normal;
            if (Config.showToast)
                ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.ResizingPleaseWait));
            resize(null, 2000);
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }

    }

    /**
     * We need to resize the surfaceview in order to align it to the top of the screen.
     * Resizing creates some problems with the display and the mouse if it's done
     * in the beginning of the native initialization when the Activity starts that's why we need
     * to add an artificial delay.
     *
     * @param newConfig Configuration changes if available
     * @param delay     Delay in ms until resing happens
     */
    public void resize(final Configuration newConfig, final long delay) {
        //XXX: ignore further mouse events until we are done resizing
        //FIXME: This is a workaround so Nougat+ devices will update their layout
        // where we have to wait till sdl settles. Though this needs proper fixing
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (mSurface != null && ((LimboSDLSurface) mSurface).getHolder() != null) {
                    ((LimboSDLSurface) mSurface).getHolder().setFixedSize(1, 1);
                    setLayout(newConfig);
                    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            ((LimboSDLSurface) mSurface).resize(false);
                        }
                    }, delay);
                }
            }
        });
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.clear();
        getMenuInflater().inflate(R.menu.sdlactivitymenu, menu);

        int maxMenuItemsShown = 4;
        int actionShow = MenuItem.SHOW_AS_ACTION_IF_ROOM;
        if (ScreenUtils.isLandscapeOrientation(this)) {
            maxMenuItemsShown = 6;
            actionShow = MenuItem.SHOW_AS_ACTION_ALWAYS;
        }

        // Remove Monitor console for SDL2 it creates 2 SDL windows and SDL for
        // android supports only 1
        menu.removeItem(menu.findItem(R.id.itemMonitor).getItemId());

        // Remove scaling for now
        menu.removeItem(menu.findItem(R.id.itemScaling).getItemId());

        // Remove external mouse for now
        menu.removeItem(menu.findItem(R.id.itemExternalMouse).getItemId());

        menu.removeItem(menu.findItem(R.id.itemCtrlAltDel).getItemId());
        menu.removeItem(menu.findItem(R.id.itemCtrlC).getItemId());

        if (MachineController.getInstance().getMachine().getSoundCard() == null) {
            menu.removeItem(menu.findItem(R.id.itemVolume).getItemId());
            maxMenuItemsShown--;
        }

        for (int i = 0; i < menu.size() && i < maxMenuItemsShown; i++) {
            menu.getItem(i).setShowAsAction(actionShow);
        }
        return true;
    }

    private void onMonitor() {
        new Thread(new Runnable() {
            public void run() {
                monitorMode = true;
                //TODO: enable qemu monitor if and when libSDL for Android allows
                // multiple windows
                sendCtrlAltKey(KeyEvent.KEYCODE_2);
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
            sendText(event.getCharacters());
            return true;
        } else if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
            onBackPressed();
            return true;
        }
        if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) {
            // We emulate right click with volume down
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                sendRightClick();
            }
            return true;
        } else if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP) {
            // We emulate middle click with volume up
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                sendMiddleClick();
            }
            return true;
        } else {
            return super.dispatchKeyEvent(event);
        }
    }

    private void sendText(String string) {
        KeyCharacterMap keyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
        KeyEvent[] keyEvents = keyCharacterMap.getEvents(string.toCharArray());
        if (keyEvents == null)
            return;
        for (KeyEvent keyEvent : keyEvents) {
            if (keyEvent.getAction() == KeyEvent.ACTION_DOWN) {
                sendKeyEvent(null, keyEvent.getKeyCode(), true);
            } else if (keyEvent.getAction() == KeyEvent.ACTION_UP) {
                sendKeyEvent(null, keyEvent.getKeyCode(), false, 10);
            }
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "Started Activity");
        if (LimboSettingsManager.getFullscreen(this)) {
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }

        saveAudioState();
        super.onCreate(savedInstanceState);
        updateMouseMode();
        restoreAudioState();
        setupVolume();
        mSingleton = this;
        MachineController.getInstance().addOnStatusChangeListener(this);
        if (MachineController.getInstance().getMachine() == null) {
            ToastUtils.toastShort(this, getString(R.string.NoVMSelectedRestart));
        }

        // So we can call stuff from static callbacks
        mSingleton = this;

        createUI(0, 0);
        setupController();
        setupToolBar(this);
        showHints(this);
        ScreenUtils.setOrientation(this);
        checkPendingActions();
    }

    private void updateMouseMode() {
        boolean enableDesktopMode = LimboSettingsManager.getDesktopMode(this);
        if (enableDesktopMode) {
            mouseMode = LimboSDLActivity.MouseMode.External;
        } else
            mouseMode = LimboSDLActivity.MouseMode.Trackpad;
    }

    private void setupController() {
        setViewListener((ViewListener) Presenter.getInstance());
    }

    public void setViewListener(ViewListener viewListener) {
        this.viewListener = viewListener;
    }

    private void saveAudioState() {
        audioTrack = SDLAudioManager.mAudioTrack;
        mAudioRecord = SDLAudioManager.mAudioRecord;
    }

    private void restoreAudioState() {
        SDLAudioManager.mAudioTrack = audioTrack;
        SDLAudioManager.mAudioRecord = mAudioRecord;
    }

    private void createUI(int width, int height) {
        mSurface = new LimboSDLSurface(this, this);

        int newWidth = width;
        int newHeight = height;
        if (newWidth == 0) {
            newWidth = RelativeLayout.LayoutParams.WRAP_CONTENT;
        }
        if (newHeight == 0) {
            newHeight = RelativeLayout.LayoutParams.WRAP_CONTENT;
        }
        setContentView(R.layout.limbo_sdl);
        mLayout = (RelativeLayout) findViewById(R.id.sdl_layout);

        setupKeyMapManager();
        RelativeLayout mLayout = (RelativeLayout) findViewById(R.id.sdl);
        RelativeLayout.LayoutParams surfaceParams = new RelativeLayout.LayoutParams(newWidth, newHeight);
        surfaceParams.addRule(RelativeLayout.CENTER_IN_PARENT);
        mLayout.addView(mSurface, surfaceParams);

    }

    private void setupKeyMapManager() {
        try {
            int size = LimboSettingsManager.getKeyMapperSize(this);
            mKeyMapManager = new KeyMapManager(this, mSurface, size, 2 * size);
            mKeyMapManager.setOnSendKeyEventListener(this);
            mKeyMapManager.setOnSendMouseEventListener(this);
            mKeyMapManager.setOnUnhandledTouchEventListener(this);
        } catch (Exception e) {
            e.printStackTrace();
            ToastUtils.toastShort(this, e.getMessage());
        }

    }

    private void toggleKeyMapper() {
        KeyboardUtils.hideKeyboard(LimboSDLActivity.this, mSurface);
        boolean shown = mKeyMapManager.toggleKeyMapper();
        if (shown) {
            //XXX: force portrait when key mapper is on edit mode
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            toggleKeyboardFlag = KeyboardUtils.onKeyboard(LimboSDLActivity.this, false, mSurface);
        } else {
            // restore
            ScreenUtils.setOrientation(this);
        }
    }

    protected void onPause() {
        if (MachineController.getInstance().isRunning())
            notifyAction(MachineAction.UPDATE_NOTIFICATION,
                    getString(R.string.VMSuspended));
        super.onPause();
    }

    public void onSelectMenuVol() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getString(R.string.Volume));

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

        LinearLayout t = createVolumePanel();
        t.setLayoutParams(volParams);

        ScrollView s = new ScrollView(this);
        s.addView(t);
        alertDialog.setView(s);
        alertDialog.setButton(android.app.Dialog.BUTTON_POSITIVE, getString(android.R.string.ok), new DialogInterface.OnClickListener() {

            public void onClick(DialogInterface dialog, int which) {
                alertDialog.cancel();
            }
        });
        alertDialog.show();

    }

    public LinearLayout createVolumePanel() {
        LinearLayout layout = new LinearLayout(this);
        layout.setPadding(20, 20, 20, 20);
        LinearLayout.LayoutParams volparams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
        SeekBar vol = new SeekBar(this);
        int volume;
        vol.setMax(maxVolume);
        volume = getCurrentVolume();
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
        if (MachineController.getInstance().isRunning())
            notifyAction(MachineAction.UPDATE_NOTIFICATION,
                    getString(R.string.VMRunning));
        super.onResume();
    }

    public void loadLibraries() {
        //XXX: Do not remove we need this to prevent loading libraries from SDL so
        // we handle libraries for specific architectures later
    }

    @Override
    public void onSendKeyEvent(int keyCode, boolean down) {
        sendKeyEvent(null, keyCode, down);
    }

    @Override
    public void onSendMouseEvent(int button, boolean down) {
        // events from key mapper should be finger tool type with a delay
        sendMouseEvent(button, down ? 0 : 1, getRelativeMode(), MotionEvent.TOOL_TYPE_FINGER, 0, 0);
    }

    @Override
    public void OnUnhandledTouchEvent(MotionEvent event) {
        onTouchEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        processTrackPadEvents(event);
        return true;
    }

    /**
     * For Virtual Trackpad we need relative coordinates so we capture the events from the
     * activity since we want to use the whole area for touch gestures therefore this should be
     * called from within the activity onTouchEvent callbacks
     *
     * @param event MotionEvent to be processed
     */
    public void processTrackPadEvents(MotionEvent event) {
        if (mouseMode == MouseMode.External)
            return;
        ((LimboSDLSurface) mSurface).onTouchProcess(mSurface, event);
    }

    private void checkPendingActions() {
        new Thread(new Runnable() {
            public void run() {
                if (pendingStop) {
                    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            pendingStop = false;
                            LimboActivityCommon.promptStopVM(LimboSDLActivity.this, viewListener);
                        }
                    }, 1000);
                } else if (pendingPause) {
                    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            pendingPause = false;
                            LimboActivityCommon.promptPause(LimboSDLActivity.this, viewListener);
                        }
                    }, 1000);
                } else if (MachineController.getInstance().isPaused()) {
                    try {
                        Thread.sleep(4000);
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }
                    String command = QmpClient.getContinueVMCommand();
                    QmpClient.sendCommand(command);
                    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (mouseMode == MouseMode.External)
                                setUIModeDesktop();
                            else
                                setTrackpadMode(screenMode == SDLScreenMode.FitToScreen);
                        }
                    }, 1000);
                }
            }
        }).start();
    }

    public void onBackPressed() {
        if (mKeyMapManager != null && mKeyMapManager.isEditMode()) {
            mKeyMapManager.useKeyMapper();
        } else if (!LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
            ActionBar bar = getSupportActionBar();
            if (bar != null) {
                if (bar.isShowing())
                    bar.hide();
                else {
                    bar.show();
                    checkScreenSize();
                }
            }
        } else {
            KeyboardUtils.hideKeyboard(this, mSurface);
            finish();
        }
    }

    private void checkScreenSize() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                if(vm_height > ((LimboSDLSurface) mSurface).getHeight()
                        || vm_width > ((LimboSDLSurface) mSurface).getWidth()
                ) {
                    ToastUtils.toast(LimboSDLActivity.this, getString(R.string.DisplayResolutionHighWarning), Gravity.BOTTOM, Toast.LENGTH_LONG);
                }
            }
        }, 2000);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        invalidateOptionsMenu();
    }

    public void onSelectMenuSDLDisplay() {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getString(R.string.display));

        LinearLayout.LayoutParams volParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);

        LinearLayout t = createSDLDisplayPanel();
        t.setLayoutParams(volParams);

        ScrollView s = new ScrollView(this);
        s.addView(t);
        alertDialog.setView(s);
        alertDialog.setButton(android.app.Dialog.BUTTON_POSITIVE, getString(R.string.Ok), new DialogInterface.OnClickListener() {

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
        Button displayMode = new Button(this);

        displayMode.setText(R.string.DisplayMode);
        displayMode.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                onDisplayMode();
            }
        });
        buttonsLayout.addView(displayMode);
        layout.addView(buttonsLayout);

        final TextView value = new TextView(this);
        String msg = getString(R.string.IdleRefreshRate) + ": " + currRate + " Hz";
        value.setText(msg);
        layout.addView(value);
        value.setLayoutParams(params);

        SeekBar rate = new SeekBar(this);
        rate.setMax(Config.MAX_DISPLAY_REFRESH_RATE);

        rate.setProgress(currRate);
        rate.setLayoutParams(params);

        rate.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int progress, boolean touch) {
                String message = getString(R.string.IdleRefreshRate) + ": " + (progress + 1) + " " + "Hz";
                value.setText(message);
            }

            public void onStartTrackingTouch(SeekBar arg0) {
            }

            public void onStopTrackingTouch(SeekBar arg0) {
                int progress = arg0.getProgress() + 1;
                int refreshMs = 1000 / progress;
                notifyAction(MachineAction.SET_SDL_REFRESH_RATE, refreshMs);
            }
        });
        layout.addView(rate);
        return layout;
    }

    public int getCurrentSDLRefreshRate() {
        return 1000 / MachineController.getInstance().getSdlRefreshRate();
    }

    private void onDisplayMode() {
        String[] items = {
                "Normal (One-To-One)",
                "Fit To Screen"
//                ,"Stretch To Screen" //TODO: Stretched
        };
        int currentScaleType = 0;
        if (screenMode == SDLScreenMode.FitToScreen) {
            currentScaleType = 1;
        } else if (screenMode == SDLScreenMode.Fullscreen)
            currentScaleType = 2;

        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle(R.string.DisplayMode);
        mBuilder.setSingleChoiceItems(items, currentScaleType, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch (i) {
                    case 0:
                        onNormalScreen();
                        break;
                    case 1:
                        if (mouseMode == MouseMode.External) {
                            ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.FitToScreenDisableDesktopMode));
                            dialog.dismiss();
                            return;
                        }
                        onFitToScreen();
                        break;
                    case 2:
                        if (mouseMode == MouseMode.External) {
                            ToastUtils.toastShort(LimboSDLActivity.this, getString(R.string.StretchScreenDisabledDesktopMode));
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

    //    private static Thread limboSDLThread = null;
    @Override
    protected synchronized void runSDLMain() {
        notifyAction(MachineAction.START_VM, null);

        //XXX: we hold the thread because SDLActivity will exit
        try {
            wait();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        Log.v(TAG, "SDLThread exited");
    }

    /**
     * Notifies when the machine resolution changes. This is called from SDL compat extensions
     * see folder jni/compat/sdl-extensions
     *
     * @param w Width
     * @param h Height
     */
    public void onVMResolutionChanged(int w, int h) {
        if (mSurface == null || LimboSDLActivity.isResizing) {
            return;
        }

        boolean refreshDisplay = false;

        if (w != vm_width || h != vm_height)
            refreshDisplay = true;
        vm_width = w;
        vm_height = h;

        Log.v(TAG, "VM resolution changed to " + vm_width + "x" + vm_height);
        if (refreshDisplay) {
            resize(null, 500);
        }
    }

    private void setLayout(Configuration newConfig) {

        boolean isLanscape = (newConfig != null
                && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)
                || ScreenUtils.isLandscapeOrientation(this);

        View sdl_layout = findViewById(R.id.sdl_layout);
        RelativeLayout.LayoutParams layoutParams;
        //normal 1-1
        if (screenMode == SDLScreenMode.Normal) {
            if (isLanscape) {
                layoutParams = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                layoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                layoutParams.addRule(RelativeLayout.CENTER_HORIZONTAL);

            } else {
                layoutParams = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                layoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                layoutParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
            }
        } else {
            //fittoscreen
            if (isLanscape) {
                layoutParams = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                );
                layoutParams.addRule(RelativeLayout.CENTER_IN_PARENT);
            } else {

                layoutParams = new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                );
                layoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                layoutParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
            }
        }
        sdl_layout.setLayoutParams(layoutParams);
        invalidateOptionsMenu();
    }

    protected void setupVolume() {
        if (am == null) {
            am = (AudioManager) mSingleton.getSystemService(Context.AUDIO_SERVICE);
            maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        }
    }

    public void setVolume(int volume) {
        if (am != null)
            am.setStreamVolume(AudioManager.STREAM_MUSIC, volume, 0);
    }

    protected int getCurrentVolume() {
        int volumeTmp = 0;
        if (am != null)
            volumeTmp = am.getStreamVolume(AudioManager.STREAM_MUSIC);
        return volumeTmp;
    }

    //XXX: We want to suspend only when app is calling onPause()
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {

    }

    public void sendRightClick() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                // we use a finger tool type to add the delay
                sendMouseEvent(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_DOWN, getRelativeMode(), MotionEvent.TOOL_TYPE_FINGER, 0, 0);
                sendMouseEvent(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, getRelativeMode(), MotionEvent.TOOL_TYPE_FINGER, 0, 0);
            }
        });
        t.start();
    }

    private int getRelativeMode() {
        return mouseMode == MouseMode.Trackpad?1:0;
    }

    public void sendMiddleClick() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                // we use a finger tool type to add the delay
                sendMouseEvent(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_DOWN, getRelativeMode(), MotionEvent.TOOL_TYPE_FINGER, 0, 0);
                sendMouseEvent(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, getRelativeMode(), MotionEvent.TOOL_TYPE_FINGER, 0, 0);
            }
        });
        t.start();
    }

    private int getToolType() {
        return mouseMode == MouseMode.Trackpad?MotionEvent.TOOL_TYPE_FINGER:MotionEvent.TOOL_TYPE_MOUSE;
    }

    public View getMainLayout() {
        return findViewById(R.id.top_layout);
    }

    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyLongPress: " + keyCode);
        return false;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!processKey(keyCode, event))
            return super.onKeyDown(keyCode, event);
        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (!processKey(keyCode, event))
            return super.onKeyUp(keyCode, event);
        return true;
    }

    public boolean processKey(int keyCode, KeyEvent event) {
        if ((keyCode == KeyEvent.KEYCODE_BACK) || (keyCode == KeyEvent.KEYCODE_FORWARD)) {
            // dismiss android back and forward keys
            return true;
        } else if (event.getKeyCode() == KeyEvent.KEYCODE_MENU) {
            return false;
        } else if (event.getAction() == KeyEvent.ACTION_DOWN) {
            sendKey(event, keyCode, event.getAction(), true);
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            sendKey(event, keyCode, event.getAction(), false);
            return true;
        } else {
            return false;
        }
    }

    private synchronized void sendKey(final KeyEvent event, final int keyCode, final int action,
                                      final boolean down) {
        if (!handleMissingKeys(keyCode, action)) {
            sendKeyEvent(event, keyCode, down);
        }
    }

    // Handles key codes missing in sdl2-keymap.h
    // This function will create them with a Shift Modifier
    private boolean handleMissingKeys(int keyCode, int action) {

        int newKeyCode;
        switch (keyCode) {
            case 77:
                newKeyCode = 9;
                break;
            case 81:
                newKeyCode = 70;
                break;
            case 17:
                newKeyCode = 15;
                break;
            case 18:
                newKeyCode = 10;
                break;
            default:
                return false;
        }
        if (action == KeyEvent.ACTION_DOWN) {
            sendKeyEvent(null, 59, true);
            sendKeyEvent(null, newKeyCode, true);
        } else {
            sendKeyEvent(null, 59, false);
            sendKeyEvent(null, newKeyCode, false);
        }
        return true;
    }

    private void delay(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    protected void sendMouseEvent(int button, int action, int relative, int toolType, float x, float y) {
        //HACK: we generate an artificial delay since the qemu main event loop
        // is probably not able to process them if the timestamps are too close together?
        sendMouseEvent(button, action, relative, toolType, x, y, action == MotionEvent.ACTION_UP ? mouseButtonDelay : 0);
    }

    private void sendMouseEvent(final int button, final int action, final int relative,
                                final int toolType, final float x, final float y, final long delayMs) {
        mouseEventsExecutor.submit(new Runnable() {
            @Override
            public void run() {

                if (mKeyMapManager.processMouseMap(button, action)) {
                    return;
                }
                if (delayMs > 0 && toolType != MotionEvent.TOOL_TYPE_MOUSE)
                    delay(delayMs);
                Log.v(TAG, "sendMouseEvent button: " + button + ", action: " + action + ", relative: " + relative + ", x = " + x + ", y = " + y + ", delay = " + delayMs);
                //XXX: for mouse events we use our jni compatibility extensions instead of the sdl native functions
                // SDLActivity.onSDLNativeMouse(button, action, x, y);
                float nx = x;
                float ny = y;
                LimboSDLSurface.MouseState mouseState = ((LimboSDLSurface) mSurface).mouseState;
                if(mouseState.isDoubleTap()){
                    nx = mouseState.taps.get(0).x;
                    ny = mouseState.taps.get(0).y;
                    Log.v(TAG, "Double tap mouse event detected: " + nx + ", " + ny);
                }
                notifyAction(MachineAction.SEND_MOUSE_EVENT, new Object[]{button, action, relative, nx, ny});
                if (delayMs > 0 && toolType != MotionEvent.TOOL_TYPE_MOUSE)
                    delay(delayMs);
            }
        });
    }

    protected void sendKeyEvent(KeyEvent event, int keycode, boolean down) {
        //HACK: we generate an artificial delay since the qemu main event loop
        // is probably not able to process them if the timestamps are too close together?
        sendKeyEvent(event, keycode, down, !down ? keyDelay : 0);
    }

    private void sendKeyEvent(final KeyEvent event, final int keycode, final boolean down, final long delayMs) {
        keyEventsExecutor.submit(new Runnable() {
            @Override
            public void run() {
                if (mKeyMapManager != null && mKeyMapManager.processKeyMap(event)) {
                    return;
                }
                if (delayMs > 0)
                    delay(delayMs);
//                Log.v(TAG, "sendKeyEvent: " + ", keycode = " + keycode + ", down = " + down + ", delay = " + delayMs);
                if (down)
                    SDLActivity.onNativeKeyDown(keycode);
                else {
                    SDLActivity.onNativeKeyUp(keycode);
                }
            }
        });
    }

    @Override
    public void onMachineStatusChanged(Machine machine, MachineController.MachineStatus status, Object o) {
        switch (status) {
            case SaveFailed:
                LimboActivityCommon.promptPausedErrorVM(this, (String) o, viewListener);
                break;
            case SaveCompleted:
                LimboActivityCommon.promptPausedVM(this, viewListener);
                break;
        }
    }

    public void notifyFieldChange(MachineProperty property, Object value) {
        if (viewListener != null)
            viewListener.onFieldChange(property, value);
    }

    public void notifyAction(MachineAction action, Object value) {
        if (viewListener != null)
            viewListener.onAction(action, value);
    }

    public enum SDLScreenMode {
        Normal,
        FitToScreen,
        Fullscreen //fullscreen not implemented yet
    }

    public enum MouseMode {
        Trackpad, External
    }
}