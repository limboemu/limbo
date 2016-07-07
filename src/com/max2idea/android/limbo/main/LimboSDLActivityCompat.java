package com.max2idea.android.limbo.main;

import org.libsdl.app.SDLActivityCommon;

import com.max2idea.android.limbo.utils.UIUtils;

import android.os.Bundle;

public class LimboSDLActivityCompat extends SDLActivityCommon {

	@Override
	public void onCreate(Bundle b) {
		
		super.onCreate(b);
		
	}

	// @Override
	// public SDLSurface getSDLSurface() {
	// // FIXME: This is needed for External Mouse
	// mSurface = new LimboSDLSurfaceCompat(getApplication());
	// mSurface.activity = this;
	//
	// return mSurface;
	// }

}
