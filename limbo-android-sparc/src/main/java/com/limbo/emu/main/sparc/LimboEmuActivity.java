
package com.limbo.emu.main.sparc;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {

        //XXX: unfortunately to enable the 32 bit version we need
        //  to have the qemu-system-sparc.so library compiled
        //  you can build the 32bit version with BUILD_GUEST=sparc64-softmmu from the ndk side
        Config.enable_sparc = true;
        Config.enable_sparc64 = false;

        Config.enable_KVM = false;

        //XXX: sparc emulators are not mttcg capable (yet)
        Config.enableMTTCG = false;

        Config.hd_if_type = "scsi";

        Config.enable_SDL_sound = false;

        Config.enableEmulatedSDCard = false;

        Config.machineFolder = Config.machineFolder + "other/sparc_machines/";

        Config.osImages.put("Debian Sparc Linux", "http://limboemulator.weebly.com/debian-sparc-linux.html");

        super.onCreate(bundle);

        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Config.logFilePath = Config.cacheDir + "/limbo/limbo-sparc-log.txt";
    }

    protected void loadQEMULib() {

        try {
            System.loadLibrary("qemu-system-sparc");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-sparc64");
        }


    }

}
