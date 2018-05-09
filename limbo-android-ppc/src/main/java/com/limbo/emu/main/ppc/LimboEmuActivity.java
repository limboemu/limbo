
package com.limbo.emu.main.ppc;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_PPC = true;
        Config.enable_PPC64 = true;
        Config.enable_KVM = false;
		//Config.enableMTTCG = true;
        Config.hd_if_type = "scsi";
        Config.logFilePath = Config.storagedir + "/limbo/limbo-ppc-log.txt";
        Config.machineFolder = Config.machineFolder + "other/ppc_machines/";
		
		Config.osImages.put("Debian PowerPC Linux", "http://limboemulator.weebly.com/debian-powerpc-linux.html");
		
		super.onCreate(bundle);
	}

    protected void loadQEMULib() {

        try {
            System.loadLibrary("qemu-system-ppc");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-ppc64");
        }

    }
}
