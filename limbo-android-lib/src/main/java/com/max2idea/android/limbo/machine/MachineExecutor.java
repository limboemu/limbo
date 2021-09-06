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

/** Our emulation abstract bridge. It can be extended to implement bridges with other native
 * emulators that support SDL.
 */
public abstract class MachineExecutor {
    private static String TAG = "MachineExecutor";

    private final MachineController machineController;

    public MachineExecutor(MachineController machineController) {
        this.machineController = machineController;
    }

    protected Machine getMachine() {
        return machineController.getMachine();
    }

    abstract public void startService();

    // TODO: create int success code instead of string
    abstract public String start();

    abstract protected void stopvm(final int restart);

    public abstract int getSdlRefreshRate();

    public abstract void setSdlRefreshRate(int refreshMs);

    public abstract void sendMouseEvent(int button, int action, int relative, float x, float y);

    public abstract String saveVM();

    public abstract void continueVM();

    public abstract MachineController.MachineStatus getSaveVMStatus();

    public abstract boolean changeRemovableDevice(MachineProperty drive, String diskValue);

    public abstract String getDeviceName(MachineProperty driveProperty);

    public abstract void updateDisplay(int width, int height, int orientation);
}
