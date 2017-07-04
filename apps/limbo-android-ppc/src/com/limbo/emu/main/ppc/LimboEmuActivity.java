
package com.limbo.emu.main.ppc;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_PPC = true;
		Config.machinedir = Config.machinedir + "other/ppc_machines/";
		
		Config.osImages.put("Debian PowerPC Linux", "http://limboemulator.weebly.com/debian-powerpc-linux.html");
		
		super.onCreate(bundle);
	}
}
