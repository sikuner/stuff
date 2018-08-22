package com.norco.hardwaretest;

import java.util.List;

import android.content.Context;
import android.net.wifi.ScanResult;
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

public class WifiFragment extends Fragment implements OnClickListener {
	
	private Button btnPass, btnFail;
	
	WifiManager wfmgr;
	WifiInfo wfInfo;
	
	private TextView tvWifiAddr;
	private TextView tvWifiAddrCon;
	private TextView tvWifiAp;
	private TextView tvWifiApCon;
	private TextView tvWifiRes;
	private Button btnRetry;
	boolean bWifiAddr = false;
	boolean bWifiAp = false;
	
	private final int WIFI_OPEN = 1001;
	private final int WIFI_INFO = 1002;
	private final int WIFI_SCAN = 1003;
	private final int FRAG_QUIT = 1009;
	
	private Handler mHandler = new Handler() {

		public void handleMessage(android.os.Message msg) {

			switch (msg.what) {
			case WIFI_OPEN: {
				
				if (!wfmgr.isWifiEnabled()) {
					wfmgr.setWifiEnabled(true);
					tvWifiAddr.setText("WIFI地址信息" + "开启WIFI中...");
					tvWifiAddr.invalidate();
					sendEmptyMessageDelayed(WIFI_INFO, 2000);
				} else {
					sendEmptyMessage(WIFI_INFO);
				}
			}
				break;
			case WIFI_INFO: {
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
					tvWifiAddr.setText("WIFI地址信息:  " + "通过");
					bWifiAddr = true;
				} else {
					tvWifiAddr.setText("WIFI地址信息:  " + "不通过");
					bWifiAddr = false;
				}
				tvWifiAddr.invalidate();

				tvWifiAddrCon.setText(str);

				sendEmptyMessage(WIFI_SCAN);
			}
				break;
			case WIFI_SCAN: {

				wfInfo = wfmgr.getConnectionInfo();
				wfmgr.startScan();
				List<ScanResult> lstScan = wfmgr.getScanResults();
				
				String strScan = "扫描WIFI热点... \n";
				
				for (int i = 0; i < lstScan.size(); i++) {
					strScan += (i + 1) + "." + lstScan.get(i).toString() + "\n";
				}
				
				tvWifiApCon.setText(strScan);
				
				if (lstScan.size() > 0) {
					tvWifiAp.setText("WIFI热点信息:  " + "通过");
					bWifiAp = true;
				} else {
					tvWifiAp.setText("WIFI热点信息:  " + "不通过");
					bWifiAp = false;
				}
				
				if (bWifiAddr && bWifiAp) {
					tvWifiRes.setText("成功");
					btnPass.setEnabled(true);
				}
				else {
					tvWifiRes.setText("失败");
				}
			}
				break;

			case FRAG_QUIT: {
				getActivity().finish();
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
		View rootView = inflater.inflate(R.layout.wifi_fragment, container,
				false);
		
		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnPass.setEnabled(false);
		
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		
		tvWifiAddr = (TextView) rootView.findViewById(R.id.tvWifiAddr);
		tvWifiAddrCon = (TextView) rootView.findViewById(R.id.tvWifiAddrCon);
		tvWifiAp = (TextView) rootView.findViewById(R.id.tvWifiAp);
		tvWifiApCon = (TextView) rootView.findViewById(R.id.tvWifiApCon);
		tvWifiRes = (TextView) rootView.findViewById(R.id.tvWifiRes);
		btnRetry = (Button) rootView.findViewById(R.id.btnRetry);
		btnRetry.setOnClickListener(this);
		
		wfmgr = (WifiManager) getActivity().getSystemService(Context.WIFI_SERVICE);
		
		return rootView;
	}

	@Override
	public void onStart() {
		super.onStart();

		mHandler.sendEmptyMessageDelayed(WIFI_OPEN, 500);
	}

	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {
		case R.id.btnPass: {
			testItem.setResult(getActivity().getString(R.string.pass));

			Fragment fg = null;
			try {
				
				fg = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos + 1]).getFragment();
				
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
				// mHandler.sendEmptyMessage(QUIT);
			}
		}
			break;
		case R.id.btnFail: {
			testItem.setResult(getActivity().getString(R.string.fail));
			mHandler.sendEmptyMessage(FRAG_QUIT);
		}
			break;
		case R.id.btnRetry: {
			
			tvWifiAddr.setText("WIFI地址信息");
			tvWifiAddrCon.setText("WIFI地址信息内容");
			tvWifiAp.setText("WIFI地址信息");
			tvWifiApCon.setText("WIFI热点信息内容");
			tvWifiRes.setText("NULL");
			
			btnPass.setEnabled(false);
			mHandler.sendEmptyMessage(WIFI_OPEN);
		}
			break;

		default:

			break;
		}
	}

}
