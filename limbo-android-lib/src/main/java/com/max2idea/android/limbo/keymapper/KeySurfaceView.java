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
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.max2idea.android.limbo.main.Config;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
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
    Paint mPaintTextSmall = new Paint();
    ExecutorService repeaterExecutor = Executors.newFixedThreadPool(1);

    private SurfaceHolder surfaceHolder;
    private KeyMapManager keyMapManager;
    private boolean surfaceCreated;
    private int buttonLayoutHeight;
    private int buttonSize;
    private int minRowLeft = -1;
    private int minRowRight = -1;
    private int vertFontOffset;
    private int horizFontOffset;

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
        mPaintText.setTypeface(Typeface.create(Typeface.MONOSPACE, Typeface.BOLD));

        mPaintTextSmall.setColor(Color.BLACK);
        mPaintTextSmall.setTextSize(24);
        mPaintTextSmall.setFakeBoldText(true);
        mPaintTextSmall.setTypeface(Typeface.create(Typeface.MONOSPACE, Typeface.BOLD));

        //offsets
        Rect result = new Rect();
        mPaintText.getTextBounds("O", 0, 1, result);
        vertFontOffset = result.height();
        horizFontOffset = result.width();
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
                if (keyMapping.isRepeat()) {
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
                if (!keyMapping.isRepeat()) {
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
                if (!keyMapping.isRepeat())
                    continue;
                if (keyMapping.getKeyCodes().size() == 0 && keyMapping.getMouseButtons().size() == 0)
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
        for (int mouseButton : keyMapping.getMouseButtons()) {
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
        for (int keyCode : keyMapping.getKeyCodes()) {
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
                if (keyMapManager.keyMapper.mapping[row][col].getKeyCodes().size() == 0
                        && keyMapManager.keyMapper.mapping[row][col].getMouseButtons().size() == 0) {
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
                        drawButton(canvas, keyMapping.getX(), keyMapping.getY(), true, keyMapping.isRepeat());
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

        if (mapping[row][col].getKeyCodes() != null && mapping[row][col].getMouseButtons() != null
                && (mapping[row][col].getKeyCodes().size() > 0 || mapping[row][col].getMouseButtons().size() > 0
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
                        || mapping[row][col].getKeyCodes().size() > 0
                        || mapping[row][col].getMouseButtons().size() > 0)) {
                    drawButton(canvas, row, col, false, mapping[row][col].isRepeat());
                }
                drawText(canvas, row, col);
            }
        }
    }

    private void drawText(Canvas canvas, int row, int col) {
        int hOffset = col < keyMapManager.keyMapper.cols / 2 ? 0 : (this.getWidth() - buttonLayoutHeight);
        int vOffset = this.getHeight() - buttonLayoutHeight;
        if (mapping[row][col].getKeyCodes() != null && mapping[row][col].getMouseButtons() != null) {
            if (mapping[row][col].getKeyCodes().size() > 0 || mapping[row][col].getMouseButtons().size() > 0) {
                String[] texts = getText(mapping[row][col]);
                int count = 0;
                for(String text : texts) {
                    if(text!=null) {
                        int vFontOffset = getVertFontOffset(count);
                        int hFontOffset = getHorizFontOffset(count)*2;
                        drawText(canvas, text, row, col, hOffset + hFontOffset, vOffset + vFontOffset,
                                count == 0? mPaintText: mPaintTextSmall, getAlignPos(count));
                    }
                    count++;
                }
            }
        }
    }

    private int getHorizFontOffset(int count) {

        if(count == 1 || count == 3) {
            return -horizFontOffset;
        } else if(count == 2 || count == 4) {
            return horizFontOffset;
        }
        return 0;
    }

    private int getVertFontOffset(int count) {

        if (count == 1 || count == 2)
            return -vertFontOffset;
        else if(count == 3 || count == 4)
            return vertFontOffset;
        return 0;
    }

    private Paint.Align getAlignPos(int pos) {
//        switch(pos) {
//            case 0:
//                return Paint.Align.CENTER;
//            case 1:
//            case 3:
//                return Paint.Align.RIGHT;
//            case 2:
//            case 4:
//                return Paint.Align.LEFT;
//        }
        return Paint.Align.CENTER;
    }

    private void drawText(Canvas canvas, String text, int row, int col, int hOffset, int vOffset, Paint paint, Paint.Align align) {
        paint.setTextAlign(align);
        canvas.drawText(text,
                (float) (hOffset + (col % (keyMapManager.keyMapper.cols / 2)) * buttonSize + buttonSize / 2),
                (float) (vOffset + row * buttonSize + buttonSize / 2),
                paint);
    }

    public void setKeyCode(KeyEvent event) {
        KeyEvent e = new KeyEvent(event.getAction(), event.getKeyCode());
        for (KeyMapper.KeyMapping keyMapping : pointers.values()) {
            keyMapping.addKeyCode(event.getKeyCode(), event);
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

    /**
     * Get the string representation of the KeyMapping to be used as a Button Label
     * on the SurfaceView
     *
     * @return The Text Strings providing a short description of the Key and Mouse Button events
     * defined by the user
     * @param keyMapping
     */
    public String[] getText(KeyMapper.KeyMapping keyMapping) {
        // cell center occupies 2 texts so we use maxKeysMouseButtons-1
        LinkedHashSet<String> keys = new LinkedHashSet<>();
        LinkedHashSet<String> btns = new LinkedHashSet<>();
        LinkedHashSet<String> meta = new LinkedHashSet<>();
        if (keyMapping.getKeyCodes() != null) {

            Iterator<Integer> iter = keyMapping.getKeyCodes().iterator();
            Iterator<Integer> iterChar = keyMapping.getUnicodeChars().iterator();
            while (iter.hasNext()) {
                int keyCode = iter.next();
                int unicodeChar = iterChar.next();
                if (KeyMapper.KeyMapping.isKeyMeta(keyCode)) {
                    if(keyMapping.hasModifiableKeys() && (keyCode == KeyEvent.KEYCODE_SHIFT_LEFT
                            || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT)
                    )
                        continue;
                    meta.add(translateCode(keyCode,-1));
                    continue;
                }
                keys.add(translateCode(keyCode, unicodeChar));
            }
        }
        if (keyMapping.getMouseButtons() != null) {
            for (int button : keyMapping.getMouseButtons()) {
                btns.add(traslateMouseBtn(button));
            }
        }

        StringBuilder textCenter = new StringBuilder();
        int centerKeys = 0;
        centerKeys = moveTextToCenter(textCenter, new ArrayList<>(keys), keys, centerKeys);
        centerKeys = moveTextToCenter(textCenter, new ArrayList<>(btns), btns, centerKeys);
        moveTextToCenter(textCenter, new ArrayList<>(meta), meta, centerKeys);

        // now start placing the texts around the square
        String [] texts = new String[KeyMapper.KeyMapping.MAX_KEY_MOUSE_BTNS -1];
        texts[0] = textCenter.toString();
        int count = 1;
        for(String metaText : meta)
            texts[count++] = metaText;
        for(String keyText : keys)
            texts[count++] = "+" + keyText;
        for(String btnText : btns)
            texts[count++] = "+" + btnText;
        return texts;
    }

    private String traslateMouseBtn(int button) {
        switch (button) {
            case Config.SDL_MOUSE_LEFT:
                return "Mbl";
            case Config.SDL_MOUSE_MIDDLE:
                return "Mbm";
            case Config.SDL_MOUSE_RIGHT:
                return "Mbr";
        }
        return null;
    }

    private int moveTextToCenter(StringBuilder textCenter,
                                 ArrayList<String> texts, HashSet<String> src, int centerKeys) {
        for(String text : texts) {
            if(centerKeys >= 2) // only 2 texts in the center
                break;
            if(!textCenter.toString().equals(""))
                textCenter.insert(0, "+");
            textCenter.insert(0, text);
            src.remove(text);
            centerKeys++;
        }
        return centerKeys;
    }

    /**
     * Retrieves the keyboard character from KeyEvents and translates Special Keys to
     * short abbreviations
     *
     * @param keycode     Android KeyEvent keyCode
     * @param unicodeChar Unicode Character if exists
     * @return The string represented by the keyCode
     */
    private String translateCode(int keycode, int unicodeChar) {
        String text;
        if (keycode == KeyEvent.KEYCODE_DPAD_UP)
            text = ((char) 0x21E7) + "";
        else if (keycode == KeyEvent.KEYCODE_DPAD_DOWN)
            text = ((char) 0x21E9) + "";
        else if (keycode == KeyEvent.KEYCODE_DPAD_RIGHT)
            text = ((char) 0x21E8) + "";
        else if (keycode == KeyEvent.KEYCODE_DPAD_LEFT)
            text = ((char) 0x21E6) + "";
        else if (keycode == KeyEvent.KEYCODE_ESCAPE)
            text = "Esc";
        else if (keycode == KeyEvent.KEYCODE_ENTER)
            text = ((char) 0x23CE) + "";
        else if (keycode == KeyEvent.KEYCODE_TAB)
            text = "Tab";
        else if (keycode == KeyEvent.KEYCODE_CTRL_LEFT)
            text = "Ctrl";
        else if (keycode == KeyEvent.KEYCODE_CTRL_RIGHT)
            text = "Ctrl";
        else if (keycode == KeyEvent.KEYCODE_ALT_LEFT)
            text = "Alt";
        else if (keycode == KeyEvent.KEYCODE_ALT_RIGHT)
            text = "Alt";
        else if (keycode == KeyEvent.KEYCODE_SHIFT_LEFT)
            text = "Shft";
        else if (keycode == KeyEvent.KEYCODE_SHIFT_RIGHT)
            text = "Shft";
        else if (keycode == KeyEvent.KEYCODE_DEL)
            text = "Bksp";
        else if (keycode == KeyEvent.KEYCODE_FORWARD_DEL)
            text = "Del";
        else if (keycode == KeyEvent.KEYCODE_INSERT)
            text = "Ins";
        else if (keycode == KeyEvent.KEYCODE_MOVE_END)
            text = "End";
        else if (keycode == KeyEvent.KEYCODE_SYSRQ)
            text = "Syrq";
        else if (keycode == KeyEvent.KEYCODE_MOVE_HOME)
            text = "Home";
        else if (keycode == KeyEvent.KEYCODE_SPACE)
            text = "Spc";
        else if (keycode == KeyEvent.KEYCODE_PAGE_UP)
            text = "PgUp";
        else if (keycode == KeyEvent.KEYCODE_PAGE_DOWN)
            text = "PgDn";
        else if (keycode == KeyEvent.KEYCODE_BREAK)
            text = "Brk";
        else if (keycode == KeyEvent.KEYCODE_SCROLL_LOCK)
            text = "Scrl";
        else if (keycode == KeyEvent.KEYCODE_NUM_LOCK)
            text = "NumL";
        else if (keycode == KeyEvent.KEYCODE_F1)
            text = "F1";
        else if (keycode == KeyEvent.KEYCODE_F2)
            text = "F2";
        else if (keycode == KeyEvent.KEYCODE_F3)
            text = "F3";
        else if (keycode == KeyEvent.KEYCODE_F4)
            text = "F4";
        else if (keycode == KeyEvent.KEYCODE_F5)
            text = "F5";
        else if (keycode == KeyEvent.KEYCODE_F6)
            text = "F6";
        else if (keycode == KeyEvent.KEYCODE_F7)
            text = "F7";
        else if (keycode == KeyEvent.KEYCODE_F8)
            text = "F8";
        else if (keycode == KeyEvent.KEYCODE_F9)
            text = "F9";
        else if (keycode == KeyEvent.KEYCODE_F10)
            text = "F10";
        else if (keycode == KeyEvent.KEYCODE_F11)
            text = "F11";
        else if (keycode == KeyEvent.KEYCODE_F12)
            text = "F12";
        else
            return ((char) unicodeChar) + "";
        return text;
    }

}
