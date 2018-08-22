package com.norco.hardwaretest;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ScreenDisplayFragment extends Fragment implements OnTouchListener, OnClickListener {

	View rootView = null;
	int color_counter = 0;
	TextView tvTitle;
	TextView tvTips;
	LinearLayout btnsLayout;
	TextView tvColorName;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		
		rootView = inflater.inflate(R.layout.screen_display_fragment, container, false);
		
		tvTitle = (TextView) rootView.findViewById(R.id.tvTitle);
		tvTitle.setVisibility(View.INVISIBLE);
		
		tvTips = (TextView) rootView.findViewById(R.id.tvTips);
		tvTips.setGravity(Gravity.CENTER_VERTICAL);
		tvTips.setText(R.string.screen_display_click_tips);
		
		btnsLayout = (LinearLayout) rootView.findViewById(R.id.btnsLayout);
		btnsLayout.setVisibility(View.INVISIBLE);
		
		rootView.setBackgroundColor(Color.RED);
		rootView.setOnTouchListener(this);
		
        rootView.findViewById(R.id.btnPass).setOnClickListener(this);
        rootView.findViewById(R.id.btnFail).setOnClickListener(this);
        
        tvColorName = (TextView) rootView.findViewById(R.id.tvColorName);
        tvColorName.setText("红色");
        tvColorName.setVisibility(View.VISIBLE);
        
		return rootView;
	}
	
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		
		int color = Color.WHITE;
		String colorName = "";
		
		switch (color_counter) {
		case 0:
			color = Color.GREEN;
			colorName = "绿色";
			break;
		case 1:
			color = Color.BLUE;
			colorName = "蓝色";
			break;
		case 2:
			color = Color.WHITE;
			colorName = "白色";
			break;
		case 3:
			color = Color.BLACK;
			colorName = "黑色";
			break;
		case 4:
			color = Color.GRAY;
			colorName = "灰色";
			break;
		default:
			{
				color_counter = 0;
				rootView.setBackgroundColor(Color.WHITE);
				tvTitle.setVisibility(View.VISIBLE);
				tvTips.setGravity(Gravity.TOP);
				tvTips.setText(R.string.screen_display_tips);
				btnsLayout.setVisibility(View.VISIBLE);
				rootView.setOnTouchListener(null);
				tvColorName.setVisibility(View.INVISIBLE);
				
				return false;
			}
		}
		
		color_counter++;
		rootView.setBackgroundColor(color);
		tvColorName.setText(colorName);
		
		return false;
	}
	
	@Override
	public void onClick(View v) {
		
		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {
		case R.id.btnPass:
			{
				testItem.setResult(getActivity().getString(R.string.pass));
				
				Fragment fg = null;
				try {
					
					fg = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos + 1]).getFragment();
					
				} catch (Exception e) {
					// TODO: handle exception
					System.out.println("ItemFragment:"+e.toString());
					fg = null;
				}
				
				if (fg != null) {
					
					Bundle b = new Bundle();
					b.putInt("position", pos + 1);
					fg.setArguments(b);
					
					getFragmentManager()
					.beginTransaction()
					.replace(R.id.fragment_container, fg)
					.commit();
				} else {
					getActivity().finish();
				}
			}
			break;
		case R.id.btnFail:
			{
				testItem.setResult(getActivity().getString(R.string.fail));
				getActivity().finish();
			}
		break;
		default:
			
			break;
		}
	}	
	
}
