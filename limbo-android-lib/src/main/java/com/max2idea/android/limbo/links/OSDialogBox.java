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
package com.max2idea.android.limbo.links;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.links.LinksManager;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

public class OSDialogBox extends Dialog {
    private static final String TAG = "OSDialogBox";

    public LinearLayout mLayout;
    private Button mDownload;
    private Button mCancel;

    public OSDialogBox(Context context) {
        super(context);
        setContentView(R.layout.oses_dialog);
        getWidgets();
        this.setTitle(R.string.DownloadsOSes);
        setupListeners();
        getWidgets();
    }

    @Override
    public void onBackPressed() {
        this.dismiss();
    }

    private void getWidgets() {
        mLayout =  findViewById(R.id.osimgl);
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
