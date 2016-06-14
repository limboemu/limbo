package com.max2idea.android.limbo.utils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

import org.json.JSONObject;

import com.max2idea.android.limbo.main.Config;

public class QmpClient {

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

			out.println("{ \"execute\": \"qmp_capabilities\" }");
			response = getResponse(in);
			out.println(command);
			response = getResponse(in);
			response = getResponse(in);
			if(response.equals("STOP")){
				
			}
	

		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			out.close();
			try {
				in.close();
				pingSocket.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		}

		return response;
	}

	private static String getResponse(BufferedReader in) throws Exception {
		// TODO Auto-generated method stub
		String line;
		StringBuilder stringBuilder = new StringBuilder("");
		do {
			line = in.readLine();
			if (line != null) {
				JSONObject object = new JSONObject(line);
				try {
					String returnStr = object.getString("return");
					if(returnStr!=null){
						break;
					}
				} catch (Exception ex) {

				}
				
				try {
					
					String event = object.getString("event");
					if(event!=null){
						return event;
					}
				} catch (Exception ex) {

				}
				
				stringBuilder.append(line);
			} else
				break;
		} while (true);
		return stringBuilder.toString();
	}

}
