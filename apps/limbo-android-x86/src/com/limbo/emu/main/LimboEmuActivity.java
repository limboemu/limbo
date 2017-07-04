package com.limbo.emu.main;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle) {
		Config.enable_X86 = true;
		Config.enable_X86_64 = true;
		
		Config.osImages.put("DSL Linux", "http://limboemulator.weebly.com/dsl-linux.html");
		Config.osImages.put("Debian Linux", "http://limboemulator.weebly.com/debian-linux.html");
		Config.osImages.put("Trinux", "http://limboemulator.weebly.com/trinux.html");
		Config.osImages.put("FreeDOS", "http://limboemulator.weebly.com/freedos.html");
		Config.osImages.put("KolibriOS", "http://limboemulator.weebly.com/kolibrios.html");
		super.onCreate(bundle);
	}
}
