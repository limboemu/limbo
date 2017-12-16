package org.libsdl.app;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLSurfaceView;
import android.util.Log;

public class ClearRenderer implements GLSurfaceView.Renderer {
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		// Do nothing special.
		Log.v("onSurfaceCreated", "...");
	}

	public void onSurfaceChanged(GL10 gl, int w, int h) {
		Log.v("onSurfaceChanged", "...");
		gl.glViewport(0, 0, w, h);
	}

	public void onDrawFrame(GL10 gl) {
		Log.v("onDrawFrame", "...");
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

	}
}
