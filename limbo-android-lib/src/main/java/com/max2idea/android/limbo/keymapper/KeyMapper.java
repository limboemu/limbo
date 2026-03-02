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

import android.view.KeyEvent;

import java.io.Serializable;
import java.util.ArrayList;

/** Definition of the Key mapper. It supports a 2d set of keys where col = 2 * rows. The keys are
 * then split to 2 sets aligned to the left and right of the screen.
 *
 */
public class KeyMapper implements Serializable {
    private static final String TAG = "KeyMapper";
    private static final long serialVersionUID = 9114128818551070254L;

    public String name;
    public KeyMapping[][] mapping;
    public int rows;
    public int cols;

    public KeyMapper(String name, int rows, int cols) {
        this.name = name;
        this.rows = rows;
        this.cols = cols;

        mapping = new KeyMapping[rows][];
        init();
    }

    private void init() {
        for (int row = 0; row < rows; row++) {
            mapping[row] = new KeyMapping[cols];
            for (int col = 0; col < cols; col++) {
                mapping[row][col] = new KeyMapping(row, col);
            }
        }
    }

    public static class KeyMapping implements Serializable {

        public final static int MAX_KEY_MOUSE_BTNS = 6;
        private static final long serialVersionUID = 7937229682428314497L;

        private ArrayList<Integer> keyCodes = new ArrayList<>();
        private ArrayList<Integer> mouseButtons = new ArrayList<>();
        private ArrayList<Integer> unicodeChars = new ArrayList<>();
        private int x, y;
        private boolean repeat;
        private boolean modifiableKeys;

        public KeyMapping(int x, int y) {
            this.x = x;
            this.y = y;
        }


        public static boolean isKeyMeta(int keyCode) {
            return keyCode == KeyEvent.KEYCODE_CTRL_LEFT
                    || keyCode == KeyEvent.KEYCODE_CTRL_RIGHT
                    || keyCode == KeyEvent.KEYCODE_ALT_LEFT
                    || keyCode == KeyEvent.KEYCODE_ALT_RIGHT
                    || keyCode == KeyEvent.KEYCODE_SHIFT_LEFT
                    || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT;
        }

        public void addKeyCode(int keyCode, KeyEvent event) {
            int unicodeChar = -1;
            if(event!=null) {
                unicodeChar = event.getUnicodeChar();
                if (keyCodes.contains(KeyEvent.KEYCODE_SHIFT_LEFT)
                        || keyCodes.contains(KeyEvent.KEYCODE_SHIFT_RIGHT)
                ) {
                    int newUnicodeChar = event.getUnicodeChar(KeyEvent.META_SHIFT_ON);
                    if (newUnicodeChar != event.getUnicodeChar()) {
                        modifiableKeys = true;
                        unicodeChar = newUnicodeChar;
                    }
                }
            }
            if (!availKeysButtons())
                return;
            if (keyCodes.contains(keyCode))
                return;
            keyCodes.add(keyCode);
            unicodeChars.add(unicodeChar);
        }

        public void addMouseButton(int button) {
            if (!availKeysButtons())
                return;
            if (mouseButtons.contains(button))
                return;
            mouseButtons.add(button);
        }

        private boolean availKeysButtons() {
            return keyCodes.size() + mouseButtons.size() < MAX_KEY_MOUSE_BTNS;
        }

        public void clear() {
            keyCodes.clear();
            unicodeChars.clear();
            mouseButtons.clear();
            modifiableKeys = false;
        }

        public void toggleRepeat() {
            repeat = !repeat;
        }

        public boolean hasModifiableKeys() {
            return modifiableKeys;
        }

        public ArrayList<Integer> getKeyCodes() {
            return new ArrayList<>(keyCodes);
        }

        public boolean isRepeat() {
            return repeat;
        }

        public ArrayList<Integer> getMouseButtons() {
            return new ArrayList<>(mouseButtons);
        }

        public int getX() {
            return x;
        }

        public int getY() {
            return y;
        }

        public ArrayList<Integer>  getUnicodeChars() {
            return new ArrayList<>(unicodeChars);
        }
    }
}


