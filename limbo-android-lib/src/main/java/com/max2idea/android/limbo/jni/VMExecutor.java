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
package com.max2idea.android.limbo.jni;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.machine.MachineController;
import com.max2idea.android.limbo.machine.MachineExecutor;
import com.max2idea.android.limbo.machine.MachineProperty;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboApplication;
import com.max2idea.android.limbo.main.LimboSDLActivity;
import com.max2idea.android.limbo.main.LimboSettingsManager;
import com.max2idea.android.limbo.qmp.QmpClient;
import com.max2idea.android.limbo.toast.ToastUtils;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;


/**
 * Class is used to start and stop the qemu process and communicate file descriptions, mouse,
 * and keyboard events.
 */
class VMExecutor extends MachineExecutor {
    private static final String TAG = "VMExecutor";

    private static final String cdDeviceName = "ide1-cd0";
    private static final String fdaDeviceName = "floppy0";
    private static final String fdbDeviceName = "floppy1";
    private static final String sdDeviceName = "sd0";

    VMExecutor(MachineController machineController) {
        super(machineController);
    }

    /**
     * This function is called when the machine resolution changes. This is called from SDL compat
     * extensions, see folder jni/compat/sdl-extensions
     *
     * @param width  Width
     * @param height Height
     */
    public static void onVMResolutionChanged(int width, int height) {
        LimboSDLActivity.getSingleton().onVMResolutionChanged(width, height);
    }

    //JNI Methods
    private native String start(String storage_dir, String base_dir, String lib_path, int sdl_scale_hint, Object[] params, int paused, String save_state_name);

    private native String stop(int restart);

    public native void setsdlrefreshrate(int value);

    public native int getsdlrefreshrate();

    public native int onmouse(int button, int action, int relative, float x, float y);

    /**
     * Prints parameters in qemu format
     *
     * @param params Parameters to be printed
     */
    public void printParams(String[] params) {
        Log.d(TAG, "Params:");
        for (int i = 0; i < params.length; i++) {
            Log.d(TAG, i + ": " + params[i]);
        }
    }

    // Translate to QEMU format
    private String getSoundCard() {
        if (Config.enableSDLSound && getMachine().getSoundCard() != null
                && !getMachine().getSoundCard().toLowerCase().equals("none"))
            return getMachine().getSoundCard();
        return null;
    }

    private String getQemuLibrary(Context context) {
        if (LimboApplication.arch == Config.Arch.x86_64) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-x86_64.so";
        } else if (LimboApplication.arch == Config.Arch.x86) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-i386.so";
        } else if (LimboApplication.arch == Config.Arch.arm) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-arm.so";
        } else if (LimboApplication.arch == Config.Arch.arm64) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-aarch64.so";
        } else if (LimboApplication.arch == Config.Arch.ppc) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-ppc.so";
        } else if (LimboApplication.arch == Config.Arch.ppc64) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-ppc64.so";
        } else if (LimboApplication.arch == Config.Arch.sparc) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-sparc.so";
        } else if (LimboApplication.arch == Config.Arch.sparc64) {
            return FileUtils.getNativeLibDir(context) + "/libqemu-system-sparc64.so";
        }
        return null;
    }

    private String getSaveStateName() {
        String machineSaveDirectory = MachineController.getInstance().getMachineSaveDir();
        return machineSaveDirectory + "/" + Config.stateFilename;
    }

    private String[] prepareParams(Context context) throws Exception {
        ArrayList<String> paramsList = new ArrayList<>();
        paramsList.add(getQemuLibrary(context));
        addUIOptions(context, paramsList);
        addCpuBoardOptions(paramsList);
        addDrives(paramsList);
        addRemovableDrives(paramsList);
        addBootOptions(paramsList);
        addGraphicsOptions(paramsList);
        addAudioOptions(paramsList);
        addNetworkOptions(paramsList);
        addGenericOptions(context, paramsList);
        addStateOptions(paramsList);
        addAdvancedOptions(paramsList);
        return paramsList.toArray(new String[0]);
    }

    /**
     * Adds the vm state file description to the qemu parameters for resuming the vm
     *
     * @param paramsList Existing parameter list to be passed to qemu
     */
    private void addStateOptions(ArrayList<String> paramsList) {
        if (MachineController.getInstance().isPaused() && !getSaveStateName().equals("")) {
            int fd_tmp = FileUtils.get_fd(getSaveStateName());
            if (fd_tmp < 0) {
                Log.e(TAG, "Error while getting fd for: " + getSaveStateName());
            } else {
                //Log.i(TAG, "Got new fd "+fd_tmp + " for: " +save_state_name);
                paramsList.add("-incoming");
                paramsList.add("fd:" + fd_tmp);
            }
        }
    }

    private void addUIOptions(Context context, ArrayList<String> paramsList) {
        if (MachineController.getInstance().isVNCEnabled()) {
            Log.v(TAG, "Enable VNC server");
            paramsList.add("-vnc");

            String vncParam = "";
            if (LimboSettingsManager.getVNCEnablePassword(context)) {
                //TODO: Allow connections from External
                // Use with x509 auth and TLS for encryption
                vncParam += ":1";
            } else {
                // Allow connections only from localhost using localsocket without
                // a password
                vncParam += Config.defaultVNCHost + ":" + Config.defaultVNCPort;
            }
            if (LimboSettingsManager.getVNCEnablePassword(context))
                vncParam += ",password";

            paramsList.add(vncParam);

            //Allow monitor console though it's only supported for VNC, SDL for android doesn't support
            // more than 1 window
            paramsList.add("-monitor");
            paramsList.add("vc");

        } else {
            //SDL needs explicit keyboard layout
            Log.v(TAG, "Disabling VNC server, using SDL instead");

            //XXX: monitor, serial, and parallel display crashes cause SDL doesn't support more than 1 window
            paramsList.add("-monitor");
            paramsList.add("none");

            paramsList.add("-serial");
            paramsList.add("none");

            paramsList.add("-parallel");
            paramsList.add("none");
        }

        if (getMachine().getKeyboard() != null) {
            paramsList.add("-k");
            paramsList.add(getMachine().getKeyboard());
        }

        if (getMachine().getMouse() != null && !getMachine().getMouse().equals("ps2")) {
            paramsList.add("-usb");
            paramsList.add("-device");
            paramsList.add(getMachine().getMouse());
        }
    }

    private void addAdvancedOptions(ArrayList<String> paramsList) {

        if (getMachine().getExtraParams() != null && !getMachine().getExtraParams().trim().equals("")) {
            String[] paramsTmp = getMachine().getExtraParams().split(" ");
            paramsList.addAll(Arrays.asList(paramsTmp));
        }
    }

    private void addAudioOptions(ArrayList<String> paramsList) {
        if (getSoundCard() != null) {
            paramsList.add("-soundhw");
            paramsList.add(getSoundCard());
        }
    }

    private void addGenericOptions(Context context, ArrayList<String> paramsList) {
        paramsList.add("-L");
        paramsList.add(LimboApplication.getBasefileDir());
        if (LimboSettingsManager.getEnableQmp(context)) {
            paramsList.add("-qmp");
            if (getQMPAllowExternal()) {
                String qmpParams = "tcp:";
                qmpParams += (":" + Config.QMPPort);
                qmpParams += ",server,nowait";
                paramsList.add(qmpParams);
            } else {
                //Specify a unix local domain as localhost to limit to local connections only
                String qmpParams = "unix:";
                qmpParams += LimboApplication.getLocalQMPSocketPath();
                qmpParams += ",server,nowait";
                paramsList.add(qmpParams);
            }
        }

        //Enable Tracing log
        if (Config.enableTracingLog) {
            paramsList.add("-D");
            paramsList.add(Config.traceLogFile);
            paramsList.add("--trace");
            paramsList.add("events=" + Config.traceEventsFile);
            paramsList.add("--trace");
            paramsList.add("file=" + Config.traceDir);
        }

        if (Config.overrideTbSize) {
            paramsList.add("-tb-size");
            paramsList.add(Config.tbSize); //Don't increase it crashes
        }

        paramsList.add("-realtime");
        paramsList.add("mlock=off");

        paramsList.add("-rtc");
        paramsList.add("base=localtime");

        paramsList.add("-nodefaults");
    }

    private void addCpuBoardOptions(ArrayList<String> paramsList) {

        //XXX: SMP is not working correctly for some guest OSes
        //so we enable multi core only under KVM
        // anyway regular emulation is not gaining any benefit unless mttcg is enabled but that
        // doesn't work for x86 guests yet
        if (getMachine().getCpuNum() > 1 &&
                (getMachine().getEnableKVM() == 1 || getMachine().getEnableMTTCG() == 1 || !Config.enableSMPOnlyOnKVM)) {
            paramsList.add("-smp");
            paramsList.add(getMachine().getCpuNum() + "");
        }

        if (getMachineType() != null && !getMachineType().equals("Default")) {
            paramsList.add("-M");
            paramsList.add(getMachineType());
        }

        //FIXME: something is wrong with quoting that doesn't let sparc qemu find the cpu def
        // for now we remove the cpu drop downlist items for sparc
        String cpu = getMachine().getCpu();
        if (getMachine().getCpu() != null && getMachine().getCpu().contains(" "))
            cpu = "'" + getMachine().getCpu() + "'"; // XXX: needed for sparc cpu names

        //XXX: we disable tsc feature for x86 since some guests are kernel panicking
        // if the cpu has not specified by user we use the internal qemu32/64
        if (getMachine().getDisableTSC() == 1 && (LimboApplication.arch == Config.Arch.x86 || LimboApplication.arch == Config.Arch.x86_64)) {
            if (cpu == null || cpu.equals("Default")) {
                if (LimboApplication.arch == Config.Arch.x86)
                    cpu = "qemu32";
                else if (LimboApplication.arch == Config.Arch.x86_64)
                    cpu = "qemu64";
            }
            cpu += ",-tsc";
        }

        if (getMachine().getDisableAcpi() != 0) {
            paramsList.add("-no-acpi"); //disable ACPI
        }
        if (getMachine().getDisableHPET() != 0) {
            paramsList.add("-no-hpet"); //        disable HPET
        }

        if (cpu != null && !cpu.equals("Default")) {
            paramsList.add("-cpu");
            paramsList.add(cpu);
        }

        paramsList.add("-m");
        paramsList.add(getMachine().getMemory() + "");

        if (getMachine().getEnableKVM() != 0) {
            paramsList.add("-enable-kvm");
        } else if (getMachine().getEnableMTTCG() != 0 && LimboApplication.isHost64Bit()) {
            //FIXME: should we do this only do for 64bit hosts?
            paramsList.add("-accel");
            String tcgParams = "tcg";
            if (getMachine().getCpuNum() > 1)
                tcgParams += ",thread=multi";
            paramsList.add(tcgParams);
        }

    }

    private String getMachineType() {
        String machineType = getMachine().getMachineType();
        if ((LimboApplication.arch == Config.Arch.x86 || LimboApplication.arch == Config.Arch.x86_64)
                && machineType == null) {
            machineType = "pc";
        } else if ((LimboApplication.arch == Config.Arch.ppc || LimboApplication.arch == Config.Arch.ppc64)
                && machineType.equals("Default")) {
            machineType = null;
        } else if ((LimboApplication.arch == Config.Arch.sparc || LimboApplication.arch == Config.Arch.sparc64)
                && machineType.equals("Default")) {
            machineType = null;
        }
        return machineType;
    }

    private void addNetworkOptions(ArrayList<String> paramsList) throws Exception {

        String network = getNetCfg();
        if (network != null) {
            paramsList.add("-net");
            if (network.equals("user")) {
                String netParams = network;
                String hostFwd = getHostFwd();
                if (hostFwd != null) {

                    //hostfwd=[tcp|udp]:[hostaddr]:hostport-[guestaddr]:guestport{,hostfwd=...}
                    // example forward ssh from guest port 2222 to guest port 22:
                    // hostfwd=tcp::2222-:22
                    if (hostFwd.startsWith("hostfwd")) {
                        throw new Exception("Invalid format for Host Forward, should be: tcp:hostport1:guestport1,udp:hostport2:questport2,...");
                    }
                    String[] hostFwdParams = hostFwd.split(",");
                    for (int i = 0; i < hostFwdParams.length; i++) {
                        netParams += ",";
                        String[] hostfwdparam = hostFwdParams[i].split(":");
                        netParams += ("hostfwd=" + hostfwdparam[0] + "::" + hostfwdparam[1] + "-:" + hostfwdparam[2]);
                    }
                }
                paramsList.add(netParams);
            } else if (network.equals("tap")) {
                paramsList.add("tap,vlan=0,ifname=tap0,script=no");
            } else if (network.equals("none")) {
                paramsList.add("none");
            } else {
                //Unknown interface
                paramsList.add("none");
            }
        }

        String networkCard = getNicCard();
        if (networkCard != null) {
            paramsList.add("-net");
            String nicParams = "nic";
            if (network.equals("tap"))
                nicParams += ",vlan=0";
            if (!networkCard.equals("Default"))
                nicParams += (",model=" + networkCard);
            paramsList.add(nicParams);
        }
    }

    private String getHostFwd() {
        if (getMachine().getNetwork().equals("User")) {
            if (getMachine().getHostFwd() != null && !getMachine().getHostFwd().equals(""))
                return getMachine().getHostFwd();
        }
        return null;
    }

    private String getNicCard() {
        if (getMachine().getNetwork() == null || getMachine().getNetwork().equals("None")) {
            return null;
        } else if (getMachine().getNetwork().equals("User")) {
            return getMachine().getNetworkCard();
        } else if (getMachine().getNetwork().equals("TAP")) {
            return getMachine().getNetworkCard();
        }
        return null;
    }

    private String getNetCfg() {
        if (getMachine().getNetwork() == null || getMachine().getNetwork().equals("None")) {
            return "none";
        } else if (getMachine().getNetwork().equals("User")) {
            return "user";
        } else if (getMachine().getNetwork().equals("TAP")) {
            return "tap";
        }
        return null;
    }

    private void addGraphicsOptions(ArrayList<String> paramsList) {
        if (getMachine().getVga() != null) {
            if (getMachine().getVga().equals("Default")) {
                //do nothing
            } else if (getMachine().getVga().equals("virtio-gpu-pci")) {
                paramsList.add("-device");
                paramsList.add(getMachine().getVga());
            } else if (getMachine().getVga().equals("nographic")) {
                paramsList.add("-nographic");
            } else {
                paramsList.add("-vga");
                paramsList.add(getMachine().getVga());
            }
        }
    }

    private void addBootOptions(ArrayList<String> paramsList) {
        if (getBootDevice() != null) {
            paramsList.add("-boot");
            paramsList.add(getBootDevice());
        }

        String kernel = getKernel();
        if (kernel != null && !kernel.equals("")) {
            paramsList.add("-kernel");
            paramsList.add(kernel);
        }

        String initrd = getInitRd();
        if (initrd != null && !initrd.equals("")) {
            paramsList.add("-initrd");
            paramsList.add(initrd);
        }

        if (getMachine().getAppend() != null && !getMachine().getAppend().equals("")) {
            paramsList.add("-append");
            paramsList.add(getMachine().getAppend());
        }
    }

    private String getBootDevice() {
        if (LimboApplication.arch == Config.Arch.arm || LimboApplication.arch == Config.Arch.arm64) {
            return null;
        } else if (getMachine().getBootDevice().equals("Default")) {
            return null;
        } else if (getMachine().getBootDevice().equals("CDROM")) {
            return "d";
        } else if (getMachine().getBootDevice().equals("Floppy")) {
            return "a";
        } else if (getMachine().getBootDevice().equals("Hard Disk")) {
            return "c";
        }
        return null;
    }

    private String getInitRd() {
        return FileUtils.encodeDocumentFilePath(getMachine().getInitRd());
    }

    private String getKernel() {
        return FileUtils.encodeDocumentFilePath(getMachine().getKernel());
    }

    public String getDriveFilePath(String driveFilePath) {
        String imgPath = driveFilePath;
        if (imgPath == null || imgPath.equals("None"))
            return null;
        imgPath = FileUtils.encodeDocumentFilePath(imgPath);
        return imgPath;
    }

    public void addDrives(ArrayList<String> paramsList) {
        addHardDisk(paramsList, getDriveFilePath(getMachine().getHdaImagePath()), 0);
        addHardDisk(paramsList, getDriveFilePath(getMachine().getHdbImagePath()), 1);
        addHardDisk(paramsList, getDriveFilePath(getMachine().getHdcImagePath()), 2);
        addHardDisk(paramsList, getDriveFilePath(getMachine().getHddImagePath()), 3);
        addSharedFolder(paramsList, getDriveFilePath(getMachine().getSharedFolderPath()));
    }

    public void addHardDisk(ArrayList<String> paramsList, String imagePath, int index) {
        if (imagePath != null && !imagePath.trim().equals("")) {
            paramsList.add("-drive"); //empty
            String param = "index=" + index;
            if (Config.enableIDEInterface) {
                param += ",if=";
                param += Config.ideInterfaceType;
            } else if (Config.enableVirtioInterface && index > 0) {
                param += ",if=";
                param += Config.virtioInterfaceType;
            }
            param += ",media=disk";
            if (!imagePath.equals("")) {
                param += ",file=" + imagePath;
            }
            paramsList.add(param);
        }
    }


    public void addSharedFolder(ArrayList<String> paramsList, String sharedFolderPath) {
        if (Config.enableSharedFolder && sharedFolderPath != null) {
            //XXX; We use hdd to mount any virtual fat drives
            paramsList.add("-drive"); //empty
            String driveParams = "index=3";
            driveParams += ",media=disk";
            if (Config.enableIDEInterface) {
                driveParams += ",if=";
                driveParams += Config.ideInterfaceType;
            }
            driveParams += ",format=raw";
            driveParams += ",file=fat:";
            driveParams += "rw:"; //Always Read/Write
            driveParams += sharedFolderPath;
            paramsList.add(driveParams);
        }

    }

    public void addRemovableDrives(ArrayList<String> paramsList) {
        String cdImagePath = getDriveFilePath(getMachine().getCdImagePath());
        if (cdImagePath != null) {
            paramsList.add("-drive"); //empty
            String param = "index=2";
            if (Config.enableIDEInterface) {
                param += ",if=";
                param += Config.ideInterfaceType;
            }
            param += ",media=cdrom";
            if (!cdImagePath.equals("")) {
                param += ",file=" + cdImagePath;
            }
            paramsList.add(param);
        }

        String fdaImagePath = getDriveFilePath(getMachine().getFdaImagePath());
        if (Config.enableEmulatedFloppy && fdaImagePath != null) {
            paramsList.add("-drive"); //empty
            String param = "index=0,if=floppy";
            if (!fdaImagePath.equals("")) {
                param += ",file=" + fdaImagePath;
            }
            paramsList.add(param);
        }

        String fdbImagePath = getDriveFilePath(getMachine().getFdbImagePath());
        if (Config.enableEmulatedFloppy && fdbImagePath != null) {
            paramsList.add("-drive"); //empty
            String param = "index=1,if=floppy";
            if (!fdbImagePath.equals("")) {
                param += ",file=" + fdbImagePath;
            }
            paramsList.add(param);
        }

        String sdImagePath = getDriveFilePath(getMachine().getSdImagePath());
        if (Config.enableEmulatedSDCard && sdImagePath != null) {
            paramsList.add("-device");
            paramsList.add("sd-card,drive=sd0,bus=sd-bus");
            paramsList.add("-drive");
            String param = "if=none,id=sd0";
            if (!sdImagePath.equals("")) {
                param += ",file=" + sdImagePath;
            }
            paramsList.add(param);
        }

    }


    /**
     * change the vnc password before we connect
     * The user is also prompted to create a certificate
     *
     * @param vncPassword The VNC password to be send to QEMU
     */
    protected void vncchangepassword(String vncPassword) throws Exception {
        String res = QmpClient.sendCommand(QmpClient.getChangeVncPasswdCommand(vncPassword));
        String desc = null;
        if (res != null && !res.equals("")) {
            JSONObject resObj = new JSONObject(res);
            if (resObj != null && !resObj.equals("") && res.contains("error")) {
                String resInfo = resObj.getString("error");
                if (resInfo != null && !resInfo.equals("")) {
                    JSONObject resInfoObj = new JSONObject(resInfo);
                    desc = resInfoObj.getString("desc");
                    Log.e(TAG, desc);
                }
            }
        }
    }

    protected String changedev(String dev, String value) {
        String response = QmpClient.sendCommand(QmpClient.getChangeDeviceCommand(dev, value));
        String displayDevValue = FileUtils.getFullPathFromDocumentFilePath(value);
        if (Config.debug)
            ToastUtils.toastLong(LimboApplication.getInstance(), Gravity.BOTTOM,
                    LimboApplication.getInstance().getString(R.string.ChangedDevice) + ": "
                            + dev + ": " + displayDevValue);
        return response;
    }

    protected String ejectdev(String dev) {
        String response = QmpClient.sendCommand(QmpClient.getEjectDeviceCommand(dev));
        if (Config.debug)
            ToastUtils.toastLong(LimboApplication.getInstance(), Gravity.BOTTOM,
                    LimboApplication.getInstance().getString(R.string.EjectedDevice) + ": " + dev);
        return response;
    }


    /**
     * Starts the service that will later start the qemu process
     */
    public void startService() {
        Intent i = new Intent(Config.ACTION_START, null, LimboApplication.getInstance(),
                MachineController.getInstance().getServiceClass());
        Bundle b = new Bundle();
        i.putExtras(b);
        Log.v(TAG, "Starting VM service");
        LimboApplication.getInstance().startService(i);
    }

    /**
     * Starts the native process. This should be called from a background thread from a
     * foreground service in order to prevent the process from being killed
     *
     * @return String from the native code vm-executor-jni.cpp
     */
    public String start() {
        String res = null;
        try {
            String[] params = prepareParams(LimboApplication.getInstance());
            printParams(params);
            // XXX: for VNC we need to resume manually after a reasonable amount of time
            if (getMachine().getPaused() == 1 && MachineController.getInstance().isVNCEnabled()) {
                continueVM(5000);
            }

            if (MachineController.getInstance().isVNCEnabled() && LimboSettingsManager.getVNCEnablePassword(LimboApplication.getInstance())) {
                changeVncPass(LimboApplication.getInstance(), 2000);
            }

            res = start(Config.storagedir, LimboApplication.getBasefileDir(), getQemuLibrary(LimboApplication.getInstance()),
                    Config.SDLHintScale, params, getMachine().getPaused(), getSaveStateName());
        } catch (Exception ex) {
            ToastUtils.toastLong(LimboApplication.getInstance(), ex.getMessage());
            return res;
        }
        return res;
    }

    private void changeVncPass(final Context context, final long delay) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                try {
                    vncchangepassword(LimboSettingsManager.getVNCPass(context));
                } catch (Exception e) {
                    ToastUtils.toastLong(LimboApplication.getInstance(),
                            context.getString(R.string.CouldNotSetVNCPass) + ": " + e.getMessage());
                    e.printStackTrace();
                }
            }
        }).start();
    }

    private void continueVM(final long delay) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                continueVM();
            }
        }).start();
    }

    public void stopvm(final int restart) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (restart != 0) {
                    QmpClient.sendCommand(QmpClient.getResetCommand());
                } else {
                    //XXX: Qmp command only halts the VM but doesn't exit so we use force close
//            QmpClient.sendCommand(QmpClient.powerDown());
                    stop(restart);
                }
            }
        }).start();
    }

    @Override
    public int getSdlRefreshRate() {
        return getsdlrefreshrate();
    }

    @Override
    public void setSdlRefreshRate(int value) {
        setsdlrefreshrate(value);
    }

    @Override
    public String getDeviceName(MachineProperty driveProperty) {
        switch (driveProperty) {
            case CDROM:
                return cdDeviceName;
            case FDA:
                return fdaDeviceName;
            case FDB:
                return fdbDeviceName;
            case SD:
                return sdDeviceName;
        }
        return null;
    }


    //TODO: re-enable getting status from the vm
    public String getVmState() {
        String res = QmpClient.sendCommand(QmpClient.getStateCommand());
        String state = "";
        if (res != null && !res.equals("")) {
            try {
                JSONObject resObj = new JSONObject(res);
                String resInfo = resObj.getString("return");
                JSONObject resInfoObj = new JSONObject(resInfo);
                state = resInfoObj.getString("status");
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        return state;
    }

    /**
     * Function sends a command via qmp to change or eject the removable device
     *
     * @param drive      The device to be changed
     * @param imagePath If its null it ejects the drive otherwise it uses the disk file at that path
     */
    public boolean changeRemovableDevice(final MachineProperty drive, final String imagePath) {
        if (!LimboSettingsManager.getEnableQmp(LimboApplication.getInstance())) {
            ToastUtils.toastShort(LimboApplication.getInstance(), LimboApplication.getInstance().getString(R.string.EnableQMPForChangingDrives));
            return false;
        }
        String dev = getDeviceName(drive);

        //XXX: first we eject any previous media
        String response = VMExecutor.this.ejectdev(dev);

        // if there is no media there is nothing else to do
        if (imagePath == null || imagePath.trim().equals("")) {
            return true;
        }

        //XXX: we encode some characters from the document file path so it's processed
        // correctly by qemu
        String imagePathConverted = FileUtils.encodeDocumentFilePath(imagePath);

        if (!FileUtils.fileValid(imagePathConverted)) {
            String msg = LimboApplication.getInstance().getString(R.string.CouldNotOpenDocFile) + " "
                    + FileUtils.getFullPathFromDocumentFilePath(imagePathConverted)
                    + "\n" + LimboApplication.getInstance().getString(R.string.PleaseReassingYourDiskFiles);
            ToastUtils.toastLong(LimboApplication.getInstance(), msg);
            return false;
        }
        response = VMExecutor.this.changedev(dev, imagePathConverted);
        if (response == null)
            return false;

        return true;
    }

    /**
     * Fuction is a pass thru from the c get_fd() function called from native code
     * This is bridged to the java code because it's the only way to open a file descriptor
     * from the native code
     *
     * @param path File path
     * @return Return value of FileUtils.get_fd()
     */
    public int get_fd(String path) {
        return FileUtils.get_fd(path);
    }

    /**
     * Fuction is a pass thru from the c close_fd() function called from native code
     * This is similar to the above get_fd but perhaps not needed.
     *
     * @param fd File Descriptor to be closed
     * @return Return value of FileUtils.close_fd()
     */
    public int close_fd(int fd) {
        return FileUtils.close_fd(fd);
    }

    @Override
    public String saveVM() {

        // Delete any previous state file
        File file = new File(getSaveStateName());
        if (file.exists()) {
            if (!file.delete()) {
                return LimboApplication.getInstance().getString(R.string.CannotDeletePreviousStateFile);
            }
        }

        if (Config.showToast)
            ToastUtils.toastShort(LimboApplication.getInstance(), LimboApplication.getInstance().getString(R.string.PleaseWaitSavingVMState));

        int currentFd = get_fd(getSaveStateName());
        String uri = "fd:" + currentFd;
        String command = QmpClient.getStopVMCommand();
        String msg = QmpClient.sendCommand(command);
        command = QmpClient.getMigrateCommand(false, false, uri);
        msg = QmpClient.sendCommand(command);
        if (msg != null) {
            return processMigrationResponse(msg);
        }
        return null;
    }

    @Override
    public void continueVM() {
        String command = QmpClient.getContinueVMCommand();
        QmpClient.sendCommand(command);
    }

    @Override
    public MachineController.MachineStatus getSaveVMStatus() {
        String pauseState = "";
        String command = QmpClient.getQueryMigrationCommand();
        String res = QmpClient.sendCommand(command);

        if (res != null && !res.equals("")) {
            try {
                JSONObject resObj = new JSONObject(res);
                String resInfo = resObj.getString("return");
                JSONObject resInfoObj = new JSONObject(resInfo);
                pauseState = resInfoObj.getString("status");
            } catch (JSONException e) {
                if (Config.debug)
                    Log.e(TAG, "Error while checking saving vm: " + e.getMessage());
            }
            if (pauseState.toUpperCase().equals("FAILED")) {
                Log.e(TAG, "Error: " + res);
            }
        }
        if (pauseState.toUpperCase().equals("ACTIVE")) {
            return MachineController.MachineStatus.Saving;
        } else if (pauseState.toUpperCase().equals("COMPLETED")) {
            return MachineController.MachineStatus.SaveCompleted;
        } else if (pauseState.toUpperCase().equals("FAILED")) {
            return MachineController.MachineStatus.SaveFailed;
        }
        return MachineController.MachineStatus.Unknown;
    }

    private String processMigrationResponse(String response) {
        String errorStr = null;
        try {
            JSONObject object = new JSONObject(response);
            errorStr = object.getString("error");
        } catch (Exception ex) {
            if (Config.debug)
                ex.printStackTrace();
        }
        if (errorStr != null) {
            String descStr = null;

            try {
                JSONObject descObj = new JSONObject(errorStr);
                descStr = descObj.getString("desc");
            } catch (Exception ex) {
                if (Config.debug)
                    ex.printStackTrace();
            }
            return descStr;
        }
        return null;
    }

    public int sendMouseEvent(int button, int action, int relative, float x, float y) {
        //XXX: Make sure that mouse motion is not triggering crashes in SDL while resizing
        if (LimboSDLActivity.isResizing) {
            return -1;
        }

        //XXX: Check boundaries, perhaps not necessary since SDL is also doing the same thing
        if (relative == 1
                || (x >= 0 && x <= LimboSDLActivity.getSingleton().vm_width && y >= 0 && y <= LimboSDLActivity.getSingleton().vm_height)
                || (action == MotionEvent.ACTION_SCROLL)) {
//            Log.v(TAG, "onmouse button: " + button + ", action: " + action + ", relative: " + relative + ", x = " + x + ", y = " + y);
            return onmouse(button, action, relative, x, y);
        }
        return -1;
    }

    public boolean getQMPAllowExternal() {
        return LimboSettingsManager.getEnableExternalQMP(LimboApplication.getInstance());
    }
}

