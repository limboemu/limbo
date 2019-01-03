package com.limbo.emu.main.arm;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.utils.Machine;

public class LimboEmuActivity extends LimboActivity {

	public void onCreate(Bundle bundle){
		Config.enable_ARM = true;
        Config.enable_ARM64 = true;

		Config.enable_KVM = true;

		Config.enableEmulatedFloppy = false;
		Config.enableEmulatedSDCard = true;

        //XXX; only for 64bit hosts, also make sure you have qemu 3.1.0 and above compiled
        if(Machine.isHost64Bit())
            Config.enableMTTCG = true;

		Config.machineFolder = Config.machineFolder + "other/arm_machines/";
		
		Config.osImages.put("Debian ARM Linux", "http://limboemulator.weebly.com/debian-arm-linux.html");

        Config.hd_if_type = "scsi";

        super.onCreate(bundle);

        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Config.logFilePath = Config.cacheDir + "/limbo/limbo-arm-log.txt";
	}

	protected void loadQEMULib(){

        try {
            System.loadLibrary("qemu-system-arm");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-aarch64");
        }

    }
}
