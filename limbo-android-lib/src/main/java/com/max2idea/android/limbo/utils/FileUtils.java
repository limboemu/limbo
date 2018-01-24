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
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.Toast;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Hashtable;

/**
 * 
 * @author dev
 */
public class FileUtils {

	public String LoadFile(Activity activity, String fileName, boolean loadFromRawFolder) throws IOException {
		// Create a InputStream to read the file into
		InputStream iS;
		if (loadFromRawFolder) {
			// get the resource id from the file name
			int rID = activity.getResources().getIdentifier(getClass().getPackage().getName() + ":raw/" + fileName,
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
		while((bytesRead = iS.read(buffer)) > 0) {
            oS.write(buffer);
        }
		oS.close();
		iS.close();

		// return the output stream as a String
		return oS.toString();
	}

	public static void saveFileContents(String dBFile, String machinesToExport) {
		// TODO Auto-generated method stub
		byteArrayToFile(machinesToExport.getBytes(), new File(dBFile));
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

	public static ArrayList<Machine> getVMs(String dBFile) {
		// TODO Auto-generated method stub
		ArrayList<Machine> machines = new ArrayList<Machine>();
		BufferedReader buffreader = null;
		InputStream instream = null;
		// Read machines from csv file
		try {
			// open the file for reading
			instream = new FileInputStream(Config.DBFile);

			// if file the available for reading
			if (instream != null) {
				// prepare the file for reading
				InputStreamReader inputreader = new InputStreamReader(instream);
				buffreader = new BufferedReader(inputreader);

				String line;

				String[] machineAttrNames = { "MACHINE_NAME", "UI", "PAUSED", "ARCH", "MACHINETYPE", "CPU", "CPUNUM",
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
					String machineAttr[] = line.split(",");
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
						else if (attrs.get(i).equals("HDA"))
							mach.hda_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("HDB"))
							mach.hdb_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("HDC"))
							mach.hdc_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("HDD"))
							mach.hdd_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("SHARED_FOLDER"))
							mach.shared_folder = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("SHARED_FOLDER_MODE"))
							mach.shared_folder_mode = Integer.parseInt(machineAttr[i].replace("\"", ""));

						// Removable Media
						else if (attrs.get(i).equals("CDROM"))
							mach.cd_iso_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("FDA"))
							mach.fda_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("FDB"))
							mach.fdb_img_path = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("SD"))
							mach.sd_img_path = machineAttr[i].replace("\"", "");

						// Misc
						else if (attrs.get(i).equals("VGA"))
							mach.vga_type = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("SOUNDCARD"))
							mach.soundcard = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("NETCONFIG"))
							mach.net_cfg = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("NICCONFIG"))
							mach.nic_driver = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("HOSTFWD"))
							mach.hostfwd = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("GUEST_FWD"))
							mach.guestfwd = machineAttr[i].replace("\"", "");

						// Depreacated
						else if (attrs.get(i).equals("HDCONFIG"))
							mach.hd_cache = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("SNAPSHOT_NAME"))
							mach.snapshot_name = machineAttr[i].replace("\"", "");
						// Other
						else if (attrs.get(i).equals("DISABLE_ACPI"))
							mach.disableacpi = Integer.parseInt(machineAttr[i].replace("\"", ""));
						else if (attrs.get(i).equals("DISABLE_HPET"))
							mach.disablehpet = Integer.parseInt(machineAttr[i].replace("\"", ""));
						else if (attrs.get(i).equals("DISABLE_FD_BOOT_CHK"))
							mach.disablefdbootchk = Integer.parseInt(machineAttr[i].replace("\"", ""));
						// else if(attrs.get(i).equals("DISABLE_TSC"))
						// mach.disablefdbootchk=Integer.parseInt(machineAttr[i].replace("\"",
						// ""));
						// else
						// if(attrs.get(i).equals("ENABLE_USBMOUSE"))
						// //Disable for now

						// Boot Settings
						else if (attrs.get(i).equals("BOOT_CONFIG"))
							mach.bootdevice = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("KERNEL"))
							mach.kernel = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("INITRD"))
							mach.initrd = machineAttr[i].replace("\"", "");
						else if (attrs.get(i).equals("APPEND"))
							mach.append = machineAttr[i].replace("\"", "");

						// Extra Params
						else if (attrs.get(i).equals("EXTRA_PARAMS"))
							mach.extra_params = machineAttr[i].replace("\"", "");

					}
					machines.add(mach);
				}

			}
		} catch (Exception ex) {
			// print stack trace.
			Log.v("Import", "Error:" + ex.getMessage());
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

	public static String getDataDir() {
		
		String dataDir = LimboActivity.activity.getApplicationInfo().dataDir;
		PackageManager m = LimboActivity.activity.getPackageManager();
		String packageName = LimboActivity.activity.getPackageName();
		Log.v("VMExecutor", "Found packageName: " + packageName);
	
		if(dataDir == null) {
			dataDir = "/data/data/" + packageName;
		}
		return dataDir;
	}


		public static String getFileContents(String filePath) {
			String contents = "";
			ArrayList<Machine> machines = new ArrayList<Machine>();
			BufferedReader buffreader = null;
			InputStream instream = null;
			
			try {
				// open the file for reading
				instream = new FileInputStream(filePath);

				// if file the available for reading
				if (instream != null) {
					// prepare the file for reading
					InputStreamReader inputreader = new InputStreamReader(instream);
					buffreader = new BufferedReader(inputreader);

					String line;
					line = buffreader.readLine();

					while (line != null) {
						line = buffreader.readLine();
						if (line == null)
							break;
						contents += (line + "\n");
					}

				}
			} catch (Exception ex) {
				Log.v("GetFileContents", "Error:" + ex.getMessage());
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

			return contents;
		}

    public static void viewLimboLog(Activity activity) {

        try {
            Intent intent = new Intent(Intent.ACTION_EDIT);
            File file = new File(Config.logFilePath);
            Uri uri = Uri.fromFile(file);
            intent.setDataAndType(uri, "text/plain");
            activity.startActivity(intent);
        }catch (Exception ex) {
            UIUtils.toastShort(activity, "Could not find a Text Viewer on your device");
        }


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

    public static HashMap<Integer, ParcelFileDescriptor> fds = new HashMap<Integer, ParcelFileDescriptor>();

    public static int get_fd(final Context context, String path) {
        int fd = 0;
        if (path == null)
            return 0;

        if (path.startsWith("/content") || path.startsWith("content://")) {
            path = path.replaceFirst("/content", "content:");

            try {
                ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(Uri.parse(path), "rw");
                fd = pfd.getFd();
                fds.put(fd, pfd);
            } catch (final FileNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(context, "Error: " + e, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        } else {
            try {
                File file = new File(path);
                if (!file.exists())
                    file.createNewFile();
                ParcelFileDescriptor pfd = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_WRITE_ONLY);
                fd = pfd.getFd();
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
        return fd;
    }


    public static int close_fd(int fd) {

        if (FileUtils.fds.containsKey(fd)) {
            ParcelFileDescriptor pfd = FileUtils.fds.get(fd);
            try {
                pfd.close();
                FileUtils.fds.remove(fd);
                return 0; // success for Native side
            } catch (IOException e) {
                e.printStackTrace();
            }

        }
        return -1;
    }


}
