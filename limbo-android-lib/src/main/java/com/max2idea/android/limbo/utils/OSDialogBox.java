package com.max2idea.android.limbo.utils;

import java.util.ArrayList;
import java.util.Enumeration;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;

public class OSDialogBox extends Dialog {
    public Spinner mOS;
    public LinearLayout mLayout;
    public String filetype;
    private Button mGetDiskImageButton;
    private Button mOk;

    public OSDialogBox(Context context) {
        super(context);

//        getWindow().setFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND, WindowManager.LayoutParams.FLAG_DIM_BEHIND);
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

    private void getWidgets() {
        mOS = (Spinner) findViewById(R.id.osimgval);
        mLayout = (LinearLayout) findViewById(R.id.osimgl);

        mGetDiskImageButton = (Button) findViewById(R.id.getDiskButton);
        mGetDiskImageButton.setEnabled(true);

        mOk = (Button) findViewById(R.id.ok);
    }

    private void setupListeners() {
        mGetDiskImageButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                String selectedOS = (String) mOS.getSelectedItem();
                String url = null;
                if (Config.osImages.containsKey(selectedOS))
                    url = Config.osImages.get(selectedOS);
                goToUrl(url);

            }
        });
        mOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dismiss();
            }
        });
        mOS.setOnItemSelectedListener(new OnItemSelectedListener() {

            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                String cd = (String) ((ArrayAdapter<?>) mOS.getAdapter()).getItem(position);
                if (position > 0)
                    mGetDiskImageButton.setEnabled(true);
                else
                    mGetDiskImageButton.setEnabled(false);
            }

            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
                // Log.v(TAG, "Nothing selected");
            }
        });


    }

    private void initUI() {
        populateSpinner();
    }


    private void populateSpinner() {
        ArrayList<String> arraySpinner = new ArrayList<>();
        arraySpinner.add("Other");
        Enumeration<String> iter = Config.osImages.keys();
        while (iter.hasMoreElements()) {
            arraySpinner.add(iter.nextElement());
        }

        ArrayAdapter<String> osAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, arraySpinner);
        osAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mOS.setAdapter(osAdapter);
        mOS.invalidate();
    }

    public void goToUrl(String url) {

        Intent i = new Intent(Intent.ACTION_VIEW);
        i.setData(Uri.parse(url));
        getContext().startActivity(i);
    }

}
