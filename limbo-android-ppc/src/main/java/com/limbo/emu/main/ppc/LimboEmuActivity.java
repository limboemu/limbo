
package com.limbo.emu.main.ppc;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.utils.LinksManager;
import com.max2idea.android.limbo.utils.Machine;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle){
        Config.enable_PPC = true;
        Config.enable_PPC64 = true;

        Config.enable_KVM = false;

        //XXX; only for 64bit hosts, also make sure you have qemu 3.1.0 ppc64-softmmu and above compiled
        if(Machine.isHost64Bit() && Config.enableMTTCG)
            Config.enableMTTCG = true;
        else
            Config.enableMTTCG = false;

        Config.enableEmulatedSDCard = false;

        Config.hd_if_type = "scsi";
        Config.machineFolder = Config.machineFolder + "other/ppc_machines/";

        Config.osImages.put("Debian PowerPC Linux", new LinksManager.LinkInfo("Debian PowerPC Linux",
                "A Linux-based light weight OS with Desktop Manager, network, and package manager",
                "https://github.com/limboemu/limbo/wiki/Debian-PowerPC-Linux",
                LinksManager.LinkType.ISO));

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
