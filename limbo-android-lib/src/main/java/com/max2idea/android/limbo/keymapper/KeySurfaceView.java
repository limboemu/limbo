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
package com.max2idea.android.limbo.keymapper;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.util.HashMap;
import java.util.HashSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * The view that renders the user-defined keys. If the user pressed outside of the user keys
 * the events are delegated to the activity main surface.
 */
public class KeySurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    public static final int SOURCE_KEYMAP_SINGLEPOINT = -1;
    public static final int SOURCE_KEYMAP_MULTIPOINT = -2;
    private static final String TAG = "KeySurfaceView";
    private static final int KEY_CELL_PADDING = 4;
    private static final long KEY_REPEAT_MS = 100;
    private final Object drawLock = new Object();

    public KeyMapper.KeyMapping[][] mapping;
    public HashMap<Integer, KeyMapper.KeyMapping> pointers = new HashMap<>();
    Paint mPaintKey = new Paint();
    Paint mPaintKeyRepeat = new Paint();
    Paint mPaintKeySelected = new Paint();
    Paint mPaintKeySelectedRepeat = new Paint();
    Paint mPaintKeyEmpty = new Paint();

    Paint mPaintText = new Paint();
    ExecutorService repeaterExecutor = Executors.newFixedThreadPool(1);

    private SurfaceHolder surfaceHolder;
    private KeyMapManager keyMapManager;
    private boolean surfaceCreated;
    private int buttonLayoutHeight;
    private int buttonSize;
    private int minRowLeft = -1;
    private int minRowRight = -1;

    public KeySurfaceView(Context context) {
        super(context);
        setupPaints();
    }

    public KeySurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setupSurfaceHolder();
        setupPaints();
    }

    public KeySurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setupSurfaceHolder();
        setupPaints();
    }

    public KeySurfaceView(Context context, KeyMapManager keyMapManager) {
        super(context);
        setupSurfaceHolder();
        setupPaints();
        this.keyMapManager = keyMapManager;
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        paint(true);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        updateDimensions();
    }

    private void setupPaints() {
        mPaintKey.setColor(Color.parseColor("#7768EFFF"));
        mPaintKeySelected.setColor(Color.parseColor("#7768B0FF"));
        mPaintKeyRepeat.setColor(Color.parseColor("#77FBC69A"));
        mPaintKeySelectedRepeat.setColor(Color.parseColor("#77FF7A6F"));
        mPaintKeyEmpty.setColor(Color.parseColor("#00FFFFFF"));

        mPaintText.setColor(Color.BLACK);
        mPaintText.setTextSize(36);
        mPaintText.setFakeBoldText(true);
        mPaintText.setTextAlign(Paint.Align.CENTER);
    }

    private void setupSurfaceHolder() {
        surfaceHolder = getHolder();
        surfaceHolder.addCallback(this);
        surfaceHolder.setFormat(PixelFormat.TRANSPARENT);
        setZOrderOnTop(true);
    }

    public boolean onTouchEvent(MotionEvent event) {
        if (!keyMapManager.isKeyMapperActive())
            return false;
        int action = event.getActionMasked();
        int pointers = 1;
        if (!keyMapManager.isEditMode())
            pointers = event.getPointerCount();

        HashSet<KeyMapper.KeyMapping> addKeys = new HashSet<>();
        HashSet<KeyMapper.KeyMapping> removeKeys = new HashSet<>();

        Integer filterIndex = null;
        if (action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_POINTER_UP)
            filterIndex = (event.getAction() & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;

        for (int i = 0; i < pointers; i++) {
            if (filterIndex != null && filterIndex != i)
                continue;
            int col = getColumnFromEvent(event, i);
            int row = getRowFromEvent(event, i);
            int pointerId = event.getPointerId(i);

            KeyMapper.KeyMapping prevKeyMapping = null;
            if (this.pointers.containsKey(pointerId))
                prevKeyMapping = this.pointers.get(pointerId);

            // XXX: if it originated from outside of the visible keys
            // don't process but delegate to the external surface view
            boolean keyPressed = isKeyPressed(row, col, action, prevKeyMapping);

            if (keyPressed) {
                processKeyPressed(pointerId, row, col, action, prevKeyMapping);
            } else {
                // delegate to the external surfaceview
                cancelKeyPressed(pointerId, prevKeyMapping);
                if (!keyMapManager.isEditMode() && keyMapManager.unhundledTouchEventListener != null) {
                    delegateEvent(event, i);
                }
            }
        }
        paint(true);
        return true;
    }

    private boolean isKeyPressed(int row, int col, int action, KeyMapper.KeyMapping prevKeyMapping) {
        boolean keyPressed = false;
        if ((action == MotionEvent.ACTION_MOVE || action == MotionEvent.ACTION_UP) && prevKeyMapping == null)
            keyPressed = false;
        else if (((col < keyMapManager.keyMapper.cols / 2 && row > minRowLeft)
                || (col >= keyMapManager.keyMapper.cols / 2 && row > minRowRight))
                && row < keyMapManager.keyMapper.rows && row >= 0
                && col < keyMapManager.keyMapper.cols && col >= 0
        )
            keyPressed = true;
        return keyPressed;
    }

    private int getRowFromEvent(MotionEvent event, int i) {
        int row = -1;
        if (event.getY(i) - (getHeight() - buttonLayoutHeight) > 0)
            row = (int) (event.getY(i) - (getHeight() - buttonLayoutHeight)) / buttonSize;
        return row;
    }

    private int getColumnFromEvent(MotionEvent event, int i) {
        int col = -1;
        if (event.getX(i) > this.getWidth() - buttonLayoutHeight) {
            col = (int) (event.getX(i) - (this.getWidth() - buttonLayoutHeight)) / buttonSize + keyMapManager.keyMapper.cols / 2;
        } else if (event.getX(i) < buttonLayoutHeight) {
            col = (int) event.getX(i) / buttonSize;
        }
        return col;
    }

    /**
     * Cancel a Key Pressed
     *
     * @param pointerId  Android MotionEvent PointerId
     * @param keyMapping KeyMapping to be Canceled
     */
    private void cancelKeyPressed(int pointerId, KeyMapper.KeyMapping keyMapping) {
        if (!keyMapManager.isEditMode() && keyMapping != null) {
            sendKeyEvents(keyMapping, false);
            sendMouseEvents(keyMapping, false);
            this.pointers.remove(pointerId);
        }
    }

    /**
     * Processes a key pressed that has been mapped by the user
     *
     * @param pointerId      Android MotionEvent pointerId
     * @param row            Key row
     * @param col            Key column
     * @param action         Android MotionEvent action
     * @param prevKeyMapping Android Previous Key pressed by the same finger, this helps us keep
     *                       track when the pointer moves over to another key
     */
    private void processKeyPressed(int pointerId, int row, int col, int action, KeyMapper.KeyMapping prevKeyMapping) {
        KeyMapper.KeyMapping keyMapping = mapping[row][col];
        if (action == MotionEvent.ACTION_DOWN
                || action == MotionEvent.ACTION_POINTER_DOWN
                || action == MotionEvent.ACTION_MOVE) {
            if (prevKeyMapping != null && this.pointers.get(pointerId) != keyMapping) {
                sendKeyEvents(prevKeyMapping, false);
                sendMouseEvents(prevKeyMapping, false);
                this.pointers.remove(pointerId);
            }
            if (!this.pointers.containsKey(pointerId)) {
                this.pointers.put(pointerId, keyMapping);
                if (keyMapping.repeat) {
                    startRepeater();
                } else {
                    sendKeyEvents(keyMapping, true);
                    sendMouseEvents(keyMapping, true);
                }
            } else if (keyMapManager.isEditMode() && action == MotionEvent.ACTION_DOWN) {
                if (this.pointers.get(pointerId) == keyMapping) {
                    this.pointers.remove(pointerId);
                }
            }
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            if (!keyMapManager.isEditMode() && prevKeyMapping != null) {
                // XXX: if the key is repeating it should send the ACTION_UP when it stops
                // otherwise if it's not repeating we send the action up here
                if (!keyMapping.repeat) {
                    sendKeyEvents(prevKeyMapping, false);
                    sendMouseEvents(prevKeyMapping, false);
                }
                this.pointers.remove(pointerId);
            }
        }
    }


    /**
     * Delegates a MotionEvent to the surfaceView for handling as a mouse movement
     *
     * @param event Original MotionEvent
     * @param index Pointer index from the MotionEvent that needs to be delegated
     */
    private void delegateEvent(MotionEvent event, int index) {
        int nAction = event.getActionMasked();
        if (nAction == MotionEvent.ACTION_POINTER_DOWN)
            nAction = MotionEvent.ACTION_DOWN;
        else if (nAction == MotionEvent.ACTION_POINTER_UP)
            nAction = MotionEvent.ACTION_UP;

        // XXX: we need to generate a new event with 1 pointer and pass it to the external surfaceview
        MotionEvent.PointerProperties pointerProperties = new MotionEvent.PointerProperties();
        event.getPointerProperties(index, pointerProperties);
        MotionEvent.PointerCoords pointerCoordinates = new MotionEvent.PointerCoords();
        event.getPointerCoords(index, pointerCoordinates);
        MotionEvent nEvent = MotionEvent.obtain(event.getDownTime(), event.getEventTime(), nAction, 1,
                new MotionEvent.PointerProperties[]{new MotionEvent.PointerProperties(pointerProperties)},
                new MotionEvent.PointerCoords[]{new MotionEvent.PointerCoords(pointerCoordinates)},
                event.getMetaState(), event.getButtonState(), event.getXPrecision(), event.getYPrecision(),
                event.getDeviceId(), event.getEdgeFlags(), event.getSource(), event.getFlags());
        if (event.getPointerCount() > 1)
            nEvent.setSource(SOURCE_KEYMAP_MULTIPOINT);
        else
            nEvent.setSource(SOURCE_KEYMAP_SINGLEPOINT);
        if(keyMapManager.unhundledTouchEventListener!=null)
            keyMapManager.unhundledTouchEventListener.OnUnhandledTouchEvent(nEvent);
    }

    /**
     * Start the repeater for keys that were selected by the user via the Key Mapper. The executor
     * guarantees only 1 thread will run. The function will process all repeating keys and will
     * exit when there are none left.
     */
    private void startRepeater() {
        repeaterExecutor.submit(new Runnable() {
            @Override
            public void run() {
                try {
                    runRepeater();
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
    }

    private void runRepeater() {
        // if there is at least one key to be repeated we go on
        boolean atLeastOneKey = true;
        while (atLeastOneKey) {
            atLeastOneKey = false;
            for (KeyMapper.KeyMapping keyMapping : pointers.values()) {
                if (!keyMapping.repeat)
                    continue;
                if (keyMapping.keyCodes.size() == 0 && keyMapping.mouseButtons.size() == 0)
                    continue;
                atLeastOneKey = true;
                sendKeyEvents(keyMapping, true);
                sendMouseEvents(keyMapping, true);
                delay(KEY_REPEAT_MS);
                sendKeyEvents(keyMapping, false);
                sendMouseEvents(keyMapping, false);
                delay(KEY_REPEAT_MS);
            }
        }
    }

    private void delay(long keyRepeatMs) {
        try {
            Thread.sleep(KEY_REPEAT_MS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    /**
     * Sends a Mouse Event to the main SDL Interface
     *
     * @param keyMapping KeyMapping containing Mouse Button Events
     * @param down       If true, send Mouse Button events with action down
     */
    private void sendMouseEvents(KeyMapper.KeyMapping keyMapping, boolean down) {
        if (keyMapManager.isEditMode())
            return;
        for (int mouseButton : keyMapping.mouseButtons) {
            keyMapManager.sendMouseEvent(mouseButton, down);
        }
    }

    /**
     * Sends a Key Event to the main SDL Interface
     *
     * @param keyMapping KeyMapping containing Key Events
     * @param down       If true, send Key Events with action down
     */
    private void sendKeyEvents(KeyMapper.KeyMapping keyMapping, boolean down) {
        if (keyMapManager.isEditMode())
            return;
        for (int keyCode : keyMapping.keyCodes) {
            keyMapManager.sendKeyEvent(keyCode, down);
        }
    }

    public void surfaceCreated(SurfaceHolder arg0) {
        this.surfaceCreated = true;
        updateDimensions();
        paint(false);
    }

    /**
     * Refreshes the Dimension of the KeyMapper Buttons, this should be called when the screen
     * configuration changes or when the layout resize, etc...
     */
    public void updateDimensions() {
        int height = getHeight();
        int width = getWidth();
        buttonLayoutHeight = 0;
        if (height > width)
            buttonLayoutHeight = width / 2;
        else
            buttonLayoutHeight = height / 2;
        if (keyMapManager.keyMapper == null)
            buttonSize = 1;
        else
            buttonSize = buttonLayoutHeight / keyMapManager.keyMapper.rows;

        minRowLeft = -1;
        minRowRight = -1;
        if (!keyMapManager.isEditMode() && keyMapManager.keyMapper != null) {
            minRowLeft = getMinRow(0, keyMapManager.keyMapper.cols / 2);
            minRowRight = getMinRow(keyMapManager.keyMapper.cols / 2, keyMapManager.keyMapper.cols);
        }
    }
    /** Gets the maximum row with all empty cells starting from the top for both left and right key
     * maps. This limit is used when detecting touch events and allows the user more space on the
     * top for trackpad movement
     * */

    /**
     * Gets the maximum row with all empty cells starting from the top for both left and right key
     * maps. This limit is used when detecting touch events and allows the user more space on the
     * top for trackpad movement.
     *
     * @param startCol Starting column
     * @param endCol   End Column
     * @return The maximum row with all empty cells
     */
    private int getMinRow(int startCol, int endCol) {
        int minRow = -1;
        for (int row = 0; row < keyMapManager.keyMapper.rows; row++) {
            int cellsMissing = 0;
            for (int col = startCol; col < endCol; col++) {
                if (keyMapManager.keyMapper.mapping[row][col].keyCodes.size() == 0
                        && keyMapManager.keyMapper.mapping[row][col].mouseButtons.size() == 0) {
                    cellsMissing++;
                } else {
                    break;
                }
            }
            if (cellsMissing == keyMapManager.keyMapper.cols / 2)
                minRow = row;
            else
                break;
        }
        return minRow;
    }

    /**
     * Paints the KeyMapper buttons
     *
     * @param clear Force clear the contents of the SurfaceView
     */
    public synchronized void paint(boolean clear) {
        Canvas canvas = null;
        if (!surfaceCreated || surfaceHolder == null) {
            Log.w(TAG, "Cannot paint surface not ready");
            return;
        }

        try {
            canvas = surfaceHolder.lockCanvas();
            synchronized (drawLock) {
                canvas.drawColor(Color.parseColor("#00FFFFFF"), PorterDuff.Mode.CLEAR);
                if (keyMapManager.keyMapper != null) {
                    for (KeyMapper.KeyMapping keyMapping : pointers.values()) {
                        drawButton(canvas, keyMapping.x, keyMapping.y, true, keyMapping.repeat);
                    }
                    drawButtons(canvas);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (canvas != null) {
                surfaceHolder.unlockCanvasAndPost(canvas);
            }
        }
    }

    private void drawButton(Canvas canvas, int row, int col, boolean isSelected, boolean repeat) {
        int hOffset = col < keyMapManager.keyMapper.cols / 2 ? 0 : (this.getWidth() - buttonLayoutHeight);
        int vOffset = this.getHeight() - buttonLayoutHeight;
        Paint mPaint;
        if (isSelected) {
            mPaint = repeat ? mPaintKeySelectedRepeat : mPaintKeySelected;
        } else {
            mPaint = repeat ? mPaintKeyRepeat : mPaintKey;
        }

        if (mapping[row][col].keyCodes != null && mapping[row][col].mouseButtons != null
                && (mapping[row][col].keyCodes.size() > 0 || mapping[row][col].mouseButtons.size() > 0
                || keyMapManager.isEditMode())) {
            canvas.drawRect((float) (hOffset + col % (keyMapManager.keyMapper.cols / 2) * buttonSize + KEY_CELL_PADDING),
                    (float) (vOffset + row * buttonSize + KEY_CELL_PADDING),
                    (float) (hOffset + (col % (keyMapManager.keyMapper.cols / 2) + 1) * buttonSize - KEY_CELL_PADDING),
                    (float) (vOffset + (row + 1) * buttonSize - KEY_CELL_PADDING),
                    mPaint);
        }
    }

    private void drawButtons(Canvas canvas) {
        for (int row = 0; row < keyMapManager.keyMapper.rows; row++) {
            for (int col = 0; col < keyMapManager.keyMapper.cols; col++) {
                if (!pointers.containsValue(mapping[row][col]) && (keyMapManager.isEditMode()
                        || mapping[row][col].keyCodes.size() > 0
                        || mapping[row][col].mouseButtons.size() > 0)) {
                    drawButton(canvas, row, col, false, mapping[row][col].repeat);
                }
                drawText(canvas, row, col);
            }
        }
    }

    private void drawText(Canvas canvas, int row, int col) {
        int hOffset = col < keyMapManager.keyMapper.cols / 2 ? 0 : (this.getWidth() - buttonLayoutHeight);
        int vOffset = this.getHeight() - buttonLayoutHeight;
        if (mapping[row][col].keyCodes != null && mapping[row][col].mouseButtons != null) {
            if (mapping[row][col].keyCodes.size() > 0 || mapping[row][col].mouseButtons.size() > 0)
                canvas.drawText(mapping[row][col].getText(),
                        (float) (hOffset + (col % (keyMapManager.keyMapper.cols / 2)) * buttonSize + buttonSize / 2),
                        (float) (vOffset + row * buttonSize + buttonSize / 2),
                        mPaintText);
        }
    }

    public void setKeyCode(KeyEvent event) {
        KeyEvent e = new KeyEvent(event.getAction(), event.getKeyCode());
        for (KeyMapper.KeyMapping keyMapping : pointers.values()) {

            if (keyMapping.keyCodes.contains(KeyEvent.KEYCODE_SHIFT_LEFT)
                    || keyMapping.keyCodes.contains(KeyEvent.KEYCODE_SHIFT_RIGHT)
            ) {
                keyMapping.addKeyCode(event.getKeyCode(), e.getUnicodeChar(KeyEvent.META_SHIFT_ON));
            } else
                keyMapping.addKeyCode(event.getKeyCode(), e.getUnicodeChar());
            break;
        }
        keyMapManager.saveKeyMappers();
    }

    public void setMouseButton(int button) {
        for (KeyMapper.KeyMapping keyMapping : pointers.values()) {
            keyMapping.addMouseButton(button);
            break;
        }
        keyMapManager.saveKeyMappers();
    }
}
