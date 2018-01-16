package com.max2idea.android.limbo.main;

import android.app.Application;

public class LimboApplication extends Application {

    @Override
	public void onCreate() {
        super.onCreate();
		try {
			Class.forName("android.os.AsyncTask");
		} catch (Throwable ignore) {
			// ignored
		}

		

	}

}
