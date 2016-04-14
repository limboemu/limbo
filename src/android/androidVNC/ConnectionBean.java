/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package android.androidVNC;

import android.androidVNC.COLORMODEL;
import android.androidVNC.VncCanvasActivity;
import android.content.ContentValues;
import android.database.sqlite.SQLiteDatabase;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import java.lang.Comparable;

import com.max2idea.android.limbo.main.Config;

/**
 * @author Michael A. MacDonald
 * 
 */
public class ConnectionBean {

	private String address = "localhost";
	private String password = "";
	private int port = 5901;
	private String colorModel = COLORMODEL.C64.nameString();
	private String InputMode = VncCanvasActivity.TOUCH_ZOOM_MODE;
	private String scaleMode = "";
	private String nickname = "limbo";
	private long forceFull = 0;
	private boolean useLocalCursor = false;
	private boolean followMouse = true;
	private String userName;
	private long id = 0;

	public ConnectionBean() {
		setAddress(Config.defaultVNCHost);
		setUserName(Config.defaultVNCUsername);
		setPassword(Config.defaultVNCPasswd);
		setPort(Config.defaultVNCPort);
		setColorModel(Config.defaultVNCColorMode);
		if (Config.enable_qemu_fullScreen)
			setScaleMode(Config.defaultFullscreenScaleMode);
		else
			setScaleMode(Config.defaultScaleModeCenter);
		setInputMode(Config.defaultInputMode);
	}

	private void setUserName(String string) {
		// TODO Auto-generated method stub
		this.userName = string;

	}

	public void setInputMode(String touchZoomMode) {
		// TODO Auto-generated method stub
		this.InputMode = touchZoomMode;

	}

	void setPort(int i) {
		// TODO Auto-generated method stub
		this.port = i;
	}

	void setColorModel(String nameString) {
		// TODO Auto-generated method stub
		this.colorModel = nameString;

	}

	void setAddress(String string) {
		// TODO Auto-generated method stub
		this.address = string;
	}

	void setPassword(String string) {
		// TODO Auto-generated method stub
		this.password = string;
	}

	public long get_Id() {
		// TODO Auto-generated method stub
		return 0;
	}

	ScaleType getScaleMode() {
		return ScaleType.valueOf(getScaleModeAsString());
	}

	private String getScaleModeAsString() {
		// TODO Auto-generated method stub
		return scaleMode;
	}

	void setScaleMode(ScaleType value) {
		setScaleModeAsString(value.toString());
	}

	private void setScaleModeAsString(String string) {
		// TODO Auto-generated method stub
		this.scaleMode = string;

	}

	public String getAddress() {
		// TODO Auto-generated method stub
		return this.address;
	}

	public void setNickname(String address2) {
		// TODO Auto-generated method stub
		this.nickname = address2;
	}

	public int getPort() {
		// TODO Auto-generated method stub
		return port;
	}

	public String getInputMode() {
		// TODO Auto-generated method stub
		return this.InputMode;
	}

	public String getPassword() {
		// TODO Auto-generated method stub
		return this.password;
	}

	public long getForceFull() {
		// TODO Auto-generated method stub
		return this.forceFull;
	}

	public boolean getUseLocalCursor() {
		// TODO Auto-generated method stub
		return this.useLocalCursor;
	}

	public String getNickname() {
		// TODO Auto-generated method stub
		return nickname;
	}

	public String getColorModel() {
		// TODO Auto-generated method stub
		return this.colorModel;
	}

	public void setForceFull(long l) {
		// TODO Auto-generated method stub
		this.forceFull = l;
	}

	public void setUseLocalCursor(boolean checked) {
		// TODO Auto-generated method stub
		this.setUseLocalCursor(checked);
	}

	public void setFollowMouse(boolean b) {
		// TODO Auto-generated method stub
		this.followMouse = b;

	}

	public boolean getFollowMouse() {
		// TODO Auto-generated method stub
		return this.followMouse;
	}

	public String getUserName() {
		// TODO Auto-generated method stub
		return userName;
	}

	public void setConnectionId(long get_Id) {
		// TODO Auto-generated method stub
		this.id = get_Id;

	}

	public boolean getFollowPan() {
		// TODO Auto-generated method stub
		return false;
	}

}
