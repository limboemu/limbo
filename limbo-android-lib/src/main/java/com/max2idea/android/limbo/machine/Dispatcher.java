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

import com.max2idea.android.limbo.main.ViewListener;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Dispatcher is responsible for routing user actions and field changes to the backend machine
 * controller preventing direct write access to the model. The Activities can be notified directly by
 * the model and update their views directly.
 */
public class Dispatcher implements ViewListener {
    private static final String TAG = "Dispatcher";
    private static Dispatcher sInstance;
    private final ExecutorService dispatcher = Executors.newFixedThreadPool(1);

    public static synchronized Dispatcher getInstance() {
        if (sInstance == null) {
            sInstance = new Dispatcher();
        }
        return sInstance;
    }

    private void setDriveValue(MachineProperty diskFileType, String drivePath) {
        switch (diskFileType) {
            case HDA:
                getMachine().setHdaImagePath(drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.HDA, drivePath);
                break;
            case HDB:
                getMachine().setHdbImagePath(drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.HDB, drivePath);
                break;
            case HDC:
                getMachine().setHdcImagePath(drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.HDC, drivePath);
                break;
            case HDD:
                getMachine().setHddImagePath(drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.HDD, drivePath);
                break;
            case SHARED_FOLDER:
                getMachine().setSharedFolderPath(drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.SHARED_DIR, drivePath);
                break;
        }
        switch (diskFileType) {
            case CDROM:
                MachineController.getInstance().changeRemovableDevice(diskFileType, drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.CDROM, drivePath);
                break;
            case FDA:
                MachineFilePaths.insertRecentFilePath(Machine.FileType.FDA, drivePath);
                MachineController.getInstance().changeRemovableDevice(diskFileType, drivePath);
                break;
            case FDB:
                MachineController.getInstance().changeRemovableDevice(diskFileType, drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.FDB, drivePath);
                break;
            case SD:
                MachineController.getInstance().changeRemovableDevice(diskFileType, drivePath);
                MachineFilePaths.insertRecentFilePath(Machine.FileType.SD, drivePath);
                break;
        }

    }

    private void changeMouse(String mouseCfg) {
        String mouseDB = mouseCfg.split(" ")[0];
        getMachine().setMouse(mouseDB);
    }

    private void setMachineEnableDevice(MachineProperty machineProperty, boolean isChecked) {
        switch (machineProperty) {
            case CDROM:
                getMachine().setEnableCDROM(isChecked);
                break;
            case FDA:
                getMachine().setEnableFDA(isChecked);
                break;
            case FDB:
                getMachine().setEnableFDB(isChecked);
                break;
            case SD:
                getMachine().setEnableSD(isChecked);
                break;
        }

    }


    private void changeUi(String ui) {
        if (ui.equals("VNC"))
            getMachine().setEnableVNC(1);
        else if (ui.equals("SDL"))
            getMachine().setEnableVNC(0);
    }

    private void setCpuNum(int cpuNum) {
        getMachine().setCpuNum(cpuNum);
    }

    public void onFieldChange(final MachineProperty property, final Object value) {
        dispatcher.submit(new Runnable() {
            @Override
            public void run() {
                requestFieldChange(property, value);
            }
        });
    }

    private void requestFieldChange(MachineProperty property, Object value) {
        if (getMachine() == null)
            return;

        switch (property) {
            case DRIVE_ENABLED:
                setDriveEnabled(value);
                break;
            case NON_REMOVABLE_DRIVE:
                setDrive(value);
                break;
            case REMOVABLE_DRIVE:
                setDrive(value);
                break;
            case SOUNDCARD:
                getMachine().setSoundCard(convertString(property, value));
                break;
            case CPU:
                getMachine().setCpu((String) value);
                break;
            case MEMORY:
                getMachine().setMemory(convertInt(property, value));
                break;
            case CPUNUM:
                setCpuNum(convertInt(property, value));
                break;
            case KERNEL:
                getMachine().setKernel(convertString(property, value));
                break;
            case INITRD:
                getMachine().setInitRd(convertString(property, value));
                break;
            case APPEND:
                getMachine().setAppend(convertString(property, value));
                break;
            case BOOT_CONFIG:
                getMachine().setBootDevice(convertString(property, value));
                break;
            case NETCONFIG:
                getMachine().setNetwork(convertString(property, value));
                break;
            case NICCONFIG:
                getMachine().setNetworkCard(convertString(property, value));
                break;
            case DISABLE_HPET:
                getMachine().setDisableHPET(convertBoolean(property, value) ? 1 : 0);
                break;
            case DISABLE_TSC:
                getMachine().setDisableTSC(convertBoolean(property, value) ? 1 : 0);
                break;
            case VGA:
                getMachine().setVga(convertString(property, value));
                break;
            case DISABLE_ACPI:
                getMachine().setDisableACPI((convertBoolean(property, value) ? 1 : 0));
                break;
            case DISABLE_FD_BOOT_CHK:
                getMachine().setDisableFdBootChk((convertBoolean(property, value) ? 1 : 0));
                break;
            case ENABLE_KVM:
                getMachine().setEnableKVM((convertBoolean(property, value) ? 1 : 0));
                break;
            case ENABLE_MTTCG:
                getMachine().setEnableMTTCG((convertBoolean(property, value) ? 1 : 0));
                break;
            case HOSTFWD:
                getMachine().setHostFwd(convertString(property, value));
                break;
            case MOUSE:
                changeMouse(convertString(property, value));
                break;
            case UI:
                changeUi(convertString(property, value));
                break;
            case KEYBOARD:
                getMachine().setKeyboard(convertString(property, value));
                break;
            case MACHINETYPE:
                getMachine().setMachineType(convertString(property, value));
                break;
            case PAUSED:
                getMachine().setPaused(convertInt(property, value));
                break;
            case EXTRA_PARAMS:
                getMachine().setExtraParams(convertString(property,value));
            default:
                throw new RuntimeException("Umapped UI field: " + property);
        }
    }

    private void setDriveEnabled(Object value) {
        Object[] params = (Object[]) value;
        MachineProperty machineDriveName = (MachineProperty) params[0];
        boolean checked = (boolean) params[1];
        setMachineEnableDevice(machineDriveName, checked);
        if (checked) {
            setDriveValue(machineDriveName, "");
        } else {
            setDriveValue(machineDriveName, null);
        }
    }

    private void setDrive(Object value) {
        Object[] params = (Object[]) value;
        MachineProperty machineDriveName = (MachineProperty) params[0];
        String diskFileValue = (String) params[1];
        if (diskFileValue.equals("None") && isDriveEnabled(machineDriveName)) {
            setDriveValue(machineDriveName, "");
        } else if (diskFileValue.equals("None") || !isDriveEnabled(machineDriveName)) {
            setDriveValue(machineDriveName, null);
        } else if (isDriveEnabled(machineDriveName)) {
            setDriveValue(machineDriveName, diskFileValue);
        }
    }

    private boolean isDriveEnabled(MachineProperty property) {
        switch (property) {
            case CDROM:
                return getMachine().isEnableCDROM();
            case FDA:
                return getMachine().isEnableFDA();
            case FDB:
                return getMachine().isEnableFDB();
            case SD:
                return getMachine().isEnableSD();
            default:
                return true;
        }
    }

    @Override
    public void onAction(final MachineAction action, final Object value) {
        dispatcher.submit(new Runnable() {
            @Override
            public void run() {
                requestAction(action, value);
            }
        });

    }

    private void requestAction(MachineAction action, Object value) {
        switch (action) {
            case DELETE_VM:
                deleteVM((Machine) value);
                break;
            case CREATE_VM:
                createVM(convertString(action, value));
                break;
            case STOP_VM:
                MachineController.getInstance().stopvm();
                break;
            case START_VM:
                MachineController.getInstance().startvm();
                break;
            case PAUSE_VM:
                MachineController.getInstance().pausevm();
                break;
            case CONTINUE_VM:
                MachineController.getInstance().continueVM(convertInt(action, value));
                break;
            case RESET_VM:
                MachineController.getInstance().restartvm();
                break;
            case LOAD_VM:
                MachineController.getInstance().setStoredMachine((String) value);
                break;
            case IMPORT_VMS:
                MachineController.getInstance().importMachines(convertString(action, value));
                break;
            case SET_SDL_REFRESH_RATE:
                changeSDLRefreshRate(value);
                break;
            case SEND_MOUSE_EVENT:
                sendMouseEvent(value);
                break;
            case INSERT_FAV:
                addDriveToList(value);
                break;
            case UPDATE_NOTIFICATION:
                MachineService.getService().updateServiceNotification(
                        MachineController.getInstance().getMachine().getName() + ": "
                                + (String) value);
            case DISPLAY_CHANGED:
                displayChanged(value);
                break;
            case ENABLE_AAUDIO:
                MachineController.getInstance().enableAaudio(convertInt(action, value));
                break;
            case FULLSCREEN:
                MachineController.getInstance().setFullscreen();
                break;
        }
    }

    private void changeSDLRefreshRate(Object value) {
        Object[] params = (Object[]) value;
        int ms = (int) params[0];
        boolean idle = (boolean) params[1];
        MachineController.getInstance().setSdlRefreshRate(ms, idle);
    }

    private void displayChanged(Object value) {
        Object[] params = (Object[]) value;
        MachineController.getInstance().updateDisplay((int) params[0], (int) params[1], (int) params[2]);
    }

    private void addDriveToList(Object value) {
        Object[] params = (Object[]) value;
        Machine.FileType fileType = (Machine.FileType) params[0];
        String filePath = (String) params[1];
        MachineFilePaths.insertRecentFilePath(fileType, filePath);
    }

    private boolean deleteVM(Machine machine) {
        return MachineController.getInstance().deleteMachine(machine);
    }

    private boolean createVM(String machineName) {
        return MachineController.getInstance().createVM(machineName);
    }


    private void sendMouseEvent(Object value) {
        Object[] params = (Object[]) value;
        MachineController.getInstance().sendMouseEvent((int) params[0], (int) params[1], (int) params[2], (float) params[3], (float) params[4]);
    }

    private Machine getMachine() {
        return MachineController.getInstance().getMachine();
    }

    private String convertString(MachineProperty property, Object value) {
        if (value instanceof String) {
            return (String) value;
        } else {
            throw new RuntimeException("Unknown property value: " + value + " for: " + property);
        }
    }

    private String convertString(MachineAction action, Object value) {
        if (value instanceof String) {
            return (String) value;
        } else if (value != null) {
            throw new RuntimeException("Unknown action value: " + value + " for: " + action);
        }
        return null;
    }

    private int convertInt(MachineProperty property, Object value) {
        if (value instanceof String) {
            return Integer.parseInt((String) value);
        } else if (value instanceof Integer) {
            return (int) value;
        } else {
            throw new RuntimeException("Unknown property value: " + value + " for: " + property);
        }
    }

    private int convertInt(MachineAction action, Object value) {
        if (value instanceof String) {
            return Integer.parseInt((String) value);
        } else if (value instanceof Integer) {
            return (int) value;
        } else {
            throw new RuntimeException("Unknown action value: " + value + " for: " + action);
        }
    }

    private boolean convertBoolean(MachineProperty property, Object value) {
        if (value instanceof Boolean) {
            return (boolean) value;
        } else {
            throw new RuntimeException("Unknown property value: " + value + " for: " + property);
        }
    }

    private boolean convertBoolean(MachineAction action, Object value) {
        if (value instanceof Boolean) {
            return (boolean) value;
        } else {
            throw new RuntimeException("Unknown action value: " + value + " for: " + action);
        }
    }

}
