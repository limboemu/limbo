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
package com.max2idea.android.limbo.keyboard;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import com.max2idea.android.limbo.machine.MachineController;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class KeyboardUtils {
    private static final String TAG = "KeyboardUtils";

    public static boolean showKeyboard(Activity activity, boolean toggle, View view) {
        // Prevent crashes from activating mouse when machine is paused
        if ( MachineController.getInstance().isPaused())
            return !toggle;

        InputMethodManager inputMgr = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (toggle || !Config.enableToggleKeyboard) {
            if (view != null) {
                view.requestFocus();
                inputMgr.showSoftInput(view, InputMethodManager.SHOW_FORCED);
            }
        } else {
            if (view != null) {
                inputMgr.hideSoftInputFromWindow(view.getWindowToken(), 0);
            }
        }
        return !toggle;
    }

    public static void hideKeyboard(Activity activity, View view) {
        InputMethodManager inputMgr = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (view != null) {
            inputMgr.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }
}
