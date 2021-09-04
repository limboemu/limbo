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
package com.max2idea.android.limbo.qmp;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.util.Log;

import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboApplication;
import com.max2idea.android.limbo.toast.ToastUtils;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

/** A simple QMP CLient that is needed for communicating with QEMU. You can use it for
 * changing VNC password, checking the status when saving the vm, and change removable drives.
  */
public class QmpClient {

	private static final String TAG = "QmpClient";
	private static final String requestCommandMode = "{ \"execute\": \"qmp_capabilities\" }";
	private static boolean external = false;

	public static void setExternal(boolean value) {
		external = value;
	}
	public synchronized static String sendCommand(String command) {
		String response = null;
		int trial=0;
		Socket pingSocket = null;
		LocalSocket localSocket = null;
		PrintWriter out = null;
		BufferedReader in = null;

		try {
		    if(external) {
                pingSocket = new Socket(Config.QMPServer, Config.QMPPort);
                pingSocket.setSoTimeout(5000);
                out = new PrintWriter(pingSocket.getOutputStream(), true);
                in = new BufferedReader(new InputStreamReader(pingSocket.getInputStream()));
		    } else {
		        localSocket = new LocalSocket();
		        String localQMPSocketPath = LimboApplication.getLocalQMPSocketPath();
                LocalSocketAddress localSocketAddr = new LocalSocketAddress(localQMPSocketPath, LocalSocketAddress.Namespace.FILESYSTEM);
                localSocket.connect(localSocketAddr);
                localSocket.setSoTimeout(5000);
                out = new PrintWriter(localSocket.getOutputStream(), true);
                in = new BufferedReader(new InputStreamReader(localSocket.getInputStream()));
            }
			sendRequest(out, QmpClient.requestCommandMode);
			response = tryGetResponse(in);
			sendRequest(out, command);
			response = tryGetResponse(in);
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (out != null)
				out.close();
			try {
				if (in != null)
					in.close();
				if (pingSocket != null)
					pingSocket.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		if(Config.debugQmp)
			Log.d(TAG, "Response: " + response);
		return response;
	}

	private static String tryGetResponse(BufferedReader in) throws Exception {
		String response = null;
		int trial = 0;
		while((response = getResponse(in)).equals("") && trial < 10){
			Thread.sleep(1000);
			trial++;
		}
		return response;
	}

	private static void sendRequest(PrintWriter out, String request) {

	    if(Config.debugQmp)
		    Log.i(TAG, "QMP request" + request);
		out.println(request);
	}

    private static String getResponse(BufferedReader in) throws Exception {
        String line;
        StringBuilder stringBuilder = new StringBuilder("");
        try {
            do {
                line = in.readLine();
                if (line != null) {
                    if(Config.debugQmp)
                        Log.i(TAG, "QMP response: " + line);
                    JSONObject object = new JSONObject(line);
                    String returnStr = null;
                    String errStr = null;

                    try {
                        if(line.contains("return"))
                            returnStr = object.getString("return");
                    } catch (Exception ex) {
						if(Config.debugQmp)
                            ex.printStackTrace();
                    }

                    if (returnStr != null) {
						stringBuilder.append(line);
						stringBuilder.append("\n");
                        break;
                    }

                    try {
                        if(line.contains("error"))
                            errStr = object.getString("error");
                    } catch (Exception ex) {
                        if(Config.debugQmp)
						    ex.printStackTrace();
                    }
                    stringBuilder.append(line);
                    stringBuilder.append("\n");
                    if (errStr != null) {
                        break;
                    }
                } else
                    break;
            } while (true);
        } catch (Exception ex) {
            Log.e(TAG, "Could not get Response: " + ex.getMessage());
            if(Config.debugQmp)
                ex.printStackTrace();
        }
        return stringBuilder.toString();
    }

	private static String getQueryMigrateResponse(BufferedReader in) throws Exception {

		String line;
		StringBuilder stringBuilder = new StringBuilder("");

		try {
			do {
				line = in.readLine();
				if (line != null) {
				    if(Config.debugQmp)
					    Log.i(TAG, "QMP query-migrate response: " + line);
					JSONObject object = new JSONObject(line);
					String returnStr = null;
					String errStr = null;

					try {
						returnStr = object.getString("return");
					} catch (Exception ex) {

					}

					if (returnStr != null) {
						break;
					}

					try {
						errStr = object.getString("error");
					} catch (Exception ex) {

					}
					stringBuilder.append(line);
					stringBuilder.append("\n");

					if (errStr != null) {
						break;
					}
				} else
					break;
			} while (true);
		} catch (Exception ex) {

		}
		return stringBuilder.toString();
	}

	public static String getMigrateCommand(boolean block, boolean inc, String uri) {
		
		// XXX: Detach should not be used via QMP according to docs
		// return "{\"execute\":\"migrate\",\"arguments\":{\"detach\":" + detach
		// + ",\"blk\":" + block + ",\"inc\":" + inc
		// + ",\"uri\":\"" + uri + "\"},\"id\":\"limbo\"}";

		// its better not to use block (full disk copy) cause its slow (though
		// safer)
		// see qmp-commands.hx for more info
		return "{\"execute\":\"migrate\",\"arguments\":{\"blk\":" + block + ",\"inc\":" + inc + ",\"uri\":\"" + uri
				+ "\"},\"id\":\"limbo\"}";
	}

    public static String getChangeVncPasswdCommand(String passwd) {
		return "{\"execute\": \"change\", \"arguments\": { \"device\": \"vnc\", \"target\": \"password\", \"arg\": \"" + passwd +"\" } }";
    }

    public static String getEjectDeviceCommand(String dev) {
        return "{ \"execute\": \"eject\", \"arguments\": { \"device\": \""+ dev +"\" } }";
    }

    public static String getChangeDeviceCommand(String dev, String value) {
        return "{ \"execute\": \"change\", \"arguments\": { \"device\": \""+dev+"\", \"target\": \"" + value + "\" } }";
    }

    public static String getQueryMigrationCommand() {
		return "{ \"execute\": \"query-migrate\" }";
	}

	public static String getStopVMCommand() {
		return "{ \"execute\": \"stop\" }";
	}

	public static String getContinueVMCommand() {
		return "{ \"execute\": \"cont\" }";
	}

	public static String getPowerDownCommand() {
		return "{ \"execute\": \"system_powerdown\" }";
	}

	public static String getResetCommand() {
		return "{ \"execute\": \"system_reset\" }";
	}

	public static String getStateCommand() {
		return "{ \"execute\": \"query-status\" }";
	}
}
