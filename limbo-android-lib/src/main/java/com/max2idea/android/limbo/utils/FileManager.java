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
import java.util.logging.Logger;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.MimeTypeMap;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import android.provider.DocumentsContract;

/**
 *
 * @author thedoc
 */
public class FileManager extends ListActivity {

	private ArrayList<String> items = null;
	private File currdir = new File(Environment.getExternalStorageDirectory().getAbsolutePath());
	private File file;
	private TextView currentDir;
	private Button select;
	private String fileType;
	private String TAG = "FileManager";

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle icicle) {
		super.onCreate(icicle);

		setContentView(R.layout.directory_list);

		// select.setOnClickListener(new OnClickListener() {
		//
		// public void onClick(View arg0) {
		// selectDir();
		// }
		// });
		currentDir = (TextView) findViewById(R.id.currDir);
		Bundle b = this.getIntent().getExtras();
		String lastDirectory = b.getString("lastDir");
		fileType = b.getString("fileType");
		Log.v(TAG, "File type is " + fileType);
		Log.v(TAG, "Last dir " + lastDirectory);
		if (lastDirectory == null) {
			lastDirectory = Environment.getExternalStorageDirectory().getPath()
			// + "/limbo"
			;
		}
		currdir = new File(lastDirectory);
		if (!currdir.isDirectory()) {
			lastDirectory = Environment.getExternalStorageDirectory().getPath();
			currdir = new File(lastDirectory);
		} else if (!currdir.exists()) {
			currdir.mkdirs();
		}
		// Log.v("**GET** ", b.getString("lastDir"));
		currentDir.setText(currdir.getPath());
		fill(currdir.listFiles());
	}

	private void fill(File[] files) {
		items = new ArrayList<String>();
		items.add(".. (Parent Directory)");

		if (files != null) {
			for (File file1 : files) {
				if (file1 != null) {
					String filename = file1.getName();
					if (filename != null && file1.isFile() && filter(file1)) {
						items.add(filename);
					} else if (filename != null && file1.isDirectory()) {
						items.add(filename);
					}
				}
			}
		}
		Collections.sort(items, comperator);
		ArrayAdapter<String> fileList = new ArrayAdapter<String>(this, R.layout.dir_row, R.id.FILE_NAME, items);
		setListAdapter(fileList);

	}

	private boolean filter(File file1) {
		return ((this.fileType.toLowerCase().startsWith("hd") || this.fileType.toLowerCase().startsWith("sd")) && (file1
				.getPath().toLowerCase().endsWith("img") || file1.getPath().toLowerCase().endsWith("qcow")
				|| file1.getPath().toLowerCase().endsWith("qcow2") || file1.getPath().toLowerCase().endsWith("vmdk")
				|| file1.getPath().toLowerCase().endsWith("vdi") || file1.getPath().toLowerCase().endsWith("cow")
				|| file1.getPath().toLowerCase().endsWith("dmg") || file1.getPath().toLowerCase().endsWith("bochs")
				|| file1.getPath().toLowerCase().endsWith("vpc") || file1.getPath().toLowerCase().endsWith("vhd")
				|| file1.getPath().toLowerCase().endsWith("fs")))
				|| (this.fileType.toLowerCase().startsWith("cd") && (file1.getPath().toLowerCase().endsWith("iso")))
				|| (this.fileType.toLowerCase().startsWith("kernel")
				// && (file1.getPath().toLowerCase().contains("vmlinuz"))
				) || (this.fileType.toLowerCase().startsWith("initrd")
				// && (file1.getPath().toLowerCase().contains("initrd"))
				) || (this.fileType.toLowerCase().startsWith("fd") && (file1.getPath().toLowerCase().endsWith("img") || file1.getPath().toLowerCase().endsWith("ima")));

	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		super.onListItemClick(l, v, position, id);
		// int selectionRowID = (int) l.getSelectedItemId();
		int selectionRowID = (int) id;
		Logger.getLogger("ItemID: " + selectionRowID);
		if (selectionRowID == 0) {
			Logger.getLogger("Top");
			fillWithParent();
		} else {

			file = new File(currdir.getPath() + "/" + items.get(selectionRowID));
			if (file == null) {
				new AlertDialog.Builder(this).setTitle("Access Denied").setMessage("Cannot retrieve directory").show();
			} else if (file.isDirectory()) {
				Logger.getLogger("Directory");
				currdir = file;
				File[] files = file.listFiles();
				if (files != null) {
					currentDir.setText(file.getPath());
					fill(files);
				} else {
					new AlertDialog.Builder(this).setTitle("Access Denied").setMessage("Cannot list directory").show();
				}
			} else {
				Logger.getLogger("File");
				this.selectFile();
				// new
				// AlertDialog.Builder(this).setTitle("Not a
				// Directory").setMessage("That's a file, not a
				// directory").show();
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

	private final int SELECT_DIR = 1;
	private final int CANCEL = 2;

	/* Creates the menu items */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(0, SELECT_DIR, 0, "Select Dir");
		menu.add(0, CANCEL, 0, "Cancel");
		return true;
	}

	/* Handles item selections */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case SELECT_DIR:
			Logger.getLogger("Select Dir: " + currdir);
			selectDir();
			return true;
		case CANCEL:
			Logger.getLogger("Cancel");
			cancel();
			return true;
		}
		return false;
	}

	public Comparator<String> comperator = new Comparator<String>() {

		public int compare(String object1, String object2) {
			return object1.toString().compareToIgnoreCase(object2.toString());
		}
	};

	public void selectDir() {
		Intent data = new Intent();
		data.putExtra("currDir", this.currdir.getPath());
		setResult(Config.FILEMAN_RETURN_CODE, data);
		finish();
	}

	public void selectFile() {
		Intent data = new Intent();
		data.putExtra("currDir", this.currdir.getPath());
		data.putExtra("file", this.file.getPath());
		data.putExtra("fileType", this.fileType);
		setResult(Config.FILEMAN_RETURN_CODE, data);
		finish();
	}

	private void cancel() {
		Intent data = new Intent();
		data.putExtra("currDir", "");
		setResult(Config.FILEMAN_RETURN_CODE, data);
		finish();
	}

	public static void promptSDCardAccess(Activity context, String fileType) {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);

        intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

		intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

		intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);

		intent.setType("*/*");

		context.startActivityForResult(intent, Config.REQUEST_SDCARD_CODE);
	}

	public static void promptDirAccess(Activity context) {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

		intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
		intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

		intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

		intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);

		context.startActivityForResult(intent, Config.REQUEST_SDCARD_DIR_CODE);
	}


	public static String getMimeType(String url) {
		String type = null;
		String extension = MimeTypeMap.getFileExtensionFromUrl(url);
		if (extension != null) {
			type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
		}
		return type;
	}
}
