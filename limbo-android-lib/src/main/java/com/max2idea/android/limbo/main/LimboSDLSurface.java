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

import android.content.Context;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;

import com.max2idea.android.limbo.machine.MachineAction;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLControllerManager;

import java.util.ArrayList;

/**
 * This class is an extension of the SurfaceView used by the SDL java portion of the code. This
 * custom class enables trackpad and external mouse support (Desktop Mode) as well as orientation
 * changes, and a usefull keymapper. The trackpad and mouse support is part our custom SDL extensions
 * (see jni/compat/sdl-extensions).
 */
public class LimboSDLSurface extends SDLActivity.ExSDLSurface
        implements View.OnKeyListener, View.OnTouchListener {
    private static final String TAG = "LimboSDLSurface";

    MouseState mouseState = new MouseState();
    private boolean firstTouch = false;

    private final LimboSDLActivity sdlActivity;

    public LimboSDLSurface(LimboSDLActivity sdlActivity, Context context) {
        super(context);
        this.sdlActivity = sdlActivity;
        //XXX: we don't process keys in this view but in LimboSDLActivity
        // here we just suppress
        setOnKeyListener(this);
        setOnTouchListener(this);
        setOnGenericMotionListener(new SDLGenericMotionListener_API12());
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged: " + width + "x" + height);
        super.surfaceChanged(holder, format, width, height);
        refreshSurfaceView();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated");
        super.surfaceCreated(holder);
        setWillNotDraw(false);
        refreshSurfaceView();
    }

    public void refreshSurfaceView() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                sdlActivity.setFullscreen();
                // notify the controller that our display has changed
                sdlActivity.notifyAction(MachineAction.DISPLAY_CHANGED,
                        new Object[]{getWidth(), getHeight(), getResources().getConfiguration().orientation});
            }
        }).start();
    }

    public boolean onTouchProcess(View v, MotionEvent event) {
        int action = event.getActionMasked();
        mouseState.x = event.getX();
        mouseState.y = event.getY();

        processMouseMovement(action, event.getToolType(0), mouseState.x, mouseState.y);
        processMouseButton(event, action, mouseState.x, mouseState.y);
        return false;
    }

    private void processMouseMovement(int action, int toolType, float x, float y) {
        if (action == MotionEvent.ACTION_MOVE) {
            if (mouseState.mouseUp) {
                mouseState.old_x = x;
                mouseState.old_y = y;
                mouseState.mouseUp = false;
            }

            float nx = x;
            float ny = y;
            if (sdlActivity.isRelativeMode(toolType)) {
                nx = (x - mouseState.old_x);
                ny = (y - mouseState.old_y);
            }
            sdlActivity.sendMouseEvent(0, MotionEvent.ACTION_MOVE, toolType, nx, ny);

            mouseState.old_x = x;
            mouseState.old_y = y;
        }
    }

    private void processMouseButton(MotionEvent event, int action, float x, float y) {
        processPendingMouseButtonDown(action, event.getToolType(0), x, y);
        int sdlMouseButton = getMouseButton(event);

        if (action == MotionEvent.ACTION_UP) {
            mouseState.addAction(event.getToolType(0), System.currentTimeMillis(), event.getActionMasked(), x, y);
            //XXX: The Button state might not be available when the action is UP
            //  we should release all mouse buttons to be safe since we don't know which one fired the event
            if (sdlMouseButton == Config.SDL_MOUSE_MIDDLE || sdlMouseButton == Config.SDL_MOUSE_RIGHT
                    || sdlMouseButton != 0) {
                if (sdlActivity.isRelativeMode(event.getToolType(0)))
                    sdlActivity.sendMouseEvent(sdlMouseButton, MotionEvent.ACTION_UP, event.getToolType(0), 0, 0);
                else
                    sdlActivity.sendMouseEvent(sdlMouseButton, MotionEvent.ACTION_UP, event.getToolType(0), x, y);

            } else { // if we don't have information about which button we can make some guesses
                guessMouseButtonUp(event.getToolType(0), x, y);
            }
            mouseState.lastMouseButtonDown = -1;
            mouseState.mouseUp = true;

        } else if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            mouseState.addAction(event.getToolType(0), System.currentTimeMillis(), event.getActionMasked(), x, y);
            //XXX: Some touch events for touchscreen mode are primary so we force left mouse button
            if (sdlMouseButton == 0 && MotionEvent.TOOL_TYPE_FINGER == event.getToolType(0)) {
                sdlMouseButton = Config.SDL_MOUSE_LEFT;
            }
            if (sdlActivity.isRelativeMode(event.getToolType(0))) {
                if (!firstTouch) {
                    sdlActivity.sendMouseEvent(sdlMouseButton, MotionEvent.ACTION_DOWN, event.getToolType(0), 0, 0);
                    firstTouch = true;
                } else {
                    setPendingMouseDown(x, y, sdlMouseButton);
                }
            } else {
                sdlActivity.sendMouseEvent(sdlMouseButton, MotionEvent.ACTION_DOWN, event.getToolType(0), x, y);
            }
            mouseState.lastMouseButtonDown = sdlMouseButton;

        }
    }

    private void processPendingMouseButtonDown(int action, int toolType, float x, float y) {
        long delta = System.currentTimeMillis() - mouseState.down_event_time;
        if (mouseState.down_pending && sdlActivity.isRelativeMode(toolType)
                && (Math.abs(x - mouseState.down_x) < 20 && Math.abs(y - mouseState.down_y) < 20)
                && ((action == MotionEvent.ACTION_MOVE && delta > 400)
                || action == MotionEvent.ACTION_UP)) {
            sdlActivity.sendMouseEvent(mouseState.down_mouse_button, MotionEvent.ACTION_DOWN, toolType, 0, 0);
            mouseState.down_pending = false;
        } else if (System.currentTimeMillis() - mouseState.down_event_time > 400) {
            mouseState.down_pending = false;
        }
    }

    private int getMouseButton(MotionEvent event) {
        int sdlMouseButton = 0;
        if (event.getButtonState() == MotionEvent.BUTTON_PRIMARY)
            sdlMouseButton = Config.SDL_MOUSE_LEFT;
        else if (event.getButtonState() == MotionEvent.BUTTON_SECONDARY)
            sdlMouseButton = Config.SDL_MOUSE_RIGHT;
        else if (event.getButtonState() == MotionEvent.BUTTON_TERTIARY)
            sdlMouseButton = Config.SDL_MOUSE_MIDDLE;
        return sdlMouseButton;
    }

    private void guessMouseButtonUp(int toolType, float x, float y) {
        //Or only the last one pressed
        if (mouseState.lastMouseButtonDown > 0) {
            if (sdlActivity.isRelativeMode(toolType)) {
                sdlActivity.sendMouseEvent(mouseState.lastMouseButtonDown, MotionEvent.ACTION_UP, toolType, 0, 0);
            } else {
                sdlActivity.sendMouseEvent(mouseState.lastMouseButtonDown, MotionEvent.ACTION_UP, toolType, x, y);
            }
        } else {
            //ALl buttons
            if (sdlActivity.isRelativeMode(toolType)) {
                sdlActivity.sendMouseEvent(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, toolType, 0, 0);
            } else {
                sdlActivity.sendMouseEvent(Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, toolType, x, y);
                sdlActivity.sendMouseEvent(Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, toolType, x, y);
                sdlActivity.sendMouseEvent(Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, toolType, x, y);
            }
        }
    }

    private void setPendingMouseDown(float x, float y, int sdlMouseButton) {
        mouseState.down_pending = true;
        mouseState.down_x = x;
        mouseState.down_y = y;
        mouseState.down_mouse_button = sdlMouseButton;
        mouseState.down_event_time = System.currentTimeMillis();
    }

    public boolean onTouch(View v, MotionEvent event) {
        return processExternalMouseEvents(v, event);
    }

    /**
     * For External Mouse we need absolute coordinates so we capture the events from the
     * surface view so this should be called from within the surfaceview's onTouch() callback
     *
     * @param v     View originating
     * @param event MotionEvent to be processed
     * @return true if event is consumed
     */
    private boolean processExternalMouseEvents(View v, MotionEvent event) {
        if (event.getToolType(0) != MotionEvent.TOOL_TYPE_FINGER
                || sdlActivity.mouseMode == LimboSDLActivity.MouseMode.TOUCHSCREEN) {
            onTouchProcess(v, event);
            return true;
        }
        return false;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        // if keys are other than keyboard (joystick, etc) we send to the parent SDLSurface
        if (event.getSource() != InputDevice.SOURCE_KEYBOARD)
            return super.onKey(v, keyCode, event);
        return false;
    }

    class MouseState {
        public float x = 0;
        public float y = 0;
        public float old_x = 0;
        public float old_y = 0;
        public float down_x = 0;
        public float down_y = 0;
        public int down_mouse_button = 0;
        public long down_event_time = 0;
        public ArrayList<MouseAction> taps = new ArrayList<>();
        private boolean mouseUp = true;
        private int lastMouseButtonDown = -1;
        private boolean down_pending = false;

        public void addAction(int toolType, long time, int actionMasked, float x, float y) {

            if (taps.size() > 1 && time - taps.get(taps.size() - 2).time > 200) {
                taps.clear();
            } else if (taps.size() == 4) {
                taps.clear();
            }
            taps.add(new MouseAction(toolType, time, actionMasked, (int) x, (int) y));

        }

        public boolean isDoubleTap() {
            if (LimboSDLActivity.mouseMode == LimboSDLActivity.MouseMode.TOUCHSCREEN
                    && taps.size() >= 3
                    && taps.get(0).toolType == MotionEvent.TOOL_TYPE_FINGER
                    && taps.get(0).action == MotionEvent.ACTION_DOWN
                    && taps.get(1).toolType == MotionEvent.TOOL_TYPE_FINGER
                    && taps.get(1).action == MotionEvent.ACTION_UP
                    && taps.get(0).x == taps.get(1).x
                    && taps.get(0).y == taps.get(1).y
                    && taps.get(2).toolType == MotionEvent.TOOL_TYPE_FINGER
                    && taps.get(2).action == MotionEvent.ACTION_DOWN
            )
                return true;
            return false;
        }

        public class MouseAction {
            private final long time;
            int action;
            int toolType;
            int x, y;

            public MouseAction(int toolType, long time, int actionMasked, int x, int y) {
                this.action = actionMasked;
                this.time = time;
                this.toolType = toolType;
                this.x = x;
                this.y = y;
            }
        }
    }

    /**
     * A motion listener for the external mouse and other peripheral which are NOT TESTED!
     */
    class SDLGenericMotionListener_API12 implements View.OnGenericMotionListener {

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
                    action = event.getActionMasked();
                    switch (action) {
                        case MotionEvent.ACTION_SCROLL:
                            x = event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0);
                            y = event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0);
                            sdlActivity.sendMouseEvent(0, action, event.getToolType(0), x, y);
                            return true;

                        case MotionEvent.ACTION_HOVER_MOVE:
                            if (Config.processMouseHistoricalEvents) {
                                final int historySize = event.getHistorySize();
                                for (int h = 0; h < historySize; h++) {
                                    float ex = event.getHistoricalX(h);
                                    float ey = event.getHistoricalY(h);
                                    float ep = event.getHistoricalPressure(h);
                                    sdlActivity.sendMouseEvent(0, action, event.getToolType(0), ex, ey);
                                }
                            }
                            float ex = event.getX();
                            float ey = event.getY();
                            float ep = event.getPressure();
                            sdlActivity.sendMouseEvent(0, action, event.getToolType(0), ex, ey);
                            return true;
                        case MotionEvent.ACTION_UP:
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            return false;
        }
    }

}
