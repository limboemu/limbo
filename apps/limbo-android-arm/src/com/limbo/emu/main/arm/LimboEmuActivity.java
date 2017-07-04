package com.limbo.emu.main.arm;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_ARM = true;
		Config.machinedir = Config.machinedir + "other/arm_machines/";
		
		Config.osImages.put("Debian ARM Linux", "http://limboemulator.weebly.com/debian-arm-linux.html");
		
		super.onCreate(bundle);
	}
}
