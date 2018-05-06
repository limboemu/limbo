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
package com.max2idea.android.limbo.utils;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.net.Uri;
import android.support.v4.provider.DocumentFile;
import android.util.Log;

import com.max2idea.android.limbo.main.Config;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author dev
 */
public class FileInstaller {

    public static void installFiles(Activity activity, boolean force) {

        Log.v("Installer", "Installing files...");
        File tmpDir = new File(Config.getBasefileDir(activity));
        if (!tmpDir.exists()) {
            tmpDir.mkdirs();
        }

        File tmpDir1 = new File(Config.getMachineDir(activity));
        if (!tmpDir1.exists()) {
            tmpDir1.mkdirs();
        }


        //Install base dir
        File dir = new File(Config.getBasefileDir(activity));
        if (dir.exists() && dir.isDirectory()) {
            //don't create again
        } else if (dir.exists() && !dir.isDirectory()) {
            Log.v("Installer", "Could not create Dir, file found: " + Config.getBasefileDir(activity));
            return;
        } else if (!dir.exists()) {
            dir.mkdir();
        }

        String destDir = Config.getBasefileDir(activity);

        //Get each file in assets under ./roms/ and install in SDCARD
        AssetManager am = activity.getResources().getAssets();
        String[] files = null;
        try {
            files = am.list("roms");
        } catch (IOException ex) {
            Logger.getLogger(FileInstaller.class.getName()).log(Level.SEVERE, null, ex);
            Log.v("Installer", "Could not install files: " + ex.getMessage());
            return;
        }

        for (int i = 0; i < files.length; i++) {
            //Log.v("Installer", "File: " + files[i]);
            String[] subfiles = null;
            try {
                subfiles = am.list("roms/" + files[i]);
            } catch (IOException ex) {
                Logger.getLogger(FileInstaller.class.getName()).log(Level.SEVERE, null, ex);
            }
            if (subfiles != null && subfiles.length > 0) {
                //Install base dir
                File dir1 = new File(Config.getBasefileDir(activity) + files[i]);
                if (dir1.exists() && dir1.isDirectory()) {
                    //don't create again
                } else if (dir1.exists() && !dir1.isDirectory()) {
                    Log.v("Installer", "Could not create Dir, file found: " + Config.getBasefileDir(activity) + files[i]);
                    return;
                } else if (!dir1.exists()) {
                    dir1.mkdir();
                }
                for (int k = 0; k < subfiles.length; k++) {

                    File file = new File(destDir, files[i] + "/" + subfiles[k]);
                    if(!file.exists() || force) {
                        Log.v("Installer", "Installing file: " + file.getPath());
                        installFile(activity, files[i] + "/" + subfiles[k], destDir, "roms", null);
                    }
                }
            } else {
                File file = new File(destDir, files[i]);
                if(!file.exists() || force) {
                    Log.v("Installer", "Installing file: " + file.getPath());
                    installFile(activity, files[i], Config.getBasefileDir(activity), "roms", null);
                }
            }
        }
//        InputStream is = am.open(srcFile);

    }

    public static boolean installFile(Context activity, String srcFile, 
    		String destDir, String assetsDir, String destFile) {
        try {
            AssetManager am = activity.getResources().getAssets(); // get the local asset manager
            InputStream is = am.open(assetsDir + "/" + srcFile); // open the input stream for reading
            File destDirF = new File(destDir);
            if (!destDirF.exists()) {
                boolean res = destDirF.mkdirs();
                if(!res){
                	UIUtils.toastShort(activity, "Could not create directory for image");
                }
            }
            
            if(destFile==null)
            	destFile=srcFile;
            OutputStream os = new FileOutputStream(destDir + "/" + destFile);
            byte[] buf = new byte[8092];
            int n;
            while ((n = is.read(buf)) > 0) {
                os.write(buf, 0, n);
            }
            os.close();
            is.close();
            return true;
        } catch (Exception ex) {
            Log.e("Installer", "failed to install file: " + destFile + ", Error:" + ex.getMessage());
            return false;
        }
    }

    public static Uri installFileSDCard(Context activity, String srcFile,
                                            Uri destDir, String assetsDir, String destFile) {

        DocumentFile destFileF = null;
        OutputStream os = null;
        InputStream is = null;
        Uri uri = null;

        try {

            DocumentFile dir = DocumentFile.fromTreeUri(activity, destDir);
            AssetManager am = activity.getResources().getAssets(); // get the local asset manager
            is = am.open(assetsDir + "/" + srcFile); // open the input stream for reading

            if(destFile==null)
                destFile=srcFile;

            //Create the file if doesn't exist
            destFileF = dir.findFile(destFile);
            if(destFileF == null) {
                destFileF = dir.createFile("application/octet-stream", destFile);
            }
            else {
                    UIUtils.toastShort(activity, "File exists, choose another filename");
                    return null;
            }

            //Write to the dest
            os = activity.getContentResolver().openOutputStream(destFileF.getUri());
            //OutputStream os = new FileOutputStream(destDir + "/" + destFile);
            byte[] buf = new byte[8092];
            int n;
            while ((n = is.read(buf)) > 0) {
                os.write(buf, 0, n);
            }

            //success
            uri = destFileF.getUri();

        } catch (Exception ex) {
            Log.e("Installer", "failed to install file: " + destFile + ", Error:" + ex.getMessage());
        } finally {
            if(os!=null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if(is!=null) {
                try {
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
        return uri;
    }
}
