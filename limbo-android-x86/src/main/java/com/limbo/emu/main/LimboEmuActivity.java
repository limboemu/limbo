package com.limbo.emu.main;

import android.os.Bundle;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.utils.LinksManager;
import com.max2idea.android.limbo.utils.Machine;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {
        Config.enable_X86 = true;
        Config.enable_X86_64 = true;

        Config.enable_KVM = true;

        //XXX; only for 64bit hosts, also make sure you have qemu 3.1.0 x86_64-softmmu and above compiled
        if(Machine.isHost64Bit() && Config.enableMTTCG)
            Config.enableMTTCG = true;
        else
            Config.enableMTTCG = false;

        Config.osImages.put("DSL Linux", new LinksManager.LinkInfo("DSL Linux",
                "A Linux-based light weight OS with Desktop Manager, network, and package manager",
                "https://github.com/limboemu/limbo/wiki/DSL-linux",
                LinksManager.LinkType.ISO));

        Config.osImages.put("Debian Linux", new LinksManager.LinkInfo("Debian Linux",
                "Another Linux-based light weight OS with Desktop Manager, network, and package manager",
                "https://github.com/limboemu/limbo/wiki/Debian-Linux",
                LinksManager.LinkType.ISO));

        Config.osImages.put("Trinux", new LinksManager.LinkInfo("Trinux",
                "A Linux-based command line only OS, network",
                "https://github.com/limboemu/limbo/wiki/Trinux",
                LinksManager.LinkType.ISO));

        Config.osImages.put("FreeDOS", new LinksManager.LinkInfo("FreeDOS",
                "A DOS-based command line only OS",
                "https://github.com/limboemu/limbo/wiki/FreeDOS",
                LinksManager.LinkType.ISO));

        Config.osImages.put("KolibriOS", new LinksManager.LinkInfo("KolibriOS",
                "A ultra light weight OS written in assembly with a Desktop Manager",
                "https://github.com/limboemu/limbo/wiki/KolibriOS",
                LinksManager.LinkType.ISO));

        super.onCreate(bundle);

        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Config.logFilePath = Config.cacheDir + "/limbo/limbo-x86-log.txt";
    }

    protected void loadQEMULib() {

        try {
            System.loadLibrary("qemu-system-i386");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-x86_64");
        }

    }
}
