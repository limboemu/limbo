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
import android.view.KeyEvent;

import com.max2idea.android.limbo.main.Config;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Iterator;

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

        public final static int maxKeysButtons = 4;
        private static final long serialVersionUID = 7937229682428314497L;

        public ArrayList<Integer> keyCodes = new ArrayList<>();
        public ArrayList<Integer> mouseButtons = new ArrayList<>();
        public ArrayList<Integer> unicodeChars = new ArrayList<>();
        public int x, y;
        public boolean repeat;

        public KeyMapping(int x, int y) {
            this.x = x;
            this.y = y;
        }

        /**
         * Get the string representation of the KeyMapping to be used as a Button Label
         * on the SurfaceView
         *
         * @return The String describing a short description of the Key and Mouse Button events
         * defined by the user
         */
        public String getText() {
            StringBuilder stringBuilder = new StringBuilder("");
            if (keyCodes != null) {
                Iterator<Integer> iter = keyCodes.iterator();
                Iterator<Integer> iterChar = unicodeChars.iterator();
                while (iter.hasNext()) {
                    int keyCode = iter.next();
                    int unicodeChar = iterChar.next();
                    if (keyCode == KeyEvent.KEYCODE_SHIFT_LEFT
                            || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT)
                        continue;
                    if (!stringBuilder.toString().equals(""))
                        stringBuilder.append("+");
                    stringBuilder.append(translateCode(keyCode, unicodeChar));
                }
            }
            if (mouseButtons != null) {
                for (int button : mouseButtons) {
                    if (!stringBuilder.toString().equals(""))
                        stringBuilder.append("+");
                    if (button == Config.SDL_MOUSE_LEFT)
                        stringBuilder.append("MBL");
                    else if (button == Config.SDL_MOUSE_MIDDLE)
                        stringBuilder.append("MBM");
                    else if (button == Config.SDL_MOUSE_RIGHT)
                        stringBuilder.append("MBR");
                }
            }
            return stringBuilder.toString();
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
                text = "ESC";
            else if (keycode == KeyEvent.KEYCODE_ENTER)
                text = ((char) 0x23CE) + "";
            else if (keycode == KeyEvent.KEYCODE_TAB)
                text = "TAB";
            else if (keycode == KeyEvent.KEYCODE_CTRL_LEFT)
                text = "Ctrl";
            else if (keycode == KeyEvent.KEYCODE_CTRL_RIGHT)
                text = "Ctrl";
            else if (keycode == KeyEvent.KEYCODE_ALT_LEFT)
                text = "Alt";
            else if (keycode == KeyEvent.KEYCODE_ALT_RIGHT)
                text = "Alt";
            else if (keycode == KeyEvent.KEYCODE_DEL)
                text = "BKS";
            else if (keycode == KeyEvent.KEYCODE_FORWARD_DEL)
                text = "DEL";
            else if (keycode == KeyEvent.KEYCODE_INSERT)
                text = "INS";
            else if (keycode == KeyEvent.KEYCODE_MOVE_END)
                text = "END";
            else if (keycode == KeyEvent.KEYCODE_SYSRQ)
                text = "SYRQ";
            else if (keycode == KeyEvent.KEYCODE_MOVE_HOME)
                text = "HOME";
            else if (keycode == KeyEvent.KEYCODE_SPACE)
                text = "SPC";
            else if (keycode == KeyEvent.KEYCODE_PAGE_UP)
                text = "PGUP";
            else if (keycode == KeyEvent.KEYCODE_PAGE_DOWN)
                text = "PGDN";
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
            else if (keycode == KeyEvent.KEYCODE_BREAK)
                text = "BRK";
            else if (keycode == KeyEvent.KEYCODE_SCROLL_LOCK)
                text = "SCRL";
            else if (keycode == KeyEvent.KEYCODE_NUM_LOCK)
                text = "NUML";
            else if (keycode == KeyEvent.KEYCODE_SHIFT_LEFT)
                text = "SHFT";
            else if (keycode == KeyEvent.KEYCODE_SHIFT_RIGHT)
                text = "SHFT";
            else
                return ((char) unicodeChar) + "";
            return text;
        }

        public void addKeyCode(int keyCode, int unicodeChar) {
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
            return keyCodes.size() + mouseButtons.size() < maxKeysButtons;
        }

        public void clear() {
            keyCodes.clear();
            unicodeChars.clear();
            mouseButtons.clear();
        }

        public void toggleRepeat() {
            repeat = !repeat;
        }
    }
}


