package org.libsdl.app;

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
import android.support.v7.app.ActionBar;
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
import android.widget.LinearLayout;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboSDLActivity;

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
		// getHolder().setType(SurfaceHolder.SURFACE_TYPE_HARDWARE);

		setFocusable(true);
		setFocusableInTouchMode(true);
		// requestFocus();
		setOnKeyListener(this);
		setOnTouchListener(this);
		gestureDetector = new GestureDetector(activity, new GestureListener());

		mDisplay = ((WindowManager) activity.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		mSensorManager = (SensorManager) activity.getSystemService(Context.SENSOR_SERVICE);

		setOnGenericMotionListener(new SDLGenericMotionListener_API12());

		// Some arbitrary defaults to avoid a potential division by zero
		mWidth = 1.0f;
		mHeight = 1.0f;
		Log.v("SDLSurface", "Got New Surface");
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		reSize(true);
	}

	public static boolean initialized = false;

	//
	public void reSize(boolean config) {

		Display display = SDLActivity.mSingleton.getWindowManager().getDefaultDisplay();
		int height = 0;
		int width = 0;

		Point size = new Point();
		display.getSize(size);
		int screen_width = size.x;
		int screen_height = size.y;

        ActionBar bar = ((SDLActivity) activity).getSupportActionBar();


        LinearLayout sdlLayout = ((SDLActivity) activity).sdlLayout;
        if(sdlLayout != null) {
            width = sdlLayout.getWidth();
            height = sdlLayout.getHeight();
        }

        //native resolution for use with external mouse
        if(Config.mouseMode == Config.MouseMode.External
                || (!LimboSDLActivity.stretchToScreen && !LimboSDLActivity.fitToScreen)) {
            width = SDLActivity.vm_width;
            height = SDLActivity.vm_height;
        }

        if(config){
            int temp = width;
            width = height;
            height = temp;
        }

		if (SDLActivity.mSingleton.getResources()
				.getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {

            if(Config.mouseMode != Config.MouseMode.External) {
                if (bar != null && bar.isShowing()) {
                    width += bar.getHeight();
                }
                if (!initialized) {
                    getHolder().setFixedSize(width, (int) (height / 2));
                } else {
                    getHolder().setFixedSize(width, (int) (width / (SDLActivity.vm_width / (float) SDLActivity.vm_height)));
                }
            }
		} else if (SDLActivity.mSingleton.getResources()
				.getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
			if (Config.enableSDLAlwaysFullscreen) {
                getHolder().setFixedSize(width, height);
            }
			else {
                getHolder().setFixedSize(width / 2, height);
            }
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
        SDLActivity.width = width;
        SDLActivity.height = height;
		int sdlFormat = 0x15151002; // SDL_PIXELFORMAT_RGB565 by default
		switch (format) {
		case PixelFormat.RGBA_8888:
			Log.v("SDL", "pixel format RGBA_8888");
			sdlFormat = 0x16462004; // SDL_PIXELFORMAT_RGBA8888
			break;
		case PixelFormat.RGBX_8888:
			Log.v("SDL", "pixel format RGBX_8888");
			sdlFormat = 0x16261804; // SDL_PIXELFORMAT_RGBX8888
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

		//redraw the surface after a rotation
		SDLActivity.dummyTouch();

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
			rightClick(e, 0);
			return true;
		} else // We emulate middle click with volume up
            if (keyCode == KeyEvent.KEYCODE_VOLUME_UP && event.getAction() == KeyEvent.ACTION_DOWN) {
                MotionEvent e = MotionEvent.obtain(1000, 1000, MotionEvent.ACTION_DOWN, 0, 0, 0, 0, 0, 0, 0,
                        InputDevice.SOURCE_TOUCHSCREEN, 0);
                middleClick(e, 0);
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

	public boolean rightClick(final MotionEvent e, final int i) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				Log.d("SDL", "Mouse Right Click");
				SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_DOWN, e.getX(i),
						e.getY(i), e.getPressure(i));
				try {
					Thread.sleep(100);
				} catch (InterruptedException ex) {
					Log.v("SDLSurface", "Interrupted: " + ex);
				}
				SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, e.getX(i),
						e.getY(i), e.getPressure(i));
			}
		});
		t.start();
		return true;

	}

    public boolean middleClick(final MotionEvent e, final int i) {
        Thread t = new Thread(new Runnable() {
            public void run() {
                Log.d("SDL", "Mouse Middle Click");
                SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_DOWN, e.getX(i),
                        e.getY(i), e.getPressure(i));
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                    Log.v("SDLSurface", "Interrupted: " + ex);
                }
                SDLActivity.onNativeTouch(e.getDeviceId(), Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, e.getX(i),
                        e.getY(i), e.getPressure(i));
            }
        });
        t.start();
        return true;

    }

	private class GestureListener extends GestureDetector.SimpleOnGestureListener {

		@Override
		public boolean onDown(MotionEvent event) {
			// Log.v("onDown", "Action=" + event.getAction() + ", X,Y=" + event.getX()
			// + "," + event.getY() + " P=" + event.getPressure());
            return true;
		}

		@Override
		public void onLongPress(MotionEvent event) {
			// Log.d("SDL", "Long Press Action=" + event.getAction() + ", X,Y="
			// + event.getX() + "," + event.getY() + " P="
			// + event.getPressure());
            if(MotionEvent.TOOL_TYPE_FINGER != event.getToolType(0))
                return;

			SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 0, 0, 0);
			Vibrator v = (Vibrator) activity.getSystemService(Context.VIBRATOR_SERVICE);
			if (v.hasVibrator()) {

				// AudioAttributes attributes =
				// v.vibrate(100, attributes);
				v.vibrate(100);
			}
		}

		public boolean onSingleTapConfirmed(MotionEvent event) {
			 float x1 = event.getX();
			 float y1 = event.getY();

			 Log.d("onSingleTapConfirmed", "Tapped at: (" + x1 + "," + y1 +
			 ")");

			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				 //Log.v("onSingleTapConfirmed", "Action=" + action + ", X,Y=" + x + "," + y + " P=" + p);
                if (event.getAction() == event.ACTION_DOWN
                        && MotionEvent.TOOL_TYPE_FINGER == event.getToolType(0)) {
                    //Log.d("SDL", "onTouch Down: " + event.getButtonState());
                    LimboSDLActivity.singleClick(event, i);
                }
			}
			return true;

		}

		// event when double tap occurs
		@Override
		public boolean onDoubleTap(MotionEvent event) {
			Log.d("onDoubleTap", "Tapped at: (" + event.getX() + "," + event.getY() + ")");

            if(Config.mouseMode == Config.MouseMode.External && MotionEvent.TOOL_TYPE_MOUSE == event.getToolType(0))
                return true;

			for (int i = 0; i < event.getPointerCount(); i++) {
				int action = event.getAction();
				float x = event.getX(i);
				float y = event.getY(i);
				float p = event.getPressure(i);

				// Log.v("onDoubleTap", "Action=" + action + ", X,Y=" + x + "," + y + " P=" + p);
				doubleClick(event, i);
			}

			return true;
		}
	}

	private void doubleClick(final MotionEvent event, final int pointer_id) {
		
		Thread t = new Thread(new Runnable() {
			public void run() {
				//Log.d("SDL", "Mouse Double Click");
				for (int i = 0; i < 2; i++) {
					SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_DOWN, 0, 0,
							0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
						// Log.v("doubletap", "Could not sleep");
					}
					SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, 0,
							0);
					try {
						Thread.sleep(50);
					} catch (InterruptedException ex) {
						// Log.v("doubletap", "Could not sleep");
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
        boolean res = false;
        if(Config.mouseMode == Config.MouseMode.External){
            res = onTouchProcess(v,event);
            res = onTouchEventProcess(event);
        }
        return res;
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

        int sdlMouseButton = 0;
        if(event.getButtonState() == MotionEvent.BUTTON_PRIMARY)
            sdlMouseButton = Config.SDL_MOUSE_LEFT;
        else if(event.getButtonState() == MotionEvent.BUTTON_SECONDARY)
            sdlMouseButton = Config.SDL_MOUSE_RIGHT;
        else if(event.getButtonState() == MotionEvent.BUTTON_TERTIARY)
            sdlMouseButton = Config.SDL_MOUSE_MIDDLE;


        if (event.getAction() == MotionEvent.ACTION_MOVE) {

			if (mouseUp) {
				old_x = x;
				old_y = y;
				mouseUp = false;
			}
			if (action == MotionEvent.ACTION_MOVE) {
                if(Config.mouseMode == Config.MouseMode.External) {
                    Log.d("SDL", "onTouch Absolute Move by=" + action + ", X,Y=" + (x) + "," + (y) + " P=" + p);
                    SDLActivity.onNativeTouch(event.getDeviceId(), 1, MotionEvent.ACTION_MOVE,
                            x * sensitivity_mult, y * sensitivity_mult, p);
                }else {
                    Log.d("SDL", "onTouch Relative Moving by=" + action + ", X,Y=" + (x -
                            old_x) + "," + (y - old_y) + " P=" + p);
                    SDLActivity.onNativeTouch(event.getDeviceId(), 0, MotionEvent.ACTION_MOVE,
                            (x - old_x) * sensitivity_mult, (y - old_y) * sensitivity_mult, p);
                }

			}
			// save current
			old_x = x;
			old_y = y;

		}
		else if (event.getAction() == event.ACTION_UP ) {
            //Log.d("SDL", "onTouch Up: " + sdlMouseButton);
            //XXX: it seems that the Button state is not available when Button up so
            //  we should release all mouse buttons to be safe since we don't know which one fired the event
            if(sdlMouseButton!=0){
                SDLActivity.onNativeTouch(event.getDeviceId(), sdlMouseButton, MotionEvent.ACTION_UP, 0, 0, p);
            }else {
                SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_LEFT, MotionEvent.ACTION_UP, 0, 0, p);
                SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_RIGHT, MotionEvent.ACTION_UP, 0, 0, p);
                SDLActivity.onNativeTouch(event.getDeviceId(), Config.SDL_MOUSE_MIDDLE, MotionEvent.ACTION_UP, 0, 0, p);
            }
			mouseUp = true;
		}
		else if (event.getAction() == event.ACTION_DOWN && Config.mouseMode == Config.MouseMode.External) {
            Log.d("SDL", "onTouch Down: " + sdlMouseButton);
            //XXX: we need this to detect external mouse button down
            if(sdlMouseButton > 0)
                SDLActivity.onNativeTouch(event.getDeviceId(), sdlMouseButton, MotionEvent.ACTION_DOWN, 0, 0, p);
        }
        return true;
	}



    class SDLGenericMotionListener_API12 implements View.OnGenericMotionListener {
        private SDLSurface mSurface;

        @Override
        public boolean onGenericMotion(View v, MotionEvent event) {
            float x, y;
            int action;

            switch (event.getSource()) {
                case InputDevice.SOURCE_JOYSTICK:
                case InputDevice.SOURCE_GAMEPAD:
                case InputDevice.SOURCE_DPAD:
                    SDLActivity.handleJoystickMotionEvent(event);
                    return true;

                case InputDevice.SOURCE_MOUSE:
                    if(Config.mouseMode == Config.MouseMode.Trackpad)
                        break;

                    action = event.getActionMasked();
                    Log.d("SDL", "onGenericMotion, action = " + action + "," + event.getX() + ", " + event.getY());
                    switch (action) {
                        case MotionEvent.ACTION_SCROLL:
                            x = event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0);
                            y = event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0);
                            Log.d("SDL", "Mouse Scroll: " + x + "," + y);
                            SDLActivity.onNativeMouse(0, action, x, y);
                            return true;

                        case MotionEvent.ACTION_HOVER_MOVE:
                            if(Config.processMouseHistoricalEvents) {
                                final int historySize = event.getHistorySize();
                                for (int h = 0; h < historySize; h++) {
                                    float ex = event.getHistoricalX(h);
                                    float ey = event.getHistoricalY(h);
                                    float ep = event.getHistoricalPressure(h);
                                    processHoverMouse(ex, ey, ep, action);
                                }
                            }

                            float ex = event.getX();
                            float ey = event.getY();
                            float ep = event.getPressure();
                            processHoverMouse(ex, ey, ep, action);
                            return true;

                        case MotionEvent.ACTION_UP:

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }

            // Event was not managed
            return false;
        }

        private void processHoverMouse(float x,float y,float p, int action) {

            Log.d("SDL", "Mouse Hover: " + x + "," + y);

            if(Config.mouseMode == Config.MouseMode.External) {
                SDLActivity.onNativeMouse(0, action, x, y);
            }
        }

    }

    protected void processHoverMouseAlt(float x,float y,float p, int action) {
        if (action == MotionEvent.ACTION_HOVER_MOVE) {
			//Log.v("onGenericMotion", "Moving to (X,Y)=(" + x
					//* LimboSDLActivity.width_mult + "," + y
					//* LimboSDLActivity.height_mult + ")");
            LimboSDLActivity.onNativeTouch(0, 1, MotionEvent.ACTION_MOVE,
                    x, y, p);
        }

        // save current
        old_x = x * LimboSDLActivity.width_mult;
        old_y = y * LimboSDLActivity.height_mult;
        return;
    }
}
