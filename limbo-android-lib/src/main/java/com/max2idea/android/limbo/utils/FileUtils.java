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

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import androidx.documentfile.provider.DocumentFile;
import android.text.Spannable;
import android.util.Log;
import android.webkit.MimeTypeMap;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

/**
 * @author dev
 */
public class FileUtils {
    private final static String TAG = "FileUtils";
    public static HashMap<Integer, FileInfo> fds = new HashMap<Integer, FileInfo>();

    public static String getNativeLibDir(Context context) {
        return context.getApplicationInfo().nativeLibraryDir;
    }

    public static String getFullPathFromDocumentFilePath(String filePath) {

        filePath = filePath.replaceAll("%3A", "^3A");
        int index = filePath.lastIndexOf("^3A");
        if (index > 0)
            filePath = filePath.substring(index + 3);
        if (!filePath.startsWith("/"))
            filePath = "/" + filePath;

//        filePath = filePath.replaceAll("%2F", "/");
//        filePath = filePath.replaceAll("\\^2F", "/");

        //remove any spaces encoded by the ASF
        try {
            filePath = URLDecoder.decode(filePath, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        return filePath;
    }

    public static String getFilenameFromPath(String filePath) {
        filePath = filePath.replaceAll("%2F", "/");
        filePath = filePath.replaceAll("%3A", "/");
        filePath = filePath.replaceAll("\\^2F", "/");
        filePath = filePath.replaceAll("\\^3A", "/");


        int index = filePath.lastIndexOf("/");
        if (index > 0)
            return filePath.substring(index + 1);
        return filePath;
    }

    public static String unconvertDocumentFilePath(String filePath) {
        if (filePath != null && filePath.startsWith("/content//")) {
            filePath = filePath.replace("/content//", "content://");
            filePath = filePath.replaceAll("\\^\\^\\^", "%");
        }
        return filePath;
    }

    public static String convertDocumentFilePath(String filePath) {
        if (filePath != null && filePath.startsWith("content://")) {
            filePath = filePath.replace("content://", "/content//");
            filePath = filePath.replaceAll("%", "\\^\\^\\^");;

        }
        return filePath;
    }

    public static void saveFileContents(String filePath, String contents) {
        // TODO: we assume that the contents are of small size so we keep in an array
        byteArrayToFile(contents.getBytes(), new File(filePath));
    }

    public static void byteArrayToFile(byte[] byteData, File filePath) {

        try {
            FileOutputStream fos = new FileOutputStream(filePath);
            fos.write(byteData);
            fos.close();

        } catch (FileNotFoundException ex) {
            System.out.println("FileNotFoundException : " + ex);
        } catch (IOException ioe) {
            System.out.println("IOException : " + ioe);
        }

    }

    public static ArrayList<Machine> getVMsFromFile(Context context, String importFilePath) {
        // TODO Auto-generated method stub
        ArrayList<Machine> machines = new ArrayList<Machine>();
        BufferedReader buffreader = null;
        InputStream instream = null;
        // Read machines from csv file
        try {
            // open the file for reading
            Log.v("CSV Parser", "Import file: " + importFilePath);
            instream = getStreamFromFilePath(context, importFilePath);

            // if file the available for reading
            if (instream != null) {
                // prepare the file for reading
                InputStreamReader inputreader = new InputStreamReader(instream);
                buffreader = new BufferedReader(inputreader);

                String line;

                String[] machineAttrNames = {"MACHINE_NAME", "UI", "PAUSED", "ARCH", "MACHINETYPE", "CPU", "CPUNUM",
                        "MEMORY", // Arch
                        "HDA", "HDB", "HDC", "HDD", "SHARED_FOLDER", "SHARED_FOLDER_MODE", // Storage
                        "CDROM", "FDA", "FDB", "SD", // Removable media
                        "VGA", "SOUNDCARD", "NETCONFIG", "NICCONFIG", "HOSTFWD", "GUESTFWD", // Misc
                        "SNAPSHOT_NAME", "HDCONFIG", "ENABLE_USBMOUSE", // Deprecated
                        "BOOT_CONFIG", "KERNEL", "INITRD", "APPEND", // Boot
                        // Settings
                        "DISABLE_ACPI", "DISABLE_HPET", "DISABLE_FD_BOOT_CHK", // Other
                        "EXTRA_PARAMS" // Extra Params
                };

                Hashtable<Integer, String> attrs = new Hashtable<Integer, String>();

                // read every line of the file into the line-variable, on line
                // at the time
                // Parse headers
                line = buffreader.readLine();
                String headers[] = line.split(",");
                for (int i = 0; i < headers.length; i++) {
                    attrs.put(i, headers[i].replace("\"", ""));
                }

                while (line != null) {
                    line = buffreader.readLine();
                    if (line == null)
                        break;
                    // do something with the line
                    // Log.v("CSV Parser", "Line: " + line);

                    // Parse
                    String machineAttr[] = line.split(",(?=(?:[^\"]*\"[^\"]*\")*[^\"]*$)", -1);
                    Machine mach = new Machine(machineAttr[0]);
                    for (int i = 0; i < machineAttr.length; i++) {
                        String attr = null;
                        if (machineAttr[i].equals("\"null\"")) {
                            continue;
                        }

                        if (attrs.get(i).equals("MACHINE_NAME"))
                            mach.machinename = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals("UI"))
                            mach.ui = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals("PAUSED"))
                            mach.paused = Integer.parseInt(machineAttr[i].replace("\"", ""));

                            // Arch
                        else if (attrs.get(i).equals("ARCH"))
                            mach.arch = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals("MACHINETYPE"))
                            mach.machine_type = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals("CPU"))
                            mach.cpu = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals("CPUNUM"))
                            mach.cpuNum = Integer.parseInt(machineAttr[i].replace("\"", ""));
                        else if (attrs.get(i).equals("MEMORY"))
                            mach.memory = Integer.parseInt(machineAttr[i].replace("\"", ""));

                            // Storage
                        else if (attrs.get(i).equals(MachineOpenHelper.HDA))
                            mach.hda_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.HDB))
                            mach.hdb_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.HDC))
                            mach.hdc_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.HDD))
                            mach.hdd_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.SHARED_FOLDER))
                            mach.shared_folder = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.SHARED_FOLDER_MODE))
                            mach.shared_folder_mode = Integer.parseInt(machineAttr[i].replace("\"", ""));

                            // Removable Media
                        else if (attrs.get(i).equals(MachineOpenHelper.CDROM))
                            mach.cd_iso_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.FDA))
                            mach.fda_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.FDB))
                            mach.fdb_img_path = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.SD))
                            mach.sd_img_path = machineAttr[i].replace("\"", "");

                            // Misc
                        else if (attrs.get(i).equals(MachineOpenHelper.VGA))
                            mach.vga_type = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.SOUNDCARD_CONFIG))
                            mach.soundcard = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.NET_CONFIG))
                            mach.net_cfg = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.NIC_CONFIG))
                            mach.nic_card = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.HOSTFWD))
                            mach.hostfwd = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.GUESTFWD))
                            mach.guestfwd = machineAttr[i].replace("\"", "");

                            // Depreacated
                        else if (attrs.get(i).equals(MachineOpenHelper.HDCACHE_CONFIG))
                            mach.hd_cache = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.SNAPSHOT_NAME))
                            mach.snapshot_name = machineAttr[i].replace("\"", "");
                            // Other
                        else if (attrs.get(i).equals(MachineOpenHelper.DISABLE_ACPI))
                            mach.disableacpi = Integer.parseInt(machineAttr[i].replace("\"", ""));
                        else if (attrs.get(i).equals(MachineOpenHelper.DISABLE_HPET))
                            mach.disablehpet = Integer.parseInt(machineAttr[i].replace("\"", ""));
                        else if (attrs.get(i).equals(MachineOpenHelper.DISABLE_TSC))
                            mach.disabletsc = Integer.parseInt(machineAttr[i].replace("\"", ""));
                        else if (attrs.get(i).equals(MachineOpenHelper.DISABLE_FD_BOOT_CHK))
                            mach.disablefdbootchk = Integer.parseInt(machineAttr[i].replace("\"", ""));
                            // else if(attrs.get(i).equals("DISABLE_TSC"))
                            // mach.disablefdbootchk=Integer.parseInt(machineAttr[i].replace("\"",
                            // ""));
                            // else
                            // if(attrs.get(i).equals("ENABLE_USBMOUSE"))
                            // //Disable for now

                            // Boot Settings
                        else if (attrs.get(i).equals(MachineOpenHelper.BOOT_CONFIG))
                            mach.bootdevice = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.KERNEL))
                            mach.kernel = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.INITRD))
                            mach.initrd = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.APPEND))
                            mach.append = machineAttr[i].replace("\"", "");

                            // Extra Params
                        else if (attrs.get(i).equals(MachineOpenHelper.EXTRA_PARAMS))
                            mach.extra_params = machineAttr[i].replace("\"", "");

                        else if (attrs.get(i).equals(MachineOpenHelper.MOUSE))
                            mach.mouse = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.KEYBOARD))
                            mach.keyboard = machineAttr[i].replace("\"", "");
                        else if (attrs.get(i).equals(MachineOpenHelper.ENABLE_MTTCG))
                            mach.enableMTTCG = Integer.parseInt(machineAttr[i].replace("\"", ""));
                        else if (attrs.get(i).equals(MachineOpenHelper.ENABLE_KVM))
                            mach.enableKVM = Integer.parseInt(machineAttr[i].replace("\"", ""));

                    }
                    Log.v("CSV Parser", "Adding Machine: " + mach.machinename);
                    machines.add(mach);
                }

            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {

            try {
                if (buffreader != null)
                    buffreader.close();

            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            try {
                if (instream != null)
                    instream.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }

        return machines;
    }

    public static InputStream getStreamFromFilePath(Context context, String importFilePath) throws FileNotFoundException {
        InputStream stream = null;
        if (importFilePath.startsWith("content://")) {
            Uri uri = Uri.parse(importFilePath);
            String mode = "rw";
            ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, mode);
            return new FileInputStream(pfd.getFileDescriptor());
        } else {
            return new FileInputStream(importFilePath);
        }
    }

    public static String getFileContents(String filePath) {

        File file = new File(filePath);
        if(!file.exists())
            return "";
        StringBuilder builder = new StringBuilder("");
        try {
            FileInputStream stream = new FileInputStream(file);
            byte[] buff = new byte[32768];
            int bytesRead = 0;
            while ((bytesRead = stream.read(buff, 0, buff.length)) > 0) {
                builder.append(new String(buff, "UTF-8"));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        String contents = builder.toString();
        return contents;
    }

    public static void viewLimboLog(final Activity activity) {

        String contents = FileUtils.getFileContents(Config.logFilePath);

        if (contents.length() > 50 * 1024)
            contents = contents.substring(0, 25 * 1024)
                            + "\n.....\n" +
                            contents.substring(contents.length() - 25 * 1024);

        final String finalContents = contents;
        final Spannable contentsFormatted = UIUtils.formatAndroidLog(contents);

        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {

                if (Config.viewLogInternally) {
                    UIUtils.UIAlertLog(activity, "Limbo Log", contentsFormatted);
                } else {
                    try {
                        Intent intent = new Intent(Intent.ACTION_EDIT);
                        File file = new File(Config.logFilePath);
                        Uri uri = Uri.fromFile(file);
                        intent.setDataAndType(uri, "text/plain");
                        activity.startActivity(intent);
                    } catch (Exception ex) {
//            UIUtils.toastShort(activity, "Could not find a Text Viewer on your device");
                        UIUtils.UIAlertLog(activity, "Limbo Log", contentsFormatted);
                    }
                }


            }
        });

    }

    public static boolean fileValid(Context context, String path) {

        if (path == null || path.equals(""))
            return true;
        if (path.startsWith("content://") || path.startsWith("/content/")) {
            int fd = get_fd(context, path);
            if (fd <= 0)
                return false;
        } else {
            File file = new File(path);
            return file.exists();
        }
        return true;
    }

    //TODO: we should pass the modes from the backend and translate them
    // instead of blindly using "rw". ie ISOs should be read only.
    public static int get_fd(final Context context, String path) {
        synchronized (fds) {
            int fd = 0;
            if (path == null)
                return 0;

//            Log.d(TAG, "Opening Filepath: " + path);
            if (path.startsWith("/content//") || path.startsWith("content://")) {
                String npath = unconvertDocumentFilePath(path);

//Is this needed?
//                FileInfo info = getExistingFd(npath);
//                if (info!=null) {
//                    ParcelFileDescriptor pfd = info.pfd;
//                    fd = pfd.getFd();
//                    Log.d(TAG, "Retrieved hashed documentfile: " + npath + ", FD: " + fd);
//                    return fd;
//                }


//                Log.d(TAG, "Opening unconverted: " + npath);
                try {
                    Uri uri = Uri.parse(npath);
                    String mode = "rw";
                    if (path.toLowerCase().endsWith(".iso"))
                        mode = "r";
                    ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, mode);
                    fd = pfd.getFd();
//                    Log.d(TAG, "Opening DocumentFile: " + npath + ", FD: " + fd);
                    fds.put(fd, new FileInfo(path, npath, pfd));

                } catch (Exception e) {
                    Log.e(TAG, "Could not open DocumentFile: " + npath + ", FD: " + fd);
                    if(Config.debug)
                        e.printStackTrace();
                }
            } else {
                //Is this needed?
//                FileInfo info = getExistingFd(path);
//                if (info!=null) {
//                    ParcelFileDescriptor pfd = info.pfd;
//                    fd = pfd.getFd();
//                    Log.d(TAG, "Retrieved hashed file: " + path + ", FD: " + fd);
//                    return fd;
//                }

                try {
                    int mode = ParcelFileDescriptor.MODE_READ_WRITE;
                    if (path.toLowerCase().endsWith(".iso"))
                        mode = ParcelFileDescriptor.MODE_READ_ONLY;

                    File file = new File(path);
                    if (!file.exists())
                        file.createNewFile();
                    ParcelFileDescriptor pfd = ParcelFileDescriptor.open(file, mode);
                    fd = pfd.getFd();
                    fds.put(fd, new FileInfo(path, path, pfd));
                    Log.d(TAG, "Opening File: " + path + ", FD: " + fd);
                } catch (Exception e) {
                    Log.e(TAG, "Could not open File: " + path + ", FD: " + fd);
                    if(Config.debug)
                        e.printStackTrace();
                }

            }
            return fd;
        }
    }

    private static FileInfo getExistingFd(String npath) {
        Set<Map.Entry<Integer, FileInfo>> fileInfoSet = fds.entrySet();
        Iterator<Map.Entry<Integer, FileInfo>> iter = fileInfoSet.iterator();
        while (iter.hasNext()) {
            Map.Entry<Integer, FileInfo> entry = iter.next();
            FileInfo fileInfo = entry.getValue();
            if (fileInfo.npath.equals(npath)) {
                return fileInfo;
            }
        }
        return null;
    }

    public static void close_fds() {
        synchronized (fds) {
            Integer[] fds = FileUtils.fds.keySet().toArray(new Integer[FileUtils.fds.keySet().size()]);
            for (int i = 0; i < fds.length; i++) {
                FileUtils.close_fd(fds[i]);
            }
        }
    }

    public static int close_fd(int fd) {
        if(!Config.closeFileDescriptors) {
            return 0;
        }
        synchronized (fds) {
//            Log.d(TAG, "Closing FD: " + fd);
            if (FileUtils.fds.containsKey(fd)) {

                FileInfo info = FileUtils.fds.get(fd);


                try {

                    ParcelFileDescriptor pfd = info.pfd;
                    try {
                        pfd.getFileDescriptor().sync();
                    } catch (IOException e) {
                        if(Config.debug) {
                            Log.w(TAG, "Syncing DocumentFile: " + info.path + ": " + fd + " : " + e);
                            e.printStackTrace();
                        }
                    }

//                    Log.d(TAG, "Closing DocumentFile: " + info.npath + ", FD: " + fd);
                    pfd.close();
                    FileUtils.fds.remove(fd);
                    return 0; // success for Native side
                } catch (IOException e) {
                    Log.e(TAG, "Error Closing DocumentFile: " + info.path + ": " + fd + " : " + e);
                    if(Config.debug)
                        e.printStackTrace();
                }


            } else {

                ParcelFileDescriptor pfd = null;
                String path = "unknown";
                try {

                    //xxx: check the hash
                    FileInfo info = FileUtils.fds.get(fd);
                    if(info!=null) {
                        pfd = info.pfd;
                        path = info.path;
//                        Log.d(TAG, "Closing hashe File FD: " + fd + ": " + info.path);
                    }

                    //xxx: else get a new parcel
                    if(pfd == null)
                        pfd = ParcelFileDescriptor.fromFd(fd);

//                    Log.d(TAG, "Closing File FD: " + fd);
                    try {
                        pfd.getFileDescriptor().sync();
                    } catch (IOException e) {
                        if(Config.debug) {
                            Log.e(TAG, "Error Syncing File: " + path + ": " + fd + " : " + e);
                            e.printStackTrace();
                        }
                    }

                    pfd.close();
                    return 0;
                } catch (Exception e) {
                    Log.e(TAG, "Error Closing File FD: " + path + ": " + fd + " : " + e);
                    if(Config.debug)
                        e.printStackTrace();
                }
            }
            return -1;
        }
    }

    public static void startLogging() {

        if (Config.logFilePath == null) {
            Log.e(TAG, "Log file is not setup");
            return;
        }

        Thread t = new Thread(new Runnable() {
            public void run() {

                FileOutputStream os = null;
                File logFile = null;
                try {
                    logFile = new File(Config.logFilePath);
                    if (logFile.exists()) {
                        logFile.delete();
                    }
                    logFile.createNewFile();
                    Runtime.getRuntime().exec("logcat -c");
                    Process process = Runtime.getRuntime().exec("logcat v main");
                    os = new FileOutputStream(logFile);
                    BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));

                    StringBuilder log = new StringBuilder("");
                    String line = "";
                    while ((line = bufferedReader.readLine()) != null) {
                        log.setLength(0);
                        log.append(line).append("\n");
                        os.write(log.toString().getBytes("UTF-8"));
                        os.flush();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                } finally {
                    try {
                        if (os != null) {
                            os.flush();
                            os.close();
                        }
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }

                }

            }
        });
        t.setName("LimboLogger");
        t.start();
    }

    public static String LoadFile(Activity activity, String fileName, boolean loadFromRawFolder) throws IOException {
        // Create a InputStream to read the file into
        InputStream iS;
        if (loadFromRawFolder) {
            // get the resource id from the file name
            int rID = activity.getResources().getIdentifier(activity.getClass().getPackage().getName() + ":raw/" + fileName,
                    null, null);
            // get the file as a stream
            iS = activity.getResources().openRawResource(rID);
        } else {
            // get the file as a stream
            iS = activity.getResources().getAssets().open(fileName);
        }

        ByteArrayOutputStream oS = new ByteArrayOutputStream();
        byte[] buffer = new byte[iS.available()];
        int bytesRead = 0;
        while ((bytesRead = iS.read(buffer)) > 0) {
            oS.write(buffer);
        }
        oS.close();
        iS.close();

        // return the output stream as a String
        return oS.toString();
    }

    public static String getExtensionFromFilename(String fileName) {
        if (fileName == null)
            return "";

        int index = fileName.lastIndexOf(".");
        if (index >= 0) {
            return fileName.substring(index + 1);
        } else
            return "";
    }

    public static int getIconForFile(String file) {
        file = file.toLowerCase();
        String ext = FileUtils.getExtensionFromFilename(file).toLowerCase();

        if(ext.equals("img") || ext.equals("qcow")
        || ext.equals("qcow2") || ext.equals("vmdk") || ext.equals("vdi") || ext.equals("cow")
                || ext.equals("dmg") || ext.equals("bochs") || ext.equals("vpc")
                || ext.equals("vhd") || ext.equals("fs"))
            return R.drawable.harddisk;
        else if(ext.equals("iso"))
            return R.drawable.cd;
        else if(ext.equals("ima"))
            return R.drawable.floppy;
        else if(ext.equals("csv"))
            return R.drawable.importvms;
        else if(file.contains("kernel")  || file.contains("vmlinuz") || file.contains("initrd"))
            return R.drawable.sysfile;
        else
            return R.drawable.close;

    }




    public static Uri saveLogFileSDCard(Activity activity,
                                       Uri destDir) {

        DocumentFile destFileF = null;
        OutputStream os = null;
        Uri uri = null;

        try {
            String logFileContents = getFileContents(Config.logFilePath);
            DocumentFile dir = DocumentFile.fromTreeUri(activity, destDir);

            //Create the file if doesn't exist
            destFileF = dir.findFile(Config.destLogFilename);
            if(destFileF == null) {
                destFileF = dir.createFile(MimeTypeMap.getSingleton().getMimeTypeFromExtension("txt"), Config.destLogFilename);
            }

            //Write to the dest
            os = activity.getContentResolver().openOutputStream(destFileF.getUri());
            os.write(logFileContents.getBytes());

            //success
            uri = destFileF.getUri();

        } catch (Exception ex) {
            UIUtils.toastShort(activity, "Failed to save log file: " + ex.getMessage());
        } finally {
            if(os!=null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
        return uri;
    }

    public static Uri exportFileSDCard(Activity activity,
                                        Uri destDir, String destFile) {

        DocumentFile destFileF = null;
        OutputStream os = null;
        Uri uri = null;

        try {
            String machinesToExport = MachineOpenHelper.getInstance(activity).exportMachines();
            DocumentFile dir = DocumentFile.fromTreeUri(activity, destDir);

            //Create the file if doesn't exist
            destFileF = dir.findFile(destFile);
            if(destFileF == null) {
                destFileF = dir.createFile(MimeTypeMap.getSingleton().getMimeTypeFromExtension("csv"), destFile);
            }
            else {
                UIUtils.toastShort(activity, "File exists, choose another filename");
                return null;
            }

            //Write to the dest
            os = activity.getContentResolver().openOutputStream(destFileF.getUri());
            os.write(machinesToExport.getBytes());

            //success
            uri = destFileF.getUri();

        } catch (Exception ex) {
            UIUtils.toastShort(activity, "Failed to export file: " + destFile + ", Error:" + ex.getMessage());
        } finally {
            if(os!=null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
        return uri;
    }

    public static String exportFileLegacy(Activity activity,
                                       String destDir, String destFile) {

        String filePath = null;
        File destFileF = new File(destDir, destFile);

        try {
            String machinesToExport = MachineOpenHelper.getInstance(activity).exportMachines();
            FileUtils.saveFileContents(destFileF.getAbsolutePath(), machinesToExport);

            //success
            filePath = destFileF.getAbsolutePath();

        } catch (Exception ex) {
            UIUtils.toastShort(activity, "Failed to export file: " + destFileF.getAbsolutePath() + ", Error:" + ex.getMessage());
        } finally {
        }
        return filePath;
    }


    public static String saveLogFileLegacy(Activity activity,
                                          String destLogFilePath) {

        String filePath = null;
        File destFileF = new File(destLogFilePath, Config.destLogFilename);

        try {
            String logFileContents = getFileContents(Config.logFilePath);
            FileUtils.saveFileContents(destFileF.getAbsolutePath(), logFileContents);

            //success
            filePath = destFileF.getAbsolutePath();

        } catch (Exception ex) {
            UIUtils.toastShort(activity, "Failed to save log file: " + destFileF.getAbsolutePath() + ", Error:" + ex.getMessage());
        } finally {
        }
        return filePath;
    }


    public static String getFileUriFromIntent(Activity activity, Intent data, boolean write) {
        if(data == null)
            return null;

        Uri uri = data.getData();
        DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
        String file = uri.toString();
        if (!file.contains("com.android.externalstorage.documents")) {
            UIUtils.showFileNotSupported(activity);
            return null;
        }
        activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if(write)
            activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

        int takeFlags = data.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if(write)
            takeFlags = takeFlags | Intent.FLAG_GRANT_WRITE_URI_PERMISSION;

        activity.getContentResolver().takePersistableUriPermission(uri, takeFlags);
        return file;
    }

    public static String getDirPathFromIntent(Activity activity, Intent data) {
        if(data == null)
            return null;
        Bundle b = data.getExtras();
        String file = b.getString("currDir");
        return file;
    }


    public static String getFilePathFromIntent(Activity activity, Intent data) {
        if(data == null)
            return null;
        Bundle b = data.getExtras();
        String file = b.getString("file");
        return file;
    }

    public static LimboActivity.FileType getFileTypeFromIntent(Activity activity, Intent data) {
        if(data == null)
            return null;
        Bundle b = data.getExtras();
        LimboActivity.FileType fileType = (LimboActivity.FileType) b.getSerializable("fileType");
        return fileType;
    }

    public static class FileInfo {
        public String path;
        public String npath;
        public ParcelFileDescriptor pfd;

        public FileInfo(String path, String npath, ParcelFileDescriptor pfd) {
            this.npath = npath;
            this.path = path;
            this.pfd = pfd;
        }
    }



    public static void saveLogToFile(final Activity activity, final String logFileDestDir) {


        Thread t = new Thread(new Runnable() {
            public void run() {

                String displayName = null;
                if (logFileDestDir.startsWith("content://")) {
                    Uri exportDirUri = Uri.parse(logFileDestDir);
                    Uri fileCreatedUri = FileUtils.saveLogFileSDCard(activity,
                            exportDirUri);
                    displayName = FileUtils.getFullPathFromDocumentFilePath(fileCreatedUri.toString());
                } else {
                    String filePath = FileUtils.saveLogFileLegacy(activity, logFileDestDir);
                    displayName = filePath;
                }

                if(displayName!=null){

                    UIUtils.toastShort(activity, "Logfile saved");
                }
            }
        });
        t.start();
    }


}
