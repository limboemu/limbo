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

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.util.ArrayList;

/**
 * Storage Implementation for our recent file paths.
 */
public class FavOpenHelper extends SQLiteOpenHelper {

    private static final String TAG = "FAVS";
    private static final int DATABASE_VERSION = 1;
    private static final String DATABASE_NAME = "FAVS";
    private static final String FAVFILE = "FAVFILE";
    private static final String FAVFILETYPE = "FAVFILETYPE";
    private static final String FAVSEQ = "FAVSEQ";
    private static final String TABLE_NAME_FAV_FILES = "favorites";
    private static final String TABLE_NAME_FAV_FILES_CREATE =
            "CREATE TABLE IF NOT EXISTS " + TABLE_NAME_FAV_FILES + " ("
                    + FAVSEQ + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + FAVFILETYPE + " TEXT, "
                    + FAVFILE + " TEXT);";


    private static FavOpenHelper sInstance;
    private SQLiteDatabase db;

    private FavOpenHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        getDB();
    }

    static FavOpenHelper getInstance() {
        return sInstance;
    }

    public static synchronized void initialize(Context context) {
        if (sInstance == null) {
            sInstance = new FavOpenHelper(context.getApplicationContext());
            sInstance.setWriteAheadLoggingEnabled(true);
        }
    }
    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
    }

    private synchronized void getDB() {
        if (db == null)
            db = getWritableDatabase();
    }

    @Override
    public void close() {
        if (db != null)
            db.close();
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

        if (newVersion > 2) {
            db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
        }
    }

    int getFavSeq(String favType, String favPath) {
        int ret = -1;
        String SELECT_FAVS = "select " + FAVSEQ
                + " from " + TABLE_NAME_FAV_FILES
                + " Where " + FAVFILE + "= ? "
                + " and " + FAVFILETYPE + "= ? "
                + ";";

        Cursor cur = db.rawQuery(SELECT_FAVS, new String[]{favPath, favType});
        cur.moveToFirst();
        while (!cur.isAfterLast()) {
            ret = cur.getInt(0);
            cur.moveToNext();
        }
        cur.close();
        return ret;
    }

    boolean insertFav(String favtype, String favpath) {
        long row = 0;
        ContentValues stateValues = new ContentValues();
        stateValues.put(FAVFILE, favpath);
        stateValues.put(FAVFILETYPE, favtype);
        try {
            row = db.insertOrThrow(TABLE_NAME_FAV_FILES, null, stateValues);
        } catch (Exception e) {
            Log.w(TAG, "Error while Insert Fav Path: " + e.getMessage());
        }
        return row>0;
    }

    ArrayList<String> getFav(String favType) {
        String qry = "select " + FAVFILE + " "
                + " from " + TABLE_NAME_FAV_FILES
                + " where " + FAVFILETYPE + " = ? "
                + ";";

        ArrayList<String> arrStr = new ArrayList<>();
        Cursor cur = db.rawQuery(qry, new String[]{favType});
        cur.moveToFirst();
        while (!cur.isAfterLast()) {
            arrStr.add(cur.getString(0));
            cur.moveToNext();
        }
        cur.close();
        return arrStr;
    }
}
