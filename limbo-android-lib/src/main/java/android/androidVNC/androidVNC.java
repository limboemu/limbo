/* 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

//
// androidVNC is the Activity for setting VNC server IP and port.
//

package android.androidVNC;

import java.util.ArrayList;

import com.limbo.emu.lib.R;

import android.app.Activity;
import android.app.ActivityManager.MemoryInfo;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;

public class androidVNC extends Activity {
	private EditText ipText;
	private EditText portText;
	private EditText passwordText;
	private Button goButton;
	private TextView repeaterText;
	private RadioGroup groupForceFullScreen;
	private Spinner colorSpinner;
	private Spinner spinnerConnection;
	private ConnectionBean selected;
	private EditText textNickname;
	private EditText textUsername;
	private CheckBox checkboxKeepPassword;
	private CheckBox checkboxLocalCursor;
	private boolean repeaterTextSet;

	@Override
	public void onCreate(Bundle icicle) {

		super.onCreate(icicle);
		setContentView(R.layout.limbo_main);

		ipText = (EditText) findViewById(R.id.textIP);
		portText = (EditText) findViewById(R.id.textPORT);
		passwordText = (EditText) findViewById(R.id.textPASSWORD);
		textNickname = (EditText) findViewById(R.id.textNickname);
		textUsername = (EditText) findViewById(R.id.textUsername);
		goButton = (Button) findViewById(R.id.buttonGO);
		((Button) findViewById(R.id.buttonRepeater))
				.setOnClickListener(new View.OnClickListener() {

					@Override
					public void onClick(View v) {
						showDialog(R.layout.repeater_dialog);
					}
				});
		((Button) findViewById(R.id.buttonImportExport))
				.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						showDialog(R.layout.importexport);
					}
				});
		colorSpinner = (Spinner) findViewById(R.id.colorformat);
		COLORMODEL[] models = COLORMODEL.values();
		ArrayAdapter<COLORMODEL> colorSpinnerAdapter = new ArrayAdapter<COLORMODEL>(
				this, android.R.layout.simple_spinner_item, models);
		groupForceFullScreen = (RadioGroup) findViewById(R.id.groupForceFullScreen);
		checkboxKeepPassword = (CheckBox) findViewById(R.id.checkboxKeepPassword);
		checkboxLocalCursor = (CheckBox) findViewById(R.id.checkboxUseLocalCursor);
		colorSpinner.setAdapter(colorSpinnerAdapter);
		colorSpinner.setSelection(0);
		spinnerConnection = (Spinner) findViewById(R.id.spinnerConnection);
		spinnerConnection
				.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
					@Override
					public void onItemSelected(AdapterView<?> ad, View view,
							int itemIndex, long id) {
						selected = (ConnectionBean) ad.getSelectedItem();
						updateViewFromSelected();
					}

					@Override
					public void onNothingSelected(AdapterView<?> ad) {
						selected = null;
					}
				});
		spinnerConnection
				.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {

					/*
					 * (non-Javadoc)
					 * 
					 * @see android.widget.AdapterView.OnItemLongClickListener#
					 * onItemLongClick(android.widget.AdapterView,
					 * android.view.View, int, long)
					 */
					@Override
					public boolean onItemLongClick(AdapterView<?> arg0,
							View arg1, int arg2, long arg3) {
						spinnerConnection.setSelection(arg2);
						selected = (ConnectionBean) spinnerConnection
								.getItemAtPosition(arg2);
						canvasStart();
						return true;
					}

				});
		repeaterText = (TextView) findViewById(R.id.textRepeaterId);
		goButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				canvasStart();
			}
		});

	}

	protected void onDestroy() {

		super.onDestroy();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreateDialog(int)
	 */
	@Override
	protected Dialog onCreateDialog(int id) {
		return new RepeaterDialog(this);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.androidvncmenu, menu);
		return true;
	}


	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if (item.getItemId() == R.id.itemSaveAsCopy){
			updateSelectedFromView();
			arriveOnPage();
		} else if (item.getItemId() == R.id.itemDeleteConnection || 
					item.getItemId() == R.id.itemOpenDoc) {
			Utils.showDocumentation(this);
		}
		return true;
	}

	private void updateViewFromSelected() {
		if (selected == null)
			return;
		ipText.setText(selected.getAddress());
		portText.setText(Integer.toString(selected.getPort()));
		if (selected.getPassword().length() > 0) {
			passwordText.setText(selected.getPassword());
		}
		groupForceFullScreen
				.check(selected.getForceFull() == BitmapImplHint.AUTO ? R.id.radioForceFullScreenAuto
						: (selected.getForceFull() == BitmapImplHint.FULL ? R.id.radioForceFullScreenOn
								: R.id.radioForceFullScreenOff));
		checkboxLocalCursor.setChecked(selected.getUseLocalCursor());
		textNickname.setText(selected.getNickname());
		COLORMODEL cm = COLORMODEL.valueOf(selected.getColorModel());
		COLORMODEL[] colors = COLORMODEL.values();
		for (int i = 0; i < colors.length; ++i) {
			if (colors[i] == cm) {
				colorSpinner.setSelection(i);
				break;
			}
		}
	}

	/**
	 * Called when changing view to match selected connection or from Repeater
	 * dialog to update the repeater information shown.
	 * 
	 * @param repeaterId
	 *            If null or empty, show text for not using repeater
	 */
	void updateRepeaterInfo(boolean useRepeater, String repeaterId) {
		if (useRepeater) {
			repeaterText.setText(repeaterId);
			repeaterTextSet = true;
		} else {
			repeaterText.setText(getText(R.string.repeater_empty_text));
			repeaterTextSet = false;
		}
	}

	private void updateSelectedFromView() {
		if (selected == null) {
			return;
		}
		selected.setAddress(ipText.getText().toString());
		try {
			selected.setPort(Integer.parseInt(portText.getText().toString()));
		} catch (NumberFormatException nfe) {

		}
		selected.setNickname(textNickname.getText().toString());
		
		selected.setForceFull(groupForceFullScreen.getCheckedRadioButtonId() == R.id.radioForceFullScreenAuto ? BitmapImplHint.AUTO
				: (groupForceFullScreen.getCheckedRadioButtonId() == R.id.radioForceFullScreenOn ? BitmapImplHint.FULL
						: BitmapImplHint.TILE));
		selected.setPassword(passwordText.getText().toString());
		
		selected.setUseLocalCursor(checkboxLocalCursor.isChecked());
		selected.setColorModel(((COLORMODEL) colorSpinner.getSelectedItem())
				.nameString());
	}

	protected void onStart() {
		super.onStart();
		arriveOnPage();
	}

	void arriveOnPage() {
		ArrayList<ConnectionBean> connections = new ArrayList<ConnectionBean>();
		connections.add(0, new ConnectionBean());
		int connectionIndex = 0;
		
		spinnerConnection.setAdapter(new ArrayAdapter<ConnectionBean>(this,
				android.R.layout.simple_spinner_item, connections
						.toArray(new ConnectionBean[connections.size()])));
		spinnerConnection.setSelection(connectionIndex, false);
		selected = connections.get(connectionIndex);
		updateViewFromSelected();
	}

	protected void onStop() {
		super.onStop();
		if (selected == null) {
			return;
		}
		updateSelectedFromView();
		
	}

	

	private void canvasStart() {
		if (selected == null)
			return;
		MemoryInfo info = Utils.getMemoryInfo(this);
		if (info.lowMemory) {
			// Low Memory situation. Prompt.
			Utils.showYesNoPrompt(
					this,
					"Continue?",
					"Android reports low system memory.\nContinue with VNC connection?",
					new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
							vnc();
						}
					}, null);
		} else
			vnc();
	}

	private void vnc() {
		updateSelectedFromView();
		Intent intent = new Intent(this, VncCanvasActivity.class);
		startActivity(intent);
	}

}
