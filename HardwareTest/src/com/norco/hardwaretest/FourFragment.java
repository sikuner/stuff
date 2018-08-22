package com.norco.hardwaretest;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;

public class FourFragment extends Fragment {
	
	private boolean operation=true, version=true, sdcard=true, usb=true, headset=true, ethernet=true, rs232=true;
	private boolean fpanel=true, sim=true, satadisk=true, screen=true, wifi=true, gpio=true, rs485=true;
	
	private CheckBox ckbOperation, ckbVersion, ckbSdCard,   ckbUsb,           ckbHeadset, ckbEthernet, ckbSerialPort;
	private CheckBox ckbFpanel,    ckbSim,     ckbSataDisk, ckbScreenDisplay, ckbWifi,    ckbGpio,     ckbRs485;
	
	private SharedPreferences prePreferences;
	private SharedPreferences.Editor editor;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.four_fragment, container,	false);
		
		ckbOperation = (CheckBox) rootView.findViewById(R.id.ckbOperation);
		ckbVersion = (CheckBox) rootView.findViewById(R.id.ckbVersion);
		ckbSdCard = (CheckBox) rootView.findViewById(R.id.ckbSdCard);
		ckbUsb = (CheckBox) rootView.findViewById(R.id.ckbUsb);
		ckbHeadset = (CheckBox) rootView.findViewById(R.id.ckbHeadset);
		ckbEthernet = (CheckBox) rootView.findViewById(R.id.ckbEthernet);
		ckbSerialPort = (CheckBox) rootView.findViewById(R.id.ckbSerialPort);
		
		ckbFpanel = (CheckBox) rootView.findViewById(R.id.ckbFpanel);
		ckbSim = (CheckBox) rootView.findViewById(R.id.ckbSim);
		ckbSataDisk = (CheckBox) rootView.findViewById(R.id.ckbSataDisk);
		ckbScreenDisplay = (CheckBox) rootView.findViewById(R.id.ckbScreenDisplay);
		ckbWifi = (CheckBox) rootView.findViewById(R.id.ckbWifi);
		ckbGpio = (CheckBox) rootView.findViewById(R.id.ckbGpio);
		ckbRs485 = (CheckBox) rootView.findViewById(R.id.ckbRs485);
		
		prePreferences = getActivity().getSharedPreferences("HardwareTestSettings", getActivity().MODE_PRIVATE);
		
		operation = prePreferences.getBoolean("operation", operation);
		version = prePreferences.getBoolean("version", version);
		sdcard = prePreferences.getBoolean("sdcard", sdcard);
		usb = prePreferences.getBoolean("usb", usb);
		headset = prePreferences.getBoolean("headset", headset);
		ethernet = prePreferences.getBoolean("ethernet", ethernet);
		rs232 = prePreferences.getBoolean("rs232", rs232);
		
		fpanel = prePreferences.getBoolean("fpanel", fpanel);
		sim = prePreferences.getBoolean("sim", sim);
		satadisk = prePreferences.getBoolean("satadisk", satadisk);
		screen = prePreferences.getBoolean("screen", screen);
		wifi = prePreferences.getBoolean("wifi", wifi);
		gpio = prePreferences.getBoolean("gpio", gpio);
		rs485 = prePreferences.getBoolean("rs485", rs485);
		
		ckbOperation.setChecked(operation);
		ckbVersion.setChecked(version);
		ckbSdCard.setChecked(sdcard);
		ckbUsb.setChecked(usb);
		ckbHeadset.setChecked(headset);
		ckbEthernet.setChecked(ethernet);
		ckbSerialPort.setChecked(rs232);
		
		ckbFpanel.setChecked(fpanel);
		ckbSim.setChecked(sim);
		ckbSataDisk.setChecked(satadisk);
		ckbScreenDisplay.setChecked(screen);
		ckbWifi.setChecked(wifi);
		ckbGpio.setChecked(gpio);
		ckbRs485.setChecked(rs485);
		
		return rootView;
	}
	
	public int doSaveAs() {
		
		editor = prePreferences.edit();
		
		editor.putBoolean("operation", ckbOperation.isChecked());
		editor.putBoolean("version", ckbVersion.isChecked());
		editor.putBoolean("sdcard", ckbSdCard.isChecked());
		editor.putBoolean("usb", ckbUsb.isChecked());
		editor.putBoolean("headset", ckbHeadset.isChecked());
		editor.putBoolean("ethernet", ckbEthernet.isChecked());
		editor.putBoolean("rs232", ckbSerialPort.isChecked());
		
		editor.putBoolean("fpanel", ckbFpanel.isChecked());
		editor.putBoolean("sim", ckbSim.isChecked());
		editor.putBoolean("satadisk", ckbSataDisk.isChecked());
		editor.putBoolean("screen", ckbScreenDisplay.isChecked());
		editor.putBoolean("wifi", ckbWifi.isChecked());
		editor.putBoolean("gpio", ckbGpio.isChecked());
		editor.putBoolean("rs485", ckbRs485.isChecked());
		
		boolean b = editor.commit();
		if (b) {
			return 0;
		}else {
			return -1;
		}
	}
	
}
