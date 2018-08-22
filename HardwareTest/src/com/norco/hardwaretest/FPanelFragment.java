package com.norco.hardwaretest;

import java.util.ArrayList;
import java.util.List;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

public class FPanelFragment extends Fragment implements OnClickListener {
	
	Spinner spDisplay;
	
	List<String> lsDsp;
	
	Switch swHddLed, swNetLed, swPowerLed, swPowerBtn, swResetBtn;
	TextView tvFpanelAllRes;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fpanel_fragment, container,
				false);
		
		rootView.findViewById(R.id.btnPass).setOnClickListener(this);
		rootView.findViewById(R.id.btnFail).setOnClickListener(this);
		spDisplay = (Spinner) rootView.findViewById(R.id.spDisplay);
		spDisplay.setPrompt("屏幕数量是?");
		lsDsp = new ArrayList<String>();
		lsDsp.add("1");
		lsDsp.add("2");
		lsDsp.add("3");
		lsDsp.add("4");
		ArrayAdapter<String> spAdapter = new ArrayAdapter<String>(
				getActivity(), android.R.layout.simple_spinner_item, lsDsp);
		spDisplay.setAdapter(spAdapter);
		
		swHddLed = (Switch) rootView.findViewById(R.id.swHddLed);
		swNetLed = (Switch) rootView.findViewById(R.id.swNetLed);
		swPowerLed = (Switch) rootView.findViewById(R.id.swPowerLed);
		swPowerBtn = (Switch) rootView.findViewById(R.id.swPowerBtn);
		swResetBtn = (Switch) rootView.findViewById(R.id.swResetBtn);
		tvFpanelAllRes = (TextView) rootView.findViewById(R.id.tvFpanelAllRes);
		
		return rootView;
	}

	@Override
	public void onClick(View v) {

		int pos = getArguments().getInt("position");
		TestItemE testItem = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]);
		switch (v.getId()) {
		case R.id.btnPass: {
			
			{
				if (!swHddLed.isChecked()
				 || !swNetLed.isChecked()
				 || !swPowerLed.isChecked()
				 || !swPowerBtn.isChecked()
				 || !swResetBtn.isChecked()) {
					Toast.makeText(getActivity(), "测试项中存在 失败!", Toast.LENGTH_SHORT).show();
					return;
				}
			}
			
			testItem.setResult(getActivity().getString(R.string.pass));
//			System.out.println("testItem.getName()"+":"+testItem.getName());
//			System.out.println("testItem.getResult()"+":"+testItem.getResult());
//			Toast.makeText(getActivity(), "测试项中存在 失败!", Toast.LENGTH_SHORT).show();
			
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
		default:

			break;
		}
	}

}
