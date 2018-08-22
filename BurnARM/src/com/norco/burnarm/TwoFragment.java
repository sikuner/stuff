package com.norco.burnarm;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

public class TwoFragment extends Fragment {

	private EditText etPeriod;
	private EditText etDuration;
	
	private int periodSec;// = 5;
	private int durationSec;// = 60;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.two_fragment, container,
				false);

		etPeriod = (EditText) rootView.findViewById(R.id.etPeriod);
		etDuration = (EditText) rootView.findViewById(R.id.etDuration);
		etPeriod.setText(Integer.toString(((BurnApp) getActivity()
				.getApplicationContext()).getPeriodSec()));
		etDuration.setText(Integer.toString(((BurnApp) getActivity()
				.getApplicationContext()).getDurationSec()));

		return rootView;
	}

	public int doSaveAs() {

		periodSec = Integer.parseInt(etPeriod.getText().toString());
		durationSec = Integer.parseInt(etDuration.getText().toString());
		
		((BurnApp) getActivity().getApplication()).setPeriodSec(periodSec);
		((BurnApp) getActivity().getApplication()).setDurationSec(durationSec);
		
		return 0;
	}

}
