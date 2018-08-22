package com.norco.burnarm;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

public class SettingsActivity extends FragmentActivity implements
		OnClickListener {

	private ViewPager mPager;

	List<Fragment> lstFragment;

	OneFragment oneFragment;
	TwoFragment twoFragment;
	ThreeFragment threeFragment;
	FourFragment fourFragment;

	Button btnOne, btnTwo, btnThree, btnFour;
	Button btnOK, btnSaveAs, btnCancel;

	ImageView ivCursor;
	int ivWidth = 0;
	int ivPadding = 0;

	// 屏幕宽度
	int screenWidth = 0;
	// 当前选中的项
	int curTab = -1;

	private SharedPreferences prePreferences;
	private Properties prop;

	// private String extPropPath = "/mnt/extsd/BurnARM.properties";
	// private String extPropPath = MainActivity.getExStoragePath() + "/"
	// + "BurnARM.properties";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_settings);

		btnOK = (Button) findViewById(R.id.btnOK);
		btnCancel = (Button) findViewById(R.id.btnCancel);
		btnSaveAs = (Button) findViewById(R.id.btnSaveAs);
		btnOK.setOnClickListener(this);
		btnCancel.setOnClickListener(this);
		btnSaveAs.setOnClickListener(this);

		btnOne = (Button) findViewById(R.id.btnOne);
		btnOne.setOnClickListener(this);
		btnTwo = (Button) findViewById(R.id.btnTwo);
		btnTwo.setOnClickListener(this);
		btnThree = (Button) findViewById(R.id.btnThree);
		btnThree.setOnClickListener(this);
		btnFour = (Button) findViewById(R.id.btnFour);
		btnFour.setOnClickListener(this);

		oneFragment = new OneFragment();
		twoFragment = new TwoFragment();
		threeFragment = new ThreeFragment();
		fourFragment = new FourFragment();

		lstFragment = new ArrayList<Fragment>();
		lstFragment.add(oneFragment);
		lstFragment.add(twoFragment);
		lstFragment.add(threeFragment);
		lstFragment.add(fourFragment);

		mPager = (ViewPager) findViewById(R.id.mPager);
		mPager.setAdapter(new MyFrageStatePagerAdapter(
				getSupportFragmentManager()));

		ivCursor = (ImageView) findViewById(R.id.ivCursor);
		ivWidth = ivCursor.getWidth();
		screenWidth = getResources().getDisplayMetrics().widthPixels;
		ivPadding = (screenWidth / lstFragment.size() - ivWidth) / 2;

	}

	/**
	 * 定义自己的ViewPager适配器。 也可以使用FragmentPagerAdapter。关于这两者之间的区别，可以自己去搜一下。
	 */
	class MyFrageStatePagerAdapter extends FragmentStatePagerAdapter {

		public MyFrageStatePagerAdapter(FragmentManager fm) {
			super(fm);
		}

		@Override
		public Fragment getItem(int position) {
			return lstFragment.get(position);
		}

		@Override
		public int getCount() {
			return lstFragment.size();
		}

		/**
		 * 每次更新完成ViewPager的内容后，调用该接口，此处复写主要是为了让导航按钮上层的覆盖层能够动态的移动
		 */
		@Override
		public void finishUpdate(ViewGroup container) {
			super.finishUpdate(container);// 这句话要放在最前面，否则会报错
			// 获取当前的视图是位于ViewGroup的第几个位置，用来更新对应的覆盖层所在的位置
			int currentItem = mPager.getCurrentItem();
			if (currentItem == curTab) {
				return;
			}

			imageMove(mPager.getCurrentItem());
			curTab = mPager.getCurrentItem();
		}

	}

	/**
	 * 移动覆盖层
	 * 
	 * @param moveToTab
	 *            目标Tab，也就是要移动到的导航选项按钮的位置 第一个导航按钮对应0，第二个对应1，以此类推
	 */
	private void imageMove(int moveToTab) {
		int startPosition = 0;
		int movetoPosition = 0;

		startPosition = curTab * (screenWidth / 4);
		movetoPosition = moveToTab * (screenWidth / 4);

		// 平移动画
		TranslateAnimation translateAnimation = new TranslateAnimation(
				startPosition, movetoPosition, 0, 0);
		translateAnimation.setFillAfter(true);
		translateAnimation.setDuration(200);
		ivCursor.startAnimation(translateAnimation);
	}

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch (v.getId()) {

		case R.id.btnOne: {
			mPager.setCurrentItem(0, true);
		}
			break;
		case R.id.btnTwo: {
			mPager.setCurrentItem(1, true);
		}
			break;
		case R.id.btnThree: {
			mPager.setCurrentItem(2, true);
		}
			break;
		case R.id.btnFour: {
			// mPager.setCurrentItem(3, true);
		}
			break;

		case R.id.btnOK: {

			switch (mPager.getCurrentItem()) {
			case 0: {
				oneFragment.doSaveAs();
			}
				break;
			case 1: {
				twoFragment.doSaveAs();
			}
				break;
			case 2: {
				threeFragment.doSaveAs();
			}
				break;
			case 3: {
				fourFragment.doSaveAs();
			}
				break;
			default:
				break;
			}

			boolean sb = ((BurnApp) getApplicationContext()).doSaveAs();
			if (sb) {
				Toast.makeText(this, "配置已保存至本地:" + BurnApp.BURNARM,
						Toast.LENGTH_SHORT).show();
			} else {
				Toast.makeText(this, "配置保存出错,请确认!", Toast.LENGTH_SHORT).show();
			}
		}
			break;
		case R.id.btnCancel: {
			finish();
		}
			break;
		case R.id.btnSaveAs: {
			boolean sb = ((BurnApp) getApplication()).doSaveAsExt();
			if (sb) {
				Toast.makeText(this, "成功导出配置文件:" + BurnApp.extPropPath,
						Toast.LENGTH_SHORT).show();
			} else {
				Toast.makeText(this, "导出文件错误,请确认!", Toast.LENGTH_SHORT).show();
			}
		}
			break;

		default:
			break;
		}
	}

}
