/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package android.androidVNC;

import com.max2idea.android.limbo.main.Config;

import android.widget.ImageView.ScaleType;

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
		setPort(Config.defaultVNCPort + 5900);
		setColorModel(Config.defaultVNCColorMode);
		if (Config.enable_qemu_fullScreen)
			setScaleMode(Config.defaultFullscreenScaleMode);
		else
			setScaleMode(Config.defaultScaleModeCenter);
		setInputMode(Config.defaultInputMode);
	}

	private void setUserName(String string) {
		
		this.userName = string;

	}

	public void setInputMode(String touchZoomMode) {
		
		this.InputMode = touchZoomMode;

	}

	void setPort(int i) {
		
		this.port = i;
	}

	void setColorModel(String nameString) {
		
		this.colorModel = nameString;

	}

	void setAddress(String string) {
		
		this.address = string;
	}

	void setPassword(String string) {
		
		this.password = string;
	}

	public long get_Id() {
		
		return 0;
	}

	ScaleType getScaleMode() {
		return ScaleType.valueOf(getScaleModeAsString());
	}

	private String getScaleModeAsString() {
		
		return scaleMode;
	}

	void setScaleMode(ScaleType value) {
		setScaleModeAsString(value.toString());
	}

	private void setScaleModeAsString(String string) {
		
		this.scaleMode = string;

	}

	public String getAddress() {
		
		return this.address;
	}

	public void setNickname(String address2) {
		
		this.nickname = address2;
	}

	public int getPort() {
		
		return port;
	}

	public String getInputMode() {
		
		return this.InputMode;
	}

	public String getPassword() {
		
		return this.password;
	}

	public long getForceFull() {
		
		return this.forceFull;
	}

	public boolean getUseLocalCursor() {
		
		return this.useLocalCursor;
	}

	public String getNickname() {
		
		return nickname;
	}

	public String getColorModel() {
		
		return this.colorModel;
	}

	public void setForceFull(long l) {
		
		this.forceFull = l;
	}

	public void setUseLocalCursor(boolean checked) {
		
		this.setUseLocalCursor(checked);
	}

	public void setFollowMouse(boolean b) {
		
		this.followMouse = b;

	}

	public boolean getFollowMouse() {
		
		return this.followMouse;
	}

	public String getUserName() {
		
		return userName;
	}

	public void setConnectionId(long get_Id) {
		
		this.id = get_Id;

	}

	public boolean getFollowPan() {
		
		return false;
	}

}
