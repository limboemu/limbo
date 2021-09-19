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
package com.max2idea.android.limbo.files;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.webkit.MimeTypeMap;

import androidx.documentfile.provider.DocumentFile;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.dialog.DialogUtils;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;
import com.max2idea.android.limbo.main.LimboApplication;
import com.max2idea.android.limbo.main.LimboSettingsManager;
import com.max2idea.android.limbo.toast.ToastUtils;

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
import java.util.HashMap;

/**
 * @author dev
 */
public class FileUtils {
    private final static String TAG = "FileUtils";
    private static final Object fdsLock = new Object();
    private static HashMap<Integer, FileInfo> fds = new HashMap<Integer, FileInfo>();

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

    public static String decodeDocumentFilePath(String filePath) {
        if (filePath != null && filePath.startsWith("/content//")) {
            filePath = filePath.replace("/content//", "content://");
            filePath = filePath.replaceAll("\\^\\^\\^", "%");
        }
        return filePath;
    }

    /**
     * Encode the path % chars to ^
     *
     * @param filePath
     * @return
     */
    public static String encodeDocumentFilePath(String filePath) {
        if (filePath != null && filePath.startsWith("content://")) {
            filePath = filePath.replace("content://", "/content//");
            filePath = filePath.replaceAll("%", "\\^\\^\\^");
            ;

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

    public static InputStream getStreamFromFilePath(String filePath) throws FileNotFoundException {
        if (filePath.startsWith("content://")) {
            Uri uri = Uri.parse(filePath);
            String mode = "rw";
            ParcelFileDescriptor pfd = LimboApplication.getInstance().getContentResolver().openFileDescriptor(uri, mode);
            return new FileInputStream(pfd.getFileDescriptor());
        } else {
            return new FileInputStream(filePath);
        }
    }

    public static void closeFileDescriptor(String filePath) throws IOException {
        if (filePath.startsWith("content://")) {
            Uri uri = Uri.parse(filePath);
            String mode = "rw";
            ParcelFileDescriptor pfd = LimboApplication.getInstance().getContentResolver().openFileDescriptor(uri, mode);
            pfd.close();
        }
    }

    public static String getFileContents(String filePath) {
        File file = new File(filePath);
        if (!file.exists())
            return "";
        StringBuilder builder = new StringBuilder("");
        try {
            FileInputStream stream = new FileInputStream(file);
            byte[] buff = new byte[32768];
            int bytesRead = 0;
            while ((bytesRead = stream.read(buff, 0, buff.length)) > 0) {
                //FIXME: log file can be too large appending might fail with out of memory
                builder.append(new String(buff, 0, bytesRead));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        String contents = builder.toString();
        return contents;
    }

    public static boolean fileValid(String path) {
        if (path == null || path.equals(""))
            return true;
        if (path.startsWith("content://") || path.startsWith("/content/")) {
            int fd = get_fd(path);
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
    public static int get_fd(String path) {
        synchronized (fdsLock) {
            int fd = 0;
            if (path == null)
                return 0;
            if (path.startsWith("/content//") || path.startsWith("content://")) {
                String npath = decodeDocumentFilePath(path);
                try {
                    Uri uri = Uri.parse(npath);
                    String mode = "rw";
                    if (path.toLowerCase().endsWith(".iso"))
                        mode = "r";
                    ParcelFileDescriptor pfd = LimboApplication.getInstance().getContentResolver().openFileDescriptor(uri, mode);
                    fd = pfd.getFd();
                    fds.put(fd, new FileInfo(path, npath, pfd));
                    Log.d(TAG, "Opening Content Uri: " + npath + ", FD: " + fd);
                } catch (Exception e) {
                    String msg = LimboApplication.getInstance().getString(R.string.CouldNotOpenDocFile) + " "
                            + FileUtils.getFullPathFromDocumentFilePath(npath)
                            + "\n" + LimboApplication.getInstance().getString(R.string.PleaseReassingYourDiskFiles);
                    ToastUtils.toastLong(LimboApplication.getInstance(), msg);
                    e.printStackTrace();
                }
            } else {
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
                    if (Config.debug)
                        e.printStackTrace();
                }
            }
            return fd;
        }
    }

    public static void close_fds() {
        synchronized (fds) {
            Integer[] fds = FileUtils.fds.keySet().toArray(new Integer[FileUtils.fds.keySet().size()]);
            for (int i = 0; i < fds.length; i++) {
                FileUtils.close_fd(fds[i]);
            }
        }
    }

    /**
     * Closing File Descriptors
     *
     * @param fd File Descriptro to be closed
     * @return Returns 0 if fd is closed successfully otherwise -1
     */
    public static int close_fd(int fd) {
        if (!Config.closeFileDescriptors) {
            return 0;
        }
        synchronized (fds) {
            if (FileUtils.fds.containsKey(fd)) {
                FileInfo info = FileUtils.fds.get(fd);
                try {
                    ParcelFileDescriptor pfd = info.pfd;
                    if(Config.syncFilesOnClose) {
                        try {
                            pfd.getFileDescriptor().sync();
                        } catch (IOException e) {
                            if (Config.debug) {
                                Log.w(TAG, "Syncing DocumentFile: " + info.path + ": " + fd + " : " + e);
                                e.printStackTrace();
                            }
                        }
                    }
                    pfd.close();
                    FileUtils.fds.remove(fd);
                    return 0;
                } catch (IOException e) {
                    Log.e(TAG, "Error Closing DocumentFile: " + info.path + ": " + fd + " : " + e);
                    if (Config.debug)
                        e.printStackTrace();
                }
            } else {
                ParcelFileDescriptor pfd = null;

                try {
                    String path = "";
                    FileInfo info = FileUtils.fds.get(fd);
                    if (info != null) {
                        pfd = info.pfd;
                        path = info.path;
                    }
                    if (pfd == null)
                        pfd = ParcelFileDescriptor.fromFd(fd);
                    if(Config.syncFilesOnClose) {
                        try {
                            pfd.getFileDescriptor().sync();
                        } catch (IOException e) {
                            if (Config.debug) {
                                Log.w(TAG, "Error Syncing File: " + path + ": " + fd + " : " + e);
                                e.printStackTrace();
                            }
                        }
                    }
                    pfd.close();
                    return 0;
                } catch (Exception e) {
                    Log.e(TAG, "Error Closing File FD: " + fd + " : " + e);
                    if (Config.debug)
                        e.printStackTrace();
                }
            }
            return -1;
        }
    }


    public static void startLogging() {
        if (Config.logFilePath == null) {
            Log.w(TAG, "Log file is not ready");
            return;
        }
        Thread t = new Thread(new Runnable() {
            public void run() {
                FileOutputStream os = null;
                File logFile = null;
                try {
                    logFile = new File(Config.logFilePath);
                    if (logFile.exists()) {
                        if (!logFile.delete()) {
                            Log.w(TAG, "Could not delete previous log file!");
                        }
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

        if (ext.equals("img") || ext.equals("qcow")
                || ext.equals("qcow2") || ext.equals("vmdk") || ext.equals("vdi") || ext.equals("cow")
                || ext.equals("dmg") || ext.equals("bochs") || ext.equals("vpc")
                || ext.equals("vhd") || ext.equals("fs"))
            return R.drawable.harddisk;
        else if (ext.equals("iso"))
            return R.drawable.cd;
        else if (ext.equals("ima"))
            return R.drawable.floppy;
        else if (ext.equals("csv"))
            return R.drawable.importvms;
        else if (file.contains("kernel") || file.contains("vmlinuz") || file.contains("initrd"))
            return R.drawable.sysfile;
        else
            return R.drawable.close;

    }


    public static Uri saveLogFileSDCard(Activity activity, Uri destDir) {
        DocumentFile destFileF;
        OutputStream os = null;
        Uri uri = null;

        try {
            String logFileContents = getFileContents(Config.logFilePath);
            DocumentFile dir = DocumentFile.fromTreeUri(activity, destDir);
            if (dir == null)
                throw new Exception("Could not get log path directory");

            //Create the file if doesn't exist
            destFileF = dir.findFile(Config.destLogFilename);
            if (destFileF == null) {
                destFileF = dir.createFile(MimeTypeMap.getSingleton().getMimeTypeFromExtension("txt"), Config.destLogFilename);
            }

            //Write to the dest
            os = activity.getContentResolver().openOutputStream(destFileF.getUri());
            os.write(logFileContents.getBytes());

            //success
            uri = destFileF.getUri();

        } catch (Exception ex) {
            ToastUtils.toastShort(activity, activity.getString(R.string.FailedToSaveLogFile) + ex.getMessage());
        } finally {
            if (os != null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
        return uri;
    }

    public static Uri exportFileContents(Activity activity,
                                         Uri destDir, String destFile, byte[] contents) {

        DocumentFile destFileF = null;
        OutputStream os = null;
        Uri uri = null;

        try {
            DocumentFile dir = DocumentFile.fromTreeUri(activity, destDir);

            //Create the file if doesn't exist
            destFileF = dir.findFile(destFile);
            if (destFileF == null) {
                destFileF = dir.createFile(MimeTypeMap.getSingleton().getMimeTypeFromExtension("csv"), destFile);
            } else {
                ToastUtils.toastShort(activity, activity.getString(R.string.FileExistsChooseAnotherFilename));
                return null;
            }

            //Write to the dest
            os = activity.getContentResolver().openOutputStream(destFileF.getUri());
            os.write(contents);

            //success
            uri = destFileF.getUri();

        } catch (Exception ex) {
            ToastUtils.toastShort(activity, activity.getString(R.string.FailedToExportFile) +": " + destFile + ", " + activity.getString(R.string.Error) + ":" + ex.getMessage());
        } finally {
            if (os != null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
        return uri;
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
            ToastUtils.toastShort(activity, activity.getString(R.string.FailedSaveLogFile) + ": " + destFileF.getAbsolutePath() + ", " + activity.getString(R.string.Error)+ ": " + ex.getMessage());
        } finally {
        }
        return filePath;
    }


    public static String getFileUriFromIntent(Activity activity, Intent data, boolean write) {
        if (data == null)
            return null;

        Uri uri = data.getData();
        DocumentFile pickedFile = DocumentFile.fromSingleUri(activity, uri);
        String file = uri.toString();
        if (!file.contains("com.android.externalstorage.documents")) {
            showFileNotSupported(activity);
            return null;
        }
        activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if (write)
            activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        activity.grantUriPermission(activity.getPackageName(), uri, Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

        int takeFlags = data.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if (write)
            takeFlags = takeFlags | Intent.FLAG_GRANT_WRITE_URI_PERMISSION;

        activity.getContentResolver().takePersistableUriPermission(uri, takeFlags);
        return file;
    }

    public static String getDirPathFromIntent(Activity activity, Intent data) {
        if (data == null)
            return null;
        Bundle b = data.getExtras();
        String file = b.getString("currDir");
        return file;
    }


    public static String getFilePathFromIntent(Activity activity, Intent data) {
        if (data == null)
            return null;
        Bundle b = data.getExtras();
        String file = b.getString("file");
        return file;
    }

    public static FileType getFileTypeFromIntent(Activity activity, Intent data) {
        if (data == null)
            return null;
        Bundle b = data.getExtras();
        FileType fileType = (FileType) b.getSerializable("fileType");
        return fileType;
    }

    public static void showFileNotSupported(Activity context) {
        DialogUtils.UIAlert(context, context.getString(R.string.Error), context.getString(R.string.FilePathNotSupportedWarning));
    }

    public static void saveLogToFile(final Activity activity, final String logFileDestDir) {
        new Thread(new Runnable() {
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

                if (displayName != null) {

                    ToastUtils.toastShort(activity, activity.getString(R.string.LogfileSaved));
                }
            }
        }).start();
    }

    public static String convertFilePath(String text, int position) {
        try {
            text = FileUtils.getFullPathFromDocumentFilePath(text);
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }

        return text;
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

    public static String createImgFromTemplate(Context context, String templateImage, String destImage, FileType imgType) {

        String imagesDir = LimboSettingsManager.getImagesDir(context);
        String displayName = null;
        String filePath = null;
        if (imagesDir.startsWith("content://")) {
            Uri imagesDirUri = Uri.parse(imagesDir);
            Uri fileCreatedUri = FileInstaller.installImageTemplateToSDCard(context, templateImage,
                    imagesDirUri, "hdtemplates", destImage);
            if (fileCreatedUri != null) {
                displayName = FileUtils.getFullPathFromDocumentFilePath(fileCreatedUri.toString());
                filePath = fileCreatedUri.toString();
            }
        } else {
            filePath = FileInstaller.installImageTemplateToExternalStorage(context, templateImage, imagesDir, "hdtemplates", destImage);
            displayName = filePath;
        }
        if (displayName != null) {
            ToastUtils.toastShort(context, context.getString(R.string.ImageCreated) + ": " + displayName);
            return filePath;
        }
        return null;
    }
}
