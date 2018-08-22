package com.norco.hardwaretest;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class OperationFragment extends Fragment implements OnClickListener {

	private EditText etOperator, etBoardSN, etProcedure;

	private SharedPreferences prePreferences;
	private Editor editor;
	
	private EditText etBoardSN1, etComputerSN;
	private Button btnRead1, btnSave1, btnWrite1, btnRead2, btnSave2, btnWrite2;

	private String operator = "";
	private String boardsn = "", computersn = "";
	private String nprocedure = "";

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.operation_fragment,
				container, false);

		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);

		etOperator = (EditText) rootView.findViewById(R.id.etOperator);
		// etBoardSN = (EditText) rootView.findViewById(R.id.etBoardSN);
		etProcedure = (EditText) rootView.findViewById(R.id.etProcedure);

		etBoardSN1 = (EditText) rootView.findViewById(R.id.etBoardSN1);
		etComputerSN = (EditText) rootView.findViewById(R.id.etComputerSN);

		btnRead1 = (Button) rootView.findViewById(R.id.btnRead1);
		btnRead2 = (Button) rootView.findViewById(R.id.btnRead2);
		btnWrite1 = (Button) rootView.findViewById(R.id.btnWrite1);
		btnWrite2 = (Button) rootView.findViewById(R.id.btnWrite2);
		btnSave1 = (Button) rootView.findViewById(R.id.btnSave1);
		btnSave2 = (Button) rootView.findViewById(R.id.btnSave2);
		btnRead1.setOnClickListener(this);
		btnRead2.setOnClickListener(this);
		btnWrite1.setOnClickListener(this);
		btnWrite2.setOnClickListener(this);
		btnSave1.setOnClickListener(this);
		btnSave2.setOnClickListener(this);
		
		prePreferences = getActivity().getSharedPreferences(
				"HardwareTestSettings", FragmentActivity.MODE_PRIVATE);
		operator = prePreferences.getString("operator", operator);
		
		boardsn = prePreferences.getString("boardsn", boardsn);
		computersn = prePreferences.getString("computersn", computersn);
		
		nprocedure = prePreferences.getString("nprocedure", nprocedure);
		if (!operator.isEmpty()) {
			etOperator.setText(operator);
		}
		if (!boardsn.isEmpty()) {
			etBoardSN1.setText(boardsn);
		}
		if (!computersn.isEmpty()) {
			etComputerSN.setText(computersn);
		}
		if (!nprocedure.isEmpty()) {
			etProcedure.setText(nprocedure);
		}

		return rootView;
	}

	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity
				.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {

		case R.id.btnRead1: {
			// etBoardSN1;
			// phoneInfo += "\n SERIAL: " + android.os.Build.SERIAL;
			etBoardSN1.setText("" + android.os.Build.SERIAL);
			// android.os.Build.SERIAL
			// SystemProperties.get(property, UNKNOWN);
			// SystemProperties.
		}
			break;
		case R.id.btnWrite1: {
			
			editor = prePreferences.edit();			
			boardsn = etBoardSN1.getText().toString().trim();
			editor.putString("boardsn", boardsn);
			editor.commit();
			
			String cmd = String.format("wsn norco_write_sn_12345 %s",
					etBoardSN1.getText().toString());
			RootCmd(cmd);

		}
			break;
		case R.id.btnRead2: {
			// etComputerSN

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

				String computerSN = res.trim();
				etComputerSN.setText(computerSN);

			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		}
			break;
		case R.id.btnWrite2: {
			
			editor = prePreferences.edit();
			computersn = etComputerSN.getText().toString().trim();
			editor.putString("computersn", computersn);
			editor.commit();
			
			String cmd = String.format("wsn2 norco_write_sn_12345 %s",
					etComputerSN.getText().toString());
			RootCmd(cmd);

		}
			break;

		case R.id.btnSave1:
			{
				editor = prePreferences.edit();
				
				boardsn = etBoardSN1.getText().toString().trim();
				editor.putString("boardsn", boardsn);
				
				editor.commit();
			}
			break;
		case R.id.btnSave2:
			{
				editor = prePreferences.edit();
				
				computersn = etComputerSN.getText().toString().trim();
				editor.putString("computersn", computersn);
				
				editor.commit();
			}
			break;
		case R.id.btnPass: {

			// 保存数据
			{

				if (etOperator.getText().toString().isEmpty()
						|| etProcedure.getText().toString().isEmpty()) {

					Toast.makeText(getActivity(), "操作员/序列号/工序 不可以为空, 请确认!",
							Toast.LENGTH_SHORT).show();

					return;
				}

				if (etBoardSN1.getText().toString().isEmpty()) { // 如果没有录入
					boardsn = android.os.Build.SERIAL;
				} else {
					boardsn = etBoardSN1.getText().toString();
				}

				if (etComputerSN.getText().toString().isEmpty()) {
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

						computersn = res.trim();

					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				} else {
					computersn = etComputerSN.getText().toString();
				}
				
				editor = prePreferences.edit();
				
				editor.putString("operator", etOperator.getText().toString());
				// editor.putString("boardsn", etBoardSN.getText().toString());
				editor.putString("nprocedure", etProcedure.getText().toString());
				editor.putString("boardsn", boardsn);
				editor.putString("computersn", computersn);
				
				boolean b = editor.commit();
				if (b) {
					// Toast.makeText(getActivity(), "保存成功!",
					// Toast.LENGTH_SHORT).show();
				} else {
					Toast.makeText(getActivity(), "保存不成功!", Toast.LENGTH_SHORT)
							.show();
					return;
				}
			}
			
			testItem.setResult(getActivity().getString(R.string.pass));

			Fragment fg = null;
			try {

				fg = MainActivity.getTestItemE(
						MainActivity.itemRealIndexs[pos + 1]).getFragment();

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

	public static void RootCmd(String cmd) {
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
