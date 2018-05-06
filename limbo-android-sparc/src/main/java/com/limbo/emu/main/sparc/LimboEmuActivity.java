
package com.limbo.emu.main.sparc;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {
        Config.enable_sparc = true;
        Config.machineFolder = Config.machineFolder + "other/sparc_machines/";

        Config.osImages.put("Debian Sparc Linux", "http://limboemulator.weebly.com/debian-sparc-linux.html");

        super.onCreate(bundle);
    }

    protected void loadQEMULib() {

        System.loadLibrary("qemu-system-sparc");

    }

}
