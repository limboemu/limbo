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
package com.max2idea.android.limbo.machine;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.machine.Machine.FileType;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboFileManager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Hashtable;

public class MachineImporter {
    private static final String TAG = "MachineImporter";

    private static ArrayList<Machine> getVMsFromFile(String importFilePath) {
        ArrayList<Machine> machines = new ArrayList<>();
        BufferedReader buffreader = null;
        InputStream instream = null;
        try {
            Log.v(TAG, "Import file: " + importFilePath);
            instream = FileUtils.getStreamFromFilePath(importFilePath);
            if (instream != null) {
                InputStreamReader inputreader = new InputStreamReader(instream);
                buffreader = new BufferedReader(inputreader);
                Hashtable<Integer, String> attrs = new Hashtable<>();

                String line = buffreader.readLine();
                String [] headers = line.split(",");
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
                    String [] machineAttr = line.split(",(?=(?:[^\"]*\"[^\"]*\")*[^\"]*$)", -1);
                    Machine mach = new Machine(machineAttr[0], false);
                    for (int i = 0; i < machineAttr.length; i++) {
                        if (machineAttr[i].equals("\"null\"")) {
                            continue;
                        }
                        switch (attrs.get(i)) {
                            case "MACHINE_NAME":
                                mach.setName(machineAttr[i].replace("\"", ""));
                                break;
                            case "UI":
                                String ui = machineAttr[i].replace("\"", "");
                                if (ui.equals("VNC"))
                                    mach.setEnableVNC(1);
                                break;
                            case "PAUSED":
                                mach.setPaused(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            // Arch
                            case "ARCH":
                                mach.setArch(machineAttr[i].replace("\"", ""));
                                break;
                            case "MACHINETYPE":
                                mach.setMachineType(machineAttr[i].replace("\"", ""));
                                break;
                            case "CPU":
                                mach.setCpu(machineAttr[i].replace("\"", ""));
                                break;
                            case "CPUNUM":
                                mach.setCpuNum(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            case "MEMORY":
                                mach.setMemory(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;

                            // Storage
                            case "HDA":
                                mach.setHdaImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "HDB":
                                mach.setHdbImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "HDC":
                                mach.setHdcImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "HDD":
                                mach.setHddImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "SHARED_FOLDER":
                                mach.setSharedFolderPath(machineAttr[i].replace("\"", ""));
                                break;
                            case "SHARED_FOLDER_MODE":
                                mach.setShared_folder_mode(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;

                            // Removable Media
                            case "CDROM":
                                mach.setCdImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "FDA":
                                mach.setFdaImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "FDB":
                                mach.setFdbImagePath(machineAttr[i].replace("\"", ""));
                                break;
                            case "SD":
                                mach.setSdImagePath(machineAttr[i].replace("\"", ""));
                                break;

                            // Misc
                            case "VGA":
                                mach.setVga(machineAttr[i].replace("\"", ""));
                                break;
                            case "SOUNDCARD":
                                mach.setSoundCard(machineAttr[i].replace("\"", ""));
                                break;
                            case "NETCONFIG":
                                mach.setNetwork(machineAttr[i].replace("\"", ""));
                                break;
                            case "NICCONFIG":
                                mach.setNetworkCard(machineAttr[i].replace("\"", ""));
                                break;
                            case "HOSTFWD":
                                mach.setHostFwd(machineAttr[i].replace("\"", ""));
                                break;
                            case "GUESTFWD":
                                mach.setGuestFwd(machineAttr[i].replace("\"", ""));
                                break;

                            // Other
                            case "DISABLE_ACPI":
                                mach.setDisableACPI(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            case "DISABLE_HPET":
                                mach.setDisableHPET(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            case "DISABLE_TSC":
                                mach.setDisableTSC(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            case "DISABLE_FD_BOOT_CHK":
                                mach.setDisableFdBootChk(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;

                            // Boot Settings
                            case "BOOT_CONFIG":
                                mach.setBootDevice(machineAttr[i].replace("\"", ""));
                                break;
                            case "KERNEL":
                                mach.setKernel(machineAttr[i].replace("\"", ""));
                                break;
                            case "INITRD":
                                mach.setInitRd(machineAttr[i].replace("\"", ""));
                                break;
                            case "APPEND":
                                mach.setAppend(machineAttr[i].replace("\"", ""));
                                break;

                            // Extra Params
                            case "EXTRA_PARAMS":
                                mach.setExtraParams(machineAttr[i].replace("\"", ""));
                                break;

                            // Peripherals
                            case "MOUSE":
                                mach.setMouse(machineAttr[i].replace("\"", ""));
                                break;
                            case "KEYBOARD":
                                mach.setKeyboard(machineAttr[i].replace("\"", ""));
                                break;
                            case "ENABLE_MTTCG":
                                mach.setEnableMTTCG(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                            case "ENABLE_KVM":
                                mach.setEnableKVM(Integer.parseInt(machineAttr[i].replace("\"", "")));
                                break;
                        }

                    }
                    Log.v("CSV Parser", "Adding Machine: " + mach.getName());
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

                e.printStackTrace();
            }
            try {
                if (instream != null)
                    instream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            try {
                FileUtils.closeFileDescriptor(importFilePath);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        return machines;
    }

    public static void promptImportMachines(final Activity activity) {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.ImportMachines));

        LinearLayout mLayout = new LinearLayout(activity);
        mLayout.setOrientation(LinearLayout.VERTICAL);
        mLayout.setPadding(20, 20, 20, 20);

        TextView imageNameView = new TextView(activity);
        imageNameView.setVisibility(View.VISIBLE);
        imageNameView.setText(activity.getResources().getString(R.string.importInstructions));

        LinearLayout.LayoutParams searchViewParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mLayout.addView(imageNameView, searchViewParams);
        alertDialog.setView(mLayout);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        promptForImportDir(activity);
                    }
                });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.Cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        alertDialog.dismiss();
                    }
                });
        alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }

    private static void promptForImportDir(final Activity activity) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                LimboFileManager.browse(activity, FileType.IMPORT_FILE, Config.OPEN_IMPORT_FILE_REQUEST_CODE);
            }
        }).start();

    }

    public static ArrayList<Machine> importMachines(String importFilePath) {
        ArrayList<Machine> machines = MachineImporter.getVMsFromFile(importFilePath);
        for (Machine machine : machines) {
            if (MachineOpenHelper.getInstance().getMachine(machine.getName()) != null) {
                MachineOpenHelper.getInstance().deleteMachine(machine);
            }
            MachineOpenHelper.getInstance().insertMachine(machine);
        }
        return machines;
    }

}
