package com.max2idea.android.limbo.utils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

import org.json.JSONObject;

import com.max2idea.android.limbo.main.Config;

import android.util.Log;

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
			pingSocket.setSoTimeout(60000);
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
		// TODO Auto-generated method stub
		Log.i(TAG, "HMP request" + request);
		out.println(request);
	}

	private static String getResponse(BufferedReader in) throws Exception {
		// TODO Auto-generated method stub
		String line;
		StringBuilder stringBuilder = new StringBuilder("");
		do {
			line = in.readLine();
			if (line != null) {
				Log.i(TAG, "HMP response: " + line);
				JSONObject object = new JSONObject(line);
				try {
					String returnStr = object.getString("return");
					if (returnStr != null) {
						break;
					}
				} catch (Exception ex) {

				}

				stringBuilder.append(line);
			} else
				break;
		} while (true);
		return stringBuilder.toString();
	}

	public static String migrate(boolean block, boolean inc, String uri) {
		// TODO Auto-generated method stub
		// XXX: Detach should not be used via HMP according to docs
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
