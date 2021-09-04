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
package com.max2idea.android.limbo.install;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileInstaller;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.InputStream;
import java.util.ArrayList;

public class Installer extends AsyncTask<Void, Void, Void> {
    private static final String TAG = "Installer";

    private boolean force;
    private Activity activity;
    private ProgressDialog progDialog;

    private Installer(Activity activity, boolean force) {
        this.activity = activity;
        this.force = force;
    }

    public static String[] getAttrs(Context context, int res) {
        StringBuilder stringBuilder = new StringBuilder();
        try {
            InputStream stream = context.getResources().openRawResource(res);
            byte[] buff = new byte[32768];
            int bytesRead = 0;
            while ((bytesRead = stream.read(buff, 0, buff.length)) > 0) {
                stringBuilder.append(new String(buff, 0, bytesRead));
            }
        } catch(Exception ex) {
            ToastUtils.toastShort(context, context.getString(R.string.CouldNotOpenRawFile) +": " + ex);
        }
        String fileContents = stringBuilder.toString();
        return fileContents.split("\\r\\n");
    }

    public static void installFiles(Activity activity, boolean force) {
        Installer installer = new Installer(activity, force);
        installer.force = force;
        installer.execute();
    }

    @Override
    protected Void doInBackground(Void... arg0) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                progDialog = ProgressDialog.show(activity, activity.getString(R.string.PleaseWait),
                        activity.getString(R.string.InstallingBIOS),
                        true);
            }
        });
        FileInstaller.installFiles(activity, force);
        return null;
    }

    @Override
    protected void onPostExecute(Void test) {
        if (progDialog.isShowing())
            progDialog.dismiss();
    }
}
