
package com.limbo.emu.main.sparc;

import android.os.Bundle;

import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.links.LinksManager;
import com.max2idea.android.limbo.main.LimboApplication;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {
        //XXX: unfortunately to enable the 32 bit version we need
        //  to have the qemu-system-sparc.so library compiled
        //  you can build the 32bit version with BUILD_GUEST=sparc64-softmmu from the ndk side

        LimboApplication.arch = Config.Arch.sparc;
        Config.clientClass = this.getClass();
        Config.enableKVM = false;
        //XXX: sparc emulators are not mttcg capable (yet)
        Config.enableMTTCG = false;
        Config.enableSDLSound = false;
        Config.enableEmulatedSDCard = false;
        Config.machineFolder = Config.machineFolder + "other/sparc_machines/";
        Config.osImages.put(getString(R.string.DebianSparcLinux), new LinksManager.LinkInfo(getString(R.string.DebianSparcLinux),
                getString(R.string.DebianSparcLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Debian-Sparc-Linux",
                LinksManager.LinkType.ISO));
        super.onCreate(bundle);
        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Logger.setupLogFile("/limbo/limbo-sparc-log.txt");
    }

    protected void loadQEMULib() {
        try {
            System.loadLibrary("qemu-system-sparc");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-sparc64");
        }
    }

}
