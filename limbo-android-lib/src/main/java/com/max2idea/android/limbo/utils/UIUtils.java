package com.max2idea.android.limbo.utils;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.webkit.WebView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Toolbar;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import java.io.IOException;

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

	public static void showFileNotSupported(Activity context){
		UIAlert(context, "Error", "File path is not supported. Make sure you choose a file/directory from your internal storage or external sd card. Root and Download Directories are not supported.");
	}


	public static boolean onKeyboard(Activity activity, boolean toggle) {
		// Prevent crashes from activating mouse when machine is paused
		if (LimboActivity.vmexecutor.paused == 1)
			return !toggle;

		InputMethodManager inputMgr = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);

		//XXX: we need to get the focused view to make this always work
		//inputMgr.toggleSoftInput(0, 0);


		View view = activity.getCurrentFocus();
		if(toggle || !Config.enableToggleKeyboard)
			inputMgr.showSoftInput(view, InputMethodManager.SHOW_FORCED);
		else {

			if (view != null) {
				inputMgr.hideSoftInputFromWindow(view.getWindowToken(), 0);
			}
		}

        return !toggle;
	}

	public static void toastShortTop(final Context activity, final String errStr) {
		toast(activity, errStr, Gravity.TOP | Gravity.CENTER, Toast.LENGTH_SHORT);
	}

	public static void toast(final Context activity, final String errStr, final int gravity, final int length) {
		new Handler(Looper.getMainLooper()).post(new Runnable() {
			@Override
			public void run() {
				Toast toast = Toast.makeText(activity, errStr, length);
				toast.setGravity(gravity, 0, 0);
				toast.show();

			}
		});

	}

    public static void toastShort(final Context activity, final String errStr) {
		toast(activity, errStr, Gravity.CENTER | Gravity.CENTER, Toast.LENGTH_SHORT);

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
		

		UIUtils.toastShortTop(activity, "Press Volume Down for Right Click");


	}

	public static void setupToolBar(Activity activity) {
		
		Toolbar tb = (Toolbar) activity.findViewById(R.id.toolbar);
		activity.setActionBar(tb);

		// Get the ActionBar here to configure the way it behaves.
		final ActionBar ab = activity.getActionBar();
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


    public static boolean isLandscapeOrientation(Activity activity)
    {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point screenSize = new Point();
        display.getSize(screenSize);
        if(screenSize.x < screenSize.y)
            return false;
        return true;
    }


    public static void onHelp(Activity activity) {
        PackageInfo pInfo = null;

        try {
            pInfo = activity.getPackageManager().getPackageInfo(activity.getApplicationContext().getPackageName(),
                    PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
        }

        FileUtils fileutils = new FileUtils();
        try {
            UIAlertHtml(Config.APP_NAME + " v" + pInfo.versionName, fileutils.LoadFile(activity, "HELP", false), activity);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

	public static void UIAlert(Activity activity, String title, String body) {

		AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle(title);
		TextView textView = new TextView(activity);
		textView.setPadding(20,20,20,20);
		textView.setText(body);
		alertDialog.setView(textView);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				return;
			}
		});
		alertDialog.show();
	}

    public static void UIAlertHtml(String title, String html, Activity activity) {

        AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);
        WebView webview = new WebView(activity);
        webview.loadData(html, "text/html", "UTF-8");
        alertDialog.setView(webview);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                return;
            }
        });
        alertDialog.show();
    }

	public static void promptNewVersion(final Activity activity, String version) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("New Version " + version);
		TextView stateView = new TextView(activity);
		stateView.setText("There is a new version available with fixes and new features. Do you want to update?");
		stateView.setPadding(20, 20, 20, 20);
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Get New Version",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {

						// UIUtils.log("Searching...");
						Intent fileIntent = new Intent(Intent.ACTION_VIEW);
						fileIntent.setData(Uri.parse(Config.downloadLink));
						activity.startActivity(fileIntent);
					}
				});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Don't Show Again",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {

						// UIUtils.log("Searching...");
						LimboSettingsManager.setPromptUpdateVersion(activity, false);

					}
				});
		alertDialog.show();

	}

	public static void promptShowLog(final Activity activity) {

		final AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle("Show log");
		TextView stateView = new TextView(activity);
		stateView.setText("Something happened during last run, do you want to see the log?");
		stateView.setPadding(20, 20, 20, 20);
		alertDialog.setView(stateView);

		// alertDialog.setMessage(body);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Yes",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {

						FileUtils.viewLimboLog(activity);
					}
				});
		alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {


					}
				});
		alertDialog.show();

	}


}
