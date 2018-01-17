package com.max2idea.android.limbo.utils;

import android.util.Log;

import com.max2idea.android.limbo.main.Config;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class QmpClient {

	private static final String TAG = "QMPClient";
	private static String requestCommandMode = "{ \"execute\": \"qmp_capabilities\" }";

	public static String sendCommand(String command) {
		String response = null;

		Socket pingSocket = null;
		PrintWriter out = null;
		BufferedReader in = null;

		try {
			pingSocket = new Socket(Config.QMPServer, Config.QMPPort);
			pingSocket.setSoTimeout(5000);
			out = new PrintWriter(pingSocket.getOutputStream(), true);
			in = new BufferedReader(new InputStreamReader(pingSocket.getInputStream()));

			sendRequest(out, QmpClient.requestCommandMode);
			response = getResponse(in);
			sendRequest(out, command);
			response = getResponse(in);

		} catch (Exception e) {
			// TODO Auto-generated catch block
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
                    Log.i(TAG, "QMP response: " + line);
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

	public static String stop() {
		return "{ \"execute\": \"stop\" }";

	}

	public static String cont() {
		return "{ \"execute\": \"cont\" }";

	}
}
