package com.limbo.emu.main.mips;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_MIPS = true;
		Config.machinedir = Config.machinedir + "other/mips_machines/";
		super.onCreate(bundle);
	}
}
