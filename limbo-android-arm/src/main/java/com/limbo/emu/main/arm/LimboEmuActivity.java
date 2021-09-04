package com.limbo.emu.main.arm;

import android.os.Bundle;

import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.links.LinksManager;
import com.max2idea.android.limbo.main.LimboApplication;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle){
        LimboApplication.arch = Config.Arch.arm64;
        Config.clientClass = this.getClass();
        Config.enableKVM = true;
        Config.enableEmulatedFloppy = false;
        Config.enableEmulatedSDCard = true;
        //XXX; only for 64bit hosts, also make sure you have qemu 2.9.1 arm-softmmu and above compiled
        if(LimboApplication.isHost64Bit() && Config.enableMTTCG)
            Config.enableMTTCG = true;
        else
            Config.enableMTTCG = false;
        Config.machineFolder = Config.machineFolder + "other/arm_machines/";
        Config.osImages.put(getString(R.string.DebianArmLinux), new LinksManager.LinkInfo(getString(R.string.DebianArmLinux),
                getString(R.string.DebianArmLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Debian-ARM-Linux",
                LinksManager.LinkType.ISO));
        Config.ideInterfaceType = "scsi";
        super.onCreate(bundle);
        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Logger.setupLogFile(LimboApplication.getInstance().getCacheDir() + "/limbo/limbo-arm-log.txt");
    }

    protected void loadQEMULib(){

        try {
            System.loadLibrary("qemu-system-arm");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-aarch64");
        }

    }
}
