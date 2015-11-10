package com.example.hfconfig;

import java.util.ArrayList;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

public class HFSetupActivity extends Activity {

	private static String TAG = "zph_HFSetupActivity";

	private Spinner modeSpiner;
	private EditText ipEditText;
	private EditText portEditText;
	private Button confirmBtn;
	private Button clearBtn;
	private Button configOnlyBtn;
	private ApplicationUtil mApplicationUtil;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_hfsmart_config2);

		initUtils();
		initView();
	}

	private void initUtils() {
		mApplicationUtil = (ApplicationUtil) this.getApplication();
	}

	private void initView() {

		confirmBtn = (Button) findViewById(R.id.mode_btn);
		clearBtn = (Button) findViewById(R.id.clear_btn);
		modeSpiner = (Spinner) findViewById(R.id.mode_sp);
		ipEditText = (EditText) findViewById(R.id.ip_text);
		portEditText = (EditText) findViewById(R.id.port_text);
		configOnlyBtn = (Button) findViewById(R.id.config_only_btn);

		confirmBtn.setOnClickListener(new onClickModeBtnListener());
		clearBtn.setOnClickListener(new onClickModeBtnListener());
		configOnlyBtn.setOnClickListener(new onClickModeBtnListener());
		modeSpiner.setOnItemSelectedListener(new onSelectSpinerItem());
		
		ArrayList<String> list = new ArrayList<String>();

		list.add("");
		list.add("1.TCP透传");
		list.add("2.UDP透传");
		list.add("3.开放协议");

		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_spinner_item, list);
		modeSpiner.setAdapter(adapter);

	}

	class onClickModeBtnListener implements OnClickListener {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			switch (arg0.getId()) {
			case R.id.mode_btn:
				if (ipEditText.getText() != null) {
					mApplicationUtil.setIpStr(ipEditText.getText().toString());
				} else {
					mApplicationUtil.setIpStr("");
				}

				if (portEditText.getText() != null) {
					mApplicationUtil.setPortStr(portEditText.getText()
							.toString());
				} else {
					mApplicationUtil.setPortStr("");
				}
				Toast.makeText(HFSetupActivity.this, "Save Success!",
						Toast.LENGTH_SHORT).show();
				break;
			case R.id.clear_btn:
				clearAllInfos();
				break;
			case R.id.config_only_btn:
				break;
			}

		}
	}

	class onSelectSpinerItem implements OnItemSelectedListener {

		@Override
		public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
				long arg3) {
			// TODO Auto-generated method stub

			mApplicationUtil.setConfigMode(arg2);
			Log.d(TAG, "选择的是=" + arg2);

			switch (arg2) {
			case 1:// TCP
				break;
			case 2:// UDP
				break;
			case 3:// OPEN
				break;
			}
		}

		@Override
		public void onNothingSelected(AdapterView<?> arg0) {
			// TODO Auto-generated method stub

		}

	}

	private void clearAllInfos() {
		ipEditText.setText("");
		portEditText.setText("");
		modeSpiner.setSelection(0);
		mApplicationUtil.setIpStr("");
		mApplicationUtil.setPortStr("");
		mApplicationUtil.setConfigMode(0);

	}
}
