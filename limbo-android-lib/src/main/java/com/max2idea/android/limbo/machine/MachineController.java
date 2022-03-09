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

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.files.FileUtils;
import com.max2idea.android.limbo.jni.MachineExecutorFactory;
import com.max2idea.android.limbo.main.LimboApplication;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Observer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Class communicates to the qemu process via the jni bridge MachineExecutor. It is responsible for
 * starting, stopping, and retrieving he status of the virtual machine.
 */
public class MachineController {
    private static final String TAG = "MachineController";
    private static MachineController mSingleton;
    private final ExecutorService saveVmStatusExecutor = Executors.newFixedThreadPool(1);
    private final MachineExecutor machineExecutor;
    private final HashSet<OnMachineStatusChangeListener> onMachineStatusChangeListeners = new HashSet<>();
    private final HashSet<OnEventListener> onEventListeners = new HashSet<>();
    private final Class<MachineService> serviceClass;
    private final IMachineDatabase machineDatabase;
    private boolean saveVmStatusTimerQuit;
    private boolean promptedPausedVM;
    private Machine machine;

    private MachineController() {
        machineExecutor = MachineExecutorFactory.createMachineExecutor(this, MachineExecutorFactory.MachineExecutorType.QEMU);
        machineDatabase = MachineOpenHelper.getInstance();
        serviceClass = MachineService.class;
    }

    public static MachineController getInstance() {
        if (mSingleton == null) {
            mSingleton = new MachineController();
        }
        return mSingleton;
    }


    public MachineStatus getCurrStatus() {
        if(getMachine() ==null)
            return MachineStatus.Stopped;
        else if (getMachine().getPaused() == 1)
            return MachineStatus.Paused;
        else if (MachineService.getService() == null)
            return MachineController.MachineStatus.Ready;
        else if (MachineService.getService().limboThread != null)
            return MachineController.MachineStatus.Running;
        else
            return MachineController.MachineStatus.Stopped;
    }

    void continueVM(final int delay) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if(delay > 0) {
                    try {
                        Thread.sleep(delay);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                machineExecutor.continueVM();
                notifyEventListeners(Event.MachineContinued, null);
            }
        }).start();
    }

    public void addOnStatusChangeListener(OnMachineStatusChangeListener listener) {
        onMachineStatusChangeListeners.add(listener);
    }

    public void removeOnStatusChangeListener(OnMachineStatusChangeListener listener) {
        onMachineStatusChangeListeners.remove(listener);
    }

    void removeOnStatusChangeListeners() {
        onMachineStatusChangeListeners.clear();
    }

    public void addOnEventListener(OnEventListener listener) {
        onEventListeners.add(listener);
    }

    public void removeOnEventListener(OnEventListener listener) {
        onEventListeners.remove(listener);
    }

    void removeOnEventListeners() {
        onEventListeners.clear();
    }

    void stopvm() {
        machineExecutor.stopvm(0);
    }

    private void notifyMachineStatusChangeListeners(Machine machine, MachineStatus status, Object o) {
        for (OnMachineStatusChangeListener listener : onMachineStatusChangeListeners)
            listener.onMachineStatusChanged(machine, status, o);
    }

    private void notifyEventListeners(Event status, Object o) {
        for (OnEventListener listener : onEventListeners)
            listener.onEvent(machine, status, o);
    }

    void startvm() {
        machineExecutor.startService();
    }

    void restartvm() {
        new Thread(new Runnable() {
            public void run() {
                if (machineExecutor != null) {
                    Log.d(TAG, "Restarting the VM...");
                    machineExecutor.stopvm(1);
                }
            }
        }).start();
    }

    private MachineStatus checkSaveVMStatus() {
        final MachineStatus saveVmStatus = machineExecutor.getSaveVMStatus();
        if (saveVmStatus == MachineStatus.SaveCompleted) {
            saveStateVMDB();
            if (promptedPausedVM)
                stopSaveVmStatusTimer();
            getMachineExecutor().getMachine().setPaused(1);
        }
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                if (saveVmStatus == MachineStatus.SaveCompleted) {
                    MachineController.getInstance().promptedPausedVM = true;
                    notifyMachineStatusChangeListeners(machine, saveVmStatus, null);
                } else if (saveVmStatus == MachineStatus.SaveFailed) {
                    notifyMachineStatusChangeListeners(machine, saveVmStatus, null);
                }
            }
        }, 1000);
        return saveVmStatus;
    }

    private void checkSaveStatus() {
        while (!saveVmStatusTimerQuit) {
            MachineStatus status = checkSaveVMStatus();
            Log.d(TAG, "State Status: " + status);
            if (status == MachineStatus.Unknown
                    || status == MachineStatus.SaveCompleted
                    || status == MachineStatus.SaveFailed
            ) {
                Log.d(TAG, "Saving state is done: " + status);
                stopSaveVmStatusTimer();
                return;
            }
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
        Log.d(TAG, "Save state complete");
    }

    private void stopSaveVmStatusTimer() {
        saveVmStatusTimerQuit = true;
    }

    private void saveVmStatusTimerLoop() {
        while (!saveVmStatusTimerQuit) {
            checkSaveStatus();
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
    }

    private void execSaveVmStatusTimer() {
        saveVmStatusExecutor.submit(new Runnable() {
            public void run() {
                startSaveVmStatusTimer();
            }
        });
    }

    private void startSaveVmStatusTimer() {
        stopSaveVmStatusTimer();
        saveVmStatusTimerQuit = false;
        try {
            saveVmStatusTimerLoop();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    void pausevm() {
        new Thread(new Runnable() {
            public void run() {
                String error = machineExecutor.saveVM();
                if (error != null)
                    MachineController.getInstance().notifyMachineStatusChangeListeners(machine, MachineStatus.SaveFailed, error);
                MachineController.getInstance().execSaveVmStatusTimer();
            }
        }).start();
    }

    private MachineExecutor getMachineExecutor() {
        return machineExecutor;
    }

    public String getMachineSaveDir() {
        return LimboApplication.getMachineDir() + machineExecutor.getMachine().getName();
    }

    void changeRemovableDevice(MachineProperty property, String value) {
        if (isRunning()) {
            boolean res = getMachineExecutor().changeRemovableDevice(property, value);
            if(!res)
                value = null;
        }
        switch (property) {
            case CDROM:
                getMachine().setCdImagePath(value);
                break;
            case FDA:
                getMachine().setFdaImagePath(value);
                break;
            case FDB:
                getMachine().setFdbImagePath(value);
                break;
            case SHARED_FOLDER:
                getMachine().setSharedFolderPath(value);
                break;
        }
    }

    void sendMouseEvent(int button, int action, int relative, float x, float y) {
        machineExecutor.sendMouseEvent(button, action, relative, x, y);
    }

    public boolean isRunning() {
        return getCurrStatus() == MachineStatus.Running;
    }

    public boolean isPaused() {
        return machineExecutor.getMachine().getPaused() == 1;
    }

    public int getSdlRefreshRate(boolean idle) {
        return machineExecutor.getSdlRefreshRate(idle);
    }

    void setSdlRefreshRate(int refreshMs, boolean idle) {
        machineExecutor.setSdlRefreshRate(refreshMs, idle);
    }

    public String getMachineName() {
        return machineExecutor.getMachine().getName();
    }

    public boolean isVNCEnabled() {
        return getMachineExecutor().getMachine().getEnableVNC() == 1;
    }

    String start() {
        //TODO: figure out when the vm starts successfully and notify
        // to unset the paused flag. For now we just wait a reasonable amount
        // of time
        setPaused(0, 5000);
        return machineExecutor.start();
    }

    private void setPaused(final int value, final int delay) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                machine.setPaused(value);
            }
        }).start();
    }

    public Machine getMachine() {
        return machine;
    }

    void setMachine(Machine machine) {
        if (this.machine != machine) {
            if (this.machine != null) {
                this.machine.deleteObservers();
            }
            this.machine = machine;
            if (this.machine != null)
                this.machine.addObserver((Observer) machineDatabase);
            notifyEventListeners(Event.MachineLoaded, machine);
        }
    }

    void setStoredMachine(String value) {
        setMachine(machineDatabase.getMachine(value));
    }

    //TODO: this should be accessible via a notifier and not directly
    void saveStateVMDB() {
        machineDatabase.updateMachineFieldAsync(
                MachineController.getInstance().getMachine(), MachineProperty.PAUSED, 1 + "");
    }

    boolean createVM(String machineName) {
        if (machineDatabase.getMachine(machineName) != null) {
            notifyEventListeners(Event.MachineCreateFailed, (Integer) R.string.VMNameExistsChooseAnother);
            return false;
        }
        Machine machine = new Machine(machineName, true);
        notifyEventListeners(Event.MachineCreated, machineName);
        MachineController.getInstance().setMachine(machine);
        machineDatabase.insertMachine(getMachine());
        notifyMachineStatusChangeListeners(machine, MachineStatus.Ready, null);
        return true;
    }

    protected void importMachines(String importFilePath) {
        setMachine(null);
        ArrayList<Machine> machines = MachineImporter.importMachines(importFilePath);
        for (Machine machine : machines) {
            MachineFilePaths.insertRecentFilePath(Machine.FileType.CDROM, machine.getCdImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.HDA, machine.getHdaImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.HDB, machine.getHdbImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.HDC, machine.getHdcImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.HDD, machine.getHddImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.SHARED_DIR, machine.getSharedFolderPath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.FDA, machine.getFdaImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.FDB, machine.getFdbImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.SD, machine.getSdImagePath());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.KERNEL, machine.getKernel());
            MachineFilePaths.insertRecentFilePath(Machine.FileType.INITRD, machine.getInitRd());
        }
        notifyEventListeners(Event.MachinesImported, machines);
    }

    public ArrayList<String> getStoredMachines() {
        return machineDatabase.getMachineNames();
    }

    protected boolean deleteMachine(Machine machine) {
        return machineDatabase.deleteMachine(machine);
    }

    public Class<?> getServiceClass() {
        return serviceClass;
    }

    protected void onServiceStarted() {
        notifyMachineStatusChangeListeners(machine, getCurrStatus(), null);
    }

    protected void updateDisplay(int width, int height, int orientation) {
        machineExecutor.updateDisplay(width, height, orientation);
    }

    protected void onVMResolutionChanged(MachineExecutor machineExecutor, int vm_width, int vm_height) {
        if(machineExecutor == this.machineExecutor)
            notifyEventListeners(Event.MachineResolutionChanged, new Object[]{vm_width, vm_height});
    }

    public void setFullscreen() {
        machineExecutor.setFullscreen();
        notifyEventListeners(Event.MachineFullscreen, null);
    }

    public void enableAaudio(int value) {
        machineExecutor.enableAaudio(value);
    }

    public void ignoreBreakpointInvalidation(boolean value) {
        machineExecutor.ignoreBreakpointInvalidation(value?1:0);
    }

    public enum MachineStatus {
        Ready, Stopped, Saving, Paused, SaveCompleted, SaveFailed, Unknown, Running
    }

    public enum Event {
        MachineCreated, MachineCreateFailed, MachineLoaded, MachineResolutionChanged, MachineContinued, MachineFullscreen, MachinesImported
    }

    public interface OnMachineStatusChangeListener {
        void onMachineStatusChanged(Machine machine, MachineStatus status, Object o);
    }

    public interface OnEventListener {
        void onEvent(Machine machine, Event event, Object o);
    }

}
