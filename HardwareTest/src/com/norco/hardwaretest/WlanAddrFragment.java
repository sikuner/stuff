package com.norco.hardwaretest;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.text.format.Formatter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class WlanAddrFragment extends Fragment implements OnClickListener {

	private TextView tvResult, tvR;
	private Button btnRetest;
	private Handler mHandler;
	WifiManager wfmgr;
	WifiInfo wfInfo;
	
	private final int QUIT = 1009;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {

		View rootView = inflater.inflate(R.layout.wlan_addr_fragment,
				container, false);

		tvResult = (TextView) rootView.findViewById(R.id.tvResult);
		tvR = (TextView) rootView.findViewById(R.id.tvR);
		
		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);
		btnRetest = (Button) rootView.findViewById(R.id.btnRetest);
		btnRetest.setOnClickListener(this);

		wfmgr = (WifiManager) getActivity().getSystemService(
				Context.WIFI_SERVICE);

		mHandler = new Handler() {
			public void handleMessage(android.os.Message msg) {
				switch (msg.what) {
				case 0:
					{ 
						if (!wfmgr.isWifiEnabled()) {
							wfmgr.setWifiEnabled(true);
							tvResult.setText("开启WIFI中...");
							tvResult.invalidate();
							sendEmptyMessageDelayed(2, 2000);
						}
						else {
							sendEmptyMessage(2);
						}
					}
					break;

				case 1: // 打开WIFI
					{
						if (!wfmgr.isWifiEnabled()) {
							wfmgr.setWifiEnabled(true);
							tvResult.setText("开启WIFI中...");
							tvResult.invalidate();
						}
					}
					break;
				case 2: // 获取WIFI信息
					{
						wfmgr = (WifiManager) getActivity().getSystemService(
								Context.WIFI_SERVICE);
						wfInfo = wfmgr.getConnectionInfo();
						String str = "\n SSID:" + wfInfo.getSSID();
						
						int iIP = wfInfo.getIpAddress();
						str += "\n IP:" + Formatter.formatIpAddress(iIP);
						str += "\n BSSID:" + wfInfo.getBSSID();
						str += "\n MAC:" + wfInfo.getMacAddress();
						str += "\n RSSI:" + wfInfo.getRssi();
						str += "\n SPEED:" + wfInfo.getLinkSpeed();
						System.out.println(str);
						System.out.println("iIP = " + iIP);
						if (iIP != 0) {
							tvR.setText("WIFI测试成功");
						}
						else {
							tvR.setText("WIFI测试失败");
						}
						
						tvResult.setText(str);
					}
					break;
					
					case QUIT:
						{
							getActivity().finish();
						}
						break;
					
				default:
					break;
				}

			};
		};
		
		return rootView;
	}

	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		mHandler.sendEmptyMessageDelayed(0, 500);
	}

	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		System.out.println("onDestroy:mHandler.removeCallbacksAndMessages");
		mHandler.removeCallbacksAndMessages(null);
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
				mHandler.sendEmptyMessage(QUIT);
			}
		}
			break;
		case R.id.btnFail: {
			testItem.setResult(getActivity().getString(R.string.fail));
			mHandler.sendEmptyMessage(QUIT);
		}
			break;
		case R.id.btnRetest:
			mHandler.sendEmptyMessage(0);
			break;

		default:

			break;
		}
	}
}
