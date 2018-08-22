package com.norco.hardwaretest;

import java.io.FileInputStream;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

public class ResultActivity extends Activity implements OnClickListener {

	private ListView lstItems;
	private SimpleAdapter adapter;
	List<Map<String, ?>> lst = new ArrayList<Map<String, ?>>();

	private Button btnCommit;
	private Button btnRetest;
	private TextView tvEth0Mac, tvSN;

	String macSN; // 去掉':'的MAC地址

	private SharedPreferences prePreferences;

	// private String serverIp = "192.168.1.152";
	// private String port = "1433";
	// private String database = "NorcoTest";
	// private String username = "sa";
	// private String password = "root";

	private String serverIp = "192.168.10.3";
	private String port = "1433";
	private String database = "AIS20110702142856";
	private String username = "sa";
	private String password = "135246";

	Connection con = null;
	Statement stmt;
	ResultSet rs;

	String eString;
	boolean isConn = false;

	boolean hasUploaded = false;

	String driver = "net.sourceforge.jtds.jdbc.Driver";
	String url; // = "jdbc:jtds:sqlserver://192.168.1.152:1433/NorcoTest";
	String sql;

	String operator = "norcoer";
	String boardsn = "SN:147258369";
	String computersn = "";
	String nprocedure = "功能测试";
	String allres = "";

	public void executeSql(String sql) throws Exception {
		try {
			Class.forName(driver);
			con = DriverManager.getConnection(url, username, password);
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
				System.out
						.println("改SQL语句影响的记录有" + stmt.getUpdateCount() + "条");
				// System.out.println("change sql server records : " +
				// stmt.getUpdateCount());
				// Toast.makeText(ResultActivity.this, "改SQL语句影响的记录有" +
				// stmt.getUpdateCount() + "条", Toast.LENGTH_SHORT).show();
				hasUploaded = true;
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

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_result);

		hasUploaded = false;

		macSN = getLocalMacAddress();
		tvEth0Mac = (TextView) findViewById(R.id.tvEth0Mac);
		tvEth0Mac.setText("序列号(eth0 mac) : " + macSN + "   版型:"
				+ android.os.Build.MODEL);
		macSN = macSN.replaceAll(":", "");
		macSN = macSN.toLowerCase();

		tvSN = (TextView) findViewById(R.id.tvSN);

		lstItems = (ListView) findViewById(R.id.lstItems);
		btnCommit = (Button) findViewById(R.id.btnCommit);
		btnRetest = (Button) findViewById(R.id.btnRetest);
		btnCommit.setOnClickListener(this);
		btnRetest.setOnClickListener(this);

		String[] from = { "name", "note", "result" };
		int[] to = { R.id.itemName, R.id.itemNote, R.id.itemResult };

		for (int i = 0, j = 0; i < MainActivity.testItems.length; i++) {

			if (MainActivity.testItems[i].enabled()) {

				Map<String, String> m = new HashMap<String, String>();
				m.put("name", (++j) + "." + MainActivity.testItems[i].getName());
				// m.put("note", MainActivity.testItems[i].getNote());
				m.put("note", "l");
				m.put("result", MainActivity.testItems[i].getResult());

				lst.add(m);
			}
		}

		adapter = new SimpleAdapter(this, lst, R.layout.result_item, from, to);
		lstItems.setAdapter(adapter);

		prePreferences = getSharedPreferences("HardwareTestSettings",
				MODE_PRIVATE);
		serverIp = prePreferences.getString("serverIp", serverIp);
		port = prePreferences.getString("port", port);
		database = prePreferences.getString("database", database);
		username = prePreferences.getString("username", username);
		password = prePreferences.getString("password", password);

		operator = prePreferences.getString("operator", operator);
		boardsn = prePreferences.getString("boardsn", boardsn);
		computersn = prePreferences.getString("computersn", computersn);
		nprocedure = prePreferences.getString("nprocedure", nprocedure);

		allres = getAllRes();
		tvSN.setText("总结果:" + allres + "  主板序列号:" + boardsn + "  整机序列号:"
				+ computersn);
	}

	private String translateResult(String res) {

		if (res.contains("成功")) {
			return "PASS";
		} else if (res.contains("失败")) {
			return "FAIL";
		} else {
			return res;
		}
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.btnCommit: {

			url = String.format("jdbc:jtds:sqlserver://%s:%s/%s", serverIp,
					port, database);
			System.out.println("url = " + url);
			System.out.println("computersn = " + computersn);

			sql = String
					.format("INSERT INTO arm_test(id, orderdate, boardtype, operator, boardsn, computersn, nprocedure, fpanel, version, sim, sdcard, satadisk, usb, screen, headset, wifi, ethernet, gpio, rs232, rs485, allres)"
							+ "VALUES('%s', GETDATE(), '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
							macSN, // mac地址作为序列号
							android.os.Build.MODEL,

							operator, // OPERATION
							boardsn, computersn, nprocedure,

							translateResult(MainActivity.testItems[1]
									.getResult()), // FPANEL
							translateResult(MainActivity.testItems[2]
									.getResult()), // VERSION
							translateResult(MainActivity.testItems[3]
									.getResult()), // SIM
							translateResult(MainActivity.testItems[4]
									.getResult()), // SDCARD
							translateResult(MainActivity.testItems[5]
									.getResult()), // SATADISK
							translateResult(MainActivity.testItems[6]
									.getResult()), // USB
							translateResult(MainActivity.testItems[7]
									.getResult()), // HEADSETLOOP
							translateResult(MainActivity.testItems[8]
									.getResult()), // SCREENDISPLAY
							translateResult(MainActivity.testItems[9]
									.getResult()), // WIFI
							translateResult(MainActivity.testItems[10]
									.getResult()), // ETHERNET
							translateResult(MainActivity.testItems[11]
									.getResult()), // GPIO
							translateResult(MainActivity.testItems[12]
									.getResult()), // SERIALPORT, RS232
							translateResult(MainActivity.testItems[13]
									.getResult()), // RS485
							translateResult(allres));

			System.out.println("sql = " + sql);

			if (!hasUploaded) { // 没有上传成功
				new Thread() {
					public void run() {

						try {
							executeSql(sql);
						} catch (Exception e) {
							e.printStackTrace();
						}

						runOnUiThread(new Runnable() {
							public void run() {

								if (hasUploaded) {
									Toast.makeText(ResultActivity.this,
											"测试结果上传成功!", Toast.LENGTH_SHORT)
											.show();
								} else {
									Toast.makeText(ResultActivity.this,
											"测试结果上传失败!", Toast.LENGTH_SHORT)
											.show();
								}
							}
						});

					};

				}.start();
			} else {
				Toast.makeText(ResultActivity.this, "上传已成功,无需再次上传!",
						Toast.LENGTH_SHORT).show();
			}
		}
			break;

		case R.id.btnRetest: {
			MainActivity.result_retest = false;
			finish();
		}
			break;

		default:

			break;
		}
	}

	private String getAllRes() {

		String allres = "NULL";
		int pass_cnt = 0;
		for (int i = 1; i <= 13; i++) {
			if (MainActivity.testItems[i].getResult().contains("失败")) {
				allres = "失败";
				return allres;
			}else if(MainActivity.testItems[i].getResult().contains("成功")) {
				pass_cnt++;
			}
		}

		if (pass_cnt==13) {
			allres = "成功";
		}
		
		return allres;
	}

	public String getLocalMacAddress() {
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
			System.out.println("获取eth0 mac失败:" + io.toString());
		}

		return Mac.trim();
	}
}
