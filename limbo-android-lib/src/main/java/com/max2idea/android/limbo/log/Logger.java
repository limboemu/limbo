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
package com.max2idea.android.limbo.log;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.text.InputType;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.dialog.DialogUtils;
import com.max2idea.android.limbo.main.LimboApplication;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.File;
import java.io.IOException;
import java.util.Scanner;

/** Custom logger for android so users can view the log if something went wrong
 *
 */
public class Logger {
    public static final String TAG = "Logger";

    public static Spannable formatAndroidLog(String contents) {
        Scanner scanner = null;
        Spannable formattedString = new SpannableString(contents);
        if (contents.length() == 0)
            return formattedString;

        try {
            scanner = new Scanner(contents);
            int counter = 0;
            ForegroundColorSpan colorSpan;
            while (scanner.hasNextLine()) {
                String line = scanner.nextLine();
                //FIXME: some devices don't have standard format for the log
                if (line.startsWith("E/") || line.contains(" E ")) {
                    colorSpan = new ForegroundColorSpan(Color.parseColor("#F78181"));
                } else if (line.startsWith("W/") || line.contains(" W ")) {
                    colorSpan = new ForegroundColorSpan(Color.parseColor("#81A3F7"));
                } else if (line.startsWith("D/") || line.contains(" D ")) {
                    colorSpan = new ForegroundColorSpan(Color.parseColor("#B1FFD7"));
                } else {
                    colorSpan = null;
                }
                if (colorSpan != null) {
                    formattedString.setSpan(colorSpan, counter, counter + line.length(),
                            Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
                counter += line.length() + 1;
            }

        } catch (Exception ex) {
            Log.e(TAG, "Could not format limbo log: " + ex.getMessage());
        } finally {
            if (scanner != null) {
                try {
                    scanner.close();
                } catch (Exception ex) {
                    if (Config.debug)
                        ex.printStackTrace();
                }
            }

        }
        return formattedString;
    }

    public static void onChangeLog(Activity activity) {
        try {
            DialogUtils.UIAlert(activity, activity.getString(R.string.CHANGELOG), FileUtils.LoadFile(activity, "CHANGELOG", false),
                    0, false, activity.getString(android.R.string.ok), null, null, null, null, null);
        } catch (IOException e) {

            e.printStackTrace();
        }
    }

    public static void promptShowLog(final Activity activity) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.ShowLog));
        TextView stateView = new TextView(activity);
        stateView.setText(R.string.LogWarning);
        stateView.setPadding(20, 20, 20, 20);
        alertDialog.setView(stateView);

        // alertDialog.setMessage(body);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Yes),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        viewLimboLog(activity);
                    }
                });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.Cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                    }
                });
        alertDialog.show();
    }

    public static void UIAlertLog(final Activity activity, String title, Spannable body) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(title);
        TextView textView = new TextView(activity);
        textView.setPadding(20, 20, 20, 20);
        textView.setText(body);
        textView.setBackgroundColor(Color.BLACK);
        textView.setTextSize(12);
        textView.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        textView.setSingleLine(false);
        ScrollView view = new ScrollView(activity);
        view.addView(textView);
        alertDialog.setView(view);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Ok),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                alertDialog.dismiss();
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEUTRAL, activity.getString(R.string.CopyTo),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                ToastUtils.toastShort(activity, activity.getString(R.string.ChooseDirToSaveLogFile));
                LimboFileManager.browse(activity, FileType.LOG_DIR, Config.OPEN_LOG_FILE_DIR_REQUEST_CODE);
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }

    public static void viewLimboLog(final Activity activity) {

        String contents = FileUtils.getFileContents(Config.logFilePath);

        if (contents.length() > 50 * 1024)
            contents = contents.substring(0, 25 * 1024)
                            + "\n.....\n" +
                            contents.substring(contents.length() - 25 * 1024);

        final Spannable contentsFormatted = formatAndroidLog(contents);

        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {

                if (Config.viewLogInternally) {
                    UIAlertLog(activity, activity.getString(R.string.LimboLog), contentsFormatted);
                } else {
                    try {
                        Intent intent = new Intent(Intent.ACTION_EDIT);
                        File file = new File(Config.logFilePath);
                        Uri uri = Uri.fromFile(file);
                        intent.setDataAndType(uri, "text/plain");
                        activity.startActivity(intent);
                    } catch (Exception ex) {
                        ex.printStackTrace();
                        // fallback
                        UIAlertLog(activity, activity.getString(R.string.LimboLog), contentsFormatted);
                    }
                }
            }
        });
    }

    public static void setupLogFile(String filePath) {
        Config.logFilePath = LimboApplication.getInstance().getCacheDir() + filePath;
    }
}
