package com.max2idea.android.limbo.utils;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Iterator;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboFileManager;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.Toast;

public class OSDialogBox extends Dialog {
	private static Activity activity;
	private Button mOK;

	public OSDialogBox(Context context, int theme, Activity activity1) {
		super(context, theme);
		activity = activity1;
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND, WindowManager.LayoutParams.FLAG_DIM_BEHIND);
		setContentView(R.layout.oses_dialog);
		this.setTitle("Select");
		getWidgets();
		setupListeners();
		initUI();

	}

	@Override
	public void onBackPressed() {
		this.dismiss();
	}

	public static Spinner mOS;
	public static LinearLayout mCDLayout;
	

	public static boolean userPressedOS = true;
	

	public static String filetype;

	private void getWidgets() {
		mOS = (Spinner) findViewById(R.id.osimgval);
		mCDLayout = (LinearLayout) findViewById(R.id.osimgl);


		mOK = (Button) findViewById(R.id.okButton);
		mOK.setEnabled(false);
	}

	private void setupListeners() {
		mOK.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				String selectedOS = (String) mOS.getSelectedItem();
				String url = null;
				if(Config.osImages.containsKey(selectedOS))
					url = Config.osImages.get(selectedOS);
				browse(url);
				dismiss();
			}
		});
		mOS.setOnItemSelectedListener(new OnItemSelectedListener() {

			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				String cd = (String) ((ArrayAdapter<?>) mOS.getAdapter()).getItem(position);
				if(position >0)
					mOK.setEnabled(true);
				else
					mOK.setEnabled(false);
				
				userPressedOS = true;
			}

			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here
				// Log.v(TAG, "Nothing selected");
			}
		});


	}

	private void initUI() {
		// Set spinners to values from currmachine
		
		populateCDRom("cd");

	}

	// Set CDROM
	private static void populateCDRom(String fileType) {
		userPressedOS = false;
		
		ArrayList<String> arraySpinner = new ArrayList<String>();
		arraySpinner.add("None");
		 Enumeration<String> iter = Config.osImages.keys();
		while(iter.hasMoreElements()){
			arraySpinner.add(iter.nextElement());
		}
		
		ArrayAdapter<String> osAdapter = new ArrayAdapter<String>(activity, R.layout.custom_spinner_item, arraySpinner);
		osAdapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
		mOS.setAdapter(osAdapter);
		mOS.invalidate();
	}


	


	public static void browse(String url) {
		
		Intent i = new Intent(Intent.ACTION_VIEW);
		i.setData(Uri.parse(url));
		activity.startActivity(i);

	}

	public static Intent getFileManIntent() {
		return new Intent(activity, com.max2idea.android.limbo.main.LimboFileManager.class);
	}

}
