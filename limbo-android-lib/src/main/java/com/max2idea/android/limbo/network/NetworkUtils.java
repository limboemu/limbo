/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
package com.max2idea.android.limbo.network;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboSettingsManager;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.URL;
import java.util.Enumeration;

/** Utility Class offers simple network support.
  */
public class NetworkUtils {
    private static final String TAG = "NetworkUtils";

    public static void openURL(Activity activity, String url) {
        try {
            Intent fileIntent = new Intent(Intent.ACTION_VIEW);
            fileIntent.setData(Uri.parse(url));
            activity.startActivity(fileIntent);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    public static String getVNCAddress(Context context) {
        String addr = null;
        if (LimboSettingsManager.getEnableExternalVNC(context))
            addr = getExternalIpAddress();
        if(addr == null)
            return getLocalIpAddress();
        return addr;
    }

    public static String getLocalIpAddress() {
        return Config.defaultVNCHost;
    }

    public static String getExternalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && inetAddress.getHostAddress().contains(".")) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException ex) {
            ex.printStackTrace();
        }
        return null;
    }

    public static byte[] getContentFromUrl(String urlPath) throws IOException {
        HttpURLConnection conn = null;
        InputStream is = null;

        byte[] streamData = null;
        try {
            URL url = new URL(urlPath);
            conn = (HttpURLConnection) url.openConnection();
            conn.connect();

            is = conn.getInputStream();
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            int read = 0;
            byte[] buff = new byte[1024];
            while ((read = is.read(buff)) != -1) {
                bos.write(buff, 0, read);
            }
            streamData = bos.toByteArray();
        } catch (IOException e) {
            throw e;
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (conn != null)
                conn.disconnect();
        }
        return streamData;
    }
}
