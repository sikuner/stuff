package com.norco.hardwaretest;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.norco.utils.GpioJNI;

import android.R.integer;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;

public class GpioFragment extends Fragment implements OnClickListener, OnCheckedChangeListener {
	
	private View rootView;
	private LinearLayout btnsLayout1;
	private LinearLayout btnsLayout2;
	private final int SWITCH_ID = 9000;
	private final int ALL_SWITCH_DELTA = 100;
	
	private List<String> mGpioIdxs;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		rootView = inflater.inflate(R.layout.gpio_fragment, container,
				false);
		
		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);
		
		Switch swAll = (Switch) rootView.findViewById(R.id.swAll); 
		swAll.setId(SWITCH_ID+ALL_SWITCH_DELTA);
		swAll.setOnCheckedChangeListener(this);
		
		mGpioIdxs = new ArrayList<String>();
		getGpioIdxs(mGpioIdxs);
		
		System.out.println("GPIO18_LEVEL="+getGpioLevel(18));
		System.out.println("GPIO1_LEVEL="+getGpioLevel(1));
		
		btnsLayout1 = (LinearLayout) rootView.findViewById(R.id.btnsLayout1);
		btnsLayout2 = (LinearLayout) rootView.findViewById(R.id.btnsLayout2);
		
		int pin = 1;
		int thisPin = 1;
		for (int i = mGpioIdxs.size()-1; i >= 0; i--) {
			
			pin = Integer.parseInt(mGpioIdxs.get(i));
			while (pin > thisPin) {
				
				Switch sw = new Switch(getActivity());
				sw.setId(SWITCH_ID+thisPin);
				sw.setTextOn("空");
				sw.setTextOff("空");
				sw.setClickable(false);
				sw.setEnabled(false);
				sw.setText("PIN"+thisPin);
				sw.setGravity(Gravity.RIGHT|Gravity.CENTER_VERTICAL);
				
				if ((thisPin%2)==1) {
					btnsLayout1.addView(sw);
				}
				else {
					btnsLayout2.addView(sw);
				}
				
				thisPin++;
			}
			
			Switch sw = new Switch(getActivity());
			sw.setId(SWITCH_ID+thisPin);
			sw.setTextOn("高");
			sw.setTextOff("低");
			sw.setText("PIN"+thisPin);
			sw.setGravity(Gravity.RIGHT|Gravity.CENTER_VERTICAL);
			sw.setOnCheckedChangeListener(this);
			
			if ((thisPin%2)==1) {
				btnsLayout1.addView(sw);
			}
			else {
				btnsLayout2.addView(sw);
			}
			
			thisPin++;
		}
		
		return rootView;
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
		default:

			break;
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

		int pin = buttonView.getId() - SWITCH_ID;
		int level = isChecked ? 1 : 0;
		
		System.out.println("GpioJNI.setLevel(pin:"+pin+", "+"level"+":"+level+")");
		
		if (pin == ALL_SWITCH_DELTA) { // GPIO总开关
			
			for (int i = 0; i < mGpioIdxs.size(); i++) {
				
				CompoundButton btnView = (CompoundButton) rootView.findViewById(SWITCH_ID+Integer.parseInt(mGpioIdxs.get(i)));
				btnView.setChecked(isChecked);
			}
		}
		else { // GPIO单项操作
			setGpioLevel(pin, level);
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
	
	private int getGpioIdxs(List<String> gpioIdxs) {
		
		String gpioRegEx = "^gpio[0-9]+_value$";
		
		String regEx="[^0-9]"; 
		Pattern p = Pattern.compile(regEx);
		
		File file = new File("/dev");
		if (file.isDirectory()) {
			File[] files = file.listFiles();
			for (int i = 0; i < files.length; i++) {
				String fileName = files[i].getName();
				if (fileName.matches(gpioRegEx)) {
					gpioIdxs.add(p.matcher(fileName).replaceAll("").trim());
				}
			}
		}
		
		return 0;
	}
	
	private int getGpioLevel(int pin) {
		
		int level = -1; // 0 - 低电平, 1 - 高电平
		
		String res = "";
		
		String cmd = String.format("echo in > /dev/gpio%d_dir", pin);
		RootCmd(cmd);
		
		try {
			
			String line;
			File file = new File(String.format("/dev/gpio%d_value", pin));
			BufferedReader br = new BufferedReader(new FileReader(file));
			while (null != (line = br.readLine())) {
				res += line;				
			}
			level = Integer.parseInt(res);
			br.close();
			
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return level;
	}
	
	private int setGpioLevel(int pin, int level) {
		
		String cmd = "";
		
		cmd = String.format("echo out > /dev/gpio%d_dir", pin);
		RootCmd(cmd);
		cmd = String.format("echo %d > /dev/gpio%d_value", level, pin);
		RootCmd(cmd);
		
		return 0;
	}
	
}
