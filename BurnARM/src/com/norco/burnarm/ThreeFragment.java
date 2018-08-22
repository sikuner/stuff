package com.norco.burnarm;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

public class ThreeFragment extends Fragment {

	private EditText etBurninPos = null;
	private String burninPos = "";
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.three_fragment, container,
				false);
		
		etBurninPos = (EditText) rootView.findViewById(R.id.etBurninPos);
		burninPos = ((BurnApp)getActivity().getApplication()).getBurninPos();
		etBurninPos.setText(burninPos);
		
		return rootView;
	}
	
	public int doSaveAs(){
		
		System.out.println("etBurninPos="+etBurninPos);
		burninPos = etBurninPos.getText().toString();
		((BurnApp)getActivity().getApplication()).setBurninPos(burninPos);
		
		return 0;
	}
	
}
