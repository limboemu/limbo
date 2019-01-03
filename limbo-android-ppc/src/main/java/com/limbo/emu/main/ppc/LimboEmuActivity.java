
package com.limbo.emu.main.ppc;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_PPC = true;
        Config.enable_PPC64 = true;

        Config.enable_KVM = false;

        //XXX: ppc 64bit for QEMU 3.1.0 and above is mttcg capable
        Config.enableMTTCG = true;

        Config.hd_if_type = "scsi";
        Config.machineFolder = Config.machineFolder + "other/ppc_machines/";
		
		Config.osImages.put("Debian PowerPC Linux", "http://limboemulator.weebly.com/debian-powerpc-linux.html");
		
		super.onCreate(bundle);

        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Config.logFilePath = Config.cacheDir + "/limbo/limbo-ppc-log.txt";
	}

    protected void loadQEMULib() {

        try {
            System.loadLibrary("qemu-system-ppc");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-ppc64");
        }

    }
}
