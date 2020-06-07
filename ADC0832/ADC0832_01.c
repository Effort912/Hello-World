#include<reg52.h>
#include<intrins.h>

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
#define LCD1602_DB P1

sbit ADC_CLK = P2^0;
sbit ADC_DI = P2^1;
sbit ADC_DO = P2^2;
sbit ADC_CS = P3^1;
sbit LCD1602_RS = P1^0;
sbit LCD1602_RW = P1^1;
sbit LCD1602_E = P1^5;

bit flag1000ms = 0;
unsigned char T0RH = 0;
unsigned char T0RL = 0;
unsigned char bufMove1[16];
unsigned char bufMove2[16];
static unsigned long i = 0;
u8 ss;

void ConfigTimer0(unsigned int ms);
void InitLcd1602();
void LcdShowStr(unsigned char x, unsigned char y
					, unsigned char *str);
void Sum(unsigned char *LcdBuff);
unsigned char ADC0832(void);
void init(void);
void senddata_sum(void);

void Delay(unsigned char x)
{
	unsigned char i;

	for(i=0 ;i<x ;i++);
}

void main()
{
	unsigned char i = 0; 

	EA = 1;
	ConfigTimer0(10);
	InitLcd1602();
	init();
	while(1)
	{
		ss = ADC0832();
		senddata_sum();
		if(flag1000ms)
		{
			flag1000ms = 0;
			Sum(bufMove1);
			LcdShowStr(0 ,0 ,bufMove1);
			LcdShowStr(0 ,1 ,bufMove2);
		}		
	}
}

unsigned char ADC0832(void)
{
	u8 i ,data_c = 1;

	ADC_CS = 0;
	ADC_DO = 0;//片选，DO为高阻态
	for(i=0 ;i<10 ;i++);
	ADC_CLK = 0;
	Delay(2);
	ADC_DI = 1;

	ADC_CLK = 1;
	Delay(2);//第一个脉冲，起始位
	ADC_CLK = 0;

	Delay(2);
	ADC_DI = 1;

	ADC_CLK = 1;
	Delay(2);//第二个脉冲，DI=1表示双通道单极性输入	
	ADC_CLK = 0;

	Delay(2);
	ADC_DI = 1;

	ADC_CLK = 1;
	Delay(2);//第三个脉冲，DI=1表示选择通道1（CH2）
	ADC_CLK = 0;

	ADC_DI = 0;
	ADC_DO = 1;//DI	 转为高阻态，DO脱离高阻态为输出数据做准备
	ADC_CLK = 1;
	Delay(2);
	ADC_CLK = 0;
	Delay(2);//经实验，这加一个脉冲AD便能正确读出数据，不加的话读出的数据少一位（最低位d0读不出）

	for(i=0 ;i<8 ;i++)
	{
		ADC_CLK = 1;
		Delay(2);
		ADC_CLK = 0;
		Delay(2);
		data_c = (data_c<<1)|ADC_DO;
	}
	ADC_CS = 1;//取消片选一个转换周期结束
	return(data_c);
}

void senddata(unsigned char dat)
{
	static unsigned char k = 0;
	
	SBUF = dat;
	bufMove1[k] = dat;
	k = (k+1)%4;
	while(!TI);
	TI = 0;
}

void senddata_sum()
{
	senddata(ss/1000+0x30);
	senddata(ss%1000/100+0x30);
	senddata(ss%100/10+0x30);
	senddata(ss%10+0x30);
	senddata(0x0d);
	senddata(0x0a);
}

void init(void)
{
	SCON = 0X50;//设定串口工作方式
	PCON = 0X00;//波特率不倍增

	TMOD = 0X20;
	EA = 1;
	ES = 1;
	TL1 = 0XFD;
	TH1 = 0XFD;//波特率为9600
	TR1 = 1;
}

void ConfigTimer0(unsigned int ms)
{
	unsigned long tmp;

	tmp = 11059200 / 12;
	tmp = (tmp * ms) / 1000;
	tmp = 65536 -tmp;
	tmp = tmp + 12;
	T0RH = (unsigned char)(tmp >> 8);
	T0RL = (unsigned char) tmp;
	TMOD &= 0XF0;
	TMOD |= 0X01;
	TH0 = T0RH;
	TL0 = T0RL;
	ET0 = 1;
	TR0 = 1;
}

void LcdWaitReady()
{
	unsigned char sta;

	LCD1602_DB = 0XFF;
	LCD1602_RS = 0;
	LCD1602_RW = 1;
	do{
		LCD1602_E = 1;
		sta = LCD1602_DB;
		LCD1602_E = 0;
	}while(sta & 0x80);
}


void LcdWriteCmd(unsigned char cmd)
{
	LcdWaitReady();
	LCD1602_RS = 0;
	LCD1602_RW = 0;
	LCD1602_DB = cmd;
	LCD1602_E = 1;
	LCD1602_E = 0;
}

void LcdWriteDat(unsigned char dat)
{
	LcdWaitReady();
	LCD1602_RS = 1;
	LCD1602_RW = 0;
	LCD1602_DB = dat;
	LCD1602_E = 1;
	LCD1602_E = 0;
}

void LcdSetCursor(unsigned char x,unsigned char y)
{
	unsigned char addr;

	if(y == 0)
		addr = 0x00 + x;
	else
		addr = 0x40 + x;
	LcdWriteCmd(addr | 0x80);

}

void LcdShowStr(unsigned char x, unsigned char y, unsigned char *str)
{
	LcdSetCursor(x,y);
	while(*str != '\0')
	{
		LcdWriteDat(*str++);
	}
}


void InitLcd1602()
{
	LcdWriteCmd(0x38);
	LcdWriteCmd(0x0C);
	LcdWriteCmd(0x06);
	LcdWriteCmd(0x01);
}

void InterruptTimer0() interrupt 1
{
	static unsigned char tmr1000ms = 0;

	TH0 = T0RH;
	TL0 = T0RL;
	tmr1000ms++;
	if(tmr1000ms >= 100)
	{
		tmr1000ms = 0;
		flag1000ms = 1;
		i++;
	}
}