package com.norco.utils;


public class GpioJNI {
	
	// GPIO����Ϊ N
	// ����pin��� ��Ϊ 0,1,2,...N-1
	
	public static native int getSum(); // GPIO�ܸ���
    
    public static native int getLevel(int pin); // ��ȡ���ŵ�ƽ
    
    public static native int setLevel(int pin, int level); // �������ŵ�ƽ
    
}
