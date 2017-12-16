
package com.limbo.emu.main.sparc;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import android.os.Bundle;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_sparc = true;
		Config.machinedir = Config.machinedir + "other/sparc_machines/";
		
		Config.osImages.put("Debian Sparc Linux", "http://limboemulator.weebly.com/debian-sparc-linux.html");
		
		super.onCreate(bundle);
	}
}
