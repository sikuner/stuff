package com.norco.burnarm;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;

import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.SharedPreferences;
import android.support.v4.app.NotificationCompat;

public class BurnApp extends Application {

	public static final String BURNARM_CONFIG = "BurnARM.properties";
	public static final String BURNARM = "BurnARM";

	public static String extPropPath = getExStoragePath() + "/"
			+ "BurnARM.properties";

	private String serverIp = "192.168.10.3"; // 
	private String port = "1433";
	private String database = "AIS20110702142856";
	private String username = "sa";
	private String password = "135246";
	
	private int periodSec = 300;
	private int durationSec = 43200;
	
	private String burninPos = "C000";
	
	private SharedPreferences prePreferences = null;
	private SharedPreferences.Editor editor = null;
	
	private NotificationManager mNotificationManager;
	/** Notification���� */
	private NotificationCompat.Builder mBuilder;
	/** Notification������ */
	private int notifyId = 100;

	/** Notification��ID */

	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();
		
		extPropPath = getExStoragePath() + "/" + "BurnARM.properties";
		System.out.println("extPropPath = " + extPropPath);
		
		boolean b = loadConfigExt();
		if (!b) { // ������ɹ�����
			prePreferences = getSharedPreferences(BURNARM, MODE_PRIVATE);
			serverIp = prePreferences.getString("serverIp", serverIp);
			port = prePreferences.getString("port", port);
			database = prePreferences.getString("database", database);
			username = prePreferences.getString("username", username);
			password = prePreferences.getString("password", password);
			
			periodSec = prePreferences.getInt("periodSec", periodSec);
			durationSec = prePreferences.getInt("durationSec", durationSec);
			
			burninPos = prePreferences.getString("burninPos", burninPos);
		}
		
		mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		initNotify();
	}
	
	/** ��ʼ��֪ͨ�� */
	private void initNotify() 
	{
		mBuilder = new NotificationCompat.Builder(this);
		mBuilder.setContentTitle("���Ա���")
				.setContentText("��������")
				.setContentIntent(PendingIntent.getActivity(this, 1, new Intent(), Notification.FLAG_AUTO_CANCEL))
				// .setNumber(number)//��ʾ����
				.setTicker("�ϻ��ϴ�������...")// ֪ͨ�״γ�����֪ͨ��������������Ч����
				.setWhen(System.currentTimeMillis())// ֪ͨ������ʱ�䣬����֪ͨ��Ϣ����ʾ
				.setPriority(Notification.PRIORITY_DEFAULT)// ���ø�֪ͨ���ȼ�
				// .setAutoCancel(true)//���������־���û��������Ϳ�����֪ͨ���Զ�ȡ��
				.setOngoing(false)// ture��������Ϊһ�����ڽ��е�֪ͨ������ͨ����������ʾһ����̨����,�û���������(�粥������)����ĳ�ַ�ʽ���ڵȴ�,���ռ���豸(��һ���ļ�����,ͬ������,������������)
				.setDefaults(Notification.DEFAULT_VIBRATE)// ��֪ͨ������������ƺ���Ч������򵥡���һ�µķ�ʽ��ʹ�õ�ǰ���û�Ĭ�����ã�ʹ��defaults���ԣ�������ϣ�
				// Notification.DEFAULT_ALL Notification.DEFAULT_SOUND ������� //
				// requires VIBRATE permission
				.setSmallIcon(R.drawable.norco_logo);
	}
	
	/** ��ʾ֪ͨ�� */
	public void showNotify(String content){
		mBuilder.setContentTitle("ARM�ϻ�")
				.setContentText(content)
//				.setNumber(number)//��ʾ����
				.setTicker("ARM�ϻ�״̬");//֪ͨ�״γ�����֪ͨ��������������Ч����
		mNotificationManager.notify(notifyId, mBuilder.build());
	}
	
	public boolean doSaveAs() {

		prePreferences = getSharedPreferences(BURNARM, MODE_PRIVATE);
		editor = prePreferences.edit();
		editor.putString("serverIp", getServerIp());
		editor.putString("port", getPort());
		editor.putString("database", getDatabase());
		editor.putString("username", getUsername());
		editor.putString("password", getPassword());
		
		editor.putInt("periodSec", getPeriodSec());
		editor.putInt("durationSec", getDurationSec());
		
		editor.putString("burninPos", getBurninPos());
		
		return editor.commit();
	}
	
	public boolean loadConfigExt() {

		File extFile = new File(extPropPath);
		if (extFile.exists()) {
			Properties prop = loadConfig(extPropPath);
			if (null != prop) {

				setServerIp(prop.getProperty("serverIp"));
				setPort(prop.getProperty("port"));
				setDatabase(prop.getProperty("database"));
				setUsername(prop.getProperty("username"));
				setPassword(prop.getProperty("password"));
				
				setPeriodSec(Integer.parseInt(prop.getProperty("periodSec")));
				setDurationSec(Integer.parseInt(prop.getProperty("durationSec")));
				
				setBurninPos(prop.getProperty("burninPos"));
			}

		} else {
			return false;
		}
		
		return true;
	}

	public boolean doSaveAsExt() {

		Properties prop = loadConfig(extPropPath);
		if (null == prop) {
			return false;
		}

		prop.put("serverIp", getServerIp());
		prop.put("port", getPort());
		prop.put("database", getDatabase());
		prop.put("username", getUsername());
		prop.put("password", getPassword());
		
		prop.put("periodSec", Integer.toString(getPeriodSec()));
		prop.put("durationSec", Integer.toString(getDurationSec()));
		
		prop.put("burninPos", getBurninPos());
		
		boolean sb = saveConfig(extPropPath, prop);

		return sb;
	}

	public String getServerIp() {
		return serverIp;
	}

	public void setServerIp(String serverIp) {
		this.serverIp = serverIp;
	}

	public String getPort() {
		return port;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public String getDatabase() {
		return database;
	}

	public void setDatabase(String database) {
		this.database = database;
	}

	public String getUsername() {
		return username;
	}

	public void setUsername(String username) {
		this.username = username;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public int getPeriodSec() {
		return periodSec;
	}

	public void setPeriodSec(int periodSec) {
		this.periodSec = periodSec;
	}

	public int getDurationSec() {
		return durationSec;
	}

	public void setDurationSec(int durationSec) {
		this.durationSec = durationSec;
	}
	
	public String getBurninPos() {
		return burninPos;
	}

	public void setBurninPos(String burninPos) {
		this.burninPos = burninPos;
	}

	// ��ȡ�����ļ�
	public Properties loadConfig(String path) {

		Properties properties = new Properties();
		try {
			File file = new File(path);
			if (!file.exists()) {
				file.createNewFile();
			}

			FileInputStream s = new FileInputStream(file);
			properties.load(s);

		} catch (Exception e) {

			e.printStackTrace();
			return null;
		}

		return properties;
	}

	// ���������ļ�
	public boolean saveConfig(String file, Properties properties) {
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

	static public String getExStoragePath() {
		// "/mnt/satadisk", "/mnt/extsd", "/mnt/udisk"
		File extsdDir = new File("/mnt/extsd/");
		File satadiskDir = new File("/mnt/satadisk");
		File udiskDir = new File("/mnt/udisk/");

		if (extsdDir.canWrite()) {
			return extsdDir.getAbsolutePath();
		} else if (satadiskDir.canWrite()) {
			return satadiskDir.getAbsolutePath();
		} else if (udiskDir.canWrite()) {
			return udiskDir.getAbsolutePath();
		} else {
			return "";
		}
	}
}
