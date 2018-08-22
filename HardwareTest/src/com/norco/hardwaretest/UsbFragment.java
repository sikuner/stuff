package com.norco.hardwaretest;

import java.io.DataOutputStream;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.R.integer;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Switch;
import android.widget.TextView;

public class UsbFragment extends Fragment implements OnClickListener, OnCheckedChangeListener {

	boolean successfulMark = false;
	
	private View rootView;
	private int swUsbIDs[] = { R.id.swUsb1, R.id.swUsb2, R.id.swUsb3, R.id.swUsb4, R.id.swUsb5 };
	TextView tvUsbAllRes;
	private int checkedNum = 0;
	private Button btnUsbDev;
	private TextView tvUsbDev;
	private Button btnPass, btnFail;
	
	private List<String> usbnodes;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		rootView = inflater.inflate(R.layout.usb_fragment, container, false);
		
		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		btnPass.setEnabled(successfulMark);
		
		tvUsbAllRes = (TextView) rootView.findViewById(R.id.tvUsbAllRes);
		btnUsbDev = (Button) rootView.findViewById(R.id.btnUsbDev);
		btnUsbDev.setOnClickListener(this);
		tvUsbDev = (TextView) rootView.findViewById(R.id.tvUsbDev);
		
		for (int i = 0; i < swUsbIDs.length; i++) {
			Switch swUsb = (Switch) rootView.findViewById(swUsbIDs[i]);
			swUsb.setOnCheckedChangeListener(this);
		}
		
		tvUsbAllRes.setText(String.format("成功:%d, 失败:%d  ", checkedNum, swUsbIDs.length-checkedNum));
		
		return rootView;
	}
	
	@Override
	public void onClick(View v) {
		
		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {
		
		case R.id.btnUsbDev:
			{
				successfulMark = false;
				
				for (int i = 0; i < swUsbIDs.length; i++) {
					Switch swUsb = (Switch) rootView.findViewById(swUsbIDs[i]);
					swUsb.setChecked(false);
				}
				
				usbnodes = new ArrayList<String>();
				getUsbNodes(usbnodes);
				tvUsbDev.setText(usbnodes.toString());
				for (int i = 0; i < usbnodes.size() ; i++) {
					
					int devno = Integer.parseInt(usbnodes.get(i));
					if (devno <= 5) {
						Switch swUsb = (Switch) rootView.findViewById(swUsbIDs[devno-1]);
						swUsb.setChecked(true);
					}
				}
			}
			break;
		
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

			
			
		default:

			break;
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		
		checkedNum = 0;
		for (int i = 0; i < swUsbIDs.length; i++) {
			Switch swUsb = (Switch) rootView.findViewById(swUsbIDs[i]);
			swUsb.isChecked();
			if (swUsb.isChecked()) {
				checkedNum++;
			}
		}
		
		if (checkedNum != swUsbIDs.length) {
			
			tvUsbAllRes.setText(String.format("成功:%d, 失败:%d  ", checkedNum, swUsbIDs.length-checkedNum));
			btnPass.setEnabled(successfulMark);
			
		}
		else {
			
			successfulMark = true;
			btnPass.setEnabled(successfulMark);
			tvUsbAllRes.setText(String.format("全部成功 成功:%d, 失败:%d  ", checkedNum, swUsbIDs.length-checkedNum));
		}		
	}
	
	private int getUsbNodes(List<String> usbnodes) { 
		
		String prefix = "2-1.";
		
		File file = new File("/sys/bus/usb/drivers/usb");
		if (file.isDirectory()) {
			File[] files = file.listFiles();
			for (int i = 0; i < files.length; i++) {
				String fileName = files[i].getName();
				if (fileName.contains(prefix)) {
					usbnodes.add(fileName.substring(prefix.length()));
				}
			}
		}
		
		return 0;
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
