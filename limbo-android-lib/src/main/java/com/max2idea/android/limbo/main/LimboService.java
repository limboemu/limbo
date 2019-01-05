package com.max2idea.android.limbo.main;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.support.v7.app.NotificationCompat;
import android.support.v7.app.NotificationCompat.Builder;
import android.util.Log;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.jni.VMExecutor;
import com.max2idea.android.limbo.utils.FileUtils;
import com.max2idea.android.limbo.utils.UIUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;

public class LimboService extends Service {

	private static final String TAG = "LimboService";
	private static Notification mNotification;
	private static WifiLock mWifiLock;
	public static LimboService service;
	private static WakeLock mWakeLock;
	private NotificationManager mNotificationManager;

	@Override
	public IBinder onBind(Intent arg0) {
		
		return null;
	}

	public static VMExecutor executor;
	private static Builder builder;

	public static final int notifID = 1000;

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		final String action = intent.getAction();
		final Bundle b = intent.getExtras();
        final int ui = b.getInt("ui", 0);

		if (action.equals(Config.ACTION_START)) {
			if (LimboActivity.currMachine == null)
				return START_NOT_STICKY;

			setUpAsForeground(LimboActivity.currMachine.machinename + ": VM Running");

			FileUtils.startLogging();

			scheduleTimer();

			Thread t = new Thread(new Runnable() {
				public void run() {

				    //XXX: wait till logging starts capturing
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                    Log.v(TAG, "Starting VM: " + LimboActivity.currMachine.machinename);

                    setupLocks();

                    if(ui == Config.UI_VNC) {

                        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {

                            @Override
                            public void run() {
                                if(LimboActivity.getInstance()!=null) {
                                    LimboActivity.getInstance().startvnc();
                                } else
                                    Log.e(TAG, "Could not start VM");

                            }

                        }, 2000);
                    }

                    //Start vm
					String res = executor.startvm();

					//VM has exited
					LimboActivity.cleanup();

				}
			});
            t.setName("VMExecutor");
			t.start();


        }
		

		// Don't restart if killed
		return START_NOT_STICKY;
	}

    private void scheduleTimer() {
        Thread t = new Thread(new Runnable() {
            public void run() {
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                if (LimboActivity.getInstance() != null) {
                    LimboActivity.getInstance().startTimer();
                } else
                    Log.e(TAG, "Could not start timer");
            }
        });
        t.start();
    }


    private void setUpAsForeground(String text) {
		Class<?> clientClass = null;
		if (LimboActivity.currMachine != null) {
			if (LimboActivity.currMachine.ui != null) {
				if (LimboActivity.currMachine.ui.equals("VNC")) {
					clientClass = LimboVNCActivity.class;
				} else if (LimboActivity.currMachine.ui.equals("SDL")) {
					clientClass = LimboSDLActivity.class;
				} else {
					Log.e(TAG, "Unknown User Interface");
					return;
				}
			} else {
				// UIUtils.toastLong(service, "Machine UI is not set");
				//using VNC by default
				clientClass = LimboVNCActivity.class;
			}
		} else {
			Log.e(TAG, "No Machine selected");
			return;
		}
		Intent intent = new Intent(service.getApplicationContext(), clientClass);

		PendingIntent pi = PendingIntent.getActivity(service.getApplicationContext(), 0, intent,
				PendingIntent.FLAG_UPDATE_CURRENT);

		builder = new NotificationCompat.Builder(service);
		mNotification = builder.setContentIntent(pi).setContentTitle(getString(R.string.app_name)).setContentText(text)
				.setSmallIcon(R.drawable.limbo)
				.setLargeIcon(BitmapFactory.decodeResource(service.getResources(), R.drawable.limbo)).build();
		mNotification.tickerText = text;
		mNotification.flags |= Notification.FLAG_ONGOING_EVENT;

		service.startForeground(notifID, mNotification);



    }

	public static void updateServiceNotification(String text) {
		if (builder != null) {
			builder.setContentText(text);
			mNotification = builder.build();
			// mNotification.tickerText = text ;

			NotificationManager mNotificationManager = (NotificationManager)
            service.getApplicationContext().getSystemService(Context.NOTIFICATION_SERVICE);
			// mId allows you to update the notification later on.
			mNotificationManager.notify(notifID, mNotification);
		}
	}

	@Override
	public void onCreate() {
		Log.d(TAG, "Creating Service");
		service = this;
	}

	private void setupLocks() {

		mWifiLock = ((WifiManager) service.getApplicationContext().getSystemService(Context.WIFI_SERVICE))
				.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, Config.wifiLockTag);
		mWifiLock.setReferenceCounted(false);

		PowerManager pm = (PowerManager) service.getApplicationContext().getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, Config.wakeLockTag);
		mWakeLock.setReferenceCounted(false);

		mNotificationManager = (NotificationManager) service.getApplicationContext().getSystemService(NOTIFICATION_SERVICE);

	}

    private static void releaseLocks() {

        if (mWifiLock != null && mWifiLock.isHeld()) {
            Log.d(TAG, "Release Wifi lock...");
            mWifiLock.release();
        }

        if (mWakeLock != null && mWakeLock.isHeld()) {
            Log.d(TAG, "Release Wake lock...");
            mWakeLock.release();
        }

    }

    public static void stopService() {

		Thread t = new Thread(new Runnable() {
			public void run() {
				releaseLocks();
				if(service!=null) {
					service.stopForeground(true);
					service.stopSelf();
				}

			}
		});
		t.setName("VMExecutor");
		t.start();


	}

}
