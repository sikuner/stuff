package com.norco.burnarm;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

// 1.���к�
// 2.ʱ��
// 3.��λMAC��ַ
// 4.�ͺ�
// 5.����ʱ��
// 6.�����������״̬ () ps | grep stability
// 

public class MainActivity extends Activity implements OnClickListener {

	private TextView tvBoardSn, tvWholeSn, tvDate, tvMac, tvModel, tvUptime,
			tvTestState;
	
	private TextView tvBurninPos;
	private EditText etBurninPos;
	private Button btnBurninPosOK;
	
	private String boardSn, wholeSn, date, mac, model, uptime, testState;
	private String res;

	private Button btnRefresh, btnTest;
	private Button btnStartUpload, btnStopUpload;

	private Intent intent;

	private NotificationManager mNotificationManager;
	/** Notification���� */
	private NotificationCompat.Builder mBuilder;
	/** Notification������ */
	private int notifyId = 100; /** Notification��ID */

	

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		initNotify();
		
		intent = new Intent(MainActivity.this, StateService.class);
		
		btnRefresh = (Button) findViewById(R.id.btnRefresh);
		btnTest = (Button) findViewById(R.id.btnTest);
		btnTest.setOnClickListener(this);
		btnRefresh.setOnClickListener(this);
		
		btnStartUpload = (Button) findViewById(R.id.btnStartUpload);
		btnStopUpload = (Button) findViewById(R.id.btnStopUpload);
		btnStartUpload.setOnClickListener(this);
		btnStopUpload.setOnClickListener(this);

		tvBoardSn = (TextView) findViewById(R.id.tvBoardSn);
		tvWholeSn = (TextView) findViewById(R.id.tvWholeSn);
		tvDate = (TextView) findViewById(R.id.tvDate);
		tvMac = (TextView) findViewById(R.id.tvMac);
		tvModel = (TextView) findViewById(R.id.tvModel);
		tvUptime = (TextView) findViewById(R.id.tvUptime);
		tvTestState = (TextView) findViewById(R.id.tvTestState);
		
		tvBurninPos = (TextView) findViewById(R.id.tvBurninPos);
		etBurninPos = (EditText) findViewById(R.id.etBurninPos);
		btnBurninPosOK = (Button) findViewById(R.id.btnBurninPosOK);
		btnBurninPosOK.setOnClickListener(this);
		
		boardSn = android.os.Build.SERIAL;
		tvBoardSn.setText(boardSn);
		
		{
			try {
				Process process = Runtime.getRuntime().exec(
						"getprop ro.serialno2");
				InputStreamReader ir = new InputStreamReader(
						process.getInputStream());
				BufferedReader input = new BufferedReader(ir);

				String line = "", res = "";
				while (null != (line = input.readLine())) {
					res += line;
				}

				wholeSn = res.trim();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		tvWholeSn.setText(wholeSn);

		date = "GETDATE()";
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss",
				Locale.getDefault());
		date = sdf.format(new Date());
		tvDate.setText(date);

		mac = getLocalMacAddress();
		tvMac.setText(mac);

		model = android.os.Build.MODEL;
		tvModel.setText(model);

		long l = SystemClock.elapsedRealtime();
		int h = (int) l / 1000 / 3600;
		int m = (int) l / 1000 / 60 % 60;
		int s = (int) l / 1000 % 60;
		uptime = String.format("%02d:%02d:%02d", h, m, s);
		tvUptime.setText(uptime);

		String cmd = "ps | grep stability | busybox wc -l";
		testState = RootCmd(cmd);
		tvTestState.setText(testState);
		
		tvBurninPos.setText(((BurnApp)getApplication()).getBurninPos());
	}
	
	/** ��ʼ��֪ͨ�� */
	private void initNotify() {
		mBuilder = new NotificationCompat.Builder(this);
		mBuilder.setContentTitle("���Ա���")
				.setContentText("��������")
				.setContentIntent(PendingIntent.getActivity(this, 1, new Intent(), Notification.FLAG_AUTO_CANCEL))
				// .setNumber(number)//��ʾ����
				.setTicker("����֪ͨ����")// ֪ͨ�״γ�����֪ͨ��������������Ч����
				.setWhen(System.currentTimeMillis())// ֪ͨ������ʱ�䣬����֪ͨ��Ϣ����ʾ
				.setPriority(Notification.PRIORITY_DEFAULT)// ���ø�֪ͨ���ȼ�
				// .setAutoCancel(true)//���������־���û��������Ϳ�����֪ͨ���Զ�ȡ��
				.setOngoing(false)// ture��������Ϊһ�����ڽ��е�֪ͨ������ͨ����������ʾһ����̨����,�û���������(�粥������)����ĳ�ַ�ʽ���ڵȴ�,���ռ���豸(��һ���ļ�����,ͬ������,������������)
				.setDefaults(Notification.DEFAULT_VIBRATE)// ��֪ͨ������������ƺ���Ч������򵥡���һ�µķ�ʽ��ʹ�õ�ǰ���û�Ĭ�����ã�ʹ��defaults���ԣ�������ϣ�
				// Notification.DEFAULT_ALL Notification.DEFAULT_SOUND ������� //
				// requires VIBRATE permission
				.setSmallIcon(R.drawable.ic_launcher);
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

		case R.id.settings:
			startActivity(new Intent(MainActivity.this, SettingsActivity.class));
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	static public String getLocalMacAddress() {
		String Mac = null;
		try {
			String path = "sys/class/net/eth0/address";
			FileInputStream fis_name = new FileInputStream(path);
			byte[] buffer_name = new byte[8192];
			int byteCount_name = fis_name.read(buffer_name);
			if (byteCount_name > 0) {
				Mac = new String(buffer_name, 0, byteCount_name, "utf-8");
			}
			fis_name.close();
			if (Mac.length() == 0 || Mac == null) {
				return null;
			}
		} catch (Exception io) {
			System.out.println("��ȡeth0 macʧ��:" + io.toString());
		}

		return Mac.trim();
	}

	public String getTestState() {

		return "";
	}

	public static String RootCmd(String cmd) {

		Process process = null;
		DataOutputStream os = null;
		DataInputStream is = null;
		InputStreamReader isr = null;
		BufferedReader br = null;

		String res = "";
		String line = "";

		try {
			process = Runtime.getRuntime().exec("su");
			Log.i("tag", cmd);
			os = new DataOutputStream(process.getOutputStream());
			is = new DataInputStream(process.getInputStream());
			isr = new InputStreamReader(is, "UTF-8");
			br = new BufferedReader(isr);
			os.writeBytes(cmd + "\n");
			os.flush();

			line = br.readLine();

			os.writeBytes("exit\n");
			os.flush();
			int a = process.waitFor();
			Log.i("tag", "line:" + line);
		} catch (Exception e) {
		} finally {
			try {
				if (os != null) {
					os.close();
				}

				if (br != null) {
					br.close();
				}

				if (isr != null) {
					isr.close();
				}

				if (is != null) {
					is.close();
				}

				if (process != null) {
					process.destroy();
					process = null;
				}

			} catch (Exception e) {
			}
		}

		return line;
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		
		case R.id.btnBurninPosOK:
			{
				((BurnApp)getApplication()).setBurninPos(etBurninPos.getText().toString());
				tvBurninPos.setText(((BurnApp)getApplication()).getBurninPos());
			}
			break;
		
		case R.id.btnRefresh: {
			
			readyData();
			
			tvBoardSn.setText(boardSn);
			tvWholeSn.setText(wholeSn);
			tvDate.setText(date);
			tvMac.setText(mac);
			tvModel.setText(model);
			tvUptime.setText(uptime);
			tvTestState.setText(testState);
			
		}
			break;

		case R.id.btnTest: {
			mBuilder.setContentTitle("���Ա���").setContentText("��������")
					.setNumber(7)// ��ʾ����
					.setTicker("����֪ͨ����");// ֪ͨ�״γ�����֪ͨ��������������Ч����
			mNotificationManager.notify(notifyId, mBuilder.build());
		}
			break;

		case R.id.btnStartUpload: {
			startService(intent);
		}
			break;
		case R.id.btnStopUpload: {
			stopService(intent);
		}
			break;

		default:
			break;
		}

	}

	private void readyData() {

		boardSn = android.os.Build.SERIAL;

		{
			try {
				Process process = Runtime.getRuntime().exec(
						"getprop ro.serialno2");
				InputStreamReader ir = new InputStreamReader(
						process.getInputStream());
				BufferedReader input = new BufferedReader(ir);

				String line = "", res = "";
				while (null != (line = input.readLine())) {
					res += line;
				}

				wholeSn = res.trim();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		date = "GETDATE()";
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss",
				Locale.getDefault());
		date = sdf.format(new Date());

		mac = getLocalMacAddress();

		model = android.os.Build.MODEL;

		long l = SystemClock.elapsedRealtime();
		int h = (int) l / 1000 / 3600;
		int m = (int) l / 1000 / 60 % 60;
		int s = (int) l / 1000 % 60;
		uptime = String.format("%02d:%02d:%02d", h, m, s);

		String cmd = "ps | grep stability | busybox wc -l";
		testState = RootCmd(cmd);

		res = boardSn + "," + date + "," + mac + "," + model + "," + uptime
				+ "," + testState;
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
