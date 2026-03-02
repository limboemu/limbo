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
import android.net.Uri;
import android.os.AsyncTask;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.main.LimboSettingsManager;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.File;

public class MachineExporter extends AsyncTask<Void, Void, Void> {
    private static final String TAG = "MachineExporter";
    public String exportFilePath;
    private Activity activity;
    private String displayName;

    public MachineExporter(Activity activity, String filePath) {
        this.activity = activity;
        this.exportFilePath = filePath;
    }

    public static void promptExport(final Activity activity) {

        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.ExportFilename));

        LinearLayout mLayout = new LinearLayout(activity);
        mLayout.setPadding(20, 20, 20, 20);
        mLayout.setOrientation(LinearLayout.VERTICAL);

        final EditText exportNameView = new EditText(activity);
        exportNameView.setEnabled(true);
        exportNameView.setVisibility(View.VISIBLE);
        exportNameView.setSingleLine();
        LinearLayout.LayoutParams imageNameViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(exportNameView, imageNameViewParams);

        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Export),
                (DialogInterface.OnClickListener) null);
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.ChangeDirectory),
                (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button positiveButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        positiveButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                if (LimboSettingsManager.getExportDir(activity) == null) {
                    changeExportDir(activity);
                    return;
                }

                String exportFilename = exportNameView.getText().toString();
                if (exportFilename.trim().equals(""))
                    ToastUtils.toastShort(activity, activity.getString(R.string.ExportFilenameCannotBeEmpty));
                else {

                    if (!exportFilename.endsWith(".csv"))
                        exportFilename += ".csv";

                    exportMachines(activity, exportFilename);
                    alertDialog.dismiss();
                }
            }
        });

        Button negativeButton = alertDialog.getButton(AlertDialog.BUTTON_NEGATIVE);
        negativeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                changeExportDir(activity);
            }
        });

    }

    public static void exportMachines(Activity activity, String filePath) {
        MachineExporter exporter = new MachineExporter(activity, filePath);
        exporter.execute();
    }

    public static void changeExportDir(Activity activity) {
        ToastUtils.toastLong(activity, activity.getString(R.string.ChooseDirForVMExport));
        LimboFileManager.browse(activity, FileType.EXPORT_DIR, Config.OPEN_EXPORT_DIR_REQUEST_CODE);
    }

    public static String exportMachinesFile(Activity activity, String destDir, String destFile) {
        String filePath = null;
        File destFileF = new File(destDir, destFile);

        try {
            String machinesToExport = MachineOpenHelper.getInstance().exportMachines();
            FileUtils.saveFileContents(destFileF.getAbsolutePath(), machinesToExport);
            filePath = destFileF.getAbsolutePath();
        } catch (Exception ex) {
            ToastUtils.toastShort(activity, activity.getString(R.string.FailedToExportFile)
                    + ": " + destFileF.getAbsolutePath() + ", " + activity.getString(R.string.Error) + ":" + ex.getMessage());
        }
        return filePath;
    }

    @Override
    protected Void doInBackground(Void... arg0) {
        displayName = exportMachinesToFile(exportFilePath);
        return null;
    }

    @Override
    protected void onPostExecute(Void test) {
        if (displayName != null)
            ToastUtils.toastLong(activity, activity.getString(R.string.MachinesExported) + ": " + displayName);
    }

    protected String exportMachinesToFile(String exportFileName) {

        String exportDir = LimboSettingsManager.getExportDir(activity);
        String displayName = null;
        if (exportDir.startsWith("content://")) {
            Uri exportDirUri = Uri.parse(exportDir);
            String machinesToExport = MachineOpenHelper.getInstance().exportMachines();
            byte [] contents = machinesToExport.getBytes();
            Uri fileCreatedUri = FileUtils.exportFileContents(activity, exportDirUri, exportFileName, contents);
            displayName = FileUtils.getFullPathFromDocumentFilePath(fileCreatedUri.toString());
        } else {
            String filePath = exportMachinesFile(activity, exportDir, exportFileName);
            displayName = filePath;
        }
        return displayName;
    }
}
