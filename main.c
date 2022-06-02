#include "reg51.h"
#include "intrins.h"

sbit smg1=P2^0;      //数码管  位选
sbit smg2=P2^1;	     //        段选
sbit led1=P1^1;      //LED
sbit led2=P1^2;
sbit led3=P1^3;
sbit lamp=P3^5;      //灯
sbit beep=P3^6;      //蜂鸣器
sbit k1=P3^0;        //按钮
sbit CLK=	P1^5;    //时钟,初始状态为0
sbit DI=	P1^6;    //数据输入
sbit DO=	P1^7;    //数据输出
sbit CS=	P1^4; 	 //片选使能，低电平芯片使能

unsigned char code smgduan[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
//共阴极0-9，code 代表的是 把定义的数据表存储到 flash存储器中//
unsigned char light=0;    //光照
unsigned char time=0,flag=0;

/*********************************************
读取ADC,将模拟信号转化成数字信号
**********************************************/
unsigned char ADC()  //通道ch0
{
	unsigned char temp0,temp1,i;

	CS=1;	  //一个转换周期开始
	CLK=0;	  //为第一个脉冲做准备
	_nop_();  
	_nop_();  //空操作两个机械周期
	CS=0;	  //片选使能，低电平芯片使能，芯片开始工作
	temp0=0;
	temp1=0;
	_nop_();
	_nop_();
	DI=1;     //开始位,开始转换
	_nop_();
	_nop_();
	CLK=1;	  //第一个脉冲
	_nop_();
	_nop_();
	CLK=0;	  //第一个脉冲的下降沿，之前DI为高电平
	_nop_();
	_nop_();
	DI=0;
	_nop_();
	_nop_();
//选择通道CH0，在第2、3个脉冲的下降沿之前。DI端口输入两位数据，用于选择数据采集通道	//
	DI=1;	   //DI置1，通道选择信号
	_nop_();
	CLK=1;     //第二个脉冲
	_nop_();
	CLK=0;     //第二个脉冲下降沿
	_nop_();
	DI=0;	   //DI置零，选择通道CH0
	_nop_();
	CLK=1;	   //第三个脉冲
	_nop_();
	CLK=0;     
	_nop_();
   	DI=1;	   //第三个脉冲下沉之后，输入端DI失去作用，应置1，释放总线
//********通道选择结束开始读取转换后的二进制数****
//下降沿读数，一下进行判断和处理,共8次
for(i=0;i<8;i++)	//每个下降沿处都可以从DO读出一位数据，一共读8位，最高位在最前
   {
	temp0=temp0<<1;	//将一个数的各二进制位左移1位，高位舍弃，低位补0，依次输出
	CLK=1;
    if(DO)
   	temp0++;
   	_nop_();
   	CLK=0;
   }
for(i=0;i<8;i++)	//再读8位，最低位在最前，用于做校验
   {
	temp1=temp1>>1;	//将一个数的各二进制位右移1位，最高位为0，右移后，空缺位补0；最高位为1，空缺位补10。//
	CLK=1;
    if(DO)
   	temp1=temp1 +0x80;
   	_nop_();
   	CLK=0;
   }
	CS=1;
	if(temp1=temp0) //校验两次读数
    return temp0;	
	return temp0;	//返回数模转换的数值
}

void delay(unsigned int m) //延时若干毫秒，入口参数为m
{
	unsigned char j;
	for(j=0;j<m;j++);
}

void main()
{
	led1=0;led2=0;led3=0;
	TMOD|=0X01;	   //TMOD定时器模式寄存器，选择工作方式为16位定时器
	TH0=0X3C;	   //设置初始值，计数起点50ms
	TL0=0XB0;	
	ET0=1;         //打开定时器0中断
	EA=1;          //打开总中断
	TR0=1;         //定时器0 开始计时

	while(1)
	{
		//显示光照数据
		P0=smgduan[light/10];  //十位
		smg1=0;
		delay(100);	           //延时100ms
		smg1=1;
		P0=smgduan[light%10];  //个位
		smg2=0;
		delay(100);
		smg2=1;
		
		//停止报警
		if(!k1)
		{
			flag=1;
			beep=1;
		}
	}
}

/*********************************************
 5个中断源的排序：0代表外部中断0中断 ，1代表定时器/计数器0中断 
 2代表外部中断1中断， 3代表定时器/计数器1， 4代表串行中断的中断 
**********************************************/
void Timer0() interrupt 1
{
	unsigned int D;
	TH0=0X3C;		//重新赋初始值，保证每次进入中断函数都是50ms
	TL0=0XB0;
	if(time<10)     //0.5s
		time++;
	else
	{
		time=0;
		D=ADC();
		light=D*100/128;
		
		//判断
		if((light>30)&&(flag==0))   //强
		{
			beep=0;				    
			led1=1;led2=0;led3=0;
		}			
		if((light<31)&&(light>20))  //中
		{
			led1=0;led2=1;led3=0;
			beep=1;
			lamp=1;
			flag=0;
		}
		if(light<21)      //弱
		{
			led1=0;led2=0;led3=1;
			beep=1;
			lamp=0;
			flag=0;
		}	
	}
}