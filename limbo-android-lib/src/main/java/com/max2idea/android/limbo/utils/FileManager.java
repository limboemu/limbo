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
package com.max2idea.android.limbo.utils;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.main.LimboSettingsManager;

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
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
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

/**
 * @author thedoc
 */
public class FileManager extends ListActivity {

    private static final int REQUEST_WRITE_PERMISSION = 1001;
    private final int SELECT_DIR = 1;
    private final int CREATE_DIR = 2;
    private final int CANCEL = 3;

    public Comparator<String> comperator = new Comparator<String>() {

        public int compare(String object1, String object2) {
            if (object1.startsWith(".."))
                return -1;
            else if (object2.startsWith(".."))
                return 1;
            else if (object1.endsWith("/") && !object2.endsWith("/"))
                return -1;
            else if (!object1.endsWith("/") && object2.endsWith("/"))
                return 1;
            return object1.toString().compareToIgnoreCase(object2.toString());
        }
    };
    private ArrayList<String> items = null;
    private File currdir = new File(Environment.getExternalStorageDirectory().getAbsolutePath());
    private File file;
    private TextView currentDir;
    private Button select;
    private LimboActivity.FileType fileType;
    private static String TAG = "FileManager";
    private HashMap<String,String> filter = new HashMap<>();

    //XXX: for now we dont use filters since file extensions on images is something not so standard
    // and we don't want to hide files from the user
	private boolean filter(File filePath) {
	    String ext = FileUtils.getExtensionFromFilename(filePath.getName());
		return (filter == null || filter.isEmpty() || filter.containsKey(ext.toLowerCase()));
	}
    private SelectionMode selectionMode = SelectionMode.FILE;

    public static void browse(Activity activity, LimboActivity.FileType fileType, int requestCode) {

        String lastDir = getLastDir(activity, fileType);

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            UIUtils.toastShort(activity.getApplicationContext(), "Error: SD card is not mounted");
            UIUtils.toastShort(activity, "Error: SD card is not mounted");
            return;
        }

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP // device is old
                || LimboSettingsManager.getEnableLegacyFileManager(activity) // app configuration ASF is disallowed
                || fileType == LimboActivity.FileType.SHARED_DIR //TODO: allow sd card access for SHARED DIR (called from c-jni and create the readdir() dirent structs)
                ) {
            LimboFileManager.promptLegacyStorageAccess(activity, fileType, requestCode, lastDir);
        } else { // we use Android ASF to open the file (sd card supported)
            try {
//                if(true)
//                    throw new Exception("Test ASF missing exception");
                LimboFileManager.promptOpenFileASF(activity, fileType, getASFFileManagerRequestCode(requestCode), lastDir);

            } catch (Exception ex) {
                Log.e(TAG, "Using Legacy File Manager due to exception :" + ex.getMessage());
                //XXX; some device vendors don't have proper Android Storage Framework so we fallback to legacy file manager (sd card not supported)
                LimboFileManager.promptLegacyStorageAccess(activity, fileType, requestCode, lastDir);
            }
        }
    }

    private static String getLastDir(Context context, LimboActivity.FileType fileType) {
        if(fileType == LimboActivity.FileType.SHARED_DIR) {
            return LimboSettingsManager.getSharedDir(context);
        } else if(fileType == LimboActivity.FileType.EXPORT_DIR || fileType == LimboActivity.FileType.IMPORT_FILE) {
            return LimboSettingsManager.getExportDir(context);
        } else if(fileType == LimboActivity.FileType.IMAGE_DIR) {
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
            case Config.OPEN_LOG_FILE_DIR_REQUEST_CODE:
                return Config.OPEN_LOG_FILE_DIR_ASF_REQUEST_CODE;
                default:
                return requestCode;
        }
    }

    public static void promptLegacyStorageAccess(Activity activity, LimboActivity.FileType fileType, int requestCode, String lastDir) {

        String dir = null;
        try {

            HashMap<String,String> filterExt = getFileExt(fileType);

            Intent i = null;
            i = getFileManIntent(activity);
            Bundle b = new Bundle();
            if(lastDir!=null && !lastDir.startsWith("content://"))
                b.putString("lastDir", lastDir);
            b.putSerializable("fileType", fileType);
            b.putSerializable("filterExt", filterExt);
            i.putExtras(b);
            activity.startActivityForResult(i, requestCode);

        } catch (Exception e) {
             Log.e(TAG, "Error while starting Filemanager: " +
             e.getMessage());
        }
    }


    public static Intent getFileManIntent(Activity activity) {
        return new Intent(activity, com.max2idea.android.limbo.main.LimboFileManager.class);
    }

    protected static void promptOpenFileASF(Activity context, LimboActivity.FileType fileType, int requestCode, String lastDir) {
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

        //TODO: is there another way to get this object in the result?
//        intent.getExtras().putSerializable("fileType", fileType);

        if (!isFileTypeDirectory(fileType)) {
            String[] fileMimeTypes = getFileMimeTypes(fileType);
            if (fileMimeTypes != null) {
                for (String fileMimeType : fileMimeTypes) {
                    intent.setType(fileMimeType);
                }
            }
        }


        if(lastDir!=null && lastDir.startsWith("content://")) {
            Uri uri = Uri.parse(lastDir);
            intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, uri);
        }

        context.startActivityForResult(intent, requestCode);
    }

    private static boolean isFileTypeDirectory(LimboActivity.FileType fileType) {
        return (fileType == LimboActivity.FileType.SHARED_DIR || fileType == LimboActivity.FileType.EXPORT_DIR
                || fileType == LimboActivity.FileType.IMAGE_DIR || fileType == LimboActivity.FileType.LOG_DIR);

    }

    private static String[] getFileMimeTypes(LimboActivity.FileType fileType) {
        if(fileType == LimboActivity.FileType.IMPORT_FILE)
            return new String[]{MimeTypeMap.getSingleton().getMimeTypeFromExtension("csv")};
        else
            return new String[] {"*/*"};
    }


    private static HashMap<String,String> getFileExt(LimboActivity.FileType fileType) {
        HashMap<String,String> filterExt = new HashMap<>();

        if(fileType == LimboActivity.FileType.IMPORT_FILE)
            filterExt.put("csv","csv");

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

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.directory_list);
        select = (Button) findViewById(R.id.select_button);
        select.setOnClickListener(new View.OnClickListener() {
            public void onClick(View arg0) {
                selectDir();
            }
        });
        currentDir = (TextView) findViewById(R.id.currDir);
        Bundle b = this.getIntent().getExtras();
        String lastDirectory = b.getString("lastDir");
        fileType = (LimboActivity.FileType) b.getSerializable("fileType");
        filter = (HashMap<String,String>) b.getSerializable("filterExt");

        if (isFileTypeDirectory(fileType))
            selectionMode = SelectionMode.DIRECTORY;
        else {
            selectionMode = SelectionMode.FILE;
            select.setVisibility(View.GONE);
        }

        if (selectionMode == SelectionMode.DIRECTORY)
            setTitle("Select a Directory");
        else
            setTitle("Select a File");

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
        },500);

    }

    private void checkPermissionsAndBrowse() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                UIUtils.UIAlert(this, "WRITE ACCESS", "Warning! Providing FULL ACCESS to the write disk is discouraged!\n\n" +
                                "You either request for Shared Access to the local drive or your device doesn't have Android Storage Framework implemented. " +
                                "If you understand the risks press OK to continue", 16, false,
                        "OK, I understand", new DialogInterface.OnClickListener(){

                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {
                                ActivityCompat.requestPermissions(FileManager.this,
                                        new String [] { Manifest.permission.WRITE_EXTERNAL_STORAGE},
                                        REQUEST_WRITE_PERMISSION);
                            }
                        }, null, null, null, null);

            } else {
                ActivityCompat.requestPermissions(this,
                        new String [] { Manifest.permission.WRITE_EXTERNAL_STORAGE},
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
                    UIUtils.toastShort(this, "Feature disabled");
                    finish();
                }
                return;
            }

        }
    }

    private void fill(File[] files) {


        items = new ArrayList<String>();
        items.add(".. (Parent Directory)");

        if (files != null) {
            for (File file1 : files) {
                if (file1 != null) {
                    String filename = file1.getName();
                    if (filename != null && file1.isFile()
                            && filter(file1)
                            ) {
                        items.add(filename);
                    } else if (filename != null && file1.isDirectory()) {
                        items.add(filename + "/");
                    }
                }
            }
        }
        Collections.sort(items, comperator);
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

            file = new File(currdir.getPath() + "/" + items.get(selectionRowID));
            if (file == null) {
                UIUtils.toastShort(this, "Access Denied: cannot retrieve directory");
            } else if (!file.isDirectory() && selectionMode == SelectionMode.DIRECTORY) {
                UIUtils.toastShort(this, "Not a Directory");
            } else if (file.isDirectory()) {
currdir = file;
                File[] files = file.listFiles();
                if (files != null) {
                    currentDir.setText(file.getPath());
                    fill(files);
                } else {
                    new AlertDialog.Builder(this).setTitle("Access Denied").setMessage("Cannot list directory").show();
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
        alertDialog.setTitle("New Directory");
        final EditText dirNameTextview = new EditText(activity);
        dirNameTextview.setPadding(20, 20, 20, 20);
        dirNameTextview.setEnabled(true);
        dirNameTextview.setVisibility(View.VISIBLE);
        dirNameTextview.setSingleLine();
        alertDialog.setView(dirNameTextview);
        alertDialog.setCanceledOnTouchOutside(false);
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Create", (DialogInterface.OnClickListener) null);

        alertDialog.show();

        Button button = alertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (dirNameTextview.getText().toString().trim().equals(""))
                    UIUtils.toastShort(activity, "Directory name cannot be empty");
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
        if(!dir.exists()) {
            dir.mkdirs();
        } else {
            UIUtils.toastShort(this, "Directory already exists!");
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

    public class FileAdapter extends ArrayAdapter<String> {
        private final Context context;
        private final ArrayList<String> files;

        public FileAdapter(Context context, int layout, ArrayList<String> files) {
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
            textView.setText(files.get(position));

            int iconRes = 0;
            if (files.get(position).startsWith("..") || files.get(position).endsWith("/"))
                imageView.setImageResource(R.drawable.folder);
            else {
                iconRes = FileUtils.getIconForFile(files.get(position));
                imageView.setImageResource(iconRes);
            }
            return rowView;
        }
    }

}
