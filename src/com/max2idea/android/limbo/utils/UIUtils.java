package com.max2idea.android.limbo.utils;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.widget.Toast;

public class UIUtils {

	public static void toastLong(final Context activity, final String errStr) {
		new Handler(Looper.getMainLooper()).post(new Runnable() {
			@Override
			public void run() {

				Toast toast = Toast.makeText(activity, errStr, Toast.LENGTH_LONG);
				toast.setGravity(Gravity.CENTER | Gravity.CENTER, 0, 0);
				toast.show();

			}
		});

	}
}
