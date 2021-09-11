package com.limbo.emu.main;

import android.os.Bundle;

import com.max2idea.android.limbo.log.Logger;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.links.LinksManager;
import com.max2idea.android.limbo.main.LimboApplication;

public class LimboEmuActivity extends LimboActivity {

    public void onCreate(Bundle bundle) {
        LimboApplication.arch = Config.Arch.x86_64;
        Config.clientClass = this.getClass();
        Config.enableKVM = true;
        //XXX; only for 64bit hosts, also make sure you have qemu 3.1.0 x86_64-softmmu and above compiled
        if(LimboApplication.isHost64Bit() && Config.enableMTTCG)
            Config.enableMTTCG = true;
        else
            Config.enableMTTCG = false;
        Config.osImages.put(getString(R.string.SlaxLinux), new LinksManager.LinkInfo(getString(R.string.SlaxLinux),
                getString(R.string.SlaxLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Slax-linux",
                LinksManager.LinkType.ISO));
        Config.osImages.put(getString(R.string.SlitazLinux), new LinksManager.LinkInfo(getString(R.string.SlitazLinux),
                getString(R.string.SlitazLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Slitaz-linux",
                LinksManager.LinkType.ISO));
        Config.osImages.put(getString(R.string.DSLLinux), new LinksManager.LinkInfo(getString(R.string.DSLLinux),
                getString(R.string.DSLLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/DSL-linux",
                LinksManager.LinkType.ISO));
        Config.osImages.put(getString(R.string.DebianLinux), new LinksManager.LinkInfo(getString(R.string.DebianLinux),
                getString(R.string.DebianLinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Debian-Linux",
                LinksManager.LinkType.ISO));
        Config.osImages.put("Trinux", new LinksManager.LinkInfo(getString(R.string.Trinux),
                getString(R.string.TrinuxDescr),
                "https://github.com/limboemu/limbo/wiki/Trinux",
                LinksManager.LinkType.ISO));
        Config.osImages.put(getString(R.string.Freedos), new LinksManager.LinkInfo(getString(R.string.Freedos),
                getString(R.string.FreedosDescr),
                "https://github.com/limboemu/limbo/wiki/FreeDOS",
                LinksManager.LinkType.ISO));
        Config.osImages.put(getString(R.string.KolibriOS), new LinksManager.LinkInfo(getString(R.string.KolibriOS),
                getString(R.string.KolibriOSDescr),
                "https://github.com/limboemu/limbo/wiki/KolibriOS",
                LinksManager.LinkType.ISO));
        super.onCreate(bundle);
        //TODO: change location to something that the user will have access outside of limbo
        //  like internal storage
        Logger.setupLogFile("/limbo/limbo-x86-log.txt");
    }

    protected void loadQEMULib() {
        try {
            System.loadLibrary("qemu-system-i386");
        } catch (Error ex) {
            System.loadLibrary("qemu-system-x86_64");
        }

    }
}
