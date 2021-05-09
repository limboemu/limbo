package com.max2idea.android.limbo.utils;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.util.Log;

import com.max2idea.android.limbo.main.Config;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class QmpClient {

	private static final String TAG = "QmpClient";
	private static String requestCommandMode = "{ \"execute\": \"qmp_capabilities\" }";
	public static boolean allow_external = false;

	public synchronized static String sendCommand(String command) {
		String response = null;
		int trial=0;
		Socket pingSocket = null;
		LocalSocket localSocket = null;
		PrintWriter out = null;
		BufferedReader in = null;

		try {
		    if(allow_external) {
                pingSocket = new Socket(Config.QMPServer, Config.QMPPort);
                pingSocket.setSoTimeout(5000);
                out = new PrintWriter(pingSocket.getOutputStream(), true);
                in = new BufferedReader(new InputStreamReader(pingSocket.getInputStream()));
		    } else {
		        localSocket = new LocalSocket();
		        String localQMPSocketPath = Config.getLocalQMPSocketPath();
                LocalSocketAddress localSocketAddr = new LocalSocketAddress(localQMPSocketPath, LocalSocketAddress.Namespace.FILESYSTEM);
                localSocket.connect(localSocketAddr);
                localSocket.setSoTimeout(5000);
                out = new PrintWriter(localSocket.getOutputStream(), true);
                in = new BufferedReader(new InputStreamReader(localSocket.getInputStream()));
            }


			sendRequest(out, QmpClient.requestCommandMode);
			while(true){
                response = getResponse(in);
                if(response == null || response.equals("") || trial <10)
				    break;

                Thread.sleep(1000);
				trial++;
			}

			sendRequest(out, command);
			trial=0;
			while((response = getResponse(in)).equals("") && trial < 10){
				Thread.sleep(1000);
				trial++;
			}
		} catch (java.net.ConnectException e) {
			Log.w(TAG, "Could not connect to QMP: " + e);
			if(Config.debugQmp)
			    e.printStackTrace();
		} catch(Exception e) {
			// TODO Auto-generated catch block
            Log.e(TAG, "Error while connecting to QMP: " + e);
            if(Config.debugQmp)
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
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

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

	public static String migrate(boolean block, boolean inc, String uri) {
		
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

    public static String changevncpasswd(String passwd) {

		return "{\"execute\": \"change\", \"arguments\": { \"device\": \"vnc\", \"target\": \"password\", \"arg\": \"" + passwd +"\" } }";

    }

    public static String ejectdev(String dev) {

        return "{ \"execute\": \"eject\", \"arguments\": { \"device\": \""+ dev +"\" } }";

    }

    public static String changedev(String dev, String value) {

        return "{ \"execute\": \"change\", \"arguments\": { \"device\": \""+dev+"\", \"target\": \"" + value + "\" } }";

    }



    public static String query_migrate() {
		return "{ \"execute\": \"query-migrate\" }";

	}

	public static String save_snapshot(String snapshot_name) {
		return "{\"execute\": \"snapshot-create\", \"arguments\": {\"name\": \""+ snapshot_name+"\"} }";

	}

	public static String query_snapshot() {
		return "{ \"execute\": \"query-snapshot-status\" }";

	}

	public static String stop() {
		return "{ \"execute\": \"stop\" }";

	}

	public static String cont() {
		return "{ \"execute\": \"cont\" }";

	}

	public static String powerDown() {
		return "{ \"execute\": \"system_powerdown\" }";

	}

	public static String reset() {
		return "{ \"execute\": \"system_reset\" }";

	}

	public static String getState() {
		return "{ \"execute\": \"query-status\" }";

	}
}
