package com.norco.utils;


public class GpioJNI {
	
	// GPIO总数为 N
	// 引脚pin标号 则为 0,1,2,...N-1
	
	public static native int getSum(); // GPIO总个数
    
    public static native int getLevel(int pin); // 获取引脚电平
    
    public static native int setLevel(int pin, int level); // 设置引脚电平
    
}
