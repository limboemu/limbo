package com.max2idea.android.limbo.utils;

import com.limbo.emu.main.R;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.os.Handler;
import android.os.Looper;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
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

	public static void setOrientation(Activity activity) {
		int orientation = LimboSettingsManager.getOrientationSetting(activity);
		switch (orientation) {
		case 0:
			activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
			break;
		case 1:
			activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
			break;
		case 2:
			activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
			break;
		case 3:
			activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
			break;
		case 4:
			activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
			break;
		}
	}

	public static void showHints(Activity activity) {
		// TODO Auto-generated method stub

		// TODO Auto-generated method stub
		Toast toast = Toast.makeText(activity, "Press Volume Down for Right Click", Toast.LENGTH_SHORT);
		toast.setGravity(Gravity.TOP | Gravity.CENTER, 0, 0);
		toast.show();

	}

	public static void setupToolBar(AppCompatActivity activity) {
		// TODO Auto-generated method stub
		Toolbar tb = (Toolbar) activity.findViewById(R.id.toolbar);
		activity.setSupportActionBar(tb);

		// Get the ActionBar here to configure the way it behaves.
		final ActionBar ab = activity.getSupportActionBar();
		ab.setHomeAsUpIndicator(R.drawable.limbo); // set a custom icon for
													// the
													// default home button
		ab.setDisplayShowHomeEnabled(true); // show or hide the default home
											// button
		ab.setDisplayHomeAsUpEnabled(true);
		ab.setDisplayShowCustomEnabled(true); // enable overriding the
												// default
												// toolbar layout
		ab.setDisplayShowTitleEnabled(true); // disable the default title
												// element here (for
												// centered
												// title)
		ab.setTitle(R.string.app_name);
		
		if(!LimboSettingsManager.getAlwaysShowMenuToolbar(activity)){
			ab.hide();
		}
	}

}
