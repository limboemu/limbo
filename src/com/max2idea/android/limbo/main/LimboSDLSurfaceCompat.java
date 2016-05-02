package com.max2idea.android.limbo.main;

public class LimboSDLSurfaceCompat //extends SDLSurface
//implements View.OnGenericMotionListener
{
//	public LimboSDLSurfaceCompat(Context context) {
//		super(context);
////		this.setOnGenericMotionListener(this);
//		// TODO Auto-generated constructor stub
//	}

//	@Override
//	public boolean onGenericMotion(View v, MotionEvent event) {
//		// TODO Auto-generated method stub
//
//		if (LimboSDLActivity.enablebluetoothmouse == 0) {
//			return false;
//		}
//		float x = event.getX();
//		float y = event.getY();
//		float p = event.getPressure();
//		int action = event.getAction();
//
//		if (x < (LimboSDLActivity.width - LimboSDLActivity.vm_width) / 2) {
//			return true;
//		} else if (x > LimboSDLActivity.width
//				- (LimboSDLActivity.width - LimboSDLActivity.vm_width) / 2) {
//			return true;
//		}
//
//		if (y < (LimboSDLActivity.height - LimboSDLActivity.vm_height) / 2) {
//			return true;
//		} else if (y > LimboSDLActivity.height
//				- (LimboSDLActivity.height - LimboSDLActivity.vm_height) / 2) {
//			return true;
//		}
//
//		if (action == MotionEvent.ACTION_HOVER_MOVE) {
////			Log.v("onGenericMotion", "Moving to (X,Y)=(" + x
////					* LimboSDLActivity.width_mult + "," + y
////					* LimboSDLActivity.height_mult + ")");
//
//			LimboSDLActivity.onNativeTouch(0, 1, MotionEvent.ACTION_MOVE, x
//					* LimboSDLActivity.width_mult, y * LimboSDLActivity.height_mult, p);
//		}
//
//		if (event.getButtonState() == MotionEvent.BUTTON_SECONDARY) {
////			Log.v("onGenericMotion", "Right Click (X,Y)=" + x
////					* LimboSDLActivity.width_mult + "," + y
////					* LimboSDLActivity.height_mult + ")");
//			rightClick(event);
//		}
//
//		// save current
//		old_x = x * LimboSDLActivity.width_mult;
//		old_y = y * LimboSDLActivity.height_mult;
//		return true;
//	}
//	

}
