package com.example.hfconfig;

import android.app.Application;
import android.content.Context;
import android.widget.Toast;

public class ApplicationUtil extends Application {
	private String mIpStr;
	private String mPortStr;
	private int mConfigMod;
	private static Context mContext;

	public void setContex(Context context) {
		this.mContext = context;
	}

	public String getIpStr() {
		return this.mIpStr;
	}

	public void setIpStr(String ip) {
		this.mIpStr = ip;
	}

	public String getPortStr() {
		return this.mPortStr;
	}

	public void setPortStr(String port) {
		this.mPortStr = port;
	}

	public int getConfigMode() {
		return mConfigMod;
	}

	public void setConfigMode(int mode) {
		this.mConfigMod = mode;
	}

	public String IpStrToHexStr(String ipStr) {
		String result = "";
		if (ipStr != null) {
			String[] ip = new String[4];
			ip = ipStr.split("\\.");
			System.out.println("length=" + ip.length);
			for (int i = 0; i < ip.length; i++) {
				System.out.println("result=" + result);
				result += intStrToHexStr(ip[i]);
			}
		}

		return result.toUpperCase();
	}

	private static String intStrToHexStr(String str) {

		String hexStr = "";
		try {
			Integer num = Integer.valueOf(str);
			hexStr = Integer.toHexString(num);
			if (hexStr.length() == 1) {
				hexStr = "0" + hexStr;
			}
		} catch (Exception e) {
			if (str != null && !str.equals("")) {
				Toast.makeText(mContext, "Please Check the IP/Port is ok?",
						Toast.LENGTH_LONG).show();
			}
			System.out.println("³¬³ö·¶Î§ÁË");
		}
		return hexStr;
	}
}
