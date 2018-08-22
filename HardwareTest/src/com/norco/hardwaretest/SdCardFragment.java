package com.norco.hardwaretest;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.math.BigInteger;
import java.security.MessageDigest;

import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class SdCardFragment extends Fragment implements OnClickListener {

	private String basePath = "/mnt/extsd/" + "1kHZ.mp3";
	private String backPath = "/mnt/extsd/" + "1kHZ2.mp3";
	
	private Button btnPass, btnFail;
	
	private TextView tvState1, tvState2, tvState3, tvState4, tvRes;
	File baseFile = null;
	File backFile = null;
	
	boolean successfulMark = false;
	
	private final int FILE_EXISTS = 1001;
	private final int FILE_COPY = 1002;
	private final int FILE_MD5 = 1003;
	private final int FILE_DELETE = 1004;

	private Handler mHandler = new Handler() {

		public void handleMessage(android.os.Message msg) {

			switch (msg.what) {
			case FILE_EXISTS: {
				
				successfulMark = false;
				
				backFile = new File(backPath);
				if (backFile.exists()) {
					backFile.delete();
				}
				
				baseFile = new File(basePath);
				if (baseFile.exists()) {
					tvState1.setText("1." + basePath + ":文件已找到.");
					sendEmptyMessageDelayed(FILE_COPY, 0);
				} else {
					tvState1.setText("1." + basePath + ":文件不存在,请确认!");
				}

			}
				break;
			case FILE_COPY: {

				try {
					int bytesum = 0;
					int byteread = 0;
					
//					System.out.println("baseFile="+baseFile+",backFile="+backFile);
//					
					if (!backFile.exists()) {
						backFile.createNewFile();
						System.out.println("backFile.createNewFile()");
					}
//					
//					System.out.println("backFile:rwx="+backFile.canRead()+backFile.canWrite()+backFile.canExecute());
					
					InputStream inStream = new FileInputStream(baseFile); // 读入原文件
					FileOutputStream fs = new FileOutputStream(backFile);
					byte[] buffer = new byte[1444];
					
					while ((byteread = inStream.read(buffer)) != -1) {
						bytesum += byteread;
						fs.write(buffer, 0, byteread);
					}
					inStream.close();

				} catch (Exception e) {
					System.out.println("复制单个文件操作出错");
					e.printStackTrace();
				}
				
				tvState2.setText("2." + backPath + ":文件复制成功.");
				sendEmptyMessageDelayed(FILE_MD5, 10);
			}
				break;
			case FILE_MD5: {

				String baseMD5 = getFileMD5(baseFile);
				String backMD5 = getFileMD5(backFile);
				if (0 == baseMD5.compareTo(backMD5)) { // 相等
					tvState3.setText("3." + basePath + ":MD5校对成功.");
					
					sendEmptyMessageDelayed(FILE_DELETE, 10);
				} else {
					tvState3.setText("3." + basePath + ":MD5校对失败.");
				}
			}
				break;
			case FILE_DELETE: {
				if (backFile.exists()) {
					backFile.delete();
					tvState4.setText("4." + backPath + ":备份文件已删除.");
					
					tvRes.setText("测试结果:成功");
					successfulMark = true;
					btnPass.setEnabled(true);
				}
			}
				break;
			default:
				break;
			}
		};
	};
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.sdcard_fragment, container,
				false);
		
		btnPass = (Button) rootView.findViewById(R.id.btnPass);//.setOnClickListener(this);
		btnPass.setOnClickListener(this);
		btnFail = (Button) rootView.findViewById(R.id.btnFail); //.setOnClickListener(this);
		btnFail.setOnClickListener(this);
		btnPass.setEnabled(false);
		
		tvState1 = (TextView) rootView.findViewById(R.id.tvState1);
		tvState2 = (TextView) rootView.findViewById(R.id.tvState2);
		tvState3 = (TextView) rootView.findViewById(R.id.tvState3);
		tvState4 = (TextView) rootView.findViewById(R.id.tvState4);
		
		tvRes = (TextView) rootView.findViewById(R.id.tvRes);
		
		return rootView;
	}
	
	@Override
	public void onStart() {
		super.onStart();
		
		mHandler.sendEmptyMessageDelayed(FILE_EXISTS, 50);
	}
	
	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]);

		switch (v.getId()) {
		case R.id.btnPass: {
			
			if (!successfulMark) { // 不成功
				return;
			}
			
			testItem.setResult(getActivity().getString(R.string.pass));

			Fragment fg = null;
			try {

				fg = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos + 1]).getFragment();

			} catch (Exception e) {
				// TODO: handle exception
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

	// File file = new File("/mnt/sdcard/123.txt");
	// Log.e("MD5", getFileMD5(file));

	public static String getFileMD5(File file) {

		if (!file.isFile()) {
			return null;
		}

		MessageDigest digest = null;
		FileInputStream in = null;
		byte buffer[] = new byte[1024];
		int len;

		try {

			digest = MessageDigest.getInstance("MD5");
			in = new FileInputStream(file);

			while ((len = in.read(buffer, 0, 1024)) != -1) {
				digest.update(buffer, 0, len);
			}

			in.close();

		} catch (Exception e) {

			e.printStackTrace();

			return null;
		}

		BigInteger bigInt = new BigInteger(1, digest.digest());

		return bigInt.toString(16);
	}

	/**
	 * 复制单个文件
	 * 
	 * @param oldPath
	 *            String 原文件路径 如：c:/fqf.txt
	 * @param newPath
	 *            String 复制后路径 如：f:/fqf.txt
	 * @return boolean
	 */
	public void copyFile(String oldPath, String newPath) {
		try {
			int bytesum = 0;
			int byteread = 0;
			File oldfile = new File(oldPath);
			if (oldfile.exists()) { // 文件存在时
				InputStream inStream = new FileInputStream(oldPath); // 读入原文件
				FileOutputStream fs = new FileOutputStream(newPath);
				byte[] buffer = new byte[1444];
				int length;
				while ((byteread = inStream.read(buffer)) != -1) {
					bytesum += byteread; // 字节数 文件大小
					System.out.println(bytesum);
					fs.write(buffer, 0, byteread);
				}
				inStream.close();
			}
		} catch (Exception e) {
			System.out.println("复制单个文件操作出错");
			e.printStackTrace();
		}
	}

	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
	}
}
