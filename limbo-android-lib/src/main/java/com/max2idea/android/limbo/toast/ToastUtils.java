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
package com.max2idea.android.limbo.toast;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;

/** ToastUtils offers wrappers for showing toast notifications
 *
 */
public class ToastUtils {
    private static final String TAG = "ToastUtils";

    public static void toastLong(final Context activity, final String msg) {
        toastLong(activity, Gravity.CENTER, msg);
    }

    public static void toastLong(final Context activity, final int gravity, final String msg) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                android.widget.Toast toast = android.widget.Toast.makeText(activity, msg, android.widget.Toast.LENGTH_LONG);
                toast.setGravity(gravity, 0, 0);
                toast.show();
            }
        });
    }

    public static void toastShortTop(final Activity activity, final String msg) {
        toast(activity, msg, Gravity.TOP | Gravity.CENTER, android.widget.Toast.LENGTH_SHORT);
    }

    public static void toast(final Context context, final String msg, final int gravity, final int length) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (context instanceof Activity && ((Activity) context).isFinishing()) {
                    return;
                }
                android.widget.Toast toast = android.widget.Toast.makeText(context, msg, length);
                toast.setGravity(gravity, 0, 0);
                toast.show();
            }
        });
    }

    public static void toastShort(final Context context, final String msg) {
        toast(context, msg, Gravity.CENTER, android.widget.Toast.LENGTH_SHORT);
    }
}
