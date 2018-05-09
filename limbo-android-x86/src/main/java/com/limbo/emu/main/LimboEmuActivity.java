package com.limbo.emu.main;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {
        Config.enable_X86 = true;
        Config.enable_X86_64 = true;
        Config.enable_KVM = false;
        //Config.enableMTTCG = true;
        Config.logFilePath = Config.storagedir + "/limbo/limbo-x86-log.txt";

        Config.osImages.put("DSL Linux", "http://limboemulator.weebly.com/dsl-linux.html");
        Config.osImages.put("Debian Linux", "http://limboemulator.weebly.com/debian-linux.html");
        Config.osImages.put("Trinux", "http://limboemulator.weebly.com/trinux.html");
        Config.osImages.put("FreeDOS", "http://limboemulator.weebly.com/freedos.html");
        Config.osImages.put("KolibriOS", "http://limboemulator.weebly.com/kolibrios.html");
        super.onCreate(bundle);
    }

    protected void loadQEMULib() {

        try {
            System.loadLibrary("qemu-system-i386");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-x86_64");
        }

    }
}
