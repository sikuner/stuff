//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//  UG-2864
//
//    Dot Matrix: 128*64
//    Driver IC : SSD1306 (Solomon Systech)
//    Interface : I2C
//    Revision  :
//    Date      : 
//    Author    :
//    Editor    : Humphrey Lin
//
//  Copyright (c) WiseChip Semiconductor Inc.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <stdio.h>   
#include <linux/i2c.h>   
#include <linux/i2c-dev.h>   
#include <fcntl.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <sys/ioctl.h>   
#include <string.h>  

//#define _DEBUG_
#ifdef _DEBUG_
#define debug(fmt, args...) printf(fmt, ##args )
#else
#define debug(fmt, args...) do {} while (0)
#endif

#define I2C_FILE_NAME "/dev/i2c-1"

#define MAXRLEN 18
#define SLA_ADDR 0x3c//0x3c

#define XLevelL		0x00
#define XLevelH		0x10
#define XLevel		((XLevelH&0x0F)*16+XLevelL)
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xAF

static int i2c_fd = -1;

int init_i2c(char *device)
{
	if (device == NULL)
		return -1;

	i2c_fd = open(device, O_RDWR);
	if (i2c_fd < 0)
		printf("open %s error!\n", device);

	return i2c_fd;
}

void release_i2c(void)
{
	if (i2c_fd > 0)
		close(i2c_fd);
}

#if 0
static int read_i2c_register(unsigned char addr, unsigned char reg, unsigned char *val) 
{  
	unsigned char inbuf, outbuf;  
	struct i2c_rdwr_ioctl_data packets;  
	struct i2c_msg messages[2];  

	if (i2c_fd < 0)
		return -1;
 
	outbuf = reg;  
	messages[0].addr  = addr;  
	messages[0].flags = 0;  //wrtie
	messages[0].len   = 1;  
	messages[0].buf   = &outbuf;  

	/* The data will get returned in this structure */  
	messages[1].addr  = addr;  
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;  
	messages[1].len   = 1;  
	messages[1].buf   = &inbuf;  

	/* Send the request to the kernel and get the result back */  
	packets.msgs      = messages;  
	packets.nmsgs     = 2;  
	if (ioctl(i2c_fd, I2C_RDWR, &packets) < 0) 
	{  
		printf("Read reg:0x%x error\n", reg);  
		return -1;  
	}  
	*val = inbuf;  

	return 0;  
}  
#endif

static int write_i2c_register(unsigned char addr, unsigned char reg, unsigned char value) 
{
	unsigned char outbuf[2] = {0};  
	struct i2c_rdwr_ioctl_data packets;  
	struct i2c_msg messages[1];

	if (i2c_fd < 0)
		return -1;

	/* The first byte indicates which register we'll write */  
	outbuf[0] = reg;  
	outbuf[1] = value;

	messages[0].addr  = addr;  
	messages[0].flags = 0;  
	messages[0].len   = 2;  
	messages[0].buf   = outbuf;  

	/* Transfer the i2c packets to the kernel and verify it worked */  
	packets.msgs  = messages;  
	packets.nmsgs = 1;  
	if (ioctl(i2c_fd, I2C_RDWR, &packets) < 0) 
	{  
		printf("Write reg:0x%x error\n", reg);  
		return -1;  
	}  

	return 0;  
}  

static int write_i2c_register_ext(unsigned char addr, unsigned char reg, unsigned char *data, int len) 
{
	unsigned char *outbuf;  
	struct i2c_rdwr_ioctl_data packets;  
	struct i2c_msg messages[1];
	int i;

	if (i2c_fd < 0)
		return -1;

	outbuf = (unsigned char*)malloc(len+1);

	/* The first byte indicates which register we'll write */  
	outbuf[0] = reg;  

	for (i=0; i<len; i++)
		outbuf[1+i] = data[i];

	messages[0].addr  = addr;  
	messages[0].flags = 0;  
	messages[0].len   = len+1;  
	messages[0].buf   = outbuf;  

	/* Transfer the i2c packets to the kernel and verify it worked */  
	packets.msgs  = messages;  
	packets.nmsgs = 1;  
	if (ioctl(i2c_fd, I2C_RDWR, &packets) < 0) 
	{  
		printf("Write reg:0x%x error\n", reg);  
		free(outbuf);
		return -1;  
	}  

	free(outbuf);
	return 0;  
}  

//----------------------------------------------------------------------

void Write_Command(unsigned char Data)
{
	write_i2c_register(SLA_ADDR, 0x80, Data);
}

void Write_Data(unsigned char Data)
{
	write_i2c_register(SLA_ADDR, 0x40, Data);
}

void Write_Data_Ext(unsigned char reg, unsigned char *Data, int len)
{
	write_i2c_register_ext(SLA_ADDR, reg, Data, len);
}

void Delay(unsigned char n)
{
	usleep(n*1000);
}

void uDelay(unsigned char l)
{
	usleep(l);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Instruction Setting
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Start_Column(unsigned char d)
{
	Write_Command(0x00+d%16);		// Set Lower Column Start Address for Page Addressing Mode
						//   Default => 0x00
	Write_Command(0x10+d/16);		// Set Higher Column Start Address for Page Addressing Mode
						//   Default => 0x10
}

void Set_Addressing_Mode(unsigned char d)
{
	Write_Command(0x20);			// Set Memory Addressing Mode
	Write_Command(d);			//   Default => 0x02
						//     0x00 => Horizontal Addressing Mode
						//     0x01 => Vertical Addressing Mode
						//     0x02 => Page Addressing Mode
}

void Set_Column_Address(unsigned char a, unsigned char b)
{
	Write_Command(0x21);			// Set Column Address
	Write_Command(a);			//   Default => 0x00 (Column Start Address)
	Write_Command(b);			//   Default => 0x7F (Column End Address)
}

void Set_Page_Address(unsigned char a, unsigned char b)
{
	Write_Command(0x22);			// Set Page Address
	Write_Command(a);			//   Default => 0x00 (Page Start Address)
	Write_Command(b);			//   Default => 0x07 (Page End Address)
}

void Set_Start_Line(unsigned char d)
{
	Write_Command(0x40|d);			// Set Display Start Line
						//   Default => 0x40 (0x00)
}

void Set_Contrast_Control(unsigned char d)
{
	Write_Command(0x81);			// Set Contrast Control
	Write_Command(d);			//   Default => 0x7F
}

void Set_Charge_Pump(unsigned char d)
{
	Write_Command(0x8D);			// Set Charge Pump
	Write_Command(0x10|d);			//   Default => 0x10
						//     0x10 (0x00) => Disable Charge Pump
						//     0x14 (0x04) => Enable Charge Pump
}

void Set_Segment_Remap(unsigned char d)
{
	Write_Command(0xA0|d);			// Set Segment Re-Map
						//   Default => 0xA0
						//     0xA0 (0x00) => Column Address 0 Mapped to SEG0
						//     0xA1 (0x01) => Column Address 0 Mapped to SEG127
}

void Set_Entire_Display(unsigned char d)
{
	Write_Command(0xA4|d);			// Set Entire Display On / Off
						//   Default => 0xA4
						//     0xA4 (0x00) => Normal Display
						//     0xA5 (0x01) => Entire Display On
}

void Set_Inverse_Display(unsigned char d)
{
	Write_Command(0xA6|d);			// Set Inverse Display On/Off
						//   Default => 0xA6
						//     0xA6 (0x00) => Normal Display
						//     0xA7 (0x01) => Inverse Display On
}

void Set_Multiplex_Ratio(unsigned char d)
{
	Write_Command(0xA8);			// Set Multiplex Ratio
	Write_Command(d);			//   Default => 0x3F (1/64 Duty)
}

void Set_Display_On_Off(unsigned char d)	
{
	Write_Command(0xAE|d);			// Set Display On/Off
						//   Default => 0xAE
						//     0xAE (0x00) => Display Off
						//     0xAF (0x01) => Display On
}

void Set_Start_Page(unsigned char d)
{
	Write_Command(0xB0|d);			// Set Page Start Address for Page Addressing Mode
						//   Default => 0xB0 (0x00)
}

void Set_Common_Remap(unsigned char d)
{
	Write_Command(0xC0|d);			// Set COM Output Scan Direction
						//   Default => 0xC0
						//     0xC0 (0x00) => Scan from COM0 to 63
						//     0xC8 (0x08) => Scan from COM63 to 0
}

void Set_Display_Offset(unsigned char d)
{
	Write_Command(0xD3);			// Set Display Offset
	Write_Command(d);			//   Default => 0x00
}

void Set_Display_Clock(unsigned char d)
{
	Write_Command(0xD5);			// Set Display Clock Divide Ratio / Oscillator Frequency
	Write_Command(d);			//   Default => 0x80
						//     D[3:0] => Display Clock Divider
						//     D[7:4] => Oscillator Frequency
}

void Set_Precharge_Period(unsigned char d)
{
	Write_Command(0xD9);			// Set Pre-Charge Period
	Write_Command(d);			//   Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
						//     D[3:0] => Phase 1 Period in 1~15 Display Clocks
						//     D[7:4] => Phase 2 Period in 1~15 Display Clocks
}


void Set_Common_Config(unsigned char d)
{
	Write_Command(0xDA);			// Set COM Pins Hardware Configuration
	Write_Command(0x02|d);			//   Default => 0x12 (0x10)
						//     Alternative COM Pin Configuration
						//     Disable COM Left/Right Re-Map
}

void Set_VCOMH(unsigned char d)
{
	Write_Command(0xDB);			// Set VCOMH Deselect Level
	Write_Command(d);			//   Default => 0x20 (0.77*VCC)
}

void Set_NOP()
{
	Write_Command(0xE3);			// Command for No Operation
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_RAM(unsigned char Data)
{
unsigned char i,j;

	for(i=0;i<8;i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(0x00);

		for(j=0;j<128;j++)
		{
			Write_Data(Data);
		}
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
	unsigned char i,j;
	
	for(i=a;i<(b+1);i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(c);

		for(j=0;j<d;j++)
		{
			Write_Data(Data);
		}
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Checkboard (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Checkerboard()
{
	unsigned char i,j;
	
	for(i=0;i<8;i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(0x00);

		for(j=0;j<64;j++)
		{
			Write_Data(0x55);
			Write_Data(0xaa);
		}
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Frame (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Frame()
{
	unsigned char i,j;
	
	Set_Start_Page(0x00);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x01);
	}

	Set_Start_Page(0x01);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x80);
	}

	Set_Start_Page(0x02);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x01);
	}

	Set_Start_Page(0x07);
	Set_Start_Column(XLevel);

	for(i=0;i<Max_Column;i++)
	{
		Write_Data(0x80);
	}

	for(i=0;i<8;i++)
	{
		Set_Start_Page(i);

		for(j=0;j<Max_Column;j+=(Max_Column-1))
		{
			Set_Start_Column(XLevel+j);

			Write_Data(0xFF);
		}
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Pattern (Partial or Full Screen)
//
//    a: Start Page
//    b: End Page
//    c: Start Column
//    d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void ShowPattern(unsigned char *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
	unsigned char *Src_Pointer;
	unsigned char i;

	Src_Pointer = Data_Pointer;
	for(i = a; i < (b+1); i++)
	{
		Set_Start_Page(i);
		Set_Start_Column(c);
		
		Write_Data_Ext(0x40, Src_Pointer, d);
		Src_Pointer += d;
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Vertical / Fade Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Upward)
//       "0x01" (Downward)
//    b: Set Top Fixed Area
//    c: Set Vertical Scroll Area
//    d: Set Numbers of Row Scroll per Step
//    e: Set Time Interval between Each Scroll Step
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Vertical_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
	unsigned int i,j;	

	Write_Command(0xA3);			// Set Vertical Scroll Area
	Write_Command(b);			//   Default => 0x00 (Top Fixed Area)
	Write_Command(c);			//   Default => 0x40 (Vertical Scroll Area)

	switch(a)
	{
		case 0:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(i);
				for(j=0;j<e;j++)
				{
					uDelay(200);
				}
			}
			break;
		case 1:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(c-i);
				for(j=0;j<e;j++)
				{
					uDelay(200);
				}
			}
			break;
	}
	Set_Start_Line(0x00);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Horizontal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Rightward)
//       "0x01" (Leftward)
//    b: Define Start Page Address
//    c: Define End Page Address
//    d: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    e: Delay Time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Horizontal_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
	Write_Command(0x26|a);			// Horizontal Scroll Setup
	Write_Command(0x00);			//           => (Dummy Write for First Parameter)
	Write_Command(b);
	Write_Command(d);
	Write_Command(c);
	Write_Command(0x2F);			// Activate Scrolling
	Delay(e);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Vertical / Horizontal / Diagonal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Vertical & Rightward)
//       "0x01" (Vertical & Leftward)
//    b: Define Start Row Address (Horizontal / Diagonal Scrolling)
//    c: Define End Page Address (Horizontal / Diagonal Scrolling)
//    d: Set Top Fixed Area (Vertical Scrolling)
//    e: Set Vertical Scroll Area (Vertical Scrolling)
//    f: Set Numbers of Row Scroll per Step (Vertical / Diagonal Scrolling)
//    g: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    h: Delay Time
//    * d+e must be less than or equal to the Multiplex Ratio...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Continuous_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h)
{
	Write_Command(0xA3);			// Set Vertical Scroll Area
	Write_Command(d);			//   Default => 0x00 (Top Fixed Area)
	Write_Command(e);			//   Default => 0x40 (Vertical Scroll Area)

	Write_Command(0x29+a);			// Continuous Vertical & Horizontal Scroll Setup
	Write_Command(0x00);			//           => (Dummy Write for First Parameter)
	Write_Command(b);
	Write_Command(g);
	Write_Command(c);
	Write_Command(f);
	Write_Command(0x2F);			// Activate Scrolling
	Delay(h);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Deactivate Scrolling (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Deactivate_Scroll()
{
	Write_Command(0x2E);			// Deactivate Scrolling
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade In (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_In()
{
	unsigned int i;	

	Set_Display_On_Off(0x01);
	for(i=0;i<(Brightness+1);i++)
	{
		Set_Contrast_Control(i);
		uDelay(200);
		uDelay(200);
		uDelay(200);
	}
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Fade Out (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Fade_Out()
{
	unsigned int i;	

	for(i=(Brightness+1);i>0;i--)
	{
		Set_Contrast_Control(i-1);
		uDelay(200);
		uDelay(200);
		uDelay(200);
	}
	Set_Display_On_Off(0x00);
}

void OLED_Init_E()				// VCC Supplied Externally
{
	Set_Display_On_Off(0x00);		// Display Off (0x00/0x01)
	Set_Display_Clock(0x80);		// Set Clock as 400 Frames/Sec
	Set_Multiplex_Ratio(0x3F);		// 1/64 Duty (0x0F~0x3F)
	Set_Display_Offset(0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
	Set_Start_Line(0x00);			// Set Mapping RAM Display Start Line (0x00~0x3F)
	Set_Charge_Pump(0x00);			// Disable Embedded DC/DC Converter (0x00/0x04)
	Set_Addressing_Mode(0x02);		// Set Page Addressing Mode (0x00/0x01/0x02)
	Set_Segment_Remap(0x01);		// Set SEG/Column Mapping (0x00/0x01)
	Set_Common_Remap(0x08);			// Set COM/Row Scan Direction (0x00/0x08)
	Set_Common_Config(0x10);		// Set Sequential Configuration (0x00/0x10)
	Set_Contrast_Control(Brightness);	// Set SEG Output Current
	Set_Precharge_Period(0x22);		// Set Pre-Charge as 2 Clocks & Discharge as 2 Clock
	Set_VCOMH(0x40);			// Set VCOM Deselect Level
	Set_Entire_Display(0x00);		// Disable Entire Display On (0x00/0x01)
	Set_Inverse_Display(0x00);		// Disable Inverse Display On (0x00/0x01)

	Fill_RAM(0x00);				// Clear Screen

	Set_Display_On_Off(0x01);		// Display On (0x00/0x01)
}

void OLED_Init_I()					// VCC Generated by Internal DC/DC Circuit
{
	Set_Display_On_Off(0x00);		// Display Off (0x00/0x01)
	Set_Display_Clock(0x80);		// Set Clock as 400 Frames/Sec
	Set_Multiplex_Ratio(0x3F);		// 1/64 Duty (0x0F~0x3F)
	Set_Display_Offset(0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
	Set_Start_Line(0x00);			// Set Mapping RAM Display Start Line (0x00~0x3F)
	Set_Charge_Pump(0x04);			// Enable Embedded DC/DC Converter (0x00/0x04)
	Set_Addressing_Mode(0x02);		// Set Page Addressing Mode (0x00/0x01/0x02)
	Set_Segment_Remap(0x01);		// Set SEG/Column Mapping (0x00/0x01)
	Set_Common_Remap(0x08);			// Set COM/Row Scan Direction (0x00/0x08)
	Set_Common_Config(0x10);		// Set Sequential Configuration (0x00/0x10)
	Set_Contrast_Control(Brightness);	// Set SEG Output Current
	Set_Precharge_Period(0xF1);		// Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	Set_VCOMH(0x40);				// Set VCOM Deselect Level
	Set_Entire_Display(0x00);		// Disable Entire Display On (0x00/0x01)
	Set_Inverse_Display(0x00);		// Disable Inverse Display On (0x00/0x01)
	Deactivate_Scroll();		
	Fill_RAM(0x00);					// Clear Screen
	
	Set_Display_On_Off(0x01);		// Display On (0x00/0x01)
}

int OledDrvInit()
{
	if(init_i2c(I2C_FILE_NAME) < 0)
	{
		return -1;
	}
	
//	Deactivate_Scroll();		
//	Fill_RAM(0x00);					// Clear Screen
	OLED_Init_I();
	
	return 0;
}

void OledDrvRelease()
{
	release_i2c();
}

