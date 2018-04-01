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
package com.max2idea.android.limbo.main;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;

public class LimboSettingsManager extends PreferenceActivity {

	public static String getLastDir(Activity activity) {
		String lastDir = Environment.getExternalStorageDirectory().getPath();
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getString("lastDir", lastDir);
	}

	public static void setLastDir(Activity activity, String lastDir) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putString("lastDir", lastDir);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}

	static String getDNSServer(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getString("dnsServer", Config.defaultDNSServer);
	}

	public static void setDNSServer(Activity activity, String dnsServer) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putString("dnsServer", dnsServer);
		edit.commit();
	}

//	static String getLastUI(Activity activity) {
//		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
//		return prefs.getString("ui",  Config.defaultUI);
//	}
//
//	public static void setLastUI(Activity activity, String ui) {
//		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
//		SharedPreferences.Editor edit = prefs.edit();
//		edit.putString("ui", ui);
//		edit.commit();
//	}

	public static int getOrientationSetting(Activity activity) {

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		int orientation = prefs.getInt("orientation", 0);
		// UIUtils.log("Getting First time: " + firstTime);
		return orientation;
	}
	
	public static void setOrientationSetting(Activity activity, int orientation) {

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putInt("orientation", orientation);
		edit.commit();
	}

	public static int getKeyboardSetting(Activity activity) {

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		int orientation = prefs.getInt("keyboard", 0);
		// UIUtils.log("Getting First time: " + firstTime);
		return orientation;
	}
	
	public static void setKeyboardSetting(Activity activity, int orientation) {

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putInt("keyboard", orientation);
		edit.commit();
	}

    public static int getMouseSetting(Activity activity) {

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
        int orientation = prefs.getInt("mouse", 0);
        // UIUtils.log("Getting First time: " + firstTime);
        return orientation;
    }

    public static void setMouseSetting(Activity activity, int mouse) {

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
        SharedPreferences.Editor edit = prefs.edit();
        edit.putInt("mouse", mouse);
        edit.commit();
    }
	
	public static boolean getPromptUpdateVersion(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getBoolean("updateVersionPrompt", true);
	}
	

	public static void setPromptUpdateVersion(Activity activity, boolean flag) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("updateVersionPrompt", flag);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}
	
	static boolean getPrio(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getBoolean("HighPrio", false);
	}

	public static void setPrio(Activity activity, boolean flag) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("HighPrio", flag);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}
	
	public static boolean getAlwaysShowMenuToolbar(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getBoolean("AlwaysShowMenuToolbar", false);
	}

	public static void setAlwaysShowMenuToolbar(Activity activity, boolean flag) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("AlwaysShowMenuToolbar", flag);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}
	
	public static boolean getFullscreen(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getBoolean("ShowFullscreen", false);
	}

	public static void setFullscreen(Activity activity, boolean flag) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("ShowFullscreen", flag);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}
	
	static boolean getEnableKVM(Activity activity) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		return prefs.getBoolean("EnableKVM", false);
	}

	public static void setEnableKVM(Activity activity, boolean flag) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("EnableKVM", flag);
		edit.commit();
		// UIUtils.log("Setting First time: ");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Intent data = new Intent();
		setResult(Config.SETTINGS_RETURN_CODE, data);
		addPrefs();
	}

	public void addPrefs() {
		// addPreferencesFromResource(R.xml.settings);
	}
}
