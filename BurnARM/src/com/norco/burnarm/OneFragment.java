package com.norco.burnarm;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class OneFragment extends Fragment implements OnClickListener {

	private Button btnConn;
	private EditText etServerIP, etPort, etDatabase, etUsername, etPassword;
	private TextView tvConnTips;
	
	private Connection con = null;
	private boolean isConn = false;
	private String eString;
	
	private String serverIp;// = "192.168.1.152";
	private String port;// = "1433";
	private String database;// = "NorcoTest";
	private String username;// = "sa";
	private String password;// = "root";
	
	private String driver = "net.sourceforge.jtds.jdbc.Driver";
	private String url; // "jdbc:jtds:sqlserver://192.168.1.152:1433/NorcoTest";
	private String sql;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.one_fragment, container,
				false);
		
		btnConn = (Button) rootView.findViewById(R.id.btnConn);
		btnConn.setOnClickListener(this);
		
		etServerIP = (EditText) rootView.findViewById(R.id.etServerIP);
		etPort = (EditText) rootView.findViewById(R.id.etPort);
		etDatabase = (EditText) rootView.findViewById(R.id.etDatabase);
		etUsername = (EditText) rootView.findViewById(R.id.etUsername);
		etPassword = (EditText) rootView.findViewById(R.id.etPassword);
		
		tvConnTips = (TextView) rootView.findViewById(R.id.tvConnTips);
		
		serverIp = ((BurnApp)getActivity().getApplication()).getServerIp();
		port = ((BurnApp)getActivity().getApplication()).getPort();
		database = ((BurnApp)getActivity().getApplication()).getDatabase();
		username = ((BurnApp)getActivity().getApplication()).getUsername();
		password = ((BurnApp)getActivity().getApplication()).getPassword();
		
		etServerIP.setText(serverIp);
		etPort.setText(port);
		etDatabase.setText(database);
		etUsername.setText(username);
		etPassword.setText(password);
		
		return rootView;
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.btnConn: {

			serverIp = etServerIP.getText().toString();
			port = etPort.getText().toString();
			database = etDatabase.getText().toString();
			username = etUsername.getText().toString();
			password = etPassword.getText().toString();

			// String url =
			// "jdbc:jtds:sqlserver://192.168.1.152:1433/NorcoTest";
			url = String.format("jdbc:jtds:sqlserver://%s:%s/%s", serverIp,
					port, database);
			System.out.println("url = " + url);

			Thread t = new Thread() {
				public void run() {

					isConn = false;

					try {
						Class.forName(driver);
						con = DriverManager.getConnection(url, username,
								password);
					} catch (ClassNotFoundException e) {
						eString = e.toString();
					} catch (SQLException e) {
						eString = e.toString();
					} finally {
						if (con != null) {
							isConn = true;
							try {
								con.close();
							} catch (SQLException e) {
								eString = e.toString();
							}
							con = null;
						}

						getActivity().runOnUiThread(new Runnable() {
							public void run() {

								if (isConn) { // 可以成功连接数据库
									tvConnTips.setText("连接提示: 数据库连接成功!");
								} else {
									tvConnTips.setText("连接提示: 数据库连接出错:"
											+ eString);
								}
							}
						});
					}

				};
			};
			t.start();

		}
			break;
			
		default:
			break;
		}
	}

	public int doSaveAs() {
		
		serverIp = etServerIP.getText().toString();
		port = etPort.getText().toString();
		database = etDatabase.getText().toString();
		username = etUsername.getText().toString();
		password = etPassword.getText().toString();
				
		((BurnApp)getActivity().getApplication()).setServerIp(serverIp);
		((BurnApp)getActivity().getApplication()).setPort(port);
		((BurnApp)getActivity().getApplication()).setDatabase(database);
		((BurnApp)getActivity().getApplication()).setUsername(username);
		((BurnApp)getActivity().getApplication()).setPassword(password);
		
		return 0;
	}

} 
