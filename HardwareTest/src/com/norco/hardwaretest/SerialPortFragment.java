package com.norco.hardwaretest;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.norco.utils.SerialPort;

// ttymxc0-4     	ttymxc*
// ttyHBA1-4 		ttyHB*
// ttyHBB1-4 		ls /dev/ttymxc[0-9] /dev/ttyHB[A-Z][0-9] 2>/dev/null

public class SerialPortFragment extends Fragment implements OnClickListener,
		OnItemSelectedListener, TextWatcher {

	// private String COM_NAMES_CMD =
	// "ls /dev/ttymxc[0-9] /dev/ttyHB[A-Z][0-9] 2>/dev/null";
	private String COM_NAMES_CMD = "/system/bin/toolbox ls /dev/ttyS[0-9]";

	private Button btnPass, btnFail;

	int comIDs[] = { R.id.tvCom0, R.id.tvCom10, R.id.tvCom1, R.id.tvCom11,
			R.id.tvCom2, R.id.tvCom12, R.id.tvCom3, R.id.tvCom13, R.id.tvCom4,
			R.id.tvCom14, R.id.tvCom5, R.id.tvCom15, R.id.tvCom6, R.id.tvCom16,
			R.id.tvCom7, R.id.tvCom17 };

	int resIDs[] = { R.id.tvRes0, R.id.tvRes10, R.id.tvRes1, R.id.tvRes11,
			R.id.tvRes2, R.id.tvRes12, R.id.tvRes3, R.id.tvRes13, R.id.tvRes4,
			R.id.tvRes14, R.id.tvRes5, R.id.tvRes15, R.id.tvRes6, R.id.tvRes16,
			R.id.tvRes7, R.id.tvRes17 };

	private View rootView;
	private TextView tvComNames;

	private int currentSerialPortPosition = 0;

	List<String> lsComs;
	String TAG = "SerialPortFragment";

	private ListView lvComTests;
	private SimpleAdapter adapter;

	private List<Map<String, ?>> lst;

	Spinner spComs;

	Button btnStart, btnStop;

	final Integer LOOPBACK_SUM = new Integer(1000);
	final Integer MISSING_SUM = new Integer(10); // (mOutgoing - mIncoming) >=
													// MISSING_SUM 就是[失败]

	protected SerialPort mSerialPort;
	protected OutputStream mOutputStream;
	private InputStream mInputStream;

	private SendingThread mSendingThread;
	private ReadThread mReadThread;
	byte mValueToSend;
	boolean mByteReceivedBack;
	Object mByteReceivedBackSemaphore = new Object();
	Integer mIncoming = new Integer(0);
	Integer mOutgoing = new Integer(0);
	Integer mLost = new Integer(0);
	Integer mCorrupted = new Integer(0);

	TextView tvSend;
	TextView tvRecv;
	TextView tvLost;
	TextView tvCorrupted;
	TextView tvResult;

	private CheckBox cbMulCom;
	private boolean isMulCom = true;

	boolean mSendingThread_running = false;
	boolean mReadThread_running = false;
	boolean isFirst = true;

	private class ReadThread extends Thread {

		@Override
		public void run() {
			super.run();
			while (!isInterrupted() && mReadThread_running) {
				int size;
				try {
					byte[] buffer = new byte[64];
					if (mInputStream == null)
						return;
					size = mInputStream.read(buffer);
					if (size > 0) {
						onDataReceived(buffer, size);
					}
				} catch (IOException e) {
					e.printStackTrace();
					return;
				}
			}

		}
	}

	protected void onDataReceived(byte[] buffer, int size) {

		synchronized (mByteReceivedBackSemaphore) {
			int i;
			for (i = 0; i < size; i++) {
				if ((buffer[i] == mValueToSend) && (mByteReceivedBack == false)) {
					mValueToSend++;
					// This byte was expected
					// Wake-up the sending thread
					mByteReceivedBack = true;
					mByteReceivedBackSemaphore.notify();

				} else {
					// The byte was not expected
					mCorrupted++;
				}
			}
		}
	}

	private class SendingThread extends Thread {
		@Override
		public void run() {
			while (!isInterrupted() && mSendingThread_running) {

				synchronized (mByteReceivedBackSemaphore) {
					mByteReceivedBack = false;
					try {
						if (mOutputStream != null) {
							mOutputStream.write(mValueToSend);
						} else {
							return;
						}
					} catch (IOException e) {
						e.printStackTrace();
						return;
					}
					mOutgoing++;

					// Wait for 100ms before sending next byte, or as soon as
					// the sent byte has been read back.
					try {
						mByteReceivedBackSemaphore.wait(100);
						if (mByteReceivedBack == true) {
							// Byte has been received
							mIncoming++;
						} else {
							// Timeout
							mLost++;
						}

						getActivity().runOnUiThread(new Runnable() {
							public void run() {

								tvSend.setText(mOutgoing.toString());
								tvRecv.setText(mIncoming.toString());
								tvLost.setText(mLost.toString());
								tvCorrupted.setText(mCorrupted.toString());

								if ((mOutgoing > LOOPBACK_SUM)
										|| ((mOutgoing - mIncoming) >= MISSING_SUM)) {
									String str = mOutgoing.equals(mIncoming) ? "成功"
											: "失败";
									tvResult.setText(str);
									((TextView) rootView
											.findViewById(resIDs[currentSerialPortPosition]))
											.setText(str);
								}
							}
						});

					} catch (InterruptedException e) {

					}
				}

				if ((mOutgoing > LOOPBACK_SUM)
						|| ((mOutgoing - mIncoming) >= MISSING_SUM)) {
					mSendingThread.interrupt();
					mReadThread.interrupt();
					mSendingThread_running = false;
					mReadThread_running = false;

					if (isMulCom) {
						mHandler.sendEmptyMessageDelayed(COM_NEXT, 0);
					}
				}
			}
		}
	}

	private final int COM_START = 1001;
	private final int COM_NEXT = 1002;
	private final int COM_STOP = 1003;
	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case COM_START: {

				if (!isFirst) { // 不是第一次
					if (mSendingThread_running || mReadThread_running) {
						String str = "mReadThread.isAlive():"
								+ mReadThread.isAlive()
								+ ",mSendingThread.isAlive():"
								+ mSendingThread.isAlive();
//						Toast.makeText(getActivity(), str, Toast.LENGTH_SHORT)
//								.show();
						return;
					}
				} else {
					isFirst = false;
				}

				mIncoming = 0;
				mOutgoing = 0;
				mLost = 0;
				mCorrupted = 0;
				mSendingThread_running = true;
				mReadThread_running = true;

				tvSend.setText("0");
				tvRecv.setText("0");
				tvLost.setText("0");
				tvCorrupted.setText("0");
				tvResult.setText("NULL");

				currentSerialPortPosition = spComs.getSelectedItemPosition();
//				curComPath = toComPath(spComs.getSelectedItem().toString());
//				System.out.println("spComs : " + toComPath(spComs.getSelectedItem().toString()));
				
				try {
					System.out.println("toComPath:"
							+ toComPath(lsComs.get(currentSerialPortPosition)));
					File comFile = new File(
							toComPath(lsComs.get(currentSerialPortPosition)));
					mSerialPort = new SerialPort(comFile, 115200, 0);

					mOutputStream = mSerialPort.getOutputStream();
					mInputStream = mSerialPort.getInputStream();

					/* Create a receiving thread */
					mReadThread = new ReadThread();
					mReadThread.start();

				} catch (SecurityException e) {
					Toast.makeText(getActivity(), "R.string.error_security",
							Toast.LENGTH_SHORT).show();
				} catch (IOException e) {
					Toast.makeText(getActivity(), "R.string.error_unknown",
							Toast.LENGTH_SHORT).show();
				} catch (InvalidParameterException e) {
					Toast.makeText(getActivity(),
							"R.string.error_configuration", Toast.LENGTH_SHORT)
							.show();
				}

				if (mSerialPort != null) {
					mSendingThread = new SendingThread();
					mSendingThread.start();
				}

			}
				break;

			case COM_NEXT: {
				if (spComs.getSelectedItemPosition() + 1 < spComs.getCount()) {
					
					spComs.setSelection(spComs.getSelectedItemPosition() + 1, true);
					sendEmptyMessageDelayed(COM_START, 500);
					
				} else {
					return; // 已执行完毕
				}
			}
				break;

			case COM_STOP: {
				String str = mOutgoing.equals(mIncoming) ? "成功" : "失败";
				tvResult.setText(str);
				((TextView) rootView.findViewById(resIDs[currentSerialPortPosition])).setText(str);

				mSendingThread.interrupt();
				mReadThread.interrupt();
				mSendingThread_running = false;
				mReadThread_running = false;
			}
				break;

			default:
				break;
			}
		}
	};

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		rootView = inflater.inflate(R.layout.serialport_fragment, container,
				false);

		cbMulCom = (CheckBox) rootView.findViewById(R.id.cbMulCom);
		cbMulCom.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {

				isMulCom = ((CheckBox) v).isChecked();

			}
		});

		btnPass = (Button) rootView.findViewById(R.id.btnPass);
		btnPass.setOnClickListener(this);
		btnFail = (Button) rootView.findViewById(R.id.btnFail);
		btnFail.setOnClickListener(this);
		btnPass.setEnabled(false);

		tvSend = (TextView) rootView.findViewById(R.id.tvSend);
		tvRecv = (TextView) rootView.findViewById(R.id.tvRecv);
		tvLost = (TextView) rootView.findViewById(R.id.tvLost);
		tvCorrupted = (TextView) rootView.findViewById(R.id.tvCorrupted);
		tvResult = (TextView) rootView.findViewById(R.id.tvResult);

		spComs = (Spinner) rootView.findViewById(R.id.spComs);
		btnStart = (Button) rootView.findViewById(R.id.btnStart);
		btnStop = (Button) rootView.findViewById(R.id.btnStop);
		btnStart.setOnClickListener(this);
		btnStop.setOnClickListener(this);
		lsComs = new ArrayList<String>();
		getComNames(lsComs);

		for (int i = 0; i < lsComs.size() && i < comIDs.length; i++) {
			TextView tvCom = (TextView) rootView.findViewById(comIDs[i]);
			TextView tvRes = (TextView) rootView.findViewById(resIDs[i]);
			tvRes.addTextChangedListener(this);
			tvCom.setText(lsComs.get(i));
		}

		ArrayAdapter<String> spAdapter = new ArrayAdapter<String>(
				getActivity(), android.R.layout.simple_spinner_item, lsComs);

		spComs.setAdapter(spAdapter);
		spComs.setOnItemSelectedListener(this);

		return rootView;
	}

	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity
				.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {
		case R.id.btnPass: {
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

		case R.id.btnStart: {
			// if (!isFirst) { // 不是第一次
			// if (mSendingThread_running || mReadThread_running) {
			// String str = "mReadThread.isAlive():"
			// + mReadThread.isAlive()
			// + ",mSendingThread.isAlive():"
			// + mSendingThread.isAlive();
			// Toast.makeText(getActivity(), str, Toast.LENGTH_SHORT)
			// .show();
			// return;
			// }
			// } else {
			// isFirst = false;
			// }
			//
			// mIncoming = 0;
			// mOutgoing = 0;
			// mLost = 0;
			// mCorrupted = 0;
			// mSendingThread_running = true;
			// mReadThread_running = true;
			//
			// // tvSend.setText("0");
			// // tvRecv.setText("0");
			// // tvLost.setText("0");
			// // tvCorrupted.setText("0");
			// // tvResult.setText("NULL");
			//
			// try {
			// System.out.println("toComPath:"
			// + toComPath(lsComs.get(currentSerialPortPosition)));
			// File comFile = new File(
			// toComPath(lsComs.get(currentSerialPortPosition)));
			// mSerialPort = new SerialPort(comFile, 115200, 0);
			//
			// mOutputStream = mSerialPort.getOutputStream();
			// mInputStream = mSerialPort.getInputStream();
			//
			// /* Create a receiving thread */
			// mReadThread = new ReadThread();
			// mReadThread.start();
			//
			// } catch (SecurityException e) {
			// Toast.makeText(getActivity(), "R.string.error_security",
			// Toast.LENGTH_SHORT).show();
			// } catch (IOException e) {
			// Toast.makeText(getActivity(), "R.string.error_unknown",
			// Toast.LENGTH_SHORT).show();
			// } catch (InvalidParameterException e) {
			// Toast.makeText(getActivity(), "R.string.error_configuration",
			// Toast.LENGTH_SHORT).show();
			// }
			//
			// if (mSerialPort != null) {
			// mSendingThread = new SendingThread();
			// mSendingThread.start();
			// }

			mHandler.sendEmptyMessageDelayed(COM_START, 200);

		}
			break;
		case R.id.btnStop: {

			// String str = mOutgoing.equals(mIncoming) ? "成功" : "失败";
			// tvResult.setText(str);
			// ((TextView) rootView
			// .findViewById(resIDs[currentSerialPortPosition]))
			// .setText(str);
			//
			// mSendingThread.interrupt();
			// mReadThread.interrupt();
			// mSendingThread_running = false;
			// mReadThread_running = false;

			mHandler.sendEmptyMessageDelayed(COM_STOP, 200);
		}
			break;

		default:

			break;
		}
	}

	private int getComNames(List<String> coms) {

		File file = new File("/dev");
		if (file.isDirectory()) {
			File[] files = file.listFiles();
			for (int i = files.length - 1; i >= 0; i--) {
				String absPath = files[i].getAbsolutePath();
				if (absPath.contains("/dev/ttymxc")
				// || absPath.contains("/dev/ttyHB")
				// || absPath.contains("/dev/ttyS")
				) {
					// System.out.println("COM_NAME:" + absPath);
					if (!absPath.contains("/dev/ttymxc0")) // 除掉"/dev/ttymxc1"
					{
						coms.add(toComName(absPath));
					}
				}
			}
		}

		return 0;
	}

	@Override
	public void onItemSelected(AdapterView<?> parent, View view, int position,
			long id) {

		currentSerialPortPosition = position;
//		Toast.makeText(getActivity(), lsComs.get(currentSerialPortPosition),
//				Toast.LENGTH_SHORT).show();
		parent.setVisibility(View.VISIBLE);

		tvSend.setText("0");
		tvRecv.setText("0");
		tvLost.setText("0");
		tvCorrupted.setText("0");
		tvResult.setText("NULL");
	}

	String[] comPaths = { "/dev/ttymxc0", "/dev/ttymxc1", "/dev/ttymxc2",
			"/dev/ttymxc3", "/dev/ttymxc4", "/dev/ttyHBA0", "/dev/ttyHBA1",
			"/dev/ttyHBA2", "/dev/ttyHBA3", "/dev/ttyHBB0", "/dev/ttyHBB1",
			"/dev/ttyHBB2", "/dev/ttyHBB3" };
	String[] comNames = { "COM1", "COM2", "COM3", "COM4", "COM5", "COM6",
			"COM7", "COM8", "COM9", "COM10", "COM11", "COM12", "COM13" };

	String toComName(String comPath) {
		for (int i = 0; i < comPaths.length; i++) {
			if (0 == comPath.compareTo(comPaths[i])) {
				return comNames[i];
			}
		}

		return "NULL";
	}

	String toComPath(String comName) {
		for (int i = 0; i < comPaths.length; i++) {
			if (0 == comName.compareTo(comNames[i])) {
				return comPaths[i];
			}
		}

		return "NULL";
	}

	@Override
	public void onNothingSelected(AdapterView<?> parent) {

	}

	@Override
	public void beforeTextChanged(CharSequence s, int start, int count,
			int after) {
		// TODO Auto-generated method stub

	}

	@Override
	public void onTextChanged(CharSequence s, int start, int before, int count) {
		// TODO Auto-generated method stub

	}

	@Override
	public void afterTextChanged(Editable s) {
		// TODO Auto-generated method stub

		int count_success = 0;
		for (int i = 0; i < lsComs.size() && i < comIDs.length; i++) {
			TextView tvRes = (TextView) rootView.findViewById(resIDs[i]);
			if (tvRes.getText().toString().equals("成功")) {
				count_success++;
			}
		}

		if (count_success == lsComs.size()) {
//			Toast.makeText(getActivity(), "全部成功:" + count_success,
//					Toast.LENGTH_SHORT).show();
			btnPass.setEnabled(true);
		}
	}

}
