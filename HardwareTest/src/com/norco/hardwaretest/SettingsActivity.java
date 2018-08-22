package com.norco.hardwaretest;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;

import android.R.integer;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class SettingsActivity extends FragmentActivity implements
		OnClickListener {

	private ViewPager mPager;

	List<Fragment> lstFragment;

	OneFragment oneFragment;
	TwoFragment twoFragment;
	ThreeFragment threeFragment;
	FourFragment fourFragment;

	Button btnOne, btnTwo, btnThree, btnFour;
	Button btnOK, btnSaveAs, btnCancel;

	ImageView ivCursor;
	int ivWidth = 0;
	int ivPadding = 0;

	// 屏幕宽度
	int screenWidth = 0;
	// 当前选中的项
	int curTab = -1;

	private SharedPreferences prePreferences;
	private Properties prop;

//	private String extPropPath = "/mnt/extsd/HardwareTestSettings.properties";
	private String extPropPath = MainActivity.getExStoragePath()+"/"+"HardwareTestSettings.properties";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_settings);

		btnOne = (Button) findViewById(R.id.btnOne);
		btnOne.setOnClickListener(this);
		btnTwo = (Button) findViewById(R.id.btnTwo);
		btnTwo.setOnClickListener(this);
		btnThree = (Button) findViewById(R.id.btnThree);
		btnThree.setOnClickListener(this);
		btnFour = (Button) findViewById(R.id.btnFour);
		btnFour.setOnClickListener(this);

		btnOK = (Button) findViewById(R.id.btnOK);
		btnCancel = (Button) findViewById(R.id.btnCancel);
		btnSaveAs = (Button) findViewById(R.id.btnSaveAs);
		btnOK.setOnClickListener(this);
		btnCancel.setOnClickListener(this);
		btnSaveAs.setOnClickListener(this);

		lstFragment = new ArrayList<Fragment>();
		oneFragment = new OneFragment();
		twoFragment = new TwoFragment();
		threeFragment = new ThreeFragment();
		fourFragment = new FourFragment();

		lstFragment.add(oneFragment);
		lstFragment.add(twoFragment);
		lstFragment.add(threeFragment);
		lstFragment.add(fourFragment);

		mPager = (ViewPager) findViewById(R.id.mPager);
		mPager.setAdapter(new MyFrageStatePagerAdapter(
				getSupportFragmentManager()));

		ivCursor = (ImageView) findViewById(R.id.ivCursor);
		ivWidth = ivCursor.getWidth();
		screenWidth = getResources().getDisplayMetrics().widthPixels;
		ivPadding = (screenWidth / lstFragment.size() - ivWidth) / 2;
	}

	/**
	 * 定义自己的ViewPager适配器。 也可以使用FragmentPagerAdapter。关于这两者之间的区别，可以自己去搜一下。
	 */
	class MyFrageStatePagerAdapter extends FragmentStatePagerAdapter {

		public MyFrageStatePagerAdapter(FragmentManager fm) {
			super(fm);
		}

		@Override
		public Fragment getItem(int position) {
			return lstFragment.get(position);
		}

		@Override
		public int getCount() {
			return lstFragment.size();
		}

		/**
		 * 每次更新完成ViewPager的内容后，调用该接口，此处复写主要是为了让导航按钮上层的覆盖层能够动态的移动
		 */
		@Override
		public void finishUpdate(ViewGroup container) {
			super.finishUpdate(container);// 这句话要放在最前面，否则会报错
			// 获取当前的视图是位于ViewGroup的第几个位置，用来更新对应的覆盖层所在的位置
			int currentItem = mPager.getCurrentItem();
			if (currentItem == curTab) {
				return;
			}

			imageMove(mPager.getCurrentItem());
			curTab = mPager.getCurrentItem();
		}

	}

	/**
	 * 移动覆盖层
	 * 
	 * @param moveToTab
	 *            目标Tab，也就是要移动到的导航选项按钮的位置 第一个导航按钮对应0，第二个对应1，以此类推
	 */
	private void imageMove(int moveToTab) {
		int startPosition = 0;
		int movetoPosition = 0;

		startPosition = curTab * (screenWidth / 4);
		movetoPosition = moveToTab * (screenWidth / 4);

		// 平移动画
		TranslateAnimation translateAnimation = new TranslateAnimation(
				startPosition, movetoPosition, 0, 0);
		translateAnimation.setFillAfter(true);
		translateAnimation.setDuration(200);
		ivCursor.startAnimation(translateAnimation);
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.btnOne: {
			mPager.setCurrentItem(0, true);
		}
			break;
		case R.id.btnTwo: {
			mPager.setCurrentItem(1, true);
		}
			break;
		case R.id.btnThree: {
			// mPager.setCurrentItem(2, true);

			UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
			HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
			Log.e("SA", "get device list  = " + deviceList.size());
			Toast.makeText(this, "get device list  = " + deviceList.size(), 200)
					.show();
			Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
			while (deviceIterator.hasNext()) {
				UsbDevice device = deviceIterator.next();
				Log.e("SA", "device name = " + device.getDeviceName());
			}

		}
			break;
		case R.id.btnFour: {
			mPager.setCurrentItem(3, true);
		}
			break;

		case R.id.btnOK: {

			int res = 0;
			
			switch (mPager.getCurrentItem()) {
			case 0: {
				res = oneFragment.doSaveAs();
			}
				break;
			case 1: {
				res = twoFragment.doSaveAs();
			}
				break;
			case 2: {
				res = threeFragment.doSaveAs();
			}
				break;
			case 3: {
				res = fourFragment.doSaveAs();
			}
				break;

			default:
				break;
			}
			
			if (res == 0) {
				Toast.makeText(this, "保存成功!", Toast.LENGTH_SHORT).show();
			} else {
				Toast.makeText(this, "保存失败, 请确认!", Toast.LENGTH_SHORT).show();
			}
		}
			break;
		case R.id.btnCancel: {
			finish();
		}
			break;
		case R.id.btnSaveAs:
		// { // 配置导出
		//
		// File extsdDir = new File("/mnt/extsd/");
		// File udiskDir = new File("/mnt/udisk/");
		// File backCfgFile = null;
		//
		// File baseFile = new File(
		// "/data/data/com.norco.hardwaretest/shared_prefs/HardwareTestSettings.xml");
		// // File baseFile = new
		// //
		// File("/data/data/com.norco.mysettings/shared_prefs/HardwareTestSettings.xml");
		//
		// if (extsdDir.canWrite()) {
		// backCfgFile = new File("/mnt/extsd/HardwareTestSettings.xml");
		// } else {
		// if (udiskDir.canWrite()) {
		// backCfgFile = new File(
		// "/mnt/udisk/HardwareTestSettings.xml");
		// }
		// }
		//
		// if (backCfgFile != null) {
		//
		// // 拷贝配置
		// if (baseFile.exists()) {
		// try {
		// int bytesum = 0;
		// int byteread = 0;
		//
		// long baseLength = baseFile.length();
		//
		// InputStream inStream = new FileInputStream(baseFile); // 读入原文件
		// FileOutputStream fs = new FileOutputStream(backCfgFile);
		// byte[] buffer = new byte[1444];
		//
		// while ((byteread = inStream.read(buffer)) != -1) {
		// bytesum += byteread;
		// fs.write(buffer, 0, byteread);
		// }
		// inStream.close();
		// fs.close();
		//
		// System.out.println("bytesum = " + bytesum);
		// System.out.println("baseLength = " + baseLength);
		// if (!(bytesum < baseLength)) {
		// Toast.makeText(
		// this,
		// "成功导出配置文件:" + backCfgFile.getAbsolutePath(),
		// Toast.LENGTH_SHORT).show();
		// }
		//
		// } catch (Exception e) {
		// System.out.println("复制单个文件操作出错");
		// e.printStackTrace();
		// }
		// }
		//
		// } else { // 为空, SD卡和U盘都没有接入
		// Toast.makeText(this, "错误: SD卡和U盘均未识别,请确认!", Toast.LENGTH_SHORT)
		// .show();
		// }
		// }
		
		// 确定extPropPath具体的路径
		{
			
		}
		
		{
			// 配置
			String serverIp = "192.168.1.152";
			String port = "1433";
			String database = "NorcoTest";
			String username = "sa";
			String password = "root";
			
			String operator = "norcoer";
			String nprocedure = "功能测试";
			String pinghostip = "192.168.10.3";
			String hostips 	= "192.168.1.155;192.168.1.156";
			String ram_mb = "750";
			String rom_gb = "13";
			
			boolean operation = true, version = true, sdcard = true, usb = true, headset = true, ethernet = true, rs232 = true;
			boolean fpanel = true, sim = true, satadisk = true, screen = true, wifi = true, gpio = true, rs485 = true;

			prePreferences = getSharedPreferences("HardwareTestSettings",
					MODE_PRIVATE);

			prop = loadConfig(this, extPropPath);
			if (null == prop) {
				prop = new Properties();
			}

			prop.put("serverIp", prePreferences.getString("serverIp", serverIp));
			prop.put("port", prePreferences.getString("port", port));
			prop.put("database", prePreferences.getString("database", database));
			prop.put("username", prePreferences.getString("username", username));
			prop.put("password", prePreferences.getString("password", password));

			prop.put("operator", prePreferences.getString("operator", operator));
			prop.put("nprocedure",
					prePreferences.getString("nprocedure", nprocedure));
			prop.put("pinghostip",
					prePreferences.getString("pinghostip", pinghostip));
			prop.put("hostips",
					prePreferences.getString("hostips", hostips));
			
			prop.put("ram_mb", prePreferences.getString("ram_mb", ram_mb));
			prop.put("rom_gb", prePreferences.getString("rom_gb", rom_gb));
			
			prop.put("operation", (prePreferences.getBoolean("operation",
					operation) ? "true" : "false"));
			prop.put("version",
					(prePreferences.getBoolean("version", version) ? "true"
							: "false"));
			prop.put("sdcard",
					(prePreferences.getBoolean("sdcard", sdcard) ? "true"
							: "false"));
			prop.put("usb", (prePreferences.getBoolean("usb", usb) ? "true"
					: "false"));
			prop.put("headset",
					(prePreferences.getBoolean("headset", headset) ? "true"
							: "false"));
			prop.put("ethernet", (prePreferences.getBoolean("ethernet",
					ethernet) ? "true" : "false"));
			prop.put("rs232",
					(prePreferences.getBoolean("rs232", rs232) ? "true"
							: "false"));

			prop.put("fpanel",
					(prePreferences.getBoolean("fpanel", fpanel) ? "true"
							: "false"));
			prop.put("sim", (prePreferences.getBoolean("sim", sim) ? "true"
					: "false"));
			prop.put("satadisk", (prePreferences.getBoolean("satadisk",
					satadisk) ? "true" : "false"));
			prop.put("screen",
					(prePreferences.getBoolean("screen", screen) ? "true"
							: "false"));
			prop.put("wifi", (prePreferences.getBoolean("wifi", wifi) ? "true"
					: "false"));
			prop.put("gpio", (prePreferences.getBoolean("gpio", gpio) ? "true"
					: "false"));
			prop.put("rs485",
					(prePreferences.getBoolean("rs485", rs485) ? "true"
							: "false"));

			boolean sb = saveConfig(this, extPropPath, prop);
			if (sb) {
				Toast.makeText(this, "成功导出配置文件:" + extPropPath,
						Toast.LENGTH_SHORT).show();
			} else {
				Toast.makeText(this, "导出文件错误,请确认!", Toast.LENGTH_SHORT).show();
			}
		}
			break;

		default:
			break;
		}
	}

	// 读取配置文件
	public Properties loadConfig(Context context, String file) {

		Properties properties = new Properties();
		try {

			FileInputStream s = new FileInputStream(file);
			properties.load(s);

		} catch (Exception e) {

			e.printStackTrace();
			return null;
		}

		return properties;
	}

	// 保存配置文件
	public boolean saveConfig(Context context, String file,
			Properties properties) {
		try {
			File fil = new File(file);
			if (!fil.exists()) {
				fil.createNewFile();
			}

			FileOutputStream s = new FileOutputStream(fil);
			properties.store(s, "");

		} catch (Exception e) {

			e.printStackTrace();
			return false;
		}

		return true;
	}

}
