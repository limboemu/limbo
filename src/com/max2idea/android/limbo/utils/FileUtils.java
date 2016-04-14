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
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

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

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboActivity;

/**
 * 
 * @author dev
 */
public class FileUtils {

	public String LoadFile(Activity activity, String fileName,
			boolean loadFromRawFolder) throws IOException {
		// Create a InputStream to read the file into
		InputStream iS;
		if (loadFromRawFolder) {
			// get the resource id from the file name
			int rID = activity.getResources().getIdentifier(
					getClass().getPackage().getName() + ":raw/" + fileName,
					null, null);
			// get the file as a stream
			iS = activity.getResources().openRawResource(rID);
		} else {
			// get the file as a stream
			iS = activity.getResources().getAssets().open(fileName);
		}

		// create a buffer that has the same size as the InputStream
		byte[] buffer = new byte[iS.available()];
		// read the text file as a stream, into the buffer
		iS.read(buffer);
		// create a output stream to write the buffer into
		ByteArrayOutputStream oS = new ByteArrayOutputStream();
		// write this buffer to the output stream
		oS.write(buffer);
		// Close the Input and Output streams
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
		// Read machines from csv file
		try {
			// open the file for reading
			InputStream instream = new FileInputStream(Config.DBFile);

			// if file the available for reading
			if (instream != null) {
				// prepare the file for reading
				InputStreamReader inputreader = new InputStreamReader(instream);
				BufferedReader buffreader = new BufferedReader(inputreader);

				String line;

				String [] machineAttrNames= { "MACHINE_NAME","CPU","MEMORY","CDROM","FDA","FDB","HDA","HDB",
						"NETCONFIG","NICCONFIG","VGA","SOUNDCARD","HDCONFIG","DISABLE_ACPI",
						"DISABLE_HPET","ENABLE_USBMOUSE","SNAPSHOT_NAME","BOOT_CONFIG",
						"KERNEL","INITRD","CPUNUM","MACHINETYPE"};
				
				// read every line of the file into the line-variable, on line
				// at the time
				//Ignore headers
				line = buffreader.readLine();
				while (line != null) {
					line = buffreader.readLine();
					if(line == null) break;
					// do something with the line
//					Log.v("CSV Parser", "Line: " + line);
					
					//Parse
					String machineAttr[] = line.split(",");
					Machine mach = new Machine(machineAttr[0]);
					for(int i=0; i<machineAttr.length; i++){
						String attr = null;
						if(machineAttr[i].equals("\"null\"") ){
							continue;
						}
						
						if(machineAttrNames[i].equals("MACHINE_NAME")) mach.machinename=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("CPU")) mach.cpu=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("MEMORY")) mach.memory=Integer.parseInt(machineAttr[i].replace("\"", ""));
						else if(machineAttrNames[i].equals("CDROM")) mach.cd_iso_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("FDA")) mach.fda_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("FDB")) mach.fdb_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("HDA")) mach.hda_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("HDB")) mach.hdb_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("HDC")) mach.hdc_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("HDD")) mach.hdd_img_path=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("NETCONFIG")) mach.net_cfg=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("NICCONFIG")) mach.nic_driver=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("VGA")) mach.vga_type=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("SOUNDCARD")) mach.soundcard=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("HDCONFIG")) mach.hd_cache=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("DISABLE_ACPI")) mach.disableacpi=Integer.parseInt(machineAttr[i].replace("\"", ""));
						else if(machineAttrNames[i].equals("DISABLE_HPET")) mach.disablehpet=Integer.parseInt(machineAttr[i].replace("\"", ""));
						else if(machineAttrNames[i].equals("DISABLE_TSC")) mach.disablefdbootchk=Integer.parseInt(machineAttr[i].replace("\"", ""));
//						else if(machineAttrNames[i].equals("ENABLE_USBMOUSE")) //Disable for now
						else if(machineAttrNames[i].equals("SNAPSHOT_NAME")) mach.snapshot_name=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("BOOT_CONFIG")) mach.bootdevice=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("KERNEL")) mach.kernel=machineAttr[i].replace("\"", "");
						else if(machineAttrNames[i].equals("INITRD")) mach.initrd=machineAttr[i].replace("\"", "");
					}
					machines.add(mach);
				}

			}
		} catch (Exception ex) {
			// print stack trace.
			Log.v("Import", "Error:" + ex.getMessage());
		} finally {

		}

		return machines;
	}

	public static String getDataDir() {
		PackageManager m = LimboActivity.activity.getPackageManager();
		String packageName = LimboActivity.activity.getPackageName();
		Log.v("VMExecutor", "Found packageName: " + packageName);
		String dataDir = "";
		try {
			PackageInfo p = m.getPackageInfo(packageName, 0);
			dataDir = p.applicationInfo.dataDir;
			Log.v("VMExecutor", "Found dataDir: " + dataDir);
		} catch (NameNotFoundException e) {
			Log.w("VMExecutor",
					"Error Package name not found using /data/data/", e);
			dataDir = "/data/data/" + packageName;
		}
		return dataDir;
	}
}
