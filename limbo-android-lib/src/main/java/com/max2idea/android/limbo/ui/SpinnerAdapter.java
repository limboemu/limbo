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
package com.max2idea.android.limbo.ui;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;

import java.util.ArrayList;

/** Custom SpinnerAdapter that shows alternative display values than the ones it was originally
 *  initialized with. The alternative values that are displayed are a compact version of the file
 *  uris that are retrieved by the Android Storage Framework.
 */
public class SpinnerAdapter extends ArrayAdapter<String> {
    private static final String TAG = "SpinnerAdapter";

    int index;
    public SpinnerAdapter(Context context, int layout, ArrayList<String> items, int index) {
        super(context, layout, items);
        this.index = index;
    }

    public static int getItemPosition(Spinner spinner, String value) {
        for(int i =0; i<spinner.getCount(); i++) {
            String item = (String) spinner.getItemAtPosition(i);
            if(item.equals(value))
                return i;
        }
        return -1;
    }

    public static void addItem(Spinner spinner, String value) {
        ((ArrayAdapter<String>) spinner.getAdapter()).add(value);
    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        View view = super.getDropDownView(position, convertView, parent);
        if (position >= index) {
            TextView textView = (TextView) view.findViewById(R.id.customSpinnerDropDownItem);
            String textStr = FileUtils.convertFilePath(textView.getText() + "", position);
            textView.setText(textStr);
        }
        return view;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        if (position >= index) {
            TextView textView = (TextView) view.findViewById(R.id.customSpinnerItem);
            String textStr = FileUtils.convertFilePath(textView.getText() + "", position);
            textView.setText(textStr);
        }
        return view;
    }

    public static int getPositionFromSpinner(Spinner spinner, String value) {
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).equals(value))
                return i;
        }
        return -1;
    }

    public static void setDiskAdapterValue(final Spinner spinner, final String value) {
        spinner.post(new Runnable() {
            public void run() {
                if (value != null) {
                    int pos = SpinnerAdapter.getPositionFromSpinner(spinner, value);
                    spinner.setSelection(Math.max(pos, 0));
                } else {
                    spinner.setSelection(0);
                }
            }
        });
    }

}
