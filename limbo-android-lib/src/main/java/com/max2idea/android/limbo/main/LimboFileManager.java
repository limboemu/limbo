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
package com.max2idea.android.limbo.main;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.MimeTypeMap;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.dialog.DialogUtils;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

/**
 * Legacy File Manager Activity for older devices and devices that don't support
 * Android Storage Framework. This requires android:requestLegacyExternalStorage="true" in
 * AndroidManifest.xml.
 */
public class LimboFileManager extends ListActivity {

    private static final int REQUEST_WRITE_PERMISSION = 1001;
    private static String TAG = "FileManager";
    private final int SELECT_DIR = 1;
    private final int CREATE_DIR = 2;
    private final int CANCEL = 3;
    public Comparator<File> comparator = new Comparator<File>() {

        public int compare(File object1, File object2) {
            if (object1.getName().startsWith(".."))
                return -1;
            else if (object2.getName().startsWith(".."))
                return 1;
            else if (object1.getName().endsWith("/") && !object2.getName().endsWith("/"))
                return -1;
            else if (!object1.getName().endsWith("/") && object2.getName().endsWith("/"))
                return 1;
            return object1.toString().compareToIgnoreCase(object2.toString());
        }
    };
    private ArrayList<File> items = null;
    private File currdir = new File(Environment.getExternalStorageDirectory().getAbsolutePath());
    private File file;
    private TextView currentDir;
    private Button select;
    private FileType fileType;
    private HashMap<String, String> filter = new HashMap<>();
    private SelectionMode selectionMode = SelectionMode.FILE;

    public static void browse(Activity activity, FileType fileType, int requestCode) {

        String lastDir = getLastDir(activity, fileType);

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            ToastUtils.toastShort(activity, activity.getResources().getString(R.string.sdcardNotMounted));
            return;
        }

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP // device is old
                || LimboSettingsManager.getEnableLegacyFileManager(activity) // app configuration ASF is disallowed
                || fileType == FileType.SHARED_DIR //TODO: allow sd card access for SHARED DIR (called from c-jni and create the readdir() dirent structs)
        ) {
            LimboFileManager.promptLegacyStorageAccess(activity, fileType, requestCode, lastDir);
        } else { // we use Android ASF to open the file (sd card supported)
            try {
                LimboFileManager.promptOpenFileASF(activity, fileType, getASFFileManagerRequestCode(requestCode), lastDir);

            } catch (Exception ex) {
                Log.e(TAG, "Using Legacy File Manager due to exception :" + ex.getMessage());
                //XXX; some device vendors don't have proper Android Storage Framework so we fallback to legacy file manager (sd card not supported)
                LimboFileManager.promptLegacyStorageAccess(activity, fileType, requestCode, lastDir);
            }
        }
    }

    private static String getLastDir(Context context, FileType fileType) {
        if (fileType == FileType.SHARED_DIR) {
            return LimboSettingsManager.getSharedDir(context);
        } else if (fileType == FileType.EXPORT_DIR || fileType == FileType.IMPORT_FILE) {
            return LimboSettingsManager.getExportDir(context);
        } else if (fileType == FileType.IMAGE_DIR) {
            return LimboSettingsManager.getImagesDir(context);
        }
        return LimboSettingsManager.getLastDir(context);
    }

    private static int getASFFileManagerRequestCode(int requestCode) {
        switch (requestCode) {
            case Config.OPEN_IMAGE_FILE_REQUEST_CODE:
                return Config.OPEN_IMAGE_FILE_ASF_REQUEST_CODE;
            case Config.OPEN_IMAGE_DIR_REQUEST_CODE:
                return Config.OPEN_IMAGE_DIR_ASF_REQUEST_CODE;
            case Config.OPEN_SHARED_DIR_REQUEST_CODE:
                return Config.OPEN_SHARED_DIR_ASF_REQUEST_CODE;
            case Config.OPEN_EXPORT_DIR_REQUEST_CODE:
                return Config.OPEN_EXPORT_DIR_ASF_REQUEST_CODE;
            case Config.OPEN_IMPORT_FILE_REQUEST_CODE:
                return Config.OPEN_IMPORT_FILE_ASF_REQUEST_CODE;
            case Config.OPEN_IMPORT_BIOS_FILE_REQUEST_CODE:
                return Config.OPEN_IMPORT_BIOS_FILE_ASF_REQUEST_CODE;
            case Config.OPEN_LOG_FILE_DIR_REQUEST_CODE:
                return Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE;
            default:
                return requestCode;
        }
    }

    public static void promptLegacyStorageAccess(Activity activity, FileType fileType, int requestCode, String lastDir) {

        String dir = null;
        try {

            HashMap<String, String> filterExt = getFileExt(fileType);

            Intent i = null;
            i = getFileManIntent(activity);
            Bundle b = new Bundle();
            if (lastDir != null && !lastDir.startsWith("content://"))
                b.putString("lastDir", lastDir);
            b.putSerializable("fileType", fileType);
            b.putSerializable("filterExt", filterExt);
            i.putExtras(b);
            activity.startActivityForResult(i, requestCode);
        } catch (Exception e) {
            Log.e(TAG, "Error while starting Filemanager: " + e.getMessage());
        }
    }

    public static Intent getFileManIntent(Activity activity) {
        return new Intent(activity, com.max2idea.android.limbo.main.LimboFileManager.class);
    }

    protected static void promptOpenFileASF(Activity context, FileType fileType, int requestCode, String lastDir) {
        Intent intent = null;
        if (isFileTypeDirectory(fileType))
            intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        else
            intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);

        intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

        intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

        intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);

        if (!isFileTypeDirectory(fileType)) {
            String[] fileMimeTypes = getFileMimeTypes(fileType);
            if (fileMimeTypes != null) {
                for (String fileMimeType : fileMimeTypes) {
                    intent.setType(fileMimeType);
                }
            }
        }


        if (lastDir != null && lastDir.startsWith("content://")) {
            Uri uri = Uri.parse(lastDir);
            intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, uri);
        }

        context.startActivityForResult(intent, requestCode);
    }

    private static boolean isFileTypeDirectory(FileType fileType) {
        return (fileType == FileType.SHARED_DIR || fileType == FileType.EXPORT_DIR
                || fileType == FileType.IMAGE_DIR || fileType == FileType.LOG_DIR);

    }

    private static String[] getFileMimeTypes(FileType fileType) {
        if (fileType == FileType.IMPORT_FILE)
            return new String[]{MimeTypeMap.getSingleton().getMimeTypeFromExtension("csv")};
        else
            return new String[]{"*/*"};
    }

    private static HashMap<String, String> getFileExt(FileType fileType) {
        HashMap<String, String> filterExt = new HashMap<>();

        if (fileType == FileType.IMPORT_FILE)
            filterExt.put("csv", "csv");

        return filterExt;
    }

    public static String getMimeType(String url) {
        String type = null;
        String extension = MimeTypeMap.getFileExtensionFromUrl(url);
        if (extension != null) {
            type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
        }
        return type;
    }

    //XXX: for now we dont use filters since file extensions on images is something not so standard
    // and we don't want to hide files from the user
    private boolean filter(File filePath) {
        String ext = FileUtils.getExtensionFromFilename(filePath.getName());
        return (filter == null || filter.isEmpty() || filter.containsKey(ext.toLowerCase()));
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.directory_list);
        select = findViewById(R.id.select_button);
        select.setOnClickListener(new View.OnClickListener() {
            public void onClick(View arg0) {
                selectDir();
            }
        });
        currentDir = findViewById(R.id.currDir);
        Bundle b = this.getIntent().getExtras();
        String lastDirectory = b.getString("lastDir");
        fileType = (FileType) b.getSerializable("fileType");
        filter = (HashMap<String, String>) b.getSerializable("filterExt");

        if (isFileTypeDirectory(fileType))
            selectionMode = SelectionMode.DIRECTORY;
        else {
            selectionMode = SelectionMode.FILE;
            select.setVisibility(View.GONE);
        }

        if (selectionMode == SelectionMode.DIRECTORY)
            setTitle(getString(R.string.SelectADirectory));
        else
            setTitle(getString(R.string.SelectAFile));

        //set starting directory
        if (lastDirectory == null) {
            lastDirectory = Environment.getExternalStorageDirectory().getPath();
        }
        currdir = new File(lastDirectory);
        if (!currdir.isDirectory() || !currdir.exists()) {
            lastDirectory = Environment.getExternalStorageDirectory().getPath();
            currdir = new File(lastDirectory);
        }
        currentDir.setText(currdir.getPath());
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                checkPermissionsAndBrowse();
            }
        }, 500);
    }

    private void checkPermissionsAndBrowse() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                DialogUtils.UIAlert(this, getString(R.string.WriteAccess), getString(R.string.FullAccessWarning), 16, false,
                        getString(R.string.OkIUnderstand), new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {
                                ActivityCompat.requestPermissions(LimboFileManager.this,
                                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                                        REQUEST_WRITE_PERMISSION);
                            }
                        }, null, null, null, null);
            } else {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        REQUEST_WRITE_PERMISSION);
            }
        } else {
            fill(currdir.listFiles());
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_WRITE_PERMISSION: {
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    fill(currdir.listFiles());
                } else {
                    ToastUtils.toastShort(this, getString(R.string.FeaturDisabled));
                    finish();
                }
                return;
            }
        }
    }

    private void fill(File[] files) {


        items = new ArrayList<>();
        items.add(new File(".. (Parent Directory)"));

        if (files != null) {
            for (File file1 : files) {
                if (file1 != null) {
                    if (file1.isFile() && filter(file1)
                    ) {
                        items.add(file1);
                    } else if (file1.isDirectory()) {
                        items.add(file1);
                    }
                }
            }
        }
        Collections.sort(items, comparator);
        FileAdapter fileList = new FileAdapter(this, R.layout.dir_row, items);
        setListAdapter(fileList);

        LimboSettingsManager.setLastDir(this, currdir.getAbsolutePath());
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);
        // int selectionRowID = (int) l.getSelectedItemId();
        int selectionRowID = (int) id;
        if (selectionRowID == 0) {
            fillWithParent();
        } else {

            file = items.get(selectionRowID);
            if (file == null) {
                ToastUtils.toastShort(this, getString(R.string.AccessDeniedCannotRetrieveDirectory));
            } else if (!file.isDirectory() && selectionMode == SelectionMode.DIRECTORY) {
                ToastUtils.toastShort(this, getString(R.string.NotADirectory));
            } else if (file.isDirectory()) {
                currdir = file;
                File[] files = file.listFiles();
                if (files != null) {
                    currentDir.setText(file.getPath());
                    fill(files);
                } else {
                    new AlertDialog.Builder(this).setTitle(R.string.AccessDenied).setMessage(R.string.CannotListDirectory).show();
                }
            } else {
                this.selectFile();
            }

        }
    }

    private void fillWithParent() {
        if (currdir.getPath().equalsIgnoreCase("/")) {
            currentDir.setText(currdir.getPath());
            fill(currdir.listFiles());
        } else {
            currdir = currdir.getParentFile();
            currentDir.setText(currdir.getPath());
            fill(currdir.listFiles());
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(0, SELECT_DIR, 0, "Select Directory");
        menu.add(0, CREATE_DIR, 0, "Create Directory");
        menu.add(0, CANCEL, 0, "Cancel");

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case SELECT_DIR:
                selectDir();
                return true;
            case CREATE_DIR:
                promptCreateDir(this);
                return true;
            case CANCEL:
                cancel();
                return true;
        }
        return false;
    }


    public void promptCreateDir(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(getString(R.string.NewDirectory));
        final EditText dirNameTextview = new EditText(activity);
        dirNameTextview.setPadding(20, 20, 20, 20);
        dirNameTextview.setEnabled(true);
        dirNameTextview.setVisibility(View.VISIBLE);
        dirNameTextview.setSingleLine();
        alertDialog.setView(dirNameTextview);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.Create), (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (dirNameTextview.getText().toString().trim().equals(""))
                    ToastUtils.toastShort(activity, getString(R.string.DirNameCannotBeEmpty));
                else {
                    createDirectory(dirNameTextview.getText().toString());
                    fill(currdir.listFiles());
                    alertDialog.dismiss();
                }
            }
        });
    }

    private void createDirectory(String dirName) {
        File dir = new File(currdir, dirName);
        if (!dir.exists()) {
            dir.mkdirs();
        } else {
            ToastUtils.toastShort(this, getString(R.string.DirectoryAlreadyExists));
        }
    }

    public void selectDir() {
        Intent data = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("currDir", this.currdir.getPath());
        bundle.putSerializable("fileType", fileType);
        data.putExtras(bundle);
        setResult(Config.FILEMAN_RETURN_CODE, data);
        finish();
    }

    public void selectFile() {
        Intent data = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("currDir", this.currdir.getPath());
        bundle.putString("file", this.file.getPath());
        bundle.putSerializable("fileType", fileType);
        data.putExtras(bundle);
        setResult(Config.FILEMAN_RETURN_CODE, data);
        finish();
    }

    private void cancel() {
        Intent data = new Intent();
        data.putExtra("currDir", "");
        setResult(Config.FILEMAN_RETURN_CODE, data);
        finish();
    }


    public enum SelectionMode {
        DIRECTORY,
        FILE
    }

    public static class FileAdapter extends ArrayAdapter<File> {
        private final Context context;
        private final ArrayList<File> files;

        public FileAdapter(Context context, int layout, ArrayList<File> files) {
            super(context, layout, files);
            this.context = context;
            this.files = files;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater inflater = (LayoutInflater) context
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);

            View rowView = inflater.inflate(R.layout.dir_row, parent, false);
            TextView textView = (TextView) rowView.findViewById(R.id.FILE_NAME);
            ImageView imageView = (ImageView) rowView.findViewById(R.id.FILE_ICON);
            textView.setText(files.get(position).getName());

            int iconRes = 0;
            if (files.get(position).getName().startsWith("..") || files.get(position).isDirectory())
                imageView.setImageResource(R.drawable.folder);
            else {
                iconRes = FileUtils.getIconForFile(files.get(position).getName());
                imageView.setImageResource(iconRes);
            }
            return rowView;
        }
    }

}
