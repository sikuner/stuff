package com.norco.burnarm;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemClock;

public class StateService extends Service {

	private String sn, boardSn, wholeSn, date, mac, model, uptime, duration, burninpos, testState;
	private String res;

	private Timer timer;
	private TimerTask timerTask;

	private final int TIMER_UPLOAD = 10000;

	String driver = "net.sourceforge.jtds.jdbc.Driver";
	String url; // = "jdbc:jtds:sqlserver://192.168.1.152:1433/NorcoTest";
	String sql;

	Connection con = null;
	Statement stmt;
	ResultSet rs;

	private String serverIp;// = "192.168.1.152";
	private String port;// = "1433";
	private String database;// = "NorcoTest";
	private String username;// = "sa";
	private String password;// = "root";
	
	private int periodSec;// = 1;
	private int durationSec;// = 4 * 60 * 60;
	
	private long first_uptime = -1;
	private long last_uptime = -1;
	private boolean last_upload = false;

	private boolean hasUploaded = false;
	private int upload_cnt = 0;
	
	public void executeSql(String sql) throws Exception {

		hasUploaded = false;

		try {
			Class.forName(driver);
			con = DriverManager.getConnection(url, username, password);
			if (null == con) {
				System.out.println("DATABASE con=" + con);
				return;
			}
			
			stmt = con.createStatement();
			boolean hasResultSet = stmt.execute(sql);
			if (hasResultSet) {
				rs = stmt.getResultSet();
				java.sql.ResultSetMetaData rsmd = rs.getMetaData();
				int columnCount = rsmd.getColumnCount();
				
				while (rs.next()) {
					for (int i = 0; i < columnCount; i++) {
						System.out.print(rs.getString(i + 1) + "\t");
					}
					System.out.println("NEXT");
				}
			} else {
				
				System.out.println("改SQL语句影响的记录有" + stmt.getUpdateCount() + "条");
				// System.out.println("change sql server records : " +
				// stmt.getUpdateCount());
				// Toast.makeText(ResultActivity.this, "改SQL语句影响的记录有" +
				// stmt.getUpdateCount() + "条", Toast.LENGTH_SHORT).show();
				hasUploaded = true;
				
				if (!last_upload) {
					((BurnApp) getApplication()).showNotify("已上传老化数据,条数:"+(++upload_cnt));
				}else {
					((BurnApp) getApplication()).showNotify("已老化完毕,条数:"+(++upload_cnt));
				}
				
			}
		} finally {
			if (rs != null) {
				rs.close();
			}
			if (stmt != null) {
				stmt.close();
			}
			if (con != null) {
				con.close();
			}
		}
	}

	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {

			switch (msg.what) {
			case TIMER_UPLOAD: {
				// tvTimeValue.setText("" + msg.arg1);
				// startTime();

				url = String.format("jdbc:jtds:sqlserver://%s:%s/%s", serverIp,
						port, database);
				System.out.println("url = " + url);

				sql = String.format(
						"INSERT INTO burninarm(boardsn, wholesn, orderdate, mac, model, uptime, duration, burninpos, state)"
								+ "VALUES('%s', '%s', %s, '%s', '%s', '%s', '%s', '%s', '%s')",
								boardSn, wholeSn, date, mac, model, uptime, duration, burninpos, testState);

				System.out.println("TIMER_UPLOAD: sql = " + sql);

				new Thread() {
					public void run() {

						try {
							executeSql(sql);
						} catch (Exception e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}

					};
				}.start();

				// last_upload
				if (!last_upload) {
					startTimer();
				} 
//					else {
//					((BurnApp) getApplicationContext()).showNotify("已老化完毕,条数:"+(++upload_cnt));
//				}
			}
				break;

			default:
				break;
			}

		};
	};

	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();

		serverIp = ((BurnApp) getApplication()).getServerIp();
		port = ((BurnApp) getApplication()).getPort();
		database = ((BurnApp) getApplication()).getDatabase();
		username = ((BurnApp) getApplication()).getUsername();
		password = ((BurnApp) getApplication()).getPassword();

		periodSec = ((BurnApp) getApplication()).getPeriodSec();
		durationSec = ((BurnApp) getApplication()).getDurationSec();

		System.out.println("IN StateService.onCreate");

		System.out.println("serverIp = " + serverIp);
		System.out.println("port = " + port);
		System.out.println("database = " + database);
		System.out.println("username = " + username);
		System.out.println("password = " + password);

		System.out.println("periodSec = " + periodSec);
		System.out.println("durationSec = " + durationSec);
		
		((BurnApp) getApplication()).showNotify("准备后台上传...");
		startTimer();
		upload_cnt = 0;
	}

	@Override
	public void onStart(Intent intent, int startId) {
		// TODO Auto-generated method stub
		super.onStart(intent, startId);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// TODO Auto-generated method stub
		return super.onStartCommand(intent, flags, startId);
	}

	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		((BurnApp) getApplicationContext()).showNotify("停止后台上传.");
		stopTimer();
		System.out.println("IN StateService.onDestroy");
	}

	private void startTimer() {
		timer = new Timer();
		timerTask = new TimerTask() {

			@Override
			public void run() {
				// TODO Auto-generated method stub

				System.out.println("Timer.run() ");

				Message msg = new Message();
				msg.what = TIMER_UPLOAD;
				readyData();
				mHandler.sendMessage(msg);

			}
		};

		timer.schedule(timerTask, periodSec * 1000);
	}

	private void stopTimer() {
		timer.cancel();
	}

	private void readyData() {

		sn = android.os.Build.SERIAL;
		boardSn = android.os.Build.SERIAL;
		
		{
			try {
				Process process = Runtime.getRuntime().exec("getprop ro.serialno2");
				InputStreamReader ir = new InputStreamReader(process.getInputStream());  
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
		// SimpleDateFormat sdf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss",
		// Locale.getDefault());
		// date = sdf.format(new Date());
		
		mac = MainActivity.getLocalMacAddress();
		
		model = android.os.Build.MODEL;
		
		long l = SystemClock.elapsedRealtime();
		long iUptime = l / 1000; // 秒
		int h = (int) l / 1000 / 3600;
		int m = (int) l / 1000 / 60 % 60;
		int s = (int) l / 1000 % 60;
		uptime = String.format("%02d:%02d:%02d", h, m, s);
		if (first_uptime < 0) {
			first_uptime = iUptime;
			last_uptime = first_uptime + durationSec;
		}
		
		String cmd = "ps | grep stability | busybox wc -l";
		testState = MainActivity.RootCmd(cmd);

		if (iUptime > last_uptime) {
			last_upload = true;
			if (0 == Integer.parseInt(testState)) {
				testState = "FAIL";
			} else {
				testState = "PASS";
			}
		} else {
			if (0 == Integer.parseInt(testState)) {
				testState = "NORUNNING";
			} else {
				testState = "RUNNING";
			}
		}
		serverIp = ((BurnApp) getApplication()).getServerIp();
		
		duration = Integer.toString(((BurnApp) getApplication()).getDurationSec());
		burninpos = ((BurnApp) getApplication()).getBurninPos();
	}

	public String getUptime() {

		long l = SystemClock.elapsedRealtime();
		int h = (int) l / 1000 / 3600;
		int m = (int) l / 1000 / 60 % 60;
		int s = (int) l / 1000 % 60;

		return String.format("%02d:%02d:%02d", h, m, s);
	}

}
