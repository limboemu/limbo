
package com.limbo.emu.main.m68k;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_m68k = true;
		Config.machinedir = Config.machinedir + "other/m68k_machines/";
		super.onCreate(bundle);
	}
}
