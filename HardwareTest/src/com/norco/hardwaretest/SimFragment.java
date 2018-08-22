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

		simArgs = "\n SIM״̬:" + telMgr.getSimState();
		simArgs += "\n �绰״̬[0 �޻/1 ����/2 ժ��]:" + telMgr.getCallState();
		simArgs += "\n Ψһ���豸ID::" + telMgr.getDeviceId();
		simArgs += "\n �ֻ���:" + telMgr.getLine1Number();
		simArgs += "\n (��ǰ��ע����û�)������:" + telMgr.getNetworkOperatorName();

		if (telMgr.getSimState() == TelephonyManager.SIM_STATE_READY) {
			simArgs += "\n ����������:" + telMgr.getSimOperatorName();
		}
		
		simArgs += "\n ��ȡISO��׼�Ĺ����룬�����ʳ�;����:" + telMgr.getNetworkCountryIso();
		simArgs += "\n ��ǰʹ�õ���������:" + telMgr.getNetworkType();
		simArgs += "\n SIM�������к�:" + telMgr.getSimSerialNumber();
		
		hasOperator = (getSimOperatorName()!=null);
		simArgs += "\n ��Ӫ������:" + getSimOperatorName();
		isConn = isMobileConnected();
		simArgs += "\n �ƶ�����(3G/4G)״̬:" + (isConn ? "������" : "������");
		tvState.setText(hasOperator ? "�ɹ�" : "ʧ��");
		tvFinalRes.setText(hasOperator ? "���Խ��:�ɹ�" : "���Խ��:ʧ��");
		
		btnPass.setEnabled(hasOperator);

		tvInfo.setText(simArgs);

		return rootView;
	}

	/**
	 * ������ƶ������Ƿ�����
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

	// ��Ӫ������,�Ƿ��ȡ��
	private String getSimOperatorName() {
		
		String operatorName = null;
		String operator = telMgr.getSimOperator();
		
		if (operator != null) 
		{
			if (operator.equals("46000") || operator.equals("46002")) 
			{
				operatorName = "�й��ƶ�";
			} 
			else if (operator.equals("46001")) 
			{
				operatorName = "�й���ͨ";
			} 
			else if (operator.equals("46003")) 
			{
				operatorName = "�й�����";
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
			simArgs = "\n SIM״̬:" + telMgr.getSimState();
			simArgs += "\n �绰״̬[0 �޻/1 ����/2 ժ��]:" + telMgr.getCallState();
			simArgs += "\n Ψһ���豸ID::" + telMgr.getDeviceId();
			simArgs += "\n �ֻ���:" + telMgr.getLine1Number();
			simArgs += "\n (��ǰ��ע����û�)������:" + telMgr.getNetworkOperatorName();

			if (telMgr.getSimState() == TelephonyManager.SIM_STATE_READY) {
				simArgs += "\n ����������:" + telMgr.getSimOperatorName();
			}

			simArgs += "\n ��ȡISO��׼�Ĺ����룬�����ʳ�;����:"
					+ telMgr.getNetworkCountryIso();
			simArgs += "\n ��ǰʹ�õ���������:" + telMgr.getNetworkType();
			simArgs += "\n SIM�������к�:" + telMgr.getSimSerialNumber();

			hasOperator = (getSimOperatorName()!=null);
			simArgs += "\n ��Ӫ������:" + getSimOperatorName();
			isConn = isMobileConnected();
			simArgs += "\n �ƶ�����(3G/4G)״̬:" + (isConn ? "������" : "������");

			tvInfo.setText(simArgs);
			tvState.setText(hasOperator ? "�ɹ�" : "ʧ��");
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
			String[] SIGNAL_STRENGTH_NAMES_ZH = { "δ֪", "��", "�е�", "��", "�ܺ�" };
			
			tvPingRes.setText("�ź�ǿ��:" + SIGNAL_STRENGTH_NAMES_ZH[level]);
		}
	};
}

