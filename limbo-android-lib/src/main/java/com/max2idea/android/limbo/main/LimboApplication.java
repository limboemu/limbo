/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
package com.max2idea.android.limbo.main;

import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import com.max2idea.android.limbo.machine.Dispatcher;
import com.max2idea.android.limbo.machine.FavOpenHelper;
import com.max2idea.android.limbo.machine.MachineOpenHelper;

import java.io.File;

/**
 * We use the application context for the initiliazation of some of the Storage and
 * Controller implementations.
 */
public class LimboApplication extends Application {
    private static final String TAG = "LimboApplication";
    //Do not update these directly, see inherited project java files
    public static Config.Arch arch;
    private static Context sInstance;
    private static PackageInfo packageInfo;

    public static Context getInstance() {
        return sInstance;
    }

    public static PackageInfo getPackageInfo() {
        return packageInfo;
    }

    public static void setupPackageInfo(Context context) {
        try {
            packageInfo = context.getPackageManager().getPackageInfo(context.getClass().getPackage().getName(),
                    PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
    }

    public static String getUserId(Context context) {
        String userid = "None";
        try {
            ApplicationInfo appInfo = context.getPackageManager().getApplicationInfo(context.getClass().getPackage().getName(),
                    PackageManager.GET_META_DATA);
            userid = appInfo.uid + "";
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return userid;
    }

    public static boolean isHost64Bit() {
        return Build.SUPPORTED_64_BIT_ABIS != null && Build.SUPPORTED_64_BIT_ABIS.length > 0;
    }

    // Legacy
    public static boolean isHostX86_64() {
        if (Build.SUPPORTED_64_BIT_ABIS != null) {
            for (int i = 0; i < Build.SUPPORTED_64_BIT_ABIS.length; i++)
                if (Build.SUPPORTED_64_BIT_ABIS[i].equals("x86_64"))
                    return true;
        }
        return false;
    }

    // Legacy
    public static boolean isHostX86() {
        if (Build.SUPPORTED_32_BIT_ABIS != null) {
            for (int i = 0; i < Build.SUPPORTED_32_BIT_ABIS.length; i++)
                if (Build.SUPPORTED_32_BIT_ABIS[i].equals("x86"))
                    return true;
        }
        return false;
    }

    public static boolean isHostArm() {
        if (Build.SUPPORTED_32_BIT_ABIS != null) {
            for (int i = 0; i < Build.SUPPORTED_32_BIT_ABIS.length; i++)
                if (Build.SUPPORTED_32_BIT_ABIS[i].equals("armeabi-v7a"))
                    return true;
        }
        return false;
    }

    public static boolean isHostArmv8() {
        if (Build.SUPPORTED_64_BIT_ABIS != null) {
            for (int i = 0; i < Build.SUPPORTED_64_BIT_ABIS.length; i++)
                if (Build.SUPPORTED_64_BIT_ABIS[i].equals("arm64-v8a"))
                    return true;
        }
        return false;
    }

    public static ViewListener getViewListener() {
        return Dispatcher.getInstance();
    }

    public static String getBasefileDir() {
        return getInstance().getCacheDir() + "/limbo/";
    }

    public static String getTmpFolder() {
        return getBasefileDir() + "var/tmp"; // Do not modify
    }

    public static String getMachineDir() {
        return getBasefileDir() + Config.machineFolder;
    }

    public static String getLocalQMPSocketPath() {
        return getInstance().getCacheDir() + "/qmpsocket";
    }

    @Override
    public void onCreate() {
        super.onCreate();
        sInstance = this;
        try {
            Class.forName("android.os.AsyncTask");
        } catch (Throwable ignore) {
            // ignored
        }
        MachineOpenHelper.initialize(this);
        FavOpenHelper.initialize(this);
        setupFolders();
    }

    private void setupFolders() {
        Config.storagedir = Environment.getExternalStorageDirectory().toString();
        File folder = new File(LimboApplication.getTmpFolder());
        if (!folder.exists()) {
            boolean res = folder.mkdirs();
            if (!res) {
                Log.e(TAG, "Could not create temp folder: " + folder.getPath());
            }
        }
    }
}
