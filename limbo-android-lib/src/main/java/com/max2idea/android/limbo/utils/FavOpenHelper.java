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


    private static FavOpenHelper sInstance;
	public SQLiteDatabase db;

	
    public FavOpenHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        getDB();
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
    }

	private synchronized void getDB() {
		
		if (db == null)
			db = this.getWritableDatabase();
	}



    public static synchronized FavOpenHelper getInstance(Context context) {

        if (sInstance == null) {
            sInstance = new FavOpenHelper(context.getApplicationContext());
        }
        return sInstance;
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

        if (newVersion > 2) {
            db.execSQL(TABLE_NAME_FAV_FILES_CREATE);
        }
    }

    public int getFavSeq(String favPath, String favType) {
        int ret = -1;
        String SELECT_FAVS = "select " + this.FAVSEQ
                + " from " + this.TABLE_NAME_FAV_FILES
                + " Where " + this.FAVFILE + "= ? "
                + " and " + this.FAVFILETYPE + "= ? "
                +";";

        Cursor cur = db.rawQuery(SELECT_FAVS, new String[]{favPath, favType});
        cur.moveToFirst();
        while (cur.isAfterLast() == false) {
            ret = cur.getInt(0);
            cur.moveToNext();
        }
        cur.close();
        return ret;
    }

    public int insertFav(String favpath, String favtype) {
        int seqnum = -1;
//        Log.v("DB", "insert fav: " + favpath);
        ContentValues stateValues = new ContentValues();
        stateValues.put(FAVFILE, favpath);
        stateValues.put(this.FAVFILETYPE, favtype);
        try {
            seqnum = (int) db.insertOrThrow(this.TABLE_NAME_FAV_FILES, null, stateValues);
        } catch (Exception e) {
            //catch code
            Log.v(TAG, "Error while Insert Fav Path: " + e.getMessage());
        }

        return seqnum;
    }

    public ArrayList<String> getFav(String favType) {
        String qry = "select " + this.FAVFILE + " "
                + " from " + this.TABLE_NAME_FAV_FILES
                + " where " + this.FAVFILETYPE + " = ? "
                + ";";

        ArrayList<String> arrStr = new ArrayList<String>();
        Cursor cur = db.rawQuery(qry, new String[]{favType});
        cur.moveToFirst();
        while (cur.isAfterLast() == false) {
            arrStr.add(cur.getString(0));
            cur.moveToNext();
        }
        cur.close();
        return arrStr;
    }

    public int deleteFav(String favPath) {
        int rowsAffected = 0;
        // Insert arrFiles in
        try {
            db.delete(this.TABLE_NAME_FAV_FILES, this.FAVFILE + "=?", new String[]{favPath});
        } catch (Exception e) {
            //catch code
        }
        

        return rowsAffected;
    }

    public int deleteFiles() {
        int rowsAffected = 0;
        // Insert arrFiles in
        try {
            db.delete(this.FAVFILETYPE, null, null);
        } catch (Exception e) {
            //catch code
        }
        

        return rowsAffected;
    }
}
