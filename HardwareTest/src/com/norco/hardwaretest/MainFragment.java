package com.norco.hardwaretest;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.widget.Toast;
import android.os.Bundle;

public class MainFragment extends FragmentActivity {

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main_fragment);

		Bundle b = getIntent().getExtras();
		int pos = b.getInt("position", 0);
		System.out.println("pos=" + pos);
		
		Fragment fg = MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]).getFragment();
		
		if (savedInstanceState == null) {
			
			if (fg != null) {
				
				fg.setArguments(b);
				getSupportFragmentManager().beginTransaction()
						.add(R.id.fragment_container, fg).commit();
			} 
			else {
				Toast.makeText(MainFragment.this,
						"对比起,暂不支持:" + MainActivity.getTestItemE(MainActivity.itemRealIndexs[pos]).getName(),
						Toast.LENGTH_SHORT).show();
				
				finish();
			}
		}
	}
	
	
	
}
