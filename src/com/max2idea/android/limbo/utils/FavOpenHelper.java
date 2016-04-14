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

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import java.util.ArrayList;

/**
 *
 * @author Dev
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

    public FavOpenHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
//        UIUtils.log("Example", "Upgrading database, this will drop tables and recreate.");
//        db.execSQL("DROP TABLE IF EXISTS " + FILE_TABLE_NAME);
        if (newVersion > 2) {
            db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
        }
//        onCreate(db);
    }

    public int getFavUrlSeq(String fav, String favType) {
        int ret = -1;
        String SELECT_URLS = "select " + this.FAVSEQ
                + " from " + this.TABLE_NAME_FAV_FILES
                + " Where " + this.FAVFILE + "= ? "
                + " and " + this.FAVFILETYPE + "= ? "
                +";";

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cur = db.rawQuery(SELECT_URLS, new String[]{fav, favType});
        cur.moveToFirst();
        while (cur.isAfterLast() == false) {
            ret = cur.getInt(0);
            cur.moveToNext();
        }
        cur.close();
        db.close();
        return ret;
    }

    public int insertFavURL(String url, String urlname) {
        int seqnum = -1;
        SQLiteDatabase db = this.getReadableDatabase();
        // Insert urlname in db
//        Log.v("DB", "insert file: " + urlname);
        ContentValues stateValues = new ContentValues();
        stateValues.put(FAVFILE, url);
        stateValues.put(this.FAVFILETYPE, urlname);
        try {
            seqnum = (int) db.insertOrThrow(this.TABLE_NAME_FAV_FILES, null, stateValues);
        } catch (Exception e) {
            //catch code
            Log.v(TAG, "Error while Insert url: " + e.getMessage());
        }
        db.close();
        return seqnum;
    }

    public ArrayList<String> getFavURL(String favType) {
        String qry = "select " + this.FAVFILE + " "
                + " from " + this.TABLE_NAME_FAV_FILES
                + " where " + this.FAVFILETYPE + " = ? "
                + ";";

        ArrayList<String> arrStr = new ArrayList<String>();
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cur = db.rawQuery(qry, new String[]{favType});
        cur.moveToFirst();
        while (cur.isAfterLast() == false) {
            arrStr.add(cur.getString(0));
            cur.moveToNext();
        }
        cur.close();
        db.close();
        return arrStr;
    }

    public int deleteFavURL(String favUrl) {
        int rowsAffected = 0;
        SQLiteDatabase db = this.getReadableDatabase();
        // Insert arrFiles in
        try {
            db.delete(this.TABLE_NAME_FAV_FILES, this.FAVFILE + "=?", new String[]{favUrl});
        } catch (Exception e) {
            //catch code
        }
        db.close();

        return rowsAffected;
    }

    public int deleteFiles() {
        int rowsAffected = 0;
        SQLiteDatabase db = this.getReadableDatabase();
        // Insert arrFiles in
        try {
            db.delete(this.FAVFILETYPE, null, null);
        } catch (Exception e) {
            //catch code
        }
        db.close();

        return rowsAffected;
    }
}
