package com.norco.hardwaretest;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.SimpleAdapter;

public class MainActivity extends Activity {
	
	public static boolean result_retest = true;
	private SharedPreferences prePreferences;
	private SharedPreferences.Editor editor;
	
	public static final TestItemE[] testItems = {
		
		TestItemE.OPERATION, 
		TestItemE.VERSION, 
		TestItemE.SIM,
		TestItemE.SDCARD, 
		TestItemE.SATADISK, 
		TestItemE.USB,
		TestItemE.SCREENDISPLAY, 
		TestItemE.HEADSETLOOP, 
		TestItemE.WIFI,
		TestItemE.ETHERNET, 
		TestItemE.GPIO, 
		TestItemE.SERIALPORT,
		TestItemE.RS485,
		TestItemE.FPANEL, 
		
	};
	
	public static int itemRealIndexs[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	
	public static TestItemE getTestItemE(int index) {
		return testItems[index];
	}
	
	private ListView lvTests;
	private SimpleAdapter adapter;
	List<Map<String, ?>> lst = new ArrayList<Map<String, ?>>();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		prePreferences = getSharedPreferences("HardwareTestSettings", MODE_PRIVATE);
		
		// 导入配置
		loadConfig();
		
		TestItemE.OPERATION.setEnabled(prePreferences.getBoolean("operation", true)); 
		TestItemE.FPANEL.setEnabled(prePreferences.getBoolean("fpanel", true));
		TestItemE.VERSION.setEnabled(prePreferences.getBoolean("version", true));
		TestItemE.SIM.setEnabled(prePreferences.getBoolean("sim", true));
		TestItemE.SDCARD.setEnabled(prePreferences.getBoolean("sdcard", true)); 
		TestItemE.SATADISK.setEnabled(prePreferences.getBoolean("satadisk", true)); 
		TestItemE.USB.setEnabled(prePreferences.getBoolean("usb", true));
		TestItemE.HEADSETLOOP.setEnabled(prePreferences.getBoolean("headset", true));
		TestItemE.SCREENDISPLAY.setEnabled(prePreferences.getBoolean("screen", true));
		TestItemE.WIFI.setEnabled(prePreferences.getBoolean("wifi", true));
		TestItemE.ETHERNET.setEnabled(prePreferences.getBoolean("ethernet", true)); 
		TestItemE.GPIO.setEnabled(prePreferences.getBoolean("gpio", true));
		TestItemE.SERIALPORT.setEnabled(prePreferences.getBoolean("rs232", true));
		TestItemE.RS485.setEnabled(prePreferences.getBoolean("rs485", true)); 
		
		String[] from = { "item", "result" };
		int[] to = { R.id.tvItem, R.id.tvResult };
		
		lvTests = (ListView) findViewById(R.id.lvTests);
		for (int i = 0, j = 0; i < testItems.length; i++) {
			
			if (testItems[i].enabled()) {
				
				itemRealIndexs[j] = i;
				
				Map<String, String> m = new HashMap<String, String>();
				m.put("item", (++j) + "." + testItems[i].getName());
				m.put("result", testItems[i].getResult());
				lst.add(m);
			}
		}
		
		for (int i = 0; i < itemRealIndexs.length; i++) {
			System.out.println("itemRealIndexs["+i+"]"+"="+itemRealIndexs[i]);
		}
		
		adapter = new SimpleAdapter(this, lst, R.layout.test_item, from, to);
		lvTests.setAdapter(adapter);
		
		lvTests.setOnItemClickListener(new OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {

				Intent i = new Intent(MainActivity.this, MainFragment.class);
				Bundle b = new Bundle();
				b.putInt("position", position);
				i.putExtras(b);
				startActivity(i);
			}
		});
	}
	
	boolean isFolderExists(String strFolder) {
		
		File file = new File(strFolder);
		if (!file.exists()) 
		{
			if (file.mkdirs()) 
			{				
				return true;
			} 
			else 
			{
				return false;
			}
		}
		
		return true;
	}
	
	private int loadConfig() {
		Properties prop;
		
//		String extPropPath = "/mnt/extsd/HardwareTestSettings.properties";
		String extPropPath = getExStoragePath()+"/"+"HardwareTestSettings.properties";
		File extFile = new File(extPropPath);
		if (extFile.exists()) {
			prop = loadConfig(this, extPropPath);
			if (null != prop) {
				
				editor = prePreferences.edit();
				editor.putString("serverIp", prop.getProperty("serverIp"));
				editor.putString("port", prop.getProperty("port"));
				editor.putString("database", prop.getProperty("database"));
				editor.putString("username", prop.getProperty("username"));
				editor.putString("password", prop.getProperty("password"));
				
				editor.putString("operator", prop.getProperty("operator"));
				editor.putString("nprocedure", prop.getProperty("nprocedure"));
				editor.putString("pinghostip", prop.getProperty("pinghostip"));
				editor.putString("hostips", prop.getProperty("hostips"));
				
				editor.putString("ram_mb", prop.getProperty("ram_mb"));
				editor.putString("rom_gb", prop.getProperty("rom_gb"));
				
				editor.putBoolean("operation", Boolean.parseBoolean(prop.getProperty("operation")));
				editor.putBoolean("version", Boolean.parseBoolean(prop.getProperty("version")));
				editor.putBoolean("sdcard", Boolean.parseBoolean(prop.getProperty("sdcard")));
				editor.putBoolean("usb", Boolean.parseBoolean(prop.getProperty("usb")));
				editor.putBoolean("headset", Boolean.parseBoolean(prop.getProperty("headset")));
				editor.putBoolean("ethernet", Boolean.parseBoolean(prop.getProperty("ethernet")));
				editor.putBoolean("rs232", Boolean.parseBoolean(prop.getProperty("rs232")));
				
				editor.putBoolean("fpanel", Boolean.parseBoolean(prop.getProperty("fpanel")));
				editor.putBoolean("sim", Boolean.parseBoolean(prop.getProperty("sim")));
				editor.putBoolean("satadisk", Boolean.parseBoolean(prop.getProperty("satadisk")));
				editor.putBoolean("screen", Boolean.parseBoolean(prop.getProperty("screen")));
				editor.putBoolean("wifi", Boolean.parseBoolean(prop.getProperty("wifi")));
				editor.putBoolean("gpio", Boolean.parseBoolean(prop.getProperty("gpio")));
				editor.putBoolean("rs485", Boolean.parseBoolean(prop.getProperty("rs485")));
				
				editor.commit();
				
				return 0;
			}else {
				return -2;
			}
		}else {
			return -1;
		}
	}
	
	@Override
	protected void onStart() {

		super.onStart();
		
	}
	
	//读取配置文件 
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
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		switch (item.getItemId()) {
		case R.id.result:
			startActivity(new Intent(MainActivity.this, ResultActivity.class));
			return true;
		case R.id.settings:
			startActivity(new Intent(MainActivity.this, SettingsActivity.class));
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();

		int pass_sum = 0;

		lst.clear();
		for (int i = 0, j = 0; i < testItems.length; i++) {
			
			if (testItems[i].enabled()) {
				Map<String, String> m = new HashMap<String, String>();
				m.put("item", (++j) + "." + testItems[i].getName());
				m.put("result", testItems[i].getResult());
				
				if (testItems[i].getResult().contains("成功")) {
					pass_sum++;
				}
				
				lst.add(m);
			}			
		}
		
		adapter.notifyDataSetChanged();
		
		System.out.println("PASS_SUM = " + pass_sum);
		
		if ((pass_sum == lst.size()) && result_retest) {
			System.out.println("CHANGE INTO " + pass_sum);
			startActivity(new Intent(MainActivity.this, ResultActivity.class));
		}
		
		result_retest = true;
		// System.out.println("MainActivity:onResume");
	}

	static {

		try {
			
			System.out.println("loadLibrary  11111111111111111111111111");
			System.loadLibrary("norco_utils");
			System.out.println("loadLibrary  22222222222222222222222222");
			
		} catch (Exception e) {
			e.printStackTrace();
		}

	}
	
	static public String getExStoragePath() {
//		"/mnt/satadisk", "/mnt/extsd", "/mnt/udisk"
		File extsdDir = new File("/mnt/extsd/");
		File satadiskDir = new File("/mnt/satadisk");
		File udiskDir = new File("/mnt/udisk/");
		
		if (extsdDir.canWrite()) {
			return extsdDir.getAbsolutePath();
		}else if(satadiskDir.canWrite()) {
			return satadiskDir.getAbsolutePath();
		}else if(udiskDir.canWrite()) {
			return udiskDir.getAbsolutePath();
		}else {
			return "";
		}
	}
	
}
