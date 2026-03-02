
package com.limbo.emu.main.ppc;

import android.os.Bundle;

import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.links.LinksManager;
import com.max2idea.android.limbo.main.LimboApplication;

public class LimboEmuActivity extends LimboActivity {
    public void onCreate(Bundle bundle){
        LimboApplication.arch = Config.Arch.ppc64;
        Config.clientClass = this.getClass();
        Config.enableKVM = false;
        //XXX; only for 64bit hosts, also make sure you have qemu 3.1.0 ppc64-softmmu and above compiled
        if(LimboApplication.isHost64Bit() && Config.enableMTTCG)
            Config.enableMTTCG = true;
        else
            Config.enableMTTCG = false;
        Config.enableEmulatedSDCard = false;
        Config.machineFolder = Config.machineFolder + "other/ppc_machines/";
        Config.osImages.put(getString(R.string.DebianPowerPCLinux), new LinksManager.LinkInfo(getString(R.string.DebianPowerPCLinux),
                getString(R.string.DebianPowerPCLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Debian-PowerPC-Linux",
                LinksManager.LinkType.ISO));
        super.onCreate(bundle);
        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Logger.setupLogFile("/limbo/limbo-ppc-log.txt");
    }

    protected void loadQEMULib() {
        try {
            System.loadLibrary("qemu-system-ppc");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-ppc64");
        }
    }
}
