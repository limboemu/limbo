/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.antlersoft.android.bc;

import android.content.Context;

/**
 * Create interface implementations appropriate to the current version of the SDK;
 * implementations can allow use of higher-level SDK calls in .apk's that will still run
 * on lower-level SDK's
 * @author Michael A. MacDonald
 */
public class BCFactory {
	
	private static BCFactory _theInstance = new BCFactory();
	
	private IBCActivityManager bcActivityManager;
	private IBCHaptic bcHaptic;
	private IBCMotionEvent bcMotionEvent;
	private IBCStorageContext bcStorageContext;
	
	/**
	 * This is here so checking the static doesn't get optimized away;
	 * note we can't use SDK_INT because that is too new
	 * @return sdk version
	 */
	int getSdkVersion()
	{
		try
		{
			return Integer.parseInt(android.os.Build.VERSION.SDK);
		}
		catch (NumberFormatException nfe)
		{
			return 1;
		}
	}
	
	/**
	 * Return the implementation of IBCActivityManager appropriate for this SDK level
	 * @return
	 */
	public IBCActivityManager getBCActivityManager()
	{
		if (bcActivityManager == null)
		{
			synchronized (this)
			{
				if (bcActivityManager == null)
				{
					if (getSdkVersion() >= 5)
					{
						try
						{
							bcActivityManager = (IBCActivityManager)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCActivityManagerV5").newInstance();
						}
						catch (Exception ie)
						{
							bcActivityManager = new BCActivityManagerDefault();
							throw new RuntimeException("Error instantiating", ie);
						}
					}
					else
					{
						bcActivityManager = new BCActivityManagerDefault();
					}
				}
			}
		}
		return bcActivityManager;
	}
	
	
	
	/**
	 * Return the implementation of IBCHaptic appropriate for this SDK level
	 * 
	 * Since we dropped support of SDK levels prior to 3, there is only one version at the moment.
	 * @return
	 */
	public IBCHaptic getBCHaptic()
	{
		if (bcHaptic == null)
		{
			synchronized (this)
			{
				if (bcHaptic == null)
				{
					try
					{
						bcHaptic = (IBCHaptic)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCHapticDefault").newInstance();
					}
					catch (Exception ie)
					{
						throw new RuntimeException("Error instantiating", ie);
					}
				}
			}
		}
		return bcHaptic;
	}
	
	/**
	 * Return the implementation of IBCMotionEvent appropriate for this SDK level
	 * @return
	 */
	public IBCMotionEvent getBCMotionEvent()
	{
		if (bcMotionEvent == null)
		{
			synchronized (this)
			{
				if (bcMotionEvent == null)
				{
					if (getSdkVersion() >= 5)
					{
						try
						{
							bcMotionEvent = (IBCMotionEvent)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCMotionEvent5").newInstance();
						}
						catch (Exception ie)
						{
							throw new RuntimeException("Error instantiating", ie);
						}
					}
					else
					{
						try
						{
							bcMotionEvent = (IBCMotionEvent)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCMotionEvent4").newInstance();
						}
						catch (Exception ie)
						{
							throw new RuntimeException("Error instantiating", ie);
						}
					}
				}
			}
		}
		return bcMotionEvent;
	}

	/**
	 * 
	 * @return An implementation of IBCStorageContext appropriate for the running Android release
	 */
	public IBCStorageContext getStorageContext()
	{
		if (bcStorageContext == null)
		{
			synchronized (this)
			{
				if (bcStorageContext == null)
				{
					if (getSdkVersion() >= 8)
					{
						try
						{
							bcStorageContext = (IBCStorageContext)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCStorageContext8").newInstance();
						}
						catch (Exception ie)
						{
							throw new RuntimeException("Error instantiating", ie);
						}
					}
					else
					{
						try
						{
							bcStorageContext = (IBCStorageContext)getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCStorageContext7").newInstance();
						}
						catch (Exception ie)
						{
							throw new RuntimeException("Error instantiating", ie);
						}
					}
				}
			}
		}
		return bcStorageContext;
	}
	
	/**
	 * Returns the only instance of this class, which manages the SDK specific interface
	 * implementations
	 * @return Factory instance
	 */
	public static BCFactory getInstance()
	{
		return _theInstance;
	}
}
