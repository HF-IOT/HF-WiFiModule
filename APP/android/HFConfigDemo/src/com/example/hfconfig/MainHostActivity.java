package com.example.hfconfig;

import android.app.Activity;
import android.app.TabActivity;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.widget.TabHost;
import android.widget.TabHost.TabSpec;

public class MainHostActivity extends TabActivity {

	private TabHost mTabHost;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main);
		initTabViews();
	}

	private void initTabViews() {

		Resources res = getResources();
		mTabHost = this.getTabHost();
		TabSpec tabSpec;
		Intent intent = new Intent();
		intent.setClass(this, HFSmartConfig.class);
		tabSpec = mTabHost
				.newTabSpec("hfconfig")
				.setIndicator("hfconfig",
						res.getDrawable(android.R.drawable.ic_media_previous))
				.setContent(intent);
		mTabHost.addTab(tabSpec);
		intent = new Intent();
		intent.setClass(this, HFSetupActivity.class);
		intent.putExtra("name", "value");
		tabSpec = mTabHost
				.newTabSpec("hfsetinfo")
				.setIndicator("hfsetinfo",
						res.getDrawable(android.R.drawable.ic_media_play))
				.setContent(intent);
		mTabHost.addTab(tabSpec);
		mTabHost.setCurrentTabByTag("hfconfig");
	}

}
