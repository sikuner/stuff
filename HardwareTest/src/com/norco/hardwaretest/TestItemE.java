package com.norco.hardwaretest;

import java.lang.String;
import android.support.v4.app.Fragment;

public enum TestItemE {

	OPERATION("����¼��", "NULL", "", new OperationFragment(), true), 
	FPANEL("ǰ������", "NULL", "", new FPanelFragment(), true), 
	VERSION("�汾��Ϣ", "NULL", "",	new VersionFragment(), true), 
	SIM("SIM������", "NULL", "", new SimFragment(), true), 
	SDCARD(	"SD����д����", "NULL", "", new SdCardFragment(), true), 
	SATADISK("SATAӲ�̶�д����", "NULL", "", new SataDiskFragment(), true), 
	USB("USB�ӿڲ���", "NULL", "", new UsbFragment(), true),  
	SCREENDISPLAY("��Ļ��ʾ����", "NULL", "", new ScreenDisplayFragment(), true), 
	HEADSETLOOP("�����ػ�����", "NULL", "", new HeadsetLoopFragment(), true), 
	WIFI("WIFI����", "NULL", "", new WifiFragment(), true), 
	ETHERNET("��̫������", "NULL", "", new EthernetFragment(), true), 
	GPIO("GPIO����", "NULL", "", new GpioFragment(), true), 
	SERIALPORT("���ڲ���", "NULL", "", new SerialPortFragment(), true), 
	RS485("RS485���ڲ���", "NULL", "", new RS485Fragment(), true);
	
	private String name;
	private String result;
	private String note; // ��ע��Ϣ
	private Fragment fragment;
	private boolean enabled;
	
	public void setEnabled(boolean enabled) {
		this.enabled = enabled;
	}
	
	public boolean enabled() {
		return this.enabled;
	}
	
	private TestItemE(String name) {
		this.name = name;
		this.result = "NULL";
		this.note = "";
		this.fragment = null;
		this.enabled = true;
	}
	
	private TestItemE(String name, String result, String note, Fragment fragment, boolean enabled) {
		this.name = name;
		this.result = result;
		this.note = note;
		this.fragment = fragment;
		this.enabled = enabled;
	}

	public String getName() {
		return name;
	}

	public String getResult() {
		return result;
	}

	public void setResult(String result) {
		this.result = result;
	}

	public void setNote(String note) {
		this.note = note;
	}

	public String getNote() {
		return this.note;
	}

	public Fragment getFragment() {
		return fragment;
	}
}
