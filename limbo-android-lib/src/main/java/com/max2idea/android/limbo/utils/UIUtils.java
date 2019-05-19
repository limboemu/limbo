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
import android.graphics.Color;
import android.graphics.Point;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.text.InputType;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.webkit.WebView;
import android.widget.ArrayAdapter;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Toolbar;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Scanner;

public class UIUtils {


    private static final String TAG = "UIUtils";

    public static Spannable formatAndroidLog(String contents) {

        Scanner scanner = null;
        Spannable formattedString = new SpannableString(contents);
        if(contents.length()==0)
            return formattedString;

        try {
            scanner = new Scanner(contents);
            int counter = 0;
            ForegroundColorSpan colorSpan = null;
            while (scanner.hasNextLine()) {
                String line = scanner.nextLine();
                //FIXME: some devices don't have standard format for the log
                if (line.startsWith("E/") || line.contains(" E ")) {
                    colorSpan = new ForegroundColorSpan(Color.rgb(255, 22, 22));
                } else if (line.startsWith("W/") || line.contains(" W ")) {
                    colorSpan = new ForegroundColorSpan(Color.rgb(22, 44, 255));
                } else {
                    colorSpan = null;
                }
                if (colorSpan!= null) {
                    formattedString.setSpan(colorSpan, counter, counter + line.length(),
                            Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
                counter += line.length()+1;
            }

        }catch (Exception ex) {
            Log.e(TAG, "Could not format limbo log: " + ex.getMessage());
        } finally {
            if(scanner!=null) {
                try {
                    scanner.close();
                } catch (Exception ex) {
                    if(Config.debug)
                        ex.printStackTrace();
                }
            }

        }
        return formattedString;
    }


    public static void checkNewVersion(final Activity activity) {

        if (!LimboSettingsManager.getPromptUpdateVersion(activity)) {
            return;
        }

        HttpURLConnection conn;
        InputStream is = null;
        try {
            URL url = new URL(Config.downloadUpdateLink);
            conn = (HttpURLConnection) url.openConnection();

            conn.connect();
            is = conn.getInputStream();
            ByteArrayOutputStream bos = new ByteArrayOutputStream();

            int read = 0;
            byte[] buff = new byte[1024];
            while ((read = is.read(buff)) != -1) {
                bos.write(buff, 0, read);
            }
            byte[] streamData = bos.toByteArray();

            final String versionStr = new String(streamData).trim();
            float version = Float.parseFloat(versionStr);
            String versionName = getVersionName(versionStr);

            PackageInfo pInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(),
                    PackageManager.GET_META_DATA);
            int versionCheck = (int) (version * 100);
            if (versionCheck > pInfo.versionCode) {
                final String finalVersionName = versionName;
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        UIUtils.promptNewVersion(activity, finalVersionName);
                    }
                });
            }
        } catch (Exception ex) {
            Log.e(TAG, "Could not get new version: " + ex.getMessage());
            if(Config.debug)
                ex.printStackTrace();
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {

                    e.printStackTrace();
                }
            }
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {

                    e.printStackTrace();
                }
            }
        }
    }

    private static String getVersionName(String versionStr) {
        String [] versionSegments = versionStr.split("\\.");
        int maj = Integer.parseInt(versionSegments[0]) / 100;
        int min = Integer.parseInt(versionSegments[0]) % 100;
        int mic = 0;
        if(versionSegments.length > 1){
            mic = Integer.parseInt(versionSegments[1]);
        }
        String versionName = maj +"." + min + "." + mic;
        return versionName;
    }


    public static class LimboFileSpinnerAdapter extends ArrayAdapter<String>
    {
        int index = 0;
        public LimboFileSpinnerAdapter(Context context, int layout, ArrayList<String> items, int index)
        {

            super(context, layout, items);
            this.index = index;
        }

        @Override
        public View getDropDownView(int position, View convertView, ViewGroup parent)
        {
            View view = super.getDropDownView(position, convertView, parent);
            if(position >= index) {
                TextView textView = (TextView) view.findViewById(R.id.customSpinnerDropDownItem);
                String textStr = convertFilePath(textView.getText() + "", position);
                textView.setText(textStr);
            }
            return view;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent)
        {
            View view = super.getView(position, convertView, parent);
            if(position >= index) {
                TextView textView = (TextView)view.findViewById(R.id.customSpinnerItem);
                String textStr = convertFilePath(textView.getText() + "", position);
                textView.setText(textStr);
            }

            return view;
        }


        private String convertFilePath(String text, int position)
        {
            try {
                text = FileUtils.getFullPathFromDocumentFilePath(text);
            } catch (Exception ex) {
                if(Config.debug)
                    ex.printStackTrace();
            }

            return text;
        }
    }

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

	public static void toastShortTop(final Activity activity, final String errStr) {
		UIUtils.toast(activity, errStr, Gravity.TOP | Gravity.CENTER, Toast.LENGTH_SHORT);
	}

	public static void toast(final Context context, final String errStr, final int gravity, final int length) {
		new Handler(Looper.getMainLooper()).post(new Runnable() {
			@Override
			public void run() {
			    if(context instanceof Activity && ((Activity) context).isFinishing()) {
                    return ;
			    }
                    Toast toast = Toast.makeText(context, errStr, length);
                    toast.setGravity(gravity, 0, 0);
                    toast.show();

			}
		});

	}

    public static void toastShort(final Context context, final String errStr) {
		toast(context, errStr, Gravity.CENTER | Gravity.CENTER, Toast.LENGTH_SHORT);

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


    public static void onChangeLog(Activity activity) {
        PackageInfo pInfo = null;

        try {
            pInfo = activity.getPackageManager().getPackageInfo(activity.getClass().getPackage().getName(),
                    PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        FileUtils fileutils = new FileUtils();
        try {
            UIUtils.UIAlert(activity,"CHANGELOG", fileutils.LoadFile(activity, "CHANGELOG", false),
                    0, false, "OK", null, null, null, null, null);
        } catch (IOException e) {

            e.printStackTrace();
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


    public static void onHelp(final Activity activity) {
        PackageInfo pInfo = null;

        try {
            pInfo = activity.getPackageManager().getPackageInfo(activity.getApplicationContext().getPackageName(),
                    PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
        }

        FileUtils fileutils = new FileUtils();
        UIAlert(activity, Config.APP_NAME + " v" + pInfo.versionName,
                "Welcome to Limbo Emulator, a port of QEMU for Android devices. " +
                        "Limbo can emulate light weight operating systems by loading and running virtual disks images. " +
                        "\n\nFor Downloads, Guides, Help, ISO, and Virtual Disk images visit the Limbo Wiki. " +
                        "Make sure you always download isos and images only from websites you trust, generally use at your own risk.",
                15,
                false,
                "Go To Wiki", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        UIUtils.openURL(activity, Config.guidesLink);
                    }
                },
                "OK",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {

                    }
                }, null, null
        );
    }

    private static void openURL(Activity activity, String url) {
        try {
            Intent fileIntent = new Intent(Intent.ACTION_VIEW);
            fileIntent.setData(Uri.parse(url));
            activity.startActivity(fileIntent);
        }catch (Exception ex) {
            UIUtils.toastShort(activity, "Could not open url");
        }
    }

    public static void UIAlert(Activity activity, String title, String body) {

		AlertDialog alertDialog;
		alertDialog = new AlertDialog.Builder(activity).create();
		alertDialog.setTitle(title);
		TextView textView = new TextView(activity);
		textView.setPadding(20,20,20,20);
		textView.setText(body);
		ScrollView view = new ScrollView(activity);
		view.addView(textView);
		alertDialog.setView(view);
		alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				return;
			}
		});
		alertDialog.show();
	}

    public static void UIAlert(Activity activity, String title, String body, int textSize, boolean cancelable,
                               String button1title, DialogInterface.OnClickListener button1Listener,
                               String button2title, DialogInterface.OnClickListener button2Listener,
                               String button3title, DialogInterface.OnClickListener button3Listener
                               ) {

        AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);
        alertDialog.setCanceledOnTouchOutside(cancelable);
        TextView textView = new TextView(activity);
        textView.setPadding(20,20,20,20);
        textView.setText(body);
        if(textSize>0)
            textView.setTextSize(textSize);
        textView.setBackgroundColor(Color.WHITE);
        textView.setTextColor(Color.BLACK);
        ScrollView view = new ScrollView(activity);
        view.addView(textView);
        alertDialog.setView(view);
        if(button1title!=null)
            alertDialog.setButton(AlertDialog.BUTTON_POSITIVE, button1title, button1Listener);
        if(button2title!=null)
            alertDialog.setButton(AlertDialog.BUTTON_NEGATIVE, button2title, button2Listener);
        if(button3title!=null)
            alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, button3title, button3Listener);
        alertDialog.show();
    }

    public static void UIAlertLog(final Activity activity, String title, Spannable body) {

        AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);
        TextView textView = new TextView(activity);
        textView.setPadding(20,20,20,20);
        textView.setText(body);
        textView.setBackgroundColor(Color.BLACK);
        textView.setTextSize(12);
        textView.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        textView.setSingleLine(false);
        ScrollView view = new ScrollView(activity);
        view.addView(textView);
        alertDialog.setView(view);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                return;
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Copy To", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                UIUtils.toastShort(activity, "Choose a Directory to save the logfile");
                FileManager.browse(activity, LimboActivity.FileType.LOG_DIR, Config.OPEN_LOG_FILE_DIR_REQUEST_CODE);
                return;
            }
        });
        alertDialog.show();
    }

    public static void UIAlertHtml(String title, String body, Activity activity) {

        AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);

        try {
            WebView webview = new WebView(activity);
            webview.loadData(body, "text/html", "UTF-8");
            alertDialog.setView(webview);
        } catch (Exception ex) {
            TextView textView = new TextView(activity);
            textView.setText(body);
            alertDialog.setView(textView);
        }

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
                        openURL(activity, Config.downloadLink);
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
