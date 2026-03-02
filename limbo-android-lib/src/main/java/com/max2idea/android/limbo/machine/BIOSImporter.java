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
package com.max2idea.android.limbo.machine;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.documentfile.provider.DocumentFile;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.install.Installer;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboApplication;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Hashtable;

public class BIOSImporter {
    private static final String TAG = "BIOSImporter";

    public static void promptImportBIOSFile(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.ImportBIOSFile));

        LinearLayout mLayout = new LinearLayout(activity);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setPadding(20, 20, 20, 20);

        TextView imageNameView = new TextView(activity);
        imageNameView.setVisibility(View.VISIBLE);
        imageNameView.setText(activity.getResources().getString(R.string.importBIOSInstructions));

        LinearLayout.LayoutParams searchViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(imageNameView, searchViewParams);
        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        promptForImportBIOSFile(activity);
                    }
                });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.Cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        alertDialog.dismiss();
                    }
                });
        alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }

    private static void promptForImportBIOSFile(final Activity activity) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                LimboFileManager.browse(activity, FileType.IMPORT_BIOS_FILE, Config.OPEN_IMPORT_BIOS_FILE_REQUEST_CODE);
            }
        }).start();

    }

    public static void importBIOSFile(Activity activity, String importFilePath) {
        InputStream stream = null;
        FileOutputStream targetStream = null;
        try {
            stream = FileUtils.getStreamFromFilePath(importFilePath);
            File target = new File(LimboApplication.getBasefileDir(), FileUtils.getFilenameFromPath(importFilePath));
            targetStream = new FileOutputStream(target);
            byte[] buffer = new byte[32768];
            int bytesRead = 0;
            int totalBytes = 0;
            while ((bytesRead = stream.read(buffer, 0, buffer.length)) > 0) {
                targetStream.write(buffer, 0, bytesRead);
                totalBytes += bytesRead;
                if(totalBytes > 100 * 1024 * 1024){
                    throw new Exception("File too large");
                }
            }
            targetStream.flush();
        } catch (Exception ex) {
            ToastUtils.toastShort(activity, ex.getMessage());
            ex.printStackTrace();
        } finally {
            if(stream!=null) {
                try {
                    stream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if(targetStream!=null) {
                try {
                    targetStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

    }

}
