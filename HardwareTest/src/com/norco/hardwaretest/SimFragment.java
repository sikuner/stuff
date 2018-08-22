package com.norco.hardwaretest;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class SimFragment extends Fragment implements OnClickListener {

	TelephonyManager telMgr;
	MyPhoneStateListener myListener;

	private TextView tvInfo, tvState;
	private String simArgs;
	private Button btnState;
	private boolean isConn;
	private boolean hasOperator = false;
	private TextView tvFinalRes;

	private TextView tvPingRes;

	private Button btnPass, btnFail;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.sim_fragment, container,
				false);

		btnState = (Button) rootView.findViewById(R.id.btnState);
		btnState.setOnClickListener(this);
		tvState = (TextView) rootView.findViewById(R.id.tvState);
		tvFinalRes = (TextView) rootView.findViewById(R.id.tvFinalRes);

		telMgr = (TelephonyManager) getActivity().getSystemService(
				getActivity().TELEPHONY_SERVICE);
		myListener = new MyPhoneStateListener();
		telMgr.listen(myListener, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);

		Button btnPing = (Button) rootView.findViewById(R.id.btnPing);
		btnPing.setVisibility(View.INVISIBLE);
		TextView tvHost = (TextView) rootView.findViewById(R.id.tvHost);
		tvHost.setVisibility(View.INVISIBLE);
		tvPingRes = (TextView) rootView.findViewById(R.id.tvPingRes);
		// tvPingRes.setVisibility(View.INVISIBLE);

		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		tvInfo = (TextView) rootView.findViewById(R.id.tvInfo);

		simArgs = "\n SIM状态:" + telMgr.getSimState();
		simArgs += "\n 电话状态[0 无活动/1 响铃/2 摘机]:" + telMgr.getCallState();
		simArgs += "\n 唯一的设备ID::" + telMgr.getDeviceId();
		simArgs += "\n 手机号:" + telMgr.getLine1Number();
		simArgs += "\n (当前已注册的用户)的名字:" + telMgr.getNetworkOperatorName();

		if (telMgr.getSimState() == TelephonyManager.SIM_STATE_READY) {
			simArgs += "\n 服务商名称:" + telMgr.getSimOperatorName();
		}
		
		simArgs += "\n 获取ISO标准的国家码，即国际长途区号:" + telMgr.getNetworkCountryIso();
		simArgs += "\n 当前使用的网络类型:" + telMgr.getNetworkType();
		simArgs += "\n SIM卡的序列号:" + telMgr.getSimSerialNumber();
		
		hasOperator = (getSimOperatorName()!=null);
		simArgs += "\n 运营商名称:" + getSimOperatorName();
		isConn = isMobileConnected();
		simArgs += "\n 移动网络(3G/4G)状态:" + (isConn ? "已连接" : "不可用");
		tvState.setText(hasOperator ? "成功" : "失败");
		tvFinalRes.setText(hasOperator ? "测试结果:成功" : "测试结果:失败");
		
		btnPass.setEnabled(hasOperator);

		tvInfo.setText(simArgs);

		return rootView;
	}

	/**
	 * 检测检查移动网络是否连接
	 * 
	 * @return
	 */
	private boolean isMobileConnected() {
		ConnectivityManager cm = (ConnectivityManager) getActivity()
				.getSystemService(
						((Context) getActivity()).CONNECTIVITY_SERVICE);
		if (cm != null) {
			NetworkInfo networkInfo = cm.getActiveNetworkInfo();
			if (networkInfo != null
					&& networkInfo.getType() == ConnectivityManager.TYPE_MOBILE) {
				return true;
			}
		}

		return false;
	}

	// 运营商名称,是否获取到
	private String getSimOperatorName() {
		
		String operatorName = null;
		String operator = telMgr.getSimOperator();
		
		if (operator != null) 
		{
			if (operator.equals("46000") || operator.equals("46002")) 
			{
				operatorName = "中国移动";
			} 
			else if (operator.equals("46001")) 
			{
				operatorName = "中国联通";
			} 
			else if (operator.equals("46003")) 
			{
				operatorName = "中国电信";
			}
		}
		
		return operatorName;
	}

	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity
				.getTestItemE(MainActivity.itemRealIndexs[pos]);

		switch (v.getId()) {
		case R.id.btnPass: {
			testItem.setResult(getActivity().getString(R.string.pass));

			Fragment fg = null;
			try {

				fg = MainActivity.getTestItemE(
						MainActivity.itemRealIndexs[pos + 1]).getFragment();

			} catch (Exception e) {
				// TODO: handle exception
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

		case R.id.btnState: {
			simArgs = "\n SIM状态:" + telMgr.getSimState();
			simArgs += "\n 电话状态[0 无活动/1 响铃/2 摘机]:" + telMgr.getCallState();
			simArgs += "\n 唯一的设备ID::" + telMgr.getDeviceId();
			simArgs += "\n 手机号:" + telMgr.getLine1Number();
			simArgs += "\n (当前已注册的用户)的名字:" + telMgr.getNetworkOperatorName();

			if (telMgr.getSimState() == TelephonyManager.SIM_STATE_READY) {
				simArgs += "\n 服务商名称:" + telMgr.getSimOperatorName();
			}

			simArgs += "\n 获取ISO标准的国家码，即国际长途区号:"
					+ telMgr.getNetworkCountryIso();
			simArgs += "\n 当前使用的网络类型:" + telMgr.getNetworkType();
			simArgs += "\n SIM卡的序列号:" + telMgr.getSimSerialNumber();

			hasOperator = (getSimOperatorName()!=null);
			simArgs += "\n 运营商名称:" + getSimOperatorName();
			isConn = isMobileConnected();
			simArgs += "\n 移动网络(3G/4G)状态:" + (isConn ? "已连接" : "不可用");

			tvInfo.setText(simArgs);
			tvState.setText(hasOperator ? "成功" : "失败");
			btnPass.setEnabled(hasOperator);
			
			System.out.println("R.id.btnState = " + R.id.btnState);
			}
			break;

		default:

			break;
		}

		// System.out.println("SimFragment:" + testItem.getName() + "," +
		// testItem.getResult() + ","
		// + MainActivity.itemRealIndexs[pos]);
	}

	private class MyPhoneStateListener extends PhoneStateListener {

		public void onSignalStrengthsChanged(SignalStrength signalStrength) {

			super.onSignalStrengthsChanged(signalStrength);

			int asu = signalStrength.getGsmSignalStrength();
			int level = 0;
			if (asu <= 2 || asu == 99)
				level = 0;
			else if (asu >= 12)
				level = 4;
			else if (asu >= 8)
				level = 3;
			else if (asu >= 5)
				level = 2;
			else
				level = 1;
			
			String[] SIGNAL_STRENGTH_NAMES = { "none", "poor", "moderate",
					"good", "great" };
			String[] SIGNAL_STRENGTH_NAMES_ZH = { "未知", "差", "中等", "好", "很好" };
			
			tvPingRes.setText("信号强度:" + SIGNAL_STRENGTH_NAMES_ZH[level]);
		}
	};
}

