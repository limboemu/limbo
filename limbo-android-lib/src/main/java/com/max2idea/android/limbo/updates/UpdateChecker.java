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
package com.max2idea.android.limbo.updates;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.TextView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboSettingsManager;
import com.max2idea.android.limbo.network.NetworkUtils;

/** Software Update notifier for checking if a new version is published.
  */
public class UpdateChecker {
    private static final String TAG = "UpdateChecker";

    public static void checkNewVersion(final Activity activity) {
        if (!LimboSettingsManager.getPromptUpdateVersion(activity)) {
            return;
        }

        try {
            byte[] streamData = NetworkUtils.getContentFromUrl(Config.newVersionLink);
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
                        promptNewVersion(activity, finalVersionName);
                    }
                });
            }
        } catch (Exception ex) {
            Log.w(TAG, "Could not get new version: " + ex.getMessage());
            if (Config.debug)
                ex.printStackTrace();
        }
    }

    private static String getVersionName(String versionStr) {
        String[] versionSegments = versionStr.split("\\.");
        int maj = Integer.parseInt(versionSegments[0]) / 100;
        int min = Integer.parseInt(versionSegments[0]) % 100;
        int mic = 0;
        if (versionSegments.length > 1) {
            mic = Integer.parseInt(versionSegments[1]);
        }
        String versionName = maj + "." + min + "." + mic;
        return versionName;
    }

    public static void promptNewVersion(final Activity activity, String version) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.NewVersion) + " " + version);
        TextView stateView = new TextView(activity);
        stateView.setText(R.string.NewVersionWarning);
        stateView.setPadding(20, 20, 20, 20);
        alertDialog.setView(stateView);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.GenNewVersion),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        NetworkUtils.openURL(activity, Config.downloadLink);
                    }
                });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.DoNotShowAgain),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        LimboSettingsManager.setPromptUpdateVersion(activity, false);

                    }
                });
        alertDialog.show();

    }
}
