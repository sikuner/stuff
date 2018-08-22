package com.norco.hardwaretest;

import java.io.File;
import java.io.IOException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class HeadsetLoopFragment extends Fragment implements OnClickListener {

	HeadsetReceiver headsetReceiver;

	Button btnPlay, btnStop;
	Button btnRecStart, btnRecStop, btnPlyStart, btnPlyStop;

	TextView tvHeadsetState, tvMusicState, tvRecState;

	private String mp3FileName = "/mnt/extsd/1kHZ.mp3";
	// private String mp3FileName = "/storage/sdcard1/1kHZ.mp3";
	private String RecFileName = "/mnt/extsd/rec.3gp";
	// private String RecFileName = "/storage/sdcard1/rec.3gp";
	File mp3File = null;

	private MediaPlayer mp = null;
	private MediaPlayer mPlayer = null;
	private MediaRecorder mRecorder = null;

	boolean isExists = false;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {

		System.out.println("HeadsetLoopFragment::onCreateView()");

		View rootView = inflater.inflate(R.layout.headset_loop_fragment,
				container, false);

		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);

		tvHeadsetState = (TextView) rootView.findViewById(R.id.tvHeadsetState);
		tvMusicState = (TextView) rootView.findViewById(R.id.tvMusicState);
		tvRecState = (TextView) rootView.findViewById(R.id.tvRecState);

		btnPlay = (Button) rootView.findViewById(R.id.btnPlay);
		btnStop = (Button) rootView.findViewById(R.id.btnStop);
		btnPlay.setOnClickListener(this);
		btnStop.setOnClickListener(this);
		btnPlay.setEnabled(true);
		btnStop.setEnabled(false);
		
		btnRecStart = (Button) rootView.findViewById(R.id.btnRecStart);
		btnRecStop = (Button) rootView.findViewById(R.id.btnRecStop);
		btnPlyStart = (Button) rootView.findViewById(R.id.btnPlyStart);
		btnPlyStop = (Button) rootView.findViewById(R.id.btnPlyStop);
		btnRecStart.setOnClickListener(this);
		btnRecStop.setOnClickListener(this);
		btnPlyStart.setOnClickListener(this);
		btnPlyStop.setOnClickListener(this);
		btnRecStart.setEnabled(true);
		btnRecStop.setEnabled(false);
		btnPlyStart.setEnabled(false);
		btnPlyStop.setEnabled(false);
		
		// ���㲥����Ӧ�Ĺ�����
		IntentFilter intentFilter = new IntentFilter();
		intentFilter.addAction("android.intent.action.HEADSET_PLUG");
		headsetReceiver = new HeadsetReceiver();
		getActivity().registerReceiver(headsetReceiver, intentFilter);

		return rootView;
	}

	@Override
	public void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
		
		File file1 = new File("/mnt/extsd/1kHZ.mp3");
		if (file1.exists()) {
			mp3FileName = "/mnt/extsd/1kHZ.mp3";
			RecFileName = "/mnt/extsd/rec.3gp";
			isExists = true;
		} else {
			File file2 = new File("/mnt/satadisk/1kHZ.mp3");
			if (file2.exists()) {
				mp3FileName = "/mnt/satadisk/1kHZ.mp3";
				RecFileName = "/mnt/satadisk/rec.3gp";
				isExists = true;
			} else {
				tvMusicState.setText("��Ƶ�ļ�(1kHZ.mp3)������,��ȷ��!");
				isExists = false;
			}

		}
	}
	
	@Override
	public void onPause() {
		// TODO Auto-generated method stub
		super.onPause();
	}

	// �Լ�����Ĺ㲥������
	public class HeadsetReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent.hasExtra("state")) {
				if (0 == intent.getIntExtra("state", 0)) {

					tvHeadsetState.setText("δ����");

				} else if (1 == intent.getIntExtra("state", 0)) {

					tvHeadsetState.setText("�Ѳ���");

				}
			}
		}
	}

	@Override
	public void onDestroyView() {
		// TODO Auto-generated method stub
		super.onDestroyView();
		// System.out.println("HeadsetLoopFragment::onDestroyView()");

		getActivity().unregisterReceiver(headsetReceiver);

		try {
			if (mp != null) {
				mp.stop();
				mp.release();
				mp = null;
			}

			if (mPlayer != null) {
				mPlayer.release();
				mPlayer = null;
			}

			if (mRecorder != null) {

				mRecorder.stop();
				mRecorder.release();
				mRecorder = null;

			}

		} catch (Exception e) {
			tvMusicState.setText("onDestroyView �����쳣...");
			tvRecState.setText("onDestroyView �����쳣...");
			e.printStackTrace();
		}
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

		case R.id.btnPlay: {
			
			if (!isExists) { // ��Ƶ������
				
				return;
			}
			
			if (mPlayer != null) {
				mPlayer.release();
				mPlayer = null;
			}
			
			if (mRecorder != null) {
				mRecorder.stop();
				mRecorder.release();
				mRecorder = null;
			}
			btnRecStart.setEnabled(true);
			btnRecStop.setEnabled(false);
			btnPlyStart.setEnabled(false);
			btnPlyStop.setEnabled(false);
			
			if (mp == null) {
				
				mp = MediaPlayer.create(getActivity(),
						Uri.fromFile(new File(mp3FileName)));
				
				/* ��MediaPlayer.OnCompletionLister�����е�Listener */
				mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
					// @Override
					/* �����ļ���������¼� */
					public void onCompletion(MediaPlayer arg0) {
						try {
							/*
							 * �����Դ��MediaPlayer�ĸ�ֵ��ϵ ����Դ����Ϊ������������
							 */
							// mp.release();
							/* �ı�TextViewΪ���Ž��� */
							tvMusicState.setText("���ֲ��Ž���!");
							
							btnPlay.setEnabled(true);
							btnStop.setEnabled(false);

						} catch (Exception e) {
							tvMusicState.setText(e.toString());
							e.printStackTrace();
						}
					}
				});
				
				/* ��MediaPlayer.OnErrorListener�����е�Listener */
				mp.setOnErrorListener(new MediaPlayer.OnErrorListener() {
					@Override
					/* ���Ǵ������¼� */
					public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
						// TODO Auto-generated method stub
						try {
							/* ��������ʱҲ�����Դ��MediaPlayer�ĸ�ֵ */
							mp.release();
							tvMusicState.setText("���ŷ����쳣!");
						} catch (Exception e) {
							tvMusicState.setText(e.toString());
							e.printStackTrace();
						}
						return false;
					}
				});
			}

			try {
				if (mp != null) {
					mp.stop();
				}

				mp.prepare();
				mp.start();

				tvMusicState.setText("���ֲ�����...");

				btnPlay.setEnabled(false);
				btnStop.setEnabled(true);

			} catch (Exception e) {
				tvMusicState.setText("���ŷ����쳣...");
				e.printStackTrace();
			}
		}
			break;
		case R.id.btnStop: {

			try {
				if (mp != null) {
					mp.stop();
					mp.release();
					mp = null;
					
					tvMusicState.setText("���ֲ���ֹͣ, �ڴ��ͷ�");
					
					btnPlay.setEnabled(true);
					btnStop.setEnabled(false);
				}
			} catch (Exception e) {
				tvMusicState.setText("����ֹͣ�����쳣...");
				e.printStackTrace();
			}
		}
			break;

		case R.id.btnRecStart: {
			
			if (!isExists) {
				tvRecState.setText("¼���ļ�(rec.3gp)������, ��ȷ�Ͻ���SD��SATA!");
				return;
			}
			
			if (mp != null) {
				mp.stop();
				mp.release();
				mp = null;
				
				tvMusicState.setText("���ֲ���ֹͣ, �ڴ��ͷ�");
				
				btnPlay.setEnabled(true);
				btnStop.setEnabled(false);
			}
			
			if (mRecorder != null) {

				mRecorder.stop();
				mRecorder.release();
				mRecorder = null;
				tvRecState.setText("¼���ѽ���");

			}

			mRecorder = new MediaRecorder();
			mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
			mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
			mRecorder.setOutputFile(RecFileName);
			mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
			try {
				mRecorder.prepare();

			} catch (IOException e) {
				tvRecState.setText("prepare() failed");
			}
			mRecorder.start();
			tvRecState.setText("¼����...");

			btnRecStart.setEnabled(false);
			btnRecStop.setEnabled(true);
			btnPlyStart.setEnabled(false);
			btnPlyStop.setEnabled(false);
		}
			break;
		case R.id.btnRecStop: {

			if (mRecorder != null) {

				mRecorder.stop();
				mRecorder.release();
				mRecorder = null;
				tvRecState.setText("¼���ѽ���");

				btnRecStart.setEnabled(false);
				btnRecStop.setEnabled(false);
				btnPlyStart.setEnabled(true);
				btnPlyStop.setEnabled(false);
			}
		}
			break;
		case R.id.btnPlyStart: {

			if (mp != null) {
				mp.stop();
				mp.release();
				mp = null;

				btnPlay.setEnabled(true);
				btnStop.setEnabled(false);
			}

			if (mPlayer != null) {
				mPlayer.release();
				mPlayer = null;
			}

			mPlayer = new MediaPlayer();
			try {
				mPlayer.setDataSource(RecFileName);
				mPlayer.prepare();
				mPlayer.start();

				tvRecState.setText("¼��������...");

				btnRecStart.setEnabled(false);
				btnRecStop.setEnabled(false);
				btnPlyStart.setEnabled(false);
				btnPlyStop.setEnabled(true);

			} catch (IOException e) {
				
				tvRecState.setText("����ʧ��");
			}
		}
			break;
		case R.id.btnPlyStop: {

			if (mPlayer != null) {
				mPlayer.release();
				mPlayer = null;
				tvRecState.setText("¼�������ѽ���");

				btnRecStart.setEnabled(true);
				btnRecStop.setEnabled(false);
				btnPlyStart.setEnabled(false);
				btnPlyStop.setEnabled(false);
			}

		}
			break;

		default:

			break;
		}

	}

}
