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
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
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
        KeyMapManager.OnUnhandledTouchEventListener, MachineController.OnMachineStatusChangeListener,
        MachineController.OnEventListener {
    public static final int KEYBOARD = 10000;
    private static final String TAG = "LimboSDLActivity";
    private final static int keyDelay = 100;
    private final static int mouseButtonDelay = 100;

    public static boolean toggleKeyboardFlag = true;
    public static boolean isResizing = false;
    public static boolean pendingPause;
    public static boolean pendingStop;
    public static MouseMode mouseMode = MouseMode.Trackpad;
    private final ExecutorService mouseEventsExecutor = Executors.newFixedThreadPool(1);
    private final ExecutorService keyEventsExecutor = Executors.newFixedThreadPool(1);
    public DrivesDialogBox drives = null;
    public AudioManager am;
    protected int maxVolume;
    // store state
    private AudioTrack audioTrack;
    private AudioRecord mAudioRecord;
    private boolean monitorMode = false;
    private KeyMapManager mKeyMapManager;
    private ViewListener viewListener;
    private boolean quit = false;
    private View mGap;

    public void showHints() {
        ToastUtils.toastShortTop(this, getString(R.string.PressVolumeDownForRightClick));
    }

    public void setupToolBar() {
        Toolbar tb = findViewById(R.id.toolbar);
        setSupportActionBar(tb);

        // Get the ActionBar here to configure the way it behaves.
        ActionBar ab = getSupportActionBar();
        if (ab != null) {
            ab.setHomeAsUpIndicator(R.drawable.limbo); // set a custom icon
            ab.setDisplayShowHomeEnabled(true); // show or hide the default home
            ab.setDisplayHomeAsUpEnabled(true);
            ab.setDisplayShowCustomEnabled(true); // enable overriding the
            ab.setDisplayShowTitleEnabled(true); // disable the default title
            ab.setTitle(R.string.app_name);
            if (!LimboSettingsManager.getAlwaysShowMenuToolbar(this)) {
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
        quit = true;
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
        } else if (item.getItemId() == R.id.itemCtrlAltDel) {
            onCtrlAltDel();
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

        String[] items = {
                getString(R.string.TrackpadDescr),
                getString(R.string.TouchScreen),
                getString(R.string.ExternalMouseDescr)
        };
        final AlertDialog.Builder mBuilder = new AlertDialog.Builder(this);
        mBuilder.setTitle(R.string.Mouse);
        mBuilder.setSingleChoiceItems(items, -1, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int i) {
                switch (i) {
                    case 0:
                        setTrackpadMode();
                        break;
                    case 1:
                    case 2:
                        promptAbsoluteDevice(i == 2);
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

    protected void setTrackpadMode() {
        try {
            ScreenUtils.updateOrientation(this);
            mouseMode = MouseMode.Trackpad;
            invalidateOptionsMenu();
            ((LimboSDLSurface) mSurface).refreshSurfaceView();
            Presenter.getInstance().onAction(MachineAction.DISPLAY_CHANGED,
                    new Object[]{((LimboSDLSurface) mSurface).getWidth(),
                            ((LimboSDLSurface) mSurface).getHeight(), getResources().getConfiguration().orientation});
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }

    }

    private void promptAbsoluteDevice(final boolean externalMouse) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getString(R.string.desktopMode));

        LinearLayout mLayout = new LinearLayout(this);
        mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        TextView textView = new TextView(this);
        textView.setVisibility(View.VISIBLE);
        String instructions = getString(R.string.absolutePointerInstructions);
        if (externalMouse)
            instructions += "\n" + getString(R.string.externalMouseInstructions);
        textView.setText(instructions);
        mLayout.addView(textView);
        alertDialog.setView(mLayout);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                // we handle external mouse at all times
                if (!externalMouse)
                    setTouchScreenMode();
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

    protected void setTouchScreenMode() {
        try {
            mouseMode = MouseMode.TOUCHSCREEN;
            invalidateOptionsMenu();
            ((LimboSDLSurface) mSurface).refreshSurfaceView();
            Presenter.getInstance().onAction(MachineAction.DISPLAY_CHANGED,
                    new Object[]{((LimboSDLSurface) mSurface).getWidth(),
                            ((LimboSDLSurface) mSurface).getHeight(), getResources().getConfiguration().orientation});
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
        Log.v(TAG, "onCreate");
        setupScreen();
        saveAudioState();
        super.onCreate(savedInstanceState);
        mSingleton = this;
        restoreAudioState();
        setupVolume();
        setupWidgets();
        setupListeners();
        setupToolBar();
        showHints();
        ScreenUtils.updateOrientation(this);
        checkPendingActions();
    }

    private void setupScreen() {
        if (LimboSettingsManager.getFullscreen(this)) {
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
    }

    private void setupListeners() {
        MachineController.getInstance().addOnStatusChangeListener(this);
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

    private void setupWidgets() {
        mSurface = new LimboSDLSurface(this, this);

        setContentView(R.layout.limbo_sdl);
        mLayout = (RelativeLayout) findViewById(R.id.sdl_layout);

        setupKeyMapManager();
        RelativeLayout mLayout = (RelativeLayout) findViewById(R.id.sdl);
        mLayout.addView(mSurface);

        mGap = (View) findViewById(R.id.gap);
        updateLayout(getResources().getConfiguration().orientation);

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
            ScreenUtils.updateOrientation(this);
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
        sendMouseEvent(button, down ? 0 : 1, MotionEvent.TOOL_TYPE_FINGER, 0, 0);
    }

    @Override
    public void OnUnhandledTouchEvent(MotionEvent event) {
        if (isRelativeMode(event.getToolType(0)))
            processTrackPadEvents(event);
        else
            ((LimboSDLSurface) mSurface).onTouchProcess(mSurface, event);
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
        if (mouseMode == MouseMode.TOUCHSCREEN)
            return;
        ((LimboSDLSurface) mSurface).onTouchProcess(mSurface, event);
    }

    private void checkPendingActions() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                if (pendingStop) {
                    pendingStop = false;
                    LimboActivityCommon.promptStopVM(LimboSDLActivity.this, viewListener);
                } else if (pendingPause) {
                    pendingPause = false;
                    LimboActivityCommon.promptPause(LimboSDLActivity.this, viewListener);
                }
            }
        }, 1000);

        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                if (MachineController.getInstance().isPaused()) {
                    Presenter.getInstance().onAction(MachineAction.CONTINUE_VM, null);
                }
                ((LimboSDLSurface) mSurface).refreshSurfaceView();
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Presenter.getInstance().onAction(MachineAction.DISPLAY_CHANGED,
                                new Object[]{((LimboSDLSurface) mSurface).getWidth(),
                                        ((LimboSDLSurface) mSurface).getHeight(), getResources().getConfiguration().orientation});
                    }
                }, 2000);
            }
        }, 4000);
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
                }
            }
        } else {
            KeyboardUtils.hideKeyboard(this, mSurface);
            finish();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        invalidateOptionsMenu();
        updateLayout(newConfig.orientation);
    }

    public void updateLayout(int orientation) {
        if (orientation == Configuration.ORIENTATION_PORTRAIT)
            mGap.setVisibility(View.VISIBLE);
        else
            mGap.setVisibility(View.GONE);
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

    //    private static Thread limboSDLThread = null;
    @Override
    protected synchronized void runSDLMain() {
        notifyAction(MachineAction.START_VM, null);
        //XXX: we hold the thread because SDLActivity will exit
        while (!quit) {
            try {
                wait();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        Log.v(TAG, "SDLThread exited");
    }

    /**
     * Notifies when the machine resolution changes. This is called from SDL compat extensions
     * see folder jni/compat/sdl-extensions
     *
     * @param width Width
     * @param height Height
     */
    public void onVMResolutionChanged(int width, int height) {
        if (mSurface == null || LimboSDLActivity.isResizing) {
            return;
        }
        Log.v(TAG, "VM resolution changed to " + width + "x" + height);
        ((LimboSDLSurface) mSurface).refreshSurfaceView();
        Presenter.getInstance().onAction(MachineAction.DISPLAY_CHANGED,
                new Object[]{width, height, getResources().getConfiguration().orientation});
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
                sendMouseEvent(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_DOWN, MotionEvent.TOOL_TYPE_FINGER, 0, 0);
                sendMouseEvent(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, MotionEvent.TOOL_TYPE_FINGER, 0, 0);
            }
        });
        t.start();
    }


    public void sendMiddleClick() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                // we use a finger tool type to add the delay
                sendMouseEvent(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_DOWN, MotionEvent.TOOL_TYPE_FINGER, 0, 0);
                sendMouseEvent(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, MotionEvent.TOOL_TYPE_FINGER, 0, 0);
            }
        });
        t.start();
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

    /**
     * We treat as relative mode only events with TOOL_TYPE_FINGER as long as the user has not
     * selected to emulate a touch screen.
     *
     * @param toolType Event Tool type
     * @return True if the device will be expected as relative mode by the emulator
     */
    public boolean isRelativeMode(int toolType) {
        return toolType == MotionEvent.TOOL_TYPE_FINGER
                && LimboSDLActivity.mouseMode != LimboSDLActivity.MouseMode.TOUCHSCREEN;
    }


    protected void sendMouseEvent(int button, int action, int toolType, float x, float y) {
        //HACK: we generate an artificial delay since the qemu main event loop
        // is probably not able to process them if the timestamps are too close together?
        sendMouseEvent(button, action, toolType, x, y, action == MotionEvent.ACTION_UP ? mouseButtonDelay : 0);
    }

    private void sendMouseEvent(final int button, final int action, final int toolType,
                                final float x, final float y, final long delayMs) {
        mouseEventsExecutor.submit(new Runnable() {
            @Override
            public void run() {
                boolean relative = isRelativeMode(toolType);
                if (mKeyMapManager.processMouseMap(button, action)) {
                    return;
                }
                if (delayMs > 0 && toolType != MotionEvent.TOOL_TYPE_MOUSE)
                    delay(delayMs);
                //XXX: for mouse events we use our jni compatibility extensions instead of the sdl native functions
                // SDLActivity.onSDLNativeMouse(button, action, x, y);
                float nx = x;
                float ny = y;
                LimboSDLSurface.MouseState mouseState = ((LimboSDLSurface) mSurface).mouseState;
                if (mouseState.isDoubleTap()) {
                    nx = mouseState.taps.get(0).x;
                    ny = mouseState.taps.get(0).y;
                }
//                Log.v(TAG, "sendMouseEvent button: " + button + ", action: " + action
//                        + ", relative: " + relative + ", nx = " + nx + ", ny = " + ny
//                        + ", delay = " + delayMs);
                notifyAction(MachineAction.SEND_MOUSE_EVENT, new Object[]{button, action, relative ? 1 : 0, nx, ny});
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

    @Override
    public void onEvent(Machine machine, MachineController.Event event, Object o) {
        switch (event) {
            case MachineResolutionChanged:
                Object[] params = (Object[]) o;
                onVMResolutionChanged((int) params[0], (int) params[1]);
        }
    }

    public enum MouseMode {
        Trackpad, TOUCHSCREEN
    }

}