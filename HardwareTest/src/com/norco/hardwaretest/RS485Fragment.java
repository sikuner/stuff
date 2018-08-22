package com.norco.hardwaretest;

import java.io.FileDescriptor;
import java.io.InputStream;
import java.io.OutputStream;

import com.norco.utils.SerailPortOpt;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ToggleButton;

public class RS485Fragment extends Fragment implements OnClickListener,
		OnCheckedChangeListener {

	private View rootView;

	private static final String TAG = "RS485Fragment";
	private SerailPortOpt serialPort;

	protected OutputStream mOutputStream;
	private InputStream mInputStream;
	private ReadThread mReadThread;
	private SendThread mSendThread;

	private Button btnSend, btnClear;
	private ToggleButton tbRS485, tbRecvStop;
	private EditText etRecvMsg, etSendMsg;

	private class ReadThread extends Thread {
		byte[] buf = new byte[512];

		@Override
		public void run() {
			super.run();
			Log.i(TAG, "ReadThread==>buffer:" + buf.length);
			while (!isInterrupted()) {
				int size;
				if (mInputStream == null)
					return;
				size = serialPort.read485Bytes(buf);

				if (size > 0) {
					onDataReceived(buf, size);
					Log.i(TAG, "ReadThread==>" + size);
				}
			}
		}
	}

	public static String bytesToHexString(byte[] src, int size) {
		String ret = "";
		if (src == null || size <= 0) {
			return null;
		}
		for (int i = 0; i < size; i++) {
			String hex = Integer.toHexString(src[i] & 0xFF);
			Log.i(TAG, hex);
			if (hex.length() < 2) {
				hex = "0" + hex;
			}
			hex += " ";
			ret += hex;
		}
		return ret.toUpperCase();
	}
	
	protected void onDataReceived(final byte[] buf, final int size) {
		getActivity().runOnUiThread(new Runnable() {
			public void run() {
				if (etRecvMsg != null) {
					etRecvMsg.append(bytesToHexString(buf, size));
//					etRecvMsg.append(new String(buf, 0, size));
//					Log.i("etRecvMsg", new String(buf, 0, size));
				}
			}
		});
	}
	
	private class SendThread extends Thread {

		@Override
		public void run() {
			// TODO Auto-generated method stub
			Log.i(TAG, "SendThread==>run");
			super.run();
			SendMsg();
		}
	}
	/**
	 * 将两个ASCII字符合成一个字节； 如："EF"--> 0xEF
	 * 
	 * @param src0
	 *            byte
	 * @param src1
	 *            byte
	 * @return byte
	 */
	public static byte uniteBytes(byte src0, byte src1) {
		byte _b0 = Byte.decode("0x" + new String(new byte[] { src0 }))
				.byteValue();
		_b0 = (byte) (_b0 << 4);
		byte _b1 = Byte.decode("0x" + new String(new byte[] { src1 }))
				.byteValue();
		byte ret = (byte) (_b0 ^ _b1);
		return ret;
	}

	/**
	 * 将指定字符串src，以每两个字符分割转换为16进制形式 如："2B44EFD9" --> byte[]{0x2B, 0x44, 0xEF,
	 * 0xD9}
	 * 
	 * @param src
	 *            String
	 * @return byte[]
	 */
	public static byte[] HexString2Bytes(String src) {
		byte[] ret = new byte[src.length() / 2];
		byte[] tmp = src.getBytes();
		for (int i = 0; i < tmp.length / 2; i++) {
			ret[i] = uniteBytes(tmp[i * 2], tmp[i * 2 + 1]);
		}
		return ret;
	}
	
	private void SendMsg() {
		String src = etSendMsg.getText().toString();
		if (null != serialPort.mFd) {
			Log.i(TAG, src);
//			serialPort.writeBytes(src.getBytes());
			serialPort.write485Bytes(HexString2Bytes(src));
			System.out.println("SendMsg:" + HexString2Bytes(src));
		}
	}
	
	private void openSerialPort() {

		if (serialPort.mFd == null) {
			FileDescriptor fd = serialPort.open485Dev(serialPort.mDevNum);
			if (fd == null) {
				Toast.makeText(getActivity(), "SerialPort can't open", Toast.LENGTH_SHORT).show();
				return;
			}
			
			Log.i("uart port operate", "Mainactivity.java==>uart open");
			serialPort.setSpeed(serialPort.mFd, serialPort.mSpeed);
			Log.i("uart port operate", "Mainactivity.java==>uart set speed..."
					+ serialPort.mSpeed);
			serialPort.setParity(serialPort.mFd, serialPort.mDataBits,
					serialPort.mStopBits, serialPort.mParity);
			Log.i("uart port operate",
					"Mainactivity.java==>uart other params..."
							+ serialPort.mDataBits + "..."
							+ serialPort.mStopBits + "..." + serialPort.mParity);

			mInputStream = serialPort.getInputStream();
			mOutputStream = serialPort.getOutputStream();
		}
	}

	private void closeSerialPort() {

		if (mReadThread != null) {
			mReadThread.interrupt();
			// mReadThread = null;
		}

		if (serialPort.mFd != null) {
			Log.i("uart port operate", "Mainactivity.java==>uart stop");
			serialPort.close485Dev(serialPort.mFd);
			Log.i("uart port operate", "Mainactivity.java==>uart stoped");
		}
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		
		rootView = inflater.inflate(R.layout.rs485_fragment, container, false);
		
		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);
		btnSend = (Button) rootView.findViewById(R.id.btnSend);
		btnSend.setOnClickListener(this);
		btnClear = (Button) rootView.findViewById(R.id.btnClear);
		btnClear.setOnClickListener(this);
		
		tbRS485 = (ToggleButton) rootView.findViewById(R.id.tbRS485);
		tbRS485.setOnCheckedChangeListener(this);
		tbRecvStop = (ToggleButton) rootView.findViewById(R.id.tbRecvStop);
		tbRecvStop.setOnCheckedChangeListener(this);
		etRecvMsg = (EditText) rootView.findViewById(R.id.etRecvMsg);
		etSendMsg = (EditText) rootView.findViewById(R.id.etSendMsg);
		
		serialPort = new SerailPortOpt();
		serialPort.mDevNum = 1;
		serialPort.mSpeed = 115200;
		serialPort.mDataBits = 8;
		serialPort.mStopBits = 1;
		serialPort.mParity = 'n';
		
		return rootView;
	}

	@Override
	public void onDestroyView() {

		closeSerialPort();
		serialPort = null;
		System.out.println("OFF--OFF");
		super.onDestroyView();
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

		case R.id.btnSend: {
			if (serialPort.mFd == null) {
				Toast.makeText(getActivity(), "SerialPort can't open", Toast.LENGTH_SHORT).show();
				return;
			}
			
			mSendThread = new SendThread();
			mSendThread.start();
		}
			break;
		case R.id.btnClear: {
			etRecvMsg.setText("");
		}
			break;

		default:
			
			break;
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

		if ((ToggleButton) buttonView == tbRS485) {
			if (isChecked) {
				openSerialPort();
				if (serialPort.mFd == null) {
					tbRS485.setChecked(false);
				}
			} else {
				closeSerialPort();
			}
		} else if ((ToggleButton) buttonView == tbRecvStop) {
			
			if (serialPort.mFd == null) {
				Toast.makeText(getActivity(), "SerialPort can't open", Toast.LENGTH_SHORT).show();
				tbRecvStop.setChecked(false);
				return;
			}
			
			if (isChecked) {
				Log.i("uart port operate",
						"Mainactivity.java==>start ReadThread");
				mReadThread = new ReadThread();
				mReadThread.start();
				Log.i("uart port operate",
						"Mainactivity.java==>ReadThread started");
				btnSend.setEnabled(false);
			} else {

				Log.i(TAG, "togBtnReciveDate interrupt in");
				if (mReadThread != null) {
					Log.i(TAG, "togBtnReciveDate interrupt mReadThread != null");
					mReadThread.interrupt();
					btnSend.setEnabled(true);
				}
			}
		}
	}

}
