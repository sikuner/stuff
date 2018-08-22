package com.norco.hardwaretest;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

public class TwoFragment extends Fragment {
	
	private EditText etOperator, etNProcedure, etPingHostIP, etHostIPs, etRam, etRom;
	
	private String operator 	= "norcoer";
	private String nprocedure 	= "功能测试";
	private String pinghostip 	= "192.168.10.3";
	private String hostips 	= "192.168.1.155;192.168.1.156";
	private String ram_mb = "750";
	private String rom_gb = "13";
	
	private SharedPreferences prePreferences;
	private SharedPreferences.Editor editor;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.two_fragment, container, false);
		
		etOperator = (EditText) rootView.findViewById(R.id.etOperator);
		etNProcedure = (EditText) rootView.findViewById(R.id.etNProcedure);
		etPingHostIP = (EditText) rootView.findViewById(R.id.etPingHostIP);
		etHostIPs = (EditText) rootView.findViewById(R.id.etHostIPs);
		etRam = (EditText) rootView.findViewById(R.id.etRam);
		etRom = (EditText) rootView.findViewById(R.id.etRom);
		
		prePreferences = getActivity().getSharedPreferences("HardwareTestSettings", getActivity().MODE_PRIVATE);
		operator = prePreferences.getString("operator", operator);
		nprocedure = prePreferences.getString("nprocedure", nprocedure);
		pinghostip = prePreferences.getString("pinghostip", pinghostip);
		hostips = prePreferences.getString("hostips", hostips);
		ram_mb = prePreferences.getString("ram_mb", ram_mb);
		rom_gb = prePreferences.getString("rom_gb", rom_gb);
		
		etOperator.setText(operator);
		etNProcedure.setText(nprocedure);
		etPingHostIP.setText(pinghostip);
		etHostIPs.setText(hostips);
		etRam.setText(ram_mb);
		etRom.setText(rom_gb);
		
		return rootView;
	}
	
	public int doSaveAs() {
		
		editor = prePreferences.edit();
		editor.putString("operator", etOperator.getText().toString());
		editor.putString("nprocedure", etNProcedure.getText().toString());
		editor.putString("pinghostip", etPingHostIP.getText().toString());
		editor.putString("hostips", etHostIPs.getText().toString());
		editor.putString("ram_mb", etRam.getText().toString());
		editor.putString("rom_gb", etRom.getText().toString());
		boolean b = editor.commit();
		if (b) {
//			Toast.makeText(getActivity(), "保存成功!",
//					Toast.LENGTH_SHORT).show();
			return 0;
		}else {
//			Toast.makeText(getActivity(), "保存失败, 请确认!",
//					Toast.LENGTH_SHORT).show();
			return -1;
		}
	}
	
}
