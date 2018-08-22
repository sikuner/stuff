package com.norco.hardwaretest;

import java.util.List;

import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.TextView;

public class WlanApScanFragment extends Fragment implements OnClickListener {

	private TextView tvResult, tvOpen, tvScan;
	private WifiManager wfmgr;
	private WifiInfo wfInfo;

	private final int WIFI_OPEN = 1000;
	private final int WIFI_SCAN = 1001;

	private Handler hdr = new Handler() {
		public void handleMessage(android.os.Message msg) {
			wfmgr = (WifiManager) getActivity().getSystemService(
					Context.WIFI_SERVICE);

			switch (msg.what) {
			case WIFI_OPEN: {
				if (!wfmgr.isWifiEnabled()) { // 没有打开
					wfmgr.setWifiEnabled(true);
					tvOpen.setText("开启WIFI中...");
					tvOpen.invalidate();
				}
				else {
					tvOpen.setText("开启WIFI中... 通过 \n");
				}
			}
				break;
			case WIFI_SCAN: {
				if (wfmgr.isWifiEnabled()) {
					tvOpen.setText("开启WIFI中... 通过 \n");
				} else {
					tvOpen.setText("开启WIFI中... 不通过 \n");
					tvResult.setText("测试结果: 不通过");
					
					return;
				}

				wfInfo = wfmgr.getConnectionInfo();
				wfmgr.startScan();
				List<ScanResult> lstScan = wfmgr.getScanResults();

				String strScan = "扫描WIFI热点... \n";

				for (int i = 0; i < lstScan.size(); i++) {
					strScan += (i + 1) + "." + lstScan.get(i).toString() + "\n";
				}

				tvScan.setText(strScan);

				if (lstScan.size() > 0) {
					tvResult.setText("测试结果: 通过");
				} else {
					tvResult.setText("测试结果: 不通过");
				}
			}
				break;

			default:

				break;
			}

		};
	};

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.wlanap_scan_fragment,
				container, false);

		tvResult = (TextView) rootView.findViewById(R.id.tvResult);
		tvOpen = (TextView) rootView.findViewById(R.id.tvOpen);
		tvScan = (TextView) rootView.findViewById(R.id.tvScan);

		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);

		tvResult = (TextView) rootView.findViewById(R.id.tvResult);

		return rootView;
	}

	@Override
	public void onStart() {
		super.onStart();
		
		hdr.sendEmptyMessage(WIFI_OPEN);
		hdr.sendEmptyMessageDelayed(WIFI_SCAN, 1500);
	}

	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		hdr.removeCallbacksAndMessages(null);
	}
	
	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(pos);
		switch (v.getId()) {
		case R.id.btnPass: {
			testItem.setResult(getActivity().getString(R.string.pass));

			Fragment fg = null;
			try {
				fg = MainActivity.getTestItemE(pos + 1).getFragment();
				
			} catch (Exception e) {
				System.out.println("ItemFragment:" + e.toString());
				fg = null;
			}

			if (fg != null) {

				Bundle b = new Bundle();
				b.putInt("position", pos + 1);
				fg.setArguments(b);

				getFragmentManager().beginTransaction()
						.replace(R.id.fragment_container, fg).commit();
			} else {
				getActivity().finish();
			}
		}
			break;
		case R.id.btnFail: {
			testItem.setResult(getActivity().getString(R.string.fail));
			getActivity().finish();
		}
			break;
		default:

			break;
		}
	}
}
