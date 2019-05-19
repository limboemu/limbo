package com.max2idea.android.limbo.utils;

import java.util.ArrayList;
import java.util.Iterator;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;

public class OSDialogBox extends Dialog {
    public LinearLayout mLayout;
    private Button mDownload;
    private Button mCancel;

    public OSDialogBox(Context context) {
        super(context);
        setContentView(R.layout.oses_dialog);
        getWidgets();
        this.setTitle("Downloads OSes");
        setupListeners();
        getWidgets();
    }

    @Override
    public void onBackPressed() {
        this.dismiss();
    }

    private void getWidgets() {
        mLayout = (LinearLayout) findViewById(R.id.osimgl);
        mDownload = (Button) findViewById(R.id.download);
        mDownload.setEnabled(true);
        mCancel = (Button) findViewById(R.id.cancel);
    }

    private void setupListeners() {
        mDownload.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Intent intent = new Intent(getContext(), LinksManager.class);
                getContext().startActivity(intent);
            }
        });
        mCancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dismiss();
            }
        });

    }

}
