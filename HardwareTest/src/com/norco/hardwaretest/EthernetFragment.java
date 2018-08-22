package com.norco.hardwaretest;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.http.conn.util.InetAddressUtils;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SimpleAdapter;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

public class EthernetFragment extends Fragment implements OnClickListener,
		OnItemSelectedListener {

	private Button btnPass, btnFail;
	
	private Spinner spEths;
	private SimpleAdapter adapter;
	private List<String> eths;
	private EditText etHost;
	private Button btnPing;
	private ProgressDialog progressDialog;
	private TextView tvResult, tvStateRes;
	private String ethName;
	
	private TextView tvIpAddr;
	private Button btnIpAddr, btnSetIpAddr;
	
	WifiManager wfmgr;
	
	private final StringBuffer sbResult = new StringBuffer();
	
	private Button btnWifi3G;
	private TextView tvWifi3G;
	private String netState;
	private TextView tvEthRes;
	
	private String pinghostip 	= "192.168.10.3";
	private String hostips 	= "192.168.1.155;192.168.1.156";
	private String[] ethIPs;
	
	private SharedPreferences prePreferences;
	
	Map<String, String> ethMap;
	
	boolean isAllOK = false;
	
	public String getLocalIpAddress() {

		String IpAddr = "";
		List<String> netcards = new ArrayList<String>();
		String netone = "";
		
		try {
			List<NetworkInterface> nilist = Collections.list(NetworkInterface
					.getNetworkInterfaces());
			for (NetworkInterface ni : nilist) {
				List<InetAddress> ialist = Collections.list(ni
						.getInetAddresses());
								
				if (ni.getName().toLowerCase().contains("eth")) {
					
					netone = ni.getName().toLowerCase() + "(" + byte2hex(ni.getHardwareAddress()) + ") : ";
					
					for (InetAddress address : ialist) {
						if (!address.isLoopbackAddress()
								&& InetAddressUtils.isIPv4Address(address
										.getHostAddress())) {
							netone += address.getHostAddress() + " ";
						}
					}
					
					netcards.add(netone);
				}
			}
		} catch (SocketException ex) {
			Log.e("FUCK A", ex.toString());
		}
		
		return netcards.toString();
	}

	public static String byte2hex(byte[] b) {
		StringBuffer hs = new StringBuffer(b.length);
		String stmp = "";
		int len = b.length;
		for (int n = 0; n < len; n++) {
			if (n != 0) {
				hs = hs.append(':');
			}
			
			stmp = Integer.toHexString(b[n] & 0xFF);
			if (stmp.length() == 1) {
				hs = hs.append("0").append(stmp);
			} else {
				hs = hs.append(stmp);
			}
		}
		return String.valueOf(hs);
	}

	private int getEthNames(List<String> eths) { // 获取以太网卡名称: eth0, eth1

		File file = new File("/sys/class/net");
		if (file.isDirectory()) {
			File[] files = file.listFiles();
			for (int i = 0; i < files.length; i++) {
				String fileName = files[i].getName();
				if (fileName.contains("eth")) {
					eths.add(fileName);
				}
			}
		}

		return 0;
	}

	private final Handler pingHandler = new Handler() {
		@Override
		public void handleMessage(final Message msg) {
			switch (msg.what) {
			case 0: {
				System.out.println("pingHandler : 0");

				tvResult.setText(sbResult.toString());

				String strRes = tvResult.getText().toString();
				int idx = strRes.indexOf('%');
				if (idx != -1) {
					String strPercent = strRes.subSequence(idx - 3, idx)
							.toString().replace(',', ' ').trim();
					// tvEthRes.setText("TTXX="+strPercent);
					if (0 == Integer.parseInt(strPercent)) {
						tvEthRes.setText("成功");
						ethMap.put(ethName, "成功");
						displayResult();
					} else {
						tvEthRes.setText("失败");
						ethMap.put(ethName, "失败");
						displayResult();
					}
				}

				if (progressDialog.isShowing()) {
					progressDialog.dismiss();
				}
			}
				break;
			case 1: {
				System.out.println("pingHandler : 1");
				sbResult.delete(0, sbResult.length());
			}
				break;
			default:
				break;
			}
		}
	};

	public void pingPageChange(String str, int status) {

		sbResult.append(str).append("\r\n");
		pingHandler.sendEmptyMessage(0);
	};

	public void cleanScreen() {
		pingHandler.sendEmptyMessage(1);
	}

	public void doPing(final String cmdPing) {
		System.out.println("cmdPing = " + cmdPing);

		new Thread(new Runnable() {
			@Override
			public void run() {
				cleanScreen();// 调用观察者，清除屏幕上的数据
				String str;
				try {
					Process p = Runtime.getRuntime().exec(cmdPing);
					BufferedReader bufferReader = new BufferedReader(
							new InputStreamReader(p.getInputStream()));
					while ((str = bufferReader.readLine()) != null) {
						pingPageChange(str, 0);
					}
					int status = p.waitFor();// 只有status=0时，正常
					bufferReader.close();
				} catch (IOException e) {
					e.printStackTrace();
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}).start();
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.ethernet_fragment, container,
				false);
		
		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnPass.setEnabled(isAllOK);
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		
		prePreferences = getActivity().getSharedPreferences("HardwareTestSettings", getActivity().MODE_PRIVATE);
		pinghostip = prePreferences.getString("pinghostip", pinghostip);
		hostips = prePreferences.getString("hostips", hostips);
		ethIPs = hostips.split(";");
		
		spEths = (Spinner) rootView.findViewById(R.id.spEths);
		btnPing = (Button) rootView.findViewById(R.id.btnPing);
		btnPing.setOnClickListener(this);
		tvResult = (TextView) rootView.findViewById(R.id.tvResult);
		etHost = (EditText) rootView.findViewById(R.id.etHost);
		etHost.setText(pinghostip);
		
		tvStateRes = (TextView) rootView.findViewById(R.id.tvStateRes);
		
		tvEthRes = (TextView) rootView.findViewById(R.id.tvEthRes);
		
		tvIpAddr = (TextView) rootView.findViewById(R.id.tvIpAddr);
		btnIpAddr = (Button) rootView.findViewById(R.id.btnIpAddr);
		btnIpAddr.setOnClickListener(this);
		
		btnSetIpAddr = (Button) rootView.findViewById(R.id.btnSetIpAddr);
		btnSetIpAddr.setOnClickListener(this);
		
		tvWifi3G = (TextView) rootView.findViewById(R.id.tvWifi3G);
		btnWifi3G = (Button) rootView.findViewById(R.id.btnWifi3G);
		btnWifi3G.setOnClickListener(this);
		
		wfmgr = (WifiManager) getActivity().getSystemService(Context.WIFI_SERVICE);
		
		eths = new ArrayList<String>();
		getEthNames(eths);
		ethMap = new HashMap<String, String>();
		for (int i = 0; i < eths.size(); i++) {
			ethMap.put(eths.get(i), "NULL");
		}
		
		ArrayAdapter<String> spAdapter = new ArrayAdapter<String>(
				getActivity(), android.R.layout.simple_spinner_item, eths);
		
		spEths.setAdapter(spAdapter);
		spEths.setOnItemSelectedListener(this);
		
		return rootView;
	}
	
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		wfmgr.setWifiEnabled(false); // WIFI关闭
		setMobileData(getActivity(), false);
	}

	@Override
	public void onDestroyView() {
		// TODO Auto-generated method stub
		super.onDestroyView();
		wfmgr.setWifiEnabled(true); // WIFI开启
		setMobileData(getActivity(), true);
	}

	public ProgressDialog getProgressDialog(String msg) {
		ProgressDialog progressDialog = new ProgressDialog(getActivity());
		progressDialog.setCancelable(true);
		progressDialog.setIndeterminate(true);
		progressDialog.setMessage(msg);
		progressDialog.setCancelable(true);
		
		return progressDialog;
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
				getActivity().finish();
			}
		}
			break;
		case R.id.btnFail: {
			testItem.setResult(getActivity().getString(R.string.fail));
			getActivity().finish();
		}
			break;
		case R.id.btnPing: {
			// progressDialog = ProgressDialog.show(getActivity(), "Ping",
			// "Loading, Please wait...");
			progressDialog = ProgressDialog.show(getActivity(), "Ping",
					"Loading, Please wait...", false, true);
			// getProgressDialog("Loading, Please wait...").show();
			// String cmd = "/system/bin/ping -I eth0 -c 4 192.168.1.253";
			String cmd = String.format("/system/bin/ping -I %s -c 4 %s",
					ethName, etHost.getText().toString());
			System.out.println("cmd = " + cmd);
			doPing(cmd);
		}
			break;
		case R.id.btnIpAddr: {
			tvIpAddr.setText(getLocalIpAddress());
		}
			break;

		case R.id.btnSetIpAddr:	{
				
				String cmd;
				for (int i = 0; i < eths.size(); i++) {
					
					cmd = String.format("busybox ifconfig %s down", eths.get(i));
					RootCmd(cmd);
					cmd = String.format("busybox ifconfig %s up", eths.get(i));
					RootCmd(cmd);
					cmd = String.format("busybox ifconfig %s %s netmask 255.255.255.0", eths.get(i), ethIPs[i]);
					RootCmd(cmd);
				}
			}
			break;			
			
		case R.id.btnWifi3G: {
			// int status = wifiManager.getWifiState();
			// wifistatus.setText("wifi状态 ="+status);
			// WifiManager.WIFI_STATE_DISABLED == wfmgr.getWifiState();
			// WifiManager.WIFI_STATE_ENABLED
			int state = wfmgr.getWifiState();
			switch (wfmgr.getWifiState()) {
			case WifiManager.WIFI_STATE_DISABLED: {
				netState = "WIFI已关闭";
			}
				break;

			case WifiManager.WIFI_STATE_ENABLED: {
				netState = "WIFI已打开";
			}
				break;

			default: {
				netState = "WIFI未知";
			}
				break;
			}
			
			netState += "  ";
			
			ArrayList<Object> arg = new ArrayList<Object>();
			boolean monetState = getMobileDataState(getActivity(),
					arg.toArray());
			if (monetState) {
				netState += "移动网络已开启";
			} else {
				netState += "移动网络未开启";
			}
			
			tvWifi3G.setText(netState);
		}
			break;

		default:

			break;
		}
	}

	@Override
	public void onItemSelected(AdapterView<?> parent, View view, int position,
			long id) {
		tvEthRes.setText("NULL");
		ethName = eths.get(position);
		Toast.makeText(getActivity(), ethName, Toast.LENGTH_SHORT).show();
		tvResult.setText("");
	}

	@Override
	public void onNothingSelected(AdapterView<?> parent) {
		// TODO Auto-generated method stub

	}

	private void displayResult() {
		
		String strAllRes = "";
		
		String strRes = "";
		int ethOKNum = 0;
		for (int i = 0; i < eths.size(); i++) {

			if (i != 0) {
				strRes += ", ";
			}

			String str = ethMap.get(eths.get(i));
			if (str.contains("成功")) {
				ethOKNum++;
			}
			
			strRes += eths.get(i) + ":" + ethMap.get(eths.get(i));
		}
		
		isAllOK = (ethOKNum == eths.size());
		strAllRes = "测试结果:" + (isAllOK ? "成功" : "失败") + "  ("
				+ strRes + ")";
		
		btnPass.setEnabled(isAllOK);
		tvStateRes.setText(strAllRes);
	}
	
	/**
	 * 设置手机的移动数据
	 */
	public void setMobileData(Context pContext, boolean pBoolean) {

		try {

			ConnectivityManager mConnectivityManager = (ConnectivityManager) pContext
					.getSystemService(Context.CONNECTIVITY_SERVICE);

			Class ownerClass = mConnectivityManager.getClass();

			Class[] argsClass = new Class[1];
			argsClass[0] = boolean.class;

			Method method = ownerClass.getMethod("setMobileDataEnabled",
					argsClass);

			method.invoke(mConnectivityManager, pBoolean);

		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			System.out.println("移动数据设置错误: " + e.toString());
		}
	}

	/**
	 * 返回手机移动数据的状态
	 * 
	 * @param pContext
	 * @param arg
	 *            默认填null
	 * @return true 连接 false 未连接
	 */
	public boolean getMobileDataState(Context pContext, Object[] arg) {

		try {

			ConnectivityManager mConnectivityManager = (ConnectivityManager) pContext
					.getSystemService(Context.CONNECTIVITY_SERVICE);

			Class ownerClass = mConnectivityManager.getClass();

			Class[] argsClass = null;
			if (arg != null) {
				argsClass = new Class[1];
				argsClass[0] = arg.getClass();
			}

			Method method = ownerClass.getMethod("getMobileDataEnabled",
					argsClass);

			Boolean isOpen = (Boolean) method.invoke(mConnectivityManager, arg);

			return isOpen;

		} catch (Exception e) {
			// TODO: handle exception

			System.out.println("得到移动数据状态出错");
			return false;
		}
		
	}
	
	public static void RootCmd(String cmd) 
	{
		System.out.println("cmd = " + cmd);
		
		Process process = null;
		DataOutputStream os = null;
		
		try {
			process = Runtime.getRuntime().exec("su");
			Log.i("tag", cmd);
			os = new DataOutputStream(process.getOutputStream());
			os.writeBytes(cmd + "\n");
			os.writeBytes("exit\n");
			os.flush();
			int a = process.waitFor();
			Log.i("tag", "reslut:" + a);
		} catch (Exception e) {
		} finally {
			try {
				if (os != null) {
					os.close();
				}
				// process.destroy();
			} catch (Exception e) {
			}
		}
	}
	
}
