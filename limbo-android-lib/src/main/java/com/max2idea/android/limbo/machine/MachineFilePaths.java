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

import java.util.ArrayList;

public class MachineFilePaths {

    public static void insertRecentFilePath(Machine.FileType fileType, String filePath) {
        if (fileType == null || filePath == null || filePath.equals(""))
            return;
        if (!isRecentFilePathStored(fileType, filePath)) {
            FavOpenHelper.getInstance().insertFav(fileType.name().toLowerCase(), filePath);
        }
    }

    public static boolean isRecentFilePathStored(Machine.FileType type, String filePath) {
        return FavOpenHelper.getInstance().getFavSeq(type.toString().toLowerCase(), filePath) >= 0;
    }
    synchronized
    public static  ArrayList<String> getRecentFilePaths(Machine.FileType fileType) {
        return FavOpenHelper.getInstance().getFav(fileType.toString().toLowerCase());
    }

}
