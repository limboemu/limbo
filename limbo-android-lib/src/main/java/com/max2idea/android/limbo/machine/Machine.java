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

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboApplication;

import java.util.Observable;

/**
 * Class holds the machine properties. This class can notify any Observer for changes.
 */
// TODO: This class is QEM-gnostic it should be abstracted
public class Machine extends Observable {
    private static String TAG = "Machine";

    private String name;
    private String keyboard = Config.defaultKeyboardLayout;
    private String mouse = "ps2";
    private int enableVNC;
    private String arch;
    private String machineType;
    private String cpu = "Default";
    private int cpuNum = 1;
    private int memory = 128;
    private int enableMTTCG;
    private int enableKVM;
    private int disableACPI = 0;
    private int disableHPET = 0;
    private int disableFdBootChk = 0;
    private int disableTSC = 1; //disabling TSC by default
    // Storage
    private String hdaImagePath;
    private String hdbImagePath;
    private String hdcImagePath;
    private String hddImagePath;
    // HDD interface
    private String hdaInterface = "ide";
    private String hdbInterface = "ide";
    private String hdcInterface = "ide";
    private String hddInterface = "ide";

    private String sharedFolderPath;
    //Removable devices
    private boolean enableCDROM;
    private boolean enableFDA;
    private boolean enableFDB;
    private boolean enableSD;
    private String cdImagePath;
    private String fdaImagePath;
    private String fdbImagePath;
    private String sdImagePath;
    private String cdInterface = "ide";
    // Default Settings
    private String bootDevice = "Default";
    private String kernel;
    private String initRd;
    private String append;
    // net
    private String network = null;
    private String networkCard = "ne2k_pci";
    private String guestFwd;
    private String hostFwd;
    //display
    private String vga = "std";
    //sound
    private String soundCard = null;
    //extra qemu params
    private String extraParams;
    private int paused;
    private int sharedFolderMode;

    public Machine(String name, boolean loadDefaults) {
        this.name = name;
        if (loadDefaults)
            setDefaults();
    }

    public String getName() {
        return name;
    }

    void setName(String name) {
        this.name = name;
    }

    public int getEnableVNC() {
        return enableVNC;
    }

    void setEnableVNC(int enableVNC) {
        if (this.enableVNC != enableVNC) {
            this.enableVNC = enableVNC;
            setChanged();
            notifyChanged(MachineProperty.UI, enableVNC);
        }
    }

    public String getArch() {
        return arch;
    }

    void setArch(String arch) {
        if (this.arch == null || !this.arch.equals(arch)) {
            this.arch = arch;
            setChanged();
            notifyChanged(MachineProperty.ARCH, arch);
        }
    }

    public String getMachineType() {
        return machineType;
    }

    void setMachineType(String machineType) {
        if (this.machineType == null || !this.machineType.equals(machineType)) {
            this.machineType = machineType;
            setChanged();
            notifyChanged(MachineProperty.MACHINETYPE, machineType);
        }
    }

    public String getCpu() {
        return cpu;
    }

    void setCpu(String cpu) {
        if (!this.cpu.equals(cpu)) {
            this.cpu = cpu;
            setChanged();
            notifyChanged(MachineProperty.CPU, cpu);
        }
    }

    public int getCpuNum() {
        return cpuNum;
    }

    void setCpuNum(int cpuNum) {
        if (this.cpuNum != cpuNum) {
            this.cpuNum = cpuNum;
            setChanged();
            notifyChanged(MachineProperty.CPUNUM, cpuNum);
        }
    }

    public int getMemory() {
        return memory;
    }

    void setMemory(int memory) {
        if (this.memory != memory) {
            this.memory = memory;
            setChanged();
            notifyChanged(MachineProperty.MEMORY, memory);
        }
    }

    public int getEnableMTTCG() {
        return enableMTTCG;
    }

    void setEnableMTTCG(int enableMTTCG) {
        if (this.enableMTTCG != enableMTTCG) {
            this.enableMTTCG = enableMTTCG;
            setChanged();
            notifyChanged(MachineProperty.ENABLE_MTTCG, enableMTTCG);
        }
    }

    public int getEnableKVM() {
        return enableKVM;
    }

    void setEnableKVM(int enableKVM) {
        if (this.enableKVM != enableKVM) {
            this.enableKVM = enableKVM;
            setChanged();
            notifyChanged(MachineProperty.ENABLE_KVM, enableKVM);
        }
    }

    public int getDisableACPI() {
        return disableACPI;
    }

    void setDisableACPI(int disableACPI) {
        if (this.disableACPI != disableACPI) {
            this.disableACPI = disableACPI;
            setChanged();
            notifyChanged(MachineProperty.DISABLE_ACPI, disableACPI);
        }

    }

    public int getDisableFdBootChk() {
        return disableFdBootChk;
    }

    void setDisableFdBootChk(int disableFdBootChk) {
        if (this.disableFdBootChk != disableFdBootChk) {
            this.disableFdBootChk = disableFdBootChk;
            setChanged();
            notifyChanged(MachineProperty.DISABLE_FD_BOOT_CHK, disableFdBootChk);
        }

    }

    public int getDisableTSC() {
        return disableTSC;
    }

    void setDisableTSC(int disableTSC) {
        if (this.disableTSC != disableTSC) {
            this.disableTSC = disableTSC;
            setChanged();
            notifyChanged(MachineProperty.DISABLE_TSC, disableTSC);
        }

    }

    public String getHdaImagePath() {
        return hdaImagePath;
    }

    void setHdaImagePath(String hdaImagePath) {
        if (this.hdaImagePath == null || !this.hdaImagePath.equals(hdaImagePath)) {
            this.hdaImagePath = hdaImagePath;
            setChanged();
            notifyChanged(MachineProperty.HDA, hdaImagePath);
        }
    }

    public String getHdbImagePath() {
        return hdbImagePath;
    }

    void setHdbImagePath(String hdbImagePath) {
        if (this.hdbImagePath == null || !this.hdbImagePath.equals(hdbImagePath)) {
            this.hdbImagePath = hdbImagePath;
            setChanged();
            notifyChanged(MachineProperty.HDB, hdbImagePath);
        }
    }

    public String getHdcImagePath() {
        return hdcImagePath;
    }

    void setHdcImagePath(String hdcImagePath) {
        if (this.hdcImagePath == null || !this.hdcImagePath.equals(hdcImagePath)) {
            this.hdcImagePath = hdcImagePath;
            setChanged();
            notifyChanged(MachineProperty.HDC, hdcImagePath);
        }

    }

    public String getHddImagePath() {
        return hddImagePath;
    }

    void setHddImagePath(String hddImagePath) {
        if (this.hddImagePath == null || !this.hddImagePath.equals(hddImagePath)) {
            this.hddImagePath = hddImagePath;
            setChanged();
            notifyChanged(MachineProperty.HDD, hddImagePath);
        }

    }

    public String getHdaInterface() {
        return hdaInterface;
    }

    void setHdaInterface(String hdInterface) {
        if (this.hdaInterface == null || !this.hdaInterface.equals(hdInterface)) {
            this.hdaInterface = hdInterface;
            setChanged();
            notifyChanged(MachineProperty.HDA_INTERFACE, hdInterface);
        }
    }

    public String getHdbInterface() {
        return hdbInterface;
    }

    void setHdbInterface(String hdInterface) {
        if (this.hdbInterface == null || !this.hdbInterface.equals(hdInterface)) {
            this.hdbInterface = hdInterface;
            setChanged();
            notifyChanged(MachineProperty.HDB_INTERFACE, hdInterface);
        }
    }

    public String getHdcInterface() {
        return hdcInterface;
    }

    void setHdcInterface(String hdInterface) {
        if (this.hdcInterface == null || !this.hdcInterface.equals(hdInterface)) {
            this.hdcInterface = hdInterface;
            setChanged();
            notifyChanged(MachineProperty.HDC_INTERFACE, hdInterface);
        }
    }

    public String getHddInterface() {
        return hddInterface;
    }

    void setHddInterface(String hdInterface) {
        if (this.hddInterface == null || !this.hddInterface.equals(hdInterface)) {
            this.hddInterface = hdInterface;
            setChanged();
            notifyChanged(MachineProperty.HDD_INTERFACE, hdInterface);
        }
    }

    public String getSharedFolderPath() {
        return sharedFolderPath;
    }

    void setSharedFolderPath(String sharedFolderPath) {
        if (this.sharedFolderPath == null || !this.sharedFolderPath.equals(sharedFolderPath)) {
            this.sharedFolderPath = sharedFolderPath;
            setChanged();
            notifyChanged(MachineProperty.SHARED_FOLDER, sharedFolderPath);
        }

    }

    public boolean isEnableCDROM() {
        return enableCDROM;
    }

    void setEnableCDROM(boolean enableCDROM) {
        if (this.enableCDROM != enableCDROM) {
            this.enableCDROM = enableCDROM;
            setChanged();
            notifyChanged(MachineProperty.OTHER, enableCDROM);
        }
    }

    public boolean isEnableFDA() {
        return enableFDA;
    }

    void setEnableFDA(boolean enableFDA) {
        if (this.enableFDA != enableFDA) {
            this.enableFDA = enableFDA;
            setChanged();
            notifyChanged(MachineProperty.OTHER, enableFDA);
        }

    }

    public boolean isEnableFDB() {
        return enableFDB;
    }

    void setEnableFDB(boolean enableFDB) {
        if (this.enableFDB != enableFDB) {
            this.enableFDB = enableFDB;
            setChanged();
            notifyChanged(MachineProperty.OTHER, enableFDB);
        }

    }

    public boolean isEnableSD() {
        return enableSD;
    }

    void setEnableSD(boolean enableSD) {
        if (this.enableSD != enableSD) {
            this.enableSD = enableSD;
            setChanged();
            notifyChanged(MachineProperty.OTHER, enableSD);
        }

    }

    public String getCdImagePath() {
        return cdImagePath;
    }

    void setCdImagePath(String cdImagePath) {

        if (this.cdImagePath == null || !this.cdImagePath.equals(cdImagePath)) {
            this.cdImagePath = cdImagePath;
            setChanged();
            notifyChanged(MachineProperty.CDROM, cdImagePath);
        }
    }

    public String getFdaImagePath() {
        return fdaImagePath;
    }

    void setFdaImagePath(String fdaImagePath) {
        if (this.fdaImagePath == null || !this.fdaImagePath.equals(fdaImagePath)) {
            this.fdaImagePath = fdaImagePath;
            setChanged();
            notifyChanged(MachineProperty.FDA, fdaImagePath);
        }

    }

    public String getFdbImagePath() {
        return fdbImagePath;
    }

    void setFdbImagePath(String fdbImagePath) {
        if (this.fdbImagePath == null || !this.fdbImagePath.equals(fdbImagePath)) {
            this.fdbImagePath = fdbImagePath;
            setChanged();
            notifyChanged(MachineProperty.FDB, fdbImagePath);
        }
    }

    public String getSdImagePath() {
        return sdImagePath;
    }

    void setSdImagePath(String sdImagePath) {
        if (this.sdImagePath == null || !this.sdImagePath.equals(sdImagePath)) {
            this.sdImagePath = sdImagePath;
            setChanged();
            notifyChanged(MachineProperty.SD, sdImagePath);
        }

    }

    public String getCDInterface() {
        return cdInterface;
    }

    void setCdInterface(String mediaInterface) {
        if (this.cdInterface == null || !this.cdInterface.equals(mediaInterface)) {
            this.cdInterface = mediaInterface;
            setChanged();
            notifyChanged(MachineProperty.CDROM_INTERFACE, mediaInterface);
        }
    }

    public String getBootDevice() {
        return bootDevice;
    }

    void setBootDevice(String bootDevice) {
        if (this.bootDevice == null || !this.bootDevice.equals(bootDevice)) {
            this.bootDevice = bootDevice;
            setChanged();
            notifyChanged(MachineProperty.BOOT_CONFIG, bootDevice);
        }
    }

    public String getKernel() {
        return kernel;
    }

    void setKernel(String kernel) {
        if (this.kernel == null || !this.kernel.equals(kernel)) {
            this.kernel = kernel;
            setChanged();
            notifyChanged(MachineProperty.KERNEL, kernel);
        }

    }

    public String getInitRd() {
        return initRd;
    }

    void setInitRd(String initRd) {
        if (this.initRd == null || !this.initRd.equals(initRd)) {
            this.initRd = initRd;
            setChanged();
            notifyChanged(MachineProperty.INITRD, initRd);
        }

    }

    public String getAppend() {
        return append;
    }

    void setAppend(String append) {
        if (this.append == null || !this.append.equals(append)) {
            this.append = append;
            setChanged();
            notifyChanged(MachineProperty.APPEND, append);
        }

    }

    public String getNetwork() {
        return network;
    }

    void setNetwork(String network) {
        if (this.network == null || !this.network.equals(network)) {
            this.network = network;
            setChanged();
            notifyChanged(MachineProperty.NETCONFIG, network);
        }

    }

    public String getNetworkCard() {
        return networkCard;
    }

    void setNetworkCard(String networkCard) {
        if (this.networkCard == null || !this.networkCard.equals(networkCard)) {
            this.networkCard = networkCard;
            setChanged();
            notifyChanged(MachineProperty.NICCONFIG, networkCard);
        }

    }

    public String getGuestFwd() {
        return guestFwd;
    }

    void setGuestFwd(String guestFwd) {
        if (this.guestFwd == null || !this.guestFwd.equals(guestFwd)) {
            this.guestFwd = guestFwd;
            setChanged();
            notifyChanged(MachineProperty.GUESTFWD, guestFwd);
        }

    }

    public String getHostFwd() {
        return hostFwd;
    }

    void setHostFwd(String hostFwd) {
        if (this.hostFwd == null || !this.hostFwd.equals(hostFwd)) {
            this.hostFwd = hostFwd;
            setChanged();
            notifyChanged(MachineProperty.HOSTFWD, hostFwd);
        }

    }

    public String getVga() {
        return vga;
    }

    void setVga(String vga) {
        if (this.vga == null || !this.vga.equals(vga)) {
            this.vga = vga;
            setChanged();
            notifyChanged(MachineProperty.VGA, vga);
        }

    }

    public String getExtraParams() {
        return extraParams;
    }

    void setExtraParams(String extraParams) {
        if (this.extraParams == null || !this.extraParams.equals(extraParams)) {
            this.extraParams = extraParams;
            setChanged();
            notifyChanged(MachineProperty.EXTRA_PARAMS, extraParams);
        }

    }

    public int getPaused() {
        return paused;
    }

    void setPaused(int value) {
        if (this.paused != value) {
            this.paused = value;
            setChanged();
            notifyChanged(MachineProperty.PAUSED, paused);
        }
    }

    public int getShared_folder_mode() {
        return sharedFolderMode;
    }

    void setShared_folder_mode(int shared_folder_mode) {
        if (this.sharedFolderMode != shared_folder_mode) {
            this.sharedFolderMode = shared_folder_mode;
            setChanged();
            notifyChanged(MachineProperty.SHARED_FOLDER_MODE, shared_folder_mode);
        }
    }

    public boolean hasRemovableDevices() {
        return enableCDROM || enableFDA || enableFDB || enableSD;
    }

    void setDefaults() {
        if (LimboApplication.arch == Config.Arch.x86 || LimboApplication.arch == Config.Arch.x86_64) {
            arch = "x86";
            cpu = "n270";
            machineType = "pc";
            networkCard = "Default";
            disableTSC = 1;
        } else if (LimboApplication.arch == Config.Arch.arm || LimboApplication.arch == Config.Arch.arm64) {
            arch = "ARM";
            machineType = "versatilepb";
            cpu = "Default";
            networkCard = "Default";
        } else if (LimboApplication.arch == Config.Arch.ppc || LimboApplication.arch == Config.Arch.ppc64) {
            arch = "PPC";
            machineType = "g3beige";
            networkCard = "Default";
        } else if (LimboApplication.arch == Config.Arch.sparc || LimboApplication.arch == Config.Arch.sparc64) {
            arch = "SPARC";
            vga = "cg3";
            machineType = "Default";
            networkCard = "Default";
        }
    }

    public String getSoundCard() {
        return soundCard;
    }

    void setSoundCard(String soundCard) {
        if (this.soundCard == null || !this.soundCard.equals(soundCard)) {
            this.soundCard = soundCard;
            setChanged();
            notifyChanged(MachineProperty.SOUNDCARD, soundCard);
        }
    }

    public String getKeyboard() {
        return keyboard;
    }

    void setKeyboard(String keyboard) {
        if (this.keyboard == null || !this.keyboard.equals(keyboard)) {
            this.keyboard = keyboard;
            setChanged();
            notifyChanged(MachineProperty.MOUSE, mouse);
        }
    }

    public String getMouse() {
        return mouse;
    }

    void setMouse(String mouse) {
        if (this.keyboard == null || this.mouse != mouse) {
            this.mouse = mouse;
            setChanged();
            notifyChanged(MachineProperty.MOUSE, mouse);
        }
    }

    public int getDisableAcpi() {
        return disableACPI;
    }

    public int getDisableHPET() {
        return disableHPET;
    }

    void setDisableHPET(int disableHPET) {
        if (this.disableHPET != disableHPET) {
            this.disableHPET = disableHPET;
            setChanged();
            notifyChanged(MachineProperty.DISABLE_HPET, disableHPET);
        }

    }

    private void notifyChanged(MachineProperty property, Object value) {
        notifyObservers(new Object[]{property, value});
    }

    public enum FileType {
        CDROM, FDA, FDB, SD,
        HDA, HDB, HDC, HDD, SHARED_DIR,
        KERNEL, INITRD,
        EXPORT_DIR, IMAGE_DIR, LOG_DIR, IMPORT_FILE
    }

}
