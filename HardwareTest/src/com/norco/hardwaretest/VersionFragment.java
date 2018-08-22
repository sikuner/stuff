package com.norco.hardwaretest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;

import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.support.v4.app.Fragment;
import android.text.format.Formatter;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Button;
import android.widget.Switch;
import android.widget.TextView;

public class VersionFragment extends Fragment implements OnClickListener, OnCheckedChangeListener {

	private Button btnPass, btnFail;
	private TextView tvRam, tvRom, tvRes;
	private Switch swRam, swRom;
	private String strRAM, strROM;
	private String ram_mb="750", rom_gb="13";
	private long ram_byte, rom_byte;
	
	private SharedPreferences prePreferences;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		
		View rootView = inflater.inflate(R.layout.version_fragment, container,
				false);
		
		tvRam = (TextView) rootView.findViewById(R.id.tvRam);
		tvRom = (TextView) rootView.findViewById(R.id.tvRom);
		tvRes = (TextView) rootView.findViewById(R.id.tvRes);
		
		swRam = (Switch) rootView.findViewById(R.id.swRam);
		swRom = (Switch) rootView.findViewById(R.id.swRom);
		swRam.setOnCheckedChangeListener(this);
		swRom.setOnCheckedChangeListener(this);
		swRam.setChecked(false);
		swRom.setChecked(false);
		
//		android.os.Build.SERIAL
		String phoneInfo = "\n MODEL: " + android.os.Build.MODEL;
//		phoneInfo += "\n VERSION.RELEASE: " + android.os.Build.VERSION.RELEASE;
//		phoneInfo += "\n DEVICE: " + android.os.Build.DEVICE;
		phoneInfo += "\n DISPLAY: " + android.os.Build.DISPLAY;
		phoneInfo += "\n SERIAL: " + android.os.Build.SERIAL;
//		phoneInfo += "\n BRAND: " + android.os.Build.BRAND;
//		phoneInfo += "\n BOARD: " + android.os.Build.BOARD;
//		phoneInfo += "\n FINGERPRINT: " + android.os.Build.FINGERPRINT;
//		phoneInfo += "\n ID: " + android.os.Build.ID;
//		phoneInfo += "\n MANUFACTURER: " + android.os.Build.MANUFACTURER;
//		phoneInfo += "\n USER: " + android.os.Build.USER;
		
		phoneInfo += "\n";
		
		phoneInfo += "\n Android版本: " + android.os.Build.VERSION.RELEASE;
		phoneInfo += "\n CPU型号: " + getCpuModel();
		phoneInfo += "\n CPU核心数: " + getNumCores();
		phoneInfo += "\n CPU频率: " + getMaxCpuFreq();
		
		phoneInfo += "\n 屏幕分辨率(宽x高(像素)): " + getWeithAndHeight();
		
		TextView tvVersion = (TextView) rootView.findViewById(R.id.tvVersion);
		tvVersion.setText(phoneInfo);
		
		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		btnPass.setEnabled(false);
		
		return rootView;
	}
	
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		
		prePreferences = getActivity().getSharedPreferences("HardwareTestSettings", getActivity().MODE_PRIVATE);
		ram_mb = prePreferences.getString("ram_mb", ram_mb);
		rom_gb = prePreferences.getString("rom_gb", rom_gb);
		ram_byte = Integer.parseInt(ram_mb) * 1024 * 1024;
		rom_byte = Integer.parseInt(rom_gb) * 1024 * 1024 * 1024;
		
		strRAM = getSystemAvaialbeMemory()+"/"+ getTotalMemorys();
		strROM = getRomSpace();
		
		tvRam.setText(strRAM);
		tvRom.setText(strROM);
	}
	
	@Override	
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		System.out.println("VersionFragment.onDestroy");
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

	// 得到CPU型号
	public String getCpuModel() {

		String str1 = "/proc/cpuinfo";// 系统内存信息文件
		String str2;
		String[] arrayOfString;
		try {
			FileReader localFileReader = new FileReader(str1);
			BufferedReader localBufferedReader = new BufferedReader(
					localFileReader, 8192);
			str2 = localBufferedReader.readLine();

			while ((str2 = localBufferedReader.readLine()) != null) {
				if (str2.contains("Hardware")) {

					arrayOfString = str2.split(":");
					localBufferedReader.close();
					
					return arrayOfString[1];
				}
			}
			
			localBufferedReader.close();

		} catch (IOException e) {

		}
		
		return "Not found";
	}

	// CPU核心数
	public int getNumCores() {
		ProcessBuilder cmd;
		int num_core = 0;

		try {
			String[] args = { "/system/bin/cat",
					"/sys/devices/system/cpu/kernel_max" };
			cmd = new ProcessBuilder(args);

			Process process = cmd.start();
			BufferedReader reader = new BufferedReader(new InputStreamReader(
					process.getInputStream()));
			String line = reader.readLine();

			num_core = Integer.parseInt(line, 10) + 1;

			return num_core;
		} catch (IOException ex) {
			ex.printStackTrace();
		}

		return 0;
	}

	// CPU频率
	public String getMaxCpuFreq() {
		ProcessBuilder cmd;
		int max_freq = 0;
		String result = "Not found";

		try {
			String[] args = { "/system/bin/cat",
					"/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq" };
			cmd = new ProcessBuilder(args);
			
			Process process = cmd.start();
			BufferedReader reader = new BufferedReader(new InputStreamReader(
					process.getInputStream()));
			String line = reader.readLine();
			System.out.println("linex=" + line);
			
			max_freq = Integer.parseInt(line, 10);
			double freq = (double) Math
					.round(((double) max_freq) * 10.0 / 1000000.0) / 10.0;

			// Formatter.formatFileSize(getActivity(), max_freq)

			result = Double.toString(freq) + "GHz";

			return result;
		} catch (Exception e) {
			e.printStackTrace();
		}

		return result;
	}

	// 得到总内存大小
	public String getTotalMemorys() {
		long total_memory = 0;

		String str1 = "/proc/meminfo";// 系统内存信息文件
		String str2;
		String[] arrayOfString;
		try {
			FileReader localFileReader = new FileReader(str1);
			BufferedReader localBufferedReader = new BufferedReader(
					localFileReader, 8192);
			str2 = localBufferedReader.readLine();// 读取meminfo第一行，系统总内存大�?
			arrayOfString = str2.split("\\s+");
			total_memory = Integer.valueOf(arrayOfString[1]).intValue() * 1024;// 获得系统总内存，单位是KB，乘�?024转换为Byte
			localBufferedReader.close();
		} catch (IOException e) {

		}
		
		if (total_memory>ram_byte) {
			swRam.setChecked(true);
		}else {
			swRam.setChecked(false);
		}
		
		return Formatter.formatFileSize(getActivity(), total_memory);// Byte转换为KB或MB，内存大小规格化
	}
	
	// 得到可用内存大小
	public String getSystemAvaialbeMemory() {
		// 获得MemoryInfo对象
		MemoryInfo memoryInfo = new MemoryInfo();
		// 获得系统可用内存，保存在MemoryInfo对象�?
		((ActivityManager) getActivity().getSystemService(
				Context.ACTIVITY_SERVICE)).getMemoryInfo(memoryInfo);

		// 字符类型转换
		String availMemStr = Formatter.formatFileSize(getActivity(),
				memoryInfo.availMem);

		return availMemStr;
	}

	// 获取手机屏幕高度
	private String getWeithAndHeight() {
		// 这种方式在service中无法使用，
		DisplayMetrics dm = new DisplayMetrics();
		getActivity().getWindowManager().getDefaultDisplay().getMetrics(dm);
		int width = dm.widthPixels; // 宽
		int height = dm.heightPixels; // 高
		// float density = dm.density; // 屏幕密度（0.75 / 1.0 / 1.5）
		// int densityDpi = dm.densityDpi; // 屏幕密度DPI（120 / 160 / 240）
		// 在service中也能得到高和宽
		WindowManager mWindowManager = (WindowManager) getActivity()
				.getSystemService(getActivity().WINDOW_SERVICE);
		width = mWindowManager.getDefaultDisplay().getWidth();
		// Log.i("width", width+"");
		height = mWindowManager.getDefaultDisplay().getHeight();
		// Log.i("height", height+"");
		return width + "x" + height;

		// return "(像素)宽:" + width + "\n" + "(像素)高:" + height + "\n"
		// + "屏幕密度（0.75 / 1.0 / 1.5）:" + density + "\n"
		// + "屏幕密度DPI（120 / 160 / 240）:" + densityDpi + "\n";
		/*
		 * 下面的代码即可获取屏幕的尺寸。 在一个Activity的onCreate方法中，写入如下代码： DisplayMetrics metric
		 * = new DisplayMetrics();
		 * getWindowManager().getDefaultDisplay().getMetrics(metric); int width
		 * = metric.widthPixels; // 屏幕宽度（像素） int height = metric.heightPixels;
		 * // 屏幕高度（像素） float density = metric.density; // 屏幕密度（0.75 / 1.0 / 1.5）
		 * int densityDpi = metric.densityDpi; // 屏幕密度DPI（120 / 160 / 240）
		 * 
		 * 但是，需要注意的是，在一个低密度的小屏手机上，仅靠上面的代码是不能获取正确的尺寸的。
		 * 比如说，一部240x320像素的低密度手机，如果运行上述代码，获取到的屏幕尺寸是320x427。
		 * 因此，研究之后发现，若没有设定多分辨率支持的话
		 * ，Android系统会将240x320的低密度（120）尺寸转换为中等密度（160）对应的尺寸，
		 * 这样的话就大大影响了程序的编码。所以，需要在工程的AndroidManifest
		 * .xml文件中，加入supports-screens节点，具体的内容如下： <supports-screens
		 * android:smallScreens="true" android:normalScreens="true"
		 * android:largeScreens="true" android:resizeable="true"
		 * android:anyDensity="true" />
		 * 这样的话，当前的Android程序就支持了多种分辨率，那么就可以得到正确的物理尺寸了。
		 */
	}

	private String getRomSpace() {

		File path = Environment.getDataDirectory();
		StatFs stat = new StatFs(path.getPath());
		long blockCount = stat.getBlockCount();
		long blockSize = stat.getBlockSize();
		long availableBlocks = stat.getAvailableBlocks();
		
		long totalByte = blockCount * blockSize;
		long availableByte = availableBlocks * blockSize;
		
		String totalSize = Formatter.formatFileSize(getActivity()
				.getApplicationContext(), totalByte);
		String availableSize = Formatter.formatFileSize(getActivity()
				.getApplicationContext(), availableByte);
		
		if (totalByte>rom_byte) {
			swRom.setChecked(true);
		}else {
			swRom.setChecked(false);
		}
		
		return availableSize + "/" + totalSize;
	}
	
	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		
		if (swRam.isChecked() && swRom.isChecked()) {
			tvRes.setText("测试结果:成功");
			btnPass.setEnabled(true);
		}
		else {
			tvRes.setText("测试结果:失败");
			btnPass.setEnabled(false);
		}
		
	}

}
