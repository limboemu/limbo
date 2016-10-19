package org.libsdl.app;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboSDLActivity;
import com.max2idea.android.limbo.utils.UIUtils;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.os.Vibrator;
import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;

/**
 * SDLSurface. This is what we draw on, so we need to know when it's created in
 * order to do anything useful.
 * 
 * Because of this, that's where we set up the SDL thread
 */
public class SDLSurface extends GLSurfaceView
		implements SurfaceHolder.Callback, View.OnKeyListener, View.OnTouchListener, SensorEventListener {

	// Sensors
	protected static SensorManager mSensorManager;
	protected static Display mDisplay;

	// Keep track of the surface size to normalize touch events
	protected static float mWidth, mHeight;

	// Limbo Specific
	public boolean once = false;
	public float old_x = 0;
	public float old_y = 0;
	private boolean mouseUp = true;
	private boolean firstTouch = false;
	private float sensitivity_mult = (float) 1.0;
	private boolean stretchToScreen = false;
	private Activity activity;

	// Startup
	public SDLSurface(Activity activity) {
		super(activity);
		this.activity = activity;

		// getHolder().setFormat(PixelFormat.RGBA_8888);
		getHolder().addCallback(this);
		reSize();
		// getHolder().setType(SurfaceHolder.SURFACE_TYPE_HARDWARE);

		setFocusable(true);
		setFocusableInTouchMode(true);
		// requestFocus();
		setOnKeyListener(this);
		setOnTouchListener(this);
		gestureDetector = new GestureDetector(activity, new GestureListener());

		mDisplay = ((WindowManager) activity.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		mSensorManager = (SensorManager) activity.getSystemService(Context.SENSOR_SERVICE);

		// if (Build.VERSION.SDK_INT >= 12) {
		// setOnGenericMotionListener(new SDLGenericMotionListener_API12());
		// }

		// Some arbitrary defaults to avoid a potential division by zero
		mWidth = 1.0f;
		mHeight = 1.0f;
		Log.v("SDLSurface", "Got New Surface");
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		reSize();
	}

	public static boolean initialized = false;

	//
	public void reSize() {
		// TODO Auto-generated method stub
		Display display = SDLActivity.mSingleton.getWindowManager().getDefaultDisplay();
		int height;
		int width;
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB_MR2) {
			Point size = new Point();
			display.getSize(size);
			width = size.x;
			height = size.y;
		} else {
			width = display.getWidth();
			height = display.getHeight();
		}

		float currentRatio = (float) width / height;
		if (this.getHeight() != 0)
			currentRatio = (float) this.getWidth() / this.getHeight();

		if (SDLActivity.mSingleton.getResources()
				.getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
			if (!initialized) {
				getHolder().setFixedSize(width, (int) (height / 2));
			} else
				getHolder().setFixedSize(width, (int) (width / currentRatio));
		} else if (SDLActivity.mSingleton.getResources()
				.getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
			if (Config.enableSDLAlwaysFullscreen)
				getHolder().setFixedSize(width, height);
			else
				getHolder().setFixedSize(width, height / 2);
		}
		initialized = true;

	}

	public void handlePause() {
		enableSensor(Sensor.TYPE_ACCELEROMETER, false);
	}

	public void handleResume() {
		setFocusable(true);
		setFocusableInTouchMode(true);
		requestFocus();
		setOnKeyListener(this);
		setOnTouchListener(this);
		enableSensor(Sensor.TYPE_ACCELEROMETER, true);
	}

	public Surface getNativeSurface() {
		return getHolder().getSurface();
	}

	// Called when we have a valid drawing surface
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Log.v("SDL", "surfaceCreated()");
		// holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
	}

	// Called when we lose the surface
	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.v("SDL", "surfaceDestroyed()");
		// Call this *before* setting mIsSurfaceReady to 'false'
		SDLActivity.handlePause();
		SDLActivity.mIsSurfaceReady = false;
		SDLActivity.onNativeSurfaceDestroyed();
	}

	// Called when the surface is resized
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		Log.v("SDL", "surfaceChanged(" + width + "x" + height + ")");

		int sdlFormat = 0x15151002; // SDL_PIXELFORMAT_RGB565 by default
		switch (format) {
		case PixelFormat.A_8:
			Log.v("SDL", "pixel format A_8");
			break;
		case PixelFormat.LA_88:
			Log.v("SDL", "pixel format LA_88");
			break;
		case PixelFormat.L_8:
			Log.v("SDL", "pixel format L_8");
			break;
		case PixelFormat.RGBA_4444:
			Log.v("SDL", "pixel format RGBA_4444");
			sdlFormat = 0x15421002; // SDL_PIXELFORMAT_RGBA4444
			break;
		case PixelFormat.RGBA_5551:
			Log.v("SDL", "pixel format RGBA_5551");
			sdlFormat = 0x15441002; // SDL_PIXELFORMAT_RGBA5551
			break;
		case PixelFormat.RGBA_8888:
			Log.v("SDL", "pixel format RGBA_8888");
			sdlFormat = 0x16462004; // SDL_PIXELFORMAT_RGBA8888
			break;
		case PixelFormat.RGBX_8888:
			Log.v("SDL", "pixel format RGBX_8888");
			sdlFormat = 0x16261804; // SDL_PIXELFORMAT_RGBX8888
			break;
		case PixelFormat.RGB_332:
			Log.v("SDL", "pixel format RGB_332");
			sdlFormat = 0x14110801; // SDL_PIXELFORMAT_RGB332
			break;
		case PixelFormat.RGB_565:
			Log.v("SDL", "pixel format RGB_565");
			sdlFormat = 0x15151002; // SDL_PIXELFORMAT_RGB565
			break;
		case PixelFormat.RGB_888:
			Log.v("SDL", "pixel format RGB_888");
			// Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
			sdlFormat = 0x16161804; // SDL_PIXELFORMAT_RGB888
			break;
		default:
			Log.v("SDL", "pixel format unknown " + format);
			break;
		}

		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB) {
			Log.v("Limbo", "Surface Hardware Acceleration: " + this.isHardwareAccelerated());
		}

		mWidth = width;
		mHeight = height;
		SDLActivity.onNativeResize(width, height, sdlFormat, mDisplay.getRefreshRate());
		Log.v("SDL", "Window size: " + width + "x" + height);

		boolean skip = false;
		int requestedOrientation = SDLActivity.mSingleton.getRequestedOrientation();

		if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED) {
			// Accept any
		} else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT) {
			if (mWidth > mHeight) {
				skip = true;
			}
		} else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
			if (mWidth < mHeight) {
				skip = true;
			}
		}

		// Special Patch for Square Resolution: Black Berry Passport
		if (skip) {
			double min = Math.min(mWidth, mHeight);
			double max = Math.max(mWidth, mHeight);

			if (max / min < 1.20) {
				Log.v("SDL", "Don't skip on such aspect-ratio. Could be a square resolution.");
				skip = false;
			}
		}

		if (skip) {
			Log.v("SDL", "Skip .. Surface is not ready.");
			return;
		}

		// Set mIsSurfaceReady to 'true' *before* making a call to handleResume
		SDLActivity.mIsSurfaceReady = true;
		SDLActivity.onNativeSurfaceChanged();

		if (SDLActivity.mSDLThread == null) {
			// This is the entry point to the C app.
			// Start up the C app thread and enable sensor input for the first
			// time

			final Thread sdlThread = new Thread(new SDLMain(), "SDLThread");
			enableSensor(Sensor.TYPE_ACCELEROMETER, true);
			sdlThread.start();

			// Set up a listener thread to catch when the native thread ends
			SDLActivity.mSDLThread = new Thread(new Runnable() {
				@Override
				public void run() {
					try {
						sdlThread.join();
					} catch (Exception e) {
					} finally {
						// XXX: Limbo No need for this
						// Native thread has finished
						// if (!SDLActivity.mExitCalledFromJava) {
						// SDLActivity.handleNativeExit();
						// }
					}
				}
			}, "SDLThreadListener");
			SDLActivity.mSDLThread.start();
		}

		if (SDLActivity.mHasFocus) {
			SDLActivity.handleResume();
		}
	}

	// Key events
	@Override
	public boolean onKey(View v, int keyCode, KeyEvent event) {
		// Dispatch the different events depending on where they come from
		// Some SOURCE_DPAD or SOURCE_GAMEPAD are also SOURCE_KEYBOARD
		// So, we try to process them as DPAD or GAMEPAD events first, if that
		// fails we try them as KEYBOARD

		// We emulate right click with volume down
		if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN && event.getAction() == KeyEvent.ACTION_DOWN) {
			MotionEvent e = MotionEvent.obtain(1000, 1000, MotionEvent.ACTION_DOWN, 0, 0, 0, 0, 0, 0, 0,
					InputDevice.SOURCE_TOUCHSCREEN, 0);
			rightClick(e);
			return true;
		}

		if (event.getKeyCode() == KeyEvent.KEYCODE_MENU)
			return false;

		// TODO: We can enable other devices here like hardware keyboard,
		// gamepad, bluetooth keyboard
		// if ((event.getSource() & InputDevice.SOURCE_GAMEPAD) != 0
		// || (event.getSource() & InputDevice.SOURCE_DPAD) != 0) {
		// if (event.getAction() == KeyEvent.ACTION_DOWN) {
		// if (SDLActivity.onNativePadDown(event.getDeviceId(), keyCode) == 0) {
		// return true;
		// }
		// } else if (event.getAction() == KeyEvent.ACTION_UP) {
		// if (SDLActivity.onNativePadUp(event.getDeviceId(), keyCode) == 0) {
		// return true;
		// }
		// }
		// }
		//
		// if ((event.getSource() & InputDevice.SOURCE_KEYBOARD) != 0
		// || event.getKeyCode() == KeyEvent.KEYCODE_ENTER
		// || event.getKeyCode() == KeyEvent.KEYCODE_DEL
		// ) {
		if (event.getAction() == KeyEvent.ACTION_DOWN) {
			// Log.v("SDL", "key down: " + keyCode);
			if (!handleMissingKeys(keyCode, event.getAction()))
				SDLActivity.onNativeKeyDown(keyCode);
			return true;
		} else if (event.getAction() == KeyEvent.ACTION_UP) {
			// Log.v("SDL", "key up: " + keyCode);
			// try {
			// Thread.sleep(2000);
			// } catch (InterruptedException e) {
			// // TODO Auto-generated catch block
			// e.printStackTrace();
			// }
			if (!handleMissingKeys(keyCode, event.getAction()))
				SDLActivity.onNativeKeyUp(keyCode);
			return true;
		}
		// }

		return false;
	}

	// XXX: SDL is missing some key codes in sdl2-keymap.h
	// So we create them with a Shift Modifier
	private boolean handleMissingKeys(int keyCode, int action) {
		// TODO Auto-generated method stub
		int keyCodeTmp = keyCode;
		switch (keyCode) {
		case 77:
			keyCodeTmp = 9;
			break;
		case 81:
			keyCodeTmp = 70;
			break;
		case 17:
			keyCodeTmp = 15;
			break;
		case 18:
			keyCodeTmp = 10;
			break;
		default:
			return false;

		}
		if (action == KeyEvent.ACTION_DOWN) {
			SDLActivity.onNativeKeyDown(59);
			SDLActivity.onNativeKeyDown(keyCodeTmp);
		} else {
			SDLActivity.onNativeKeyUp(59);
			SDLActivity.onNativeKeyUp(keyCodeTmp);
		}
		return true;

	}

	public boolean onTouchEvent(MotionEvent event) {
		return false;
	}
	
	
	public boolean onTouchEventProcess(MotionEvent event) {
		// Log.v("onTouchEvent",
		// "Action=" + event.getAction() + ", X,Y=" + event.getX() + ","
		// + event.getY() + " P=" + event.getPressure());
		// MK
		if (event.getAction() == MotionEvent.ACTION_CANCEL)
			return true;

		if (!firstTouch) {
			Log.d("SDL", "First Touch");
			SDLActivity.onNativeTouch(event.getDeviceId(), 0, MotionEvent.ACTION_DOWN, 0, 0, 0);

			firstTouch = true;
		}
		if (event.getPointerCount() > 1) {

			// XXX: Limbo Legacy enable Right Click with 2 finger touch
			// Log.v("Right Click",
			// "Action=" + event.getAction() + ", X,Y=" + event.getX()
			// + "," + event.getY() + " P=" + event.getPressure());
			// rightClick(event);
			return true;
		} else
			return gestureDetector.onTouchEvent(event);
	}

	public boolean rightClick(final MotionEvent e) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				Log.d("SDL", "Mouse Right Click");
				SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_DOWN, e.getX(),
						e.getY(), e.getPressure());
				try {
					Thread.sleep(100);
				} catch (InterruptedException ex) {
					Log.v("SDLSurface", "Interrupted: " + ex);
				}
				SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, e.getX(),
						e.getY(), e.getPressure());
			}
		});
		t.start();
		return true;

	}

	private class GestureListener extends GestureDetector.SimpleOnGestureListener {

		@Override
		public boolean onDown(MotionEvent event) {
			// Log.v("onDown",
			// "Action=" + event.getAction() + ", X,Y=" + event.getX()
			// + "," + event.getY() + " P=" + event.getPressure());
			return true;
		}

		@Override
		public void onLongPress(MotionEvent event) {
			//Log.d("SDL", "Long Press Action=" + event.getAction() + ", X,Y=" + event.getX() + "," + event.getY() + " P="
//					+ event.getPressure());
			SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 0, 0, 0);
			Vibrator v = (Vibrator) activity.getSystemService(Context.VIBRATOR_SERVICE);
			if (v.hasVibrator()) {

				// AudioAttributes attributes =
				// v.vibrate(100, attributes);
				v.vibrate(100);
			}
		}

		public boolean onSingleTapConfirmed(MotionEvent event) {
			// float x = e.getX();
			// float y = e.getY();

			// Log.d("onSingleTapConfirmed", "Tapped at: (" + x + "," + y +
			// ")");
			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				// Log.v("onSingleTapConfirmed", "Action=" + action + ", X,Y=" +
				// x
				// + "," + y + " P=" + p);
				LimboSDLActivity.singleClick(event, i);

			}
			return true;

		}

		// event when double tap occurs
		@Override
		public boolean onDoubleTap(MotionEvent event) {
			// float x = e.getX();
			// float y = e.getY();

			// Log.d("onDoubleTap", "Tapped at: (" + x + "," + y + ")");

			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				// Log.v("onDoubleTap", "Action=" + action + ", X,Y=" + x + ","
				// + y + " P=" + p);
				doubleClick(event, i);
			}

			return true;
		}
	}

	private void doubleClick(final MotionEvent event, final int pointer_id) {
		// TODO Auto-generated method stub
		Thread t = new Thread(new Runnable() {
			public void run() {
//				Log.d("SDL", "Mouse Double Click");
				for (int i = 0; i < 2; i++) {
					SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 0, 0,
							0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
//						Log.v("doubletap", "Could not sleep");
					}
					SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, 0,
							0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
//						Log.v("doubletap", "Could not sleep");
					}
				}
			}
		});
		t.start();
	}

	GestureDetector gestureDetector;

	// Sensor events
	public void enableSensor(int sensortype, boolean enabled) {
		// TODO: This uses getDefaultSensor - what if we have >1 accels?
		if (enabled) {
			mSensorManager.registerListener(this, mSensorManager.getDefaultSensor(sensortype),
					SensorManager.SENSOR_DELAY_GAME, null);
		} else {
			mSensorManager.unregisterListener(this, mSensorManager.getDefaultSensor(sensortype));
		}
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		// TODO
	}

	@Override
	public void onSensorChanged(SensorEvent event) {
		if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
			float x, y;
			switch (mDisplay.getRotation()) {
			case Surface.ROTATION_90:
				x = -event.values[1];
				y = event.values[0];
				break;
			case Surface.ROTATION_270:
				x = event.values[1];
				y = -event.values[0];
				break;
			case Surface.ROTATION_180:
				x = -event.values[1];
				y = -event.values[0];
				break;
			default:
				x = event.values[0];
				y = event.values[1];
				break;
			}
			SDLActivity.onNativeAccel(-x / SensorManager.GRAVITY_EARTH, y / SensorManager.GRAVITY_EARTH,
					event.values[2] / SensorManager.GRAVITY_EARTH - 1);
		}
	}

	public boolean onTouch(View v, MotionEvent event) {
		return false;
	}
	
	// Touch events
	public boolean onTouchProcess(View v, MotionEvent event) {
		// Log.v("onTouch",
		// "Action=" + event.getAction() + ", X,Y=" + event.getX() + ","
		// + event.getY() + " P=" + event.getPressure());
		int action = event.getAction();
		float x = event.getX(0);
		float y = event.getY(0);
		float p = event.getPressure(0);
		
		if (event.getAction() == MotionEvent.ACTION_MOVE) {

			// for (int i = 0; i < event.getPointerCount(); i++) {
			

			if (mouseUp) {
				old_x = x;
				old_y = y;
				mouseUp = false;
			}
			if (action == MotionEvent.ACTION_MOVE) {
				//Log.d("SDL", "onTouch Moving by=" + action + ", X,Y=" + (x - old_x) + "," + (y - old_y) + " P=" + p);
				SDLActivity.onNativeTouch(event.getDeviceId(), 0, MotionEvent.ACTION_MOVE,
						(x - old_x) * sensitivity_mult, (y - old_y) * sensitivity_mult, p);
				
			}
			// save current
			old_x = x;
			old_y = y;

		} else if (event.getAction() == event.ACTION_UP) {
//			Log.d("SDL", "onTouch Up");
			SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, 0, 0);
			mouseUp = true;
		}
		return false;
	}

}
