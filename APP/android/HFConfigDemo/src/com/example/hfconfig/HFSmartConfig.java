package com.example.hfconfig;

import java.util.ArrayList;
import java.util.List;

import com.espressif.iot.esptouch.EsptouchTask;
import com.espressif.iot.esptouch.IEsptouchResult;
import com.espressif.iot.esptouch.IEsptouchTask;
import com.espressif.iot.esptouch.demo_activity.EspWifiAdminSimple;
import com.espressif.iot.esptouch.task.__IEsptouchTask;

import android.R.integer;
import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnCancelListener;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;
import android.widget.Toast;

public class HFSmartConfig extends Activity {

	private static String TAG = "zphlog";
	private ApplicationUtil mApplicationUtil;
	private Button configBtn;
	private TextView ssidLabel;
	private EditText passwdText;
	private Spinner countSp;

	private EspWifiAdminSimple mWifiAdmin;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_hfsmart_config1);

		initUtils();
		initViews();
	}

	protected void onResume() {
		super.onResume();
		String apSsid = mWifiAdmin.getWifiConnectedSsid();
		if (apSsid != null) {
			ssidLabel.setText(apSsid);
		} else {
			ssidLabel.setText("");
		}

	}

	private void initUtils() {
		mApplicationUtil = (ApplicationUtil) this.getApplication();
		mApplicationUtil.setContex(HFSmartConfig.this);
		mWifiAdmin = new EspWifiAdminSimple(this);
	}

	private void initViews() {
		configBtn = (Button) findViewById(R.id.config_btn);
		ssidLabel = (TextView) findViewById(R.id.ssid_tv);
		passwdText = (EditText) findViewById(R.id.passwd_et);
		countSp = (Spinner) findViewById(R.id.count_sp);
		configBtn.setOnClickListener(new onClickedBtn());

		ArrayList<Integer> list = new ArrayList<Integer>();
		for (int i = 0; i <= 5; i++) {
			list.add(i);
		}
		ArrayAdapter<Integer> adapter = new ArrayAdapter<Integer>(this,
				android.R.layout.simple_spinner_item, list);
		countSp.setAdapter(adapter);
		countSp.setSelection(1);
	}

	class onClickedBtn implements OnClickListener {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			switch (arg0.getId()) {
			case R.id.config_btn:
				startConfigBtn();
				break;
			default:
				break;
			}
		}

	}

	class onCountSpinerSelect implements OnItemSelectedListener {

		@Override
		public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
				long arg3) {
			// TODO Auto-generated method stub
			int num = (Integer) arg0.getItemAtPosition(arg2);
			Log.d(TAG, "选中的数字=" + num);
		}

		@Override
		public void onNothingSelected(AdapterView<?> arg0) {
			// TODO Auto-generated method stub

		}

	}

	private void startConfigBtn() {

		String apSsid = ssidLabel.getText().toString();
		String apPassword = passwdText.getText().toString();
		String apBssid = mWifiAdmin.getWifiConnectedBssid();
		String isSsidHiddenStr = "NO";
		String taskResultCountStr = Integer.toString(countSp
				.getSelectedItemPosition());

		apSsid = getAllSSID();
		apPassword = getAllPasswd();
		Log.d(TAG, "apSsid=" + apSsid + "   apPassword=" + apPassword);
		if (apPassword == null || apPassword.equals("")) {
			Toast.makeText(HFSmartConfig.this, "The HFsetInfos is null",
					Toast.LENGTH_LONG).show();
		}
		new EsptouchAsyncTask3().execute(apSsid, apBssid, apPassword,
				isSsidHiddenStr, taskResultCountStr);

	}

	private class EsptouchAsyncTask3 extends
			AsyncTask<String, Void, List<IEsptouchResult>> {

		private ProgressDialog mProgressDialog;

		private IEsptouchTask mEsptouchTask;
		// without the lock, if the user tap confirm and cancel quickly enough,
		// the bug will arise. the reason is follows:
		// 0. task is starting created, but not finished
		// 1. the task is cancel for the task hasn't been created, it do nothing
		// 2. task is created
		// 3. Oops, the task should be cancelled, but it is running
		private final Object mLock = new Object();

		@Override
		protected void onPreExecute() {
			mProgressDialog = new ProgressDialog(HFSmartConfig.this);
			mProgressDialog
					.setMessage("HFSmart is configuring, please wait for a moment...");
			mProgressDialog.setCanceledOnTouchOutside(false);
			mProgressDialog.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					synchronized (mLock) {
						if (__IEsptouchTask.DEBUG) {
							Log.i(TAG, "progress dialog is canceled");
						}
						if (mEsptouchTask != null) {
							mEsptouchTask.interrupt();
						}
					}
				}
			});
			mProgressDialog.setButton(DialogInterface.BUTTON_POSITIVE,
					"Waiting...", new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
						}
					});
			mProgressDialog.show();
			mProgressDialog.getButton(DialogInterface.BUTTON_POSITIVE)
					.setEnabled(true);
		}

		@Override
		protected List<IEsptouchResult> doInBackground(String... params) {
			int taskResultCount = -1;
			synchronized (mLock) {
				String apSsid = params[0];
				String apBssid = params[1];
				String apPassword = params[2];
				String isSsidHiddenStr = params[3];
				String taskResultCountStr = params[4];
				boolean isSsidHidden = false;
				if (isSsidHiddenStr.equals("YES")) {
					isSsidHidden = true;
				}
				taskResultCount = Integer.parseInt(taskResultCountStr);
				mEsptouchTask = new EsptouchTask(apSsid, apBssid, apPassword,
						isSsidHidden, HFSmartConfig.this);
			}
			List<IEsptouchResult> resultList = mEsptouchTask
					.executeForResults(taskResultCount);
			return resultList;
		}

		@Override
		protected void onPostExecute(List<IEsptouchResult> result) {
			mProgressDialog.getButton(DialogInterface.BUTTON_POSITIVE)
					.setEnabled(true);
			mProgressDialog.getButton(DialogInterface.BUTTON_POSITIVE).setText(
					"Confirm");
			IEsptouchResult firstResult = result.get(0);
			// check whether the task is cancelled and no results received
			if (!firstResult.isCancelled()) {
				int count = 0;
				// max results to be displayed, if it is more than
				// maxDisplayCount,
				// just show the count of redundant ones
				final int maxDisplayCount = 5;
				// the task received some results including cancelled while
				// executing before receiving enough results
				if (firstResult.isSuc()) {
					StringBuilder sb = new StringBuilder();
					for (IEsptouchResult resultInList : result) {
						sb.append("Esptouch success, bssid = "
								+ resultInList.getBssid()
								+ ",InetAddress = "
								+ resultInList.getInetAddress()
										.getHostAddress() + "\n");
						count++;
						if (count >= maxDisplayCount) {
							break;
						}
					}
					if (count < result.size()) {
						sb.append("\nthere's " + (result.size() - count)
								+ " more result(s) without showing\n");
					}
					mProgressDialog.setMessage(sb.toString());
				} else {
					mProgressDialog.setMessage("Esptouch fail");
				}
			}
		}
	}

	private String getAllSSID() {
		// return null;
		int modeNum = mApplicationUtil.getConfigMode();
		String ipStr = mApplicationUtil.getIpStr();
		String portStr = mApplicationUtil.getPortStr();
		String result = "";

		/*
		 * boolean isInfoNil = isConfigInfosNil(modeNum, ipStr, portStr); if
		 * (isInfoNil) {// 空 return mWifiAdmin.getWifiConnectedSsid(); } else
		 * {// 非空 result = mWifiAdmin.getWifiConnectedSsid() + "" +
		 * getPasswdWithEdit(passwdText); return result; }
		 */

		result = mWifiAdmin.getWifiConnectedSsid();

		return result;
	}

	// theHexSendStr(modeNum, ipStr, portStr)
	private String getAllPasswd() {
		String result = "";
		int modeNum = mApplicationUtil.getConfigMode();
		String ipStr = mApplicationUtil.getIpStr();
		String portStr = mApplicationUtil.getPortStr();
		boolean isInfoNil = isConfigInfosNil(modeNum, ipStr, portStr);
		/*
		 * if (isInfoNil) {// 空 result = getPasswdWithEdit(passwdText); return
		 * result; } else {// 非空 result = theHexSendStr(modeNum, ipStr,
		 * portStr); return result; }
		 */
		result = theHexSendStr(modeNum, ipStr, portStr) + "-"
				+ getPasswdWithEdit(passwdText) + "";
		return result;
	}

	private boolean isConfigInfosNil(int modeNum, String ipStr, String portStr) {

		Log.d(TAG, "modeNum=" + modeNum + "  ipStr" + ipStr + "  portStr="
				+ portStr);
		boolean isInfoNil = true;
		if (modeNum == 0) {
			// 返回空
			isInfoNil = true;
		} else if (modeNum == 3) {
			// 非空
			isInfoNil = false;
		} else {

			if (ipStr != null && !ipStr.equals("") && portStr != null
					&& !portStr.equals("")) {
				// 非空
				isInfoNil = false;
			} else {
				isInfoNil = true;
			}
		}
		Log.d(TAG, "获得的信息是空=" + isInfoNil);
		return isInfoNil;
	}

	private String getPasswdWithEdit(EditText edit) {
		if (edit.getText() != null) {
			return edit.getText().toString();
		} else {
			return "";
		}
	}

	private String theHexSendStr(int modeNum, String ipStr, String portStr) {

		String result = "<@#&>-" + modeNum + "-"
				+ mApplicationUtil.IpStrToHexStr(ipStr) + "-"
				+ mApplicationUtil.IpStrToHexStr(portStr);
		;
		return result;
	}
}
