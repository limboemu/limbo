package com.max2idea.android.limbo.main;

import android.app.IntentService;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;

import java.io.OutputStream;

public class LimboVNCService extends IntentService {

	public LimboVNCService() {
		super(LimboVNCService.class.getName());
	}

	private static final String TAG = "LimboVNCService";

	@Override
	public IBinder onBind(Intent arg0) {
		return null;
	}

	public static OutputStream os;

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		 super.onStartCommand(intent, flags, startId);
		
		return START_NOT_STICKY;
	}
	@Override
	public void onCreate() {
		//Log.d(TAG, "debug: Creating " + TAG);
		super.onCreate();
	}


	@Override
	protected void onHandleIntent(Intent intent) {
		
		final String action = intent.getAction();
		final Bundle b = intent.getExtras();

		if (action.equals(Config.SEND_VNC_DATA)) {

			int vncDataType = b.getInt(Config.VNC_DATA_TYPE);
			int vncByte = b.getInt(Config.VNC_BYTE);
			byte[] vncBytes = b.getByteArray(Config.VNC_BYTES);
			int offset = b.getInt(Config.VNC_OFFSET);
			int count = b.getInt(Config.VNC_COUNT);
			if (vncDataType == Config.VNC_SEND_BYTE) {
				try {
					if (os != null)
						os.write(vncByte);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			} else if (vncDataType == Config.VNC_SEND_BYTES) {
				try {
					if (os != null)
						os.write(vncBytes);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			} else if (vncDataType == Config.VNC_SEND_BYTES_OFFSET) {
				try {
					if (os != null)
						os.write(vncBytes, offset, count);
				} catch (Exception e) {
					// TODO Auto-generated catch block
                    if(Config.debug)
					    e.printStackTrace();
				}
			}

		}
	}

}
