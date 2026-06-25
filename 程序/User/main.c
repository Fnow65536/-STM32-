#include "stm32f10x.h"
#include "OLED_I2C.h"
#include "ds1302.h"
#include "ds18b20.h"
#include "usart1.h"
#include "usart2.h"
#include "led.h"
#include "delay.h"
#include "max30102_read.h"
#include "myiic.h"
#include "key.h"
#include "iic.h"
#include "stdio.h"
#include "string.h"
#include "adxl345.h"
#include "timer.h"
#include "stmflash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_SAVE_ADDR   STM32_FLASH_BASE 	+ 0X400			//设置FLASH 保存地址(必须为偶数)

unsigned char set_value=0;                                 //设置值
unsigned char  commondYear_leapYear_flag=0;		 						//平年/润年  =0表示平年，=1表示润年
float adx,ady,adz;
float acc;
float acc2,acc3;
u8 flag=0;
u16 stepcount=0;                      //步数
long mileage=0;//里程
u16 mileage_stepcount = 0;//里程的步数
u16 step = 40;//步长
long mileage_alert=1000;//提醒里程
short temperature=0;      //温度
u16 heart_rate_Min=60,heart_rate_Max=120;//心率下限上限
u16 blood_oxygen_Lowlimit=80;//血氧下限
u16 tempMax=373;//温度上限
u16 tempMin=150;//温度下限
char display[16];
u8 flashing=0;
u8 buzzer_alarm_flag=0x00; //蜂鸣器报警标志
u8 page = 0;      //页面切换变量
/*定时时间*/
long timecount_record=0;
long hour=1,minute=0,second=0;
bool start_timer_sign=0;        //开始定时标志
bool timer_remind_sign=0;   //定时时间提醒标志
bool mileage_remind_sign=0;  //里程提醒标志

u8 sendFlag = 1;
bool sendSetValueFlag=0;

int32_t heart_rate;//心率
int32_t blood_oxygen_concentration;//血氧浓度

void DisplayTime(void)//显示时间函数
{
	  unsigned char i=0,x=0;
	  u16 leapyear_flag;
	
	  if(set_value==0)
	  {
		  DS1302_DateRead(&SysDate);//读时间
	  }
	  leapyear_flag=2000+SysDate.year;
	  if((leapyear_flag%400==0)||((leapyear_flag%100!=0)
		  &&(leapyear_flag%4==0)))  //判断是否为闰年
	  {
		  commondYear_leapYear_flag=1;
	  }
	  else
	  {
		  commondYear_leapYear_flag=0;
	  }
	  if(set_value<8 && page==0)
	  {
	      OLED_ShowChar((x++)*8,0,'2',2,set_value+1-1);
	      OLED_ShowChar((x++)*8,0,'0',2,set_value+1-1);
	      OLED_ShowChar((x++)*8,0,SysDate.year/10+'0',2,set_value+1-1);
	      OLED_ShowChar((x++)*8,0,SysDate.year%10+'0',2,set_value+1-1);
	      OLED_ShowChar((x++)*8,0,'-',2,0);
	      OLED_ShowChar((x++)*8,0,SysDate.mon/10+'0',2,set_value+1-2);
	      OLED_ShowChar((x++)*8,0,SysDate.mon%10+'0',2,set_value+1-2);
	      OLED_ShowChar((x++)*8,0,'-',2,0);
	      OLED_ShowChar((x++)*8,0,SysDate.day/10+'0',2,set_value+1-3);
	      OLED_ShowChar((x++)*8,0,SysDate.day%10+'0',2,set_value+1-3);
	      OLED_ShowCN(i*16+88,0,0,set_value+1-4);//测试显示中文：周
	   switch(SysDate.week)
       {
          case 1: 
			  OLED_ShowCN(i*16+104,0,1,set_value+1-4);//测试显示中文：一
              break;

          case 2: 
			  OLED_ShowCN(i*16+104,0,2,set_value+1-4);//测试显示中文：二
              break;

         case 3: 
			  OLED_ShowCN(i*16+104,0,3,set_value+1-4);//测试显示中文：三
              break;

         case 4: 
			  OLED_ShowCN(i*16+104,0,4,set_value+1-4);//测试显示中文：四
              break;

         case 5: 
			  OLED_ShowCN(i*16+104,0,i+5,set_value+1-4);//测试显示中文：五
              break;

         case 6: 
			  OLED_ShowCN(i*16+104,0,6,set_value+1-4);//测试显示中文：六
              break;

         case 7: 
			  OLED_ShowCN(i*16+104,0,7,set_value+1-4);//测试显示中文：日
              break;
    }
    x=0;
		
	  OLED_ShowChar((x++)*8,2,SysDate.hour/10+'0',2,set_value+1-5);
	  OLED_ShowChar((x++)*8,2,SysDate.hour%10+'0',2,set_value+1-5);
	  OLED_ShowChar((x++)*8,2,':',2,0);
	  OLED_ShowChar((x++)*8,2,SysDate.min/10+'0',2,set_value+1-6);
	  OLED_ShowChar((x++)*8,2,SysDate.min%10+'0',2,set_value+1-6);
	  OLED_ShowChar((x++)*8,2,':',2,0);
	  OLED_ShowChar((x++)*8,2,SysDate.sec/10+'0',2,set_value+1-7);
	  OLED_ShowChar((x++)*8,2,SysDate.sec%10+'0',2,set_value+1-7);
	}
}

void GetHeartRateSpO2(void)
{
	unsigned char x=0;
	
	ReadHeartRateSpO2();  //读取心率血氧
	
	if(page==0)
	{
	  //显示相关的信息
	  if(((heart_rate!=0)&&(heart_rate>=heart_rate_Max||
		  heart_rate<=heart_rate_Min))&&flashing==1)
	   {
		 OLED_ShowChar((x++)*8,6,' ',2,0);
		 OLED_ShowChar((x++)*8,6,' ',2,0);
		 OLED_ShowChar((x++)*8,6,' ',2,0);
	   }
	  else
	   {
		//显示相关的信息
		OLED_ShowChar((x++)*8,6,heart_rate%1000/100+'0',2,0);
		OLED_ShowChar((x++)*8,6,heart_rate%100/10+'0',2,0);
		OLED_ShowChar((x++)*8,6,heart_rate%10+'0',2,0);
	   }
	  x=6;
	  if(((blood_oxygen_concentration!=0)&&
		  (blood_oxygen_concentration<=blood_oxygen_Lowlimit))&&flashing==1)
	   {
		 OLED_ShowChar((x++)*8,6,' ',2,0);
		 OLED_ShowChar((x++)*8,6,' ',2,0);
		 OLED_ShowChar((x++)*8,6,' ',2,0);
	   }
	  else
	   {
		//显示相关的信息
		 OLED_ShowChar((x++)*8,6,blood_oxygen_concentration%1000/100+'0',2,0);
		 OLED_ShowChar((x++)*8,6,blood_oxygen_concentration%100/10+'0',2,0);
		 OLED_ShowChar((x++)*8,6,blood_oxygen_concentration%10+'0',2,0);
	   }
	}
}

void DisplayTemperature(void)//显示温度函数
{
	  unsigned char x=10;//显示的第几个字符
		
	  temperature=DS18B20_Get_Temp();
	  if(page==0)
		{
				if((temperature<=tempMin||temperature>=tempMax)&&flashing==1)
				{
						 OLED_ShowChar((x++)*8,2,' ',2,0);
						 OLED_ShowChar((x++)*8,2,' ',2,0);
						 OLED_ShowChar((x++)*8,2,' ',2,0);
						 OLED_ShowChar((x++)*8,2,' ',2,0);
				}
				else
				{
						OLED_ShowChar((x++)*8,2,temperature/100+'0',2,0);
						OLED_ShowChar((x++)*8,2,temperature%100/10+'0',2,0);
						OLED_ShowChar((x++)*8,2,'.',2,0);
						OLED_ShowChar((x++)*8,2,temperature%10+'0',2,0);
				}
		}
}

void Store_Data(void) //存储数据
{
	  u16 DATA_BUFFER[10];               //数据缓冲区
	
	  DATA_BUFFER[0] =   stepcount;
	  DATA_BUFFER[1] =   mileage_stepcount;
	  DATA_BUFFER[2] =   step;
      DATA_BUFFER[3] =   heart_rate_Min;   
	  DATA_BUFFER[4] =   heart_rate_Max;
	  DATA_BUFFER[5] =   blood_oxygen_Lowlimit;
	  DATA_BUFFER[6] =   tempMin;
	  DATA_BUFFER[7] =   tempMax;
	  DATA_BUFFER[8] =   (mileage_alert>>16)&0xFFFF;
	  DATA_BUFFER[9] =   (mileage_alert>>0)&0xFFFF;
	
	  STMFLASH_Write(FLASH_SAVE_ADDR + 0x20,(u16*)DATA_BUFFER,10);
   	  //写入数据
	
	  Delay_ms(50);
}

void Read_Data(void) //读出数据
{
	   u16 DATA_BUFFER[10];
	
	   STMFLASH_Read(FLASH_SAVE_ADDR + 0x20,(u16*)DATA_BUFFER,10); 
	   //读取数据
	
	   stepcount                   =  DATA_BUFFER[0];  
	   mileage_stepcount           =  DATA_BUFFER[1]; 
	   step                        =  DATA_BUFFER[2]; 
	   heart_rate_Min              =  DATA_BUFFER[3];  
	   heart_rate_Max              =  DATA_BUFFER[4]; 
	   blood_oxygen_Lowlimit       =  DATA_BUFFER[5]; 
	   tempMin                     =  DATA_BUFFER[6];  
	   tempMax                     =  DATA_BUFFER[7]; 
	   mileage_alert               =  DATA_BUFFER[8]<<16|DATA_BUFFER[9]; 
}

void CheckNewMcu(void)  // 检查是否是新的单片机，是的话清空存储区，否则保留
{
	  u8 comper_str[6];
		
	  STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)comper_str,5);
	  comper_str[5] = '\0';
	  if(strstr((char *)comper_str,"FDYDZ") == NULL)  //新的单片机
		{
			 STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)"FDYDZ",5); 
			//写入“FDYDZ”，方便下次校验
			 Delay_ms(50);
			 Store_Data();//存储下初始数据
	  }
		Read_Data(); //开机读取下相关数据（步数里程卡路里步长）
		if(heart_rate_Max>300||heart_rate_Max<0) 
		{
			  heart_rate_Min=60;heart_rate_Max=120;//心率下限上限
			  blood_oxygen_Lowlimit=80;//血氧下限
			  tempMax=373;//温度上限
			  tempMin=150;//温度下限
			  stepcount         =  0;  
			  mileage_stepcount =  0; 
			  step       =  40; 
			  mileage_alert=1000;
		}
}

void GetSteps(void)//获取步数函数
{
	  static u16 tempvalue=0;
	  float tempMileage=0.0;
	  u16 x=11; 
	
	  adxl345_read_average(&adx,&ady,&adz,10);//获取数据
	  acc=ady;
	  acc3=adx;
		
		if(acc>0)//在正轴
		{
			if(acc/10>=10)
				//加速度值，在正轴值是否大于10，则视为一个周期完成，步数加1
			{
				flag=0;	
				if(stepcount<60000)stepcount++;	//步数加1
				if(mileage_stepcount<60000)mileage_stepcount++;	//步数加1
				if(tempvalue!=stepcount)//当步数发生变化才去存储步数
				{
					  tempvalue=stepcount;
					  Store_Data(); //存储步数
				}
			}
		}
		if(page==0)
		{
				if(stepcount>9999)
				{
						OLED_ShowChar((x++)*8,6,stepcount/10000+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10000/1000+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%1000/100+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%100/10+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10+'0',2,0);
				}
				else if(stepcount>999)
				{
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10000/1000+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%1000/100+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%100/10+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10+'0',2,0);
				}
				else if(stepcount>99)
				{
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%1000/100+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%100/10+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10+'0',2,0);
				}
				else if(stepcount>9)
				{
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%100/10+'0',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10+'0',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
				}
				else
				{
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,stepcount%10+'0',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
						OLED_ShowChar((x++)*8,6,' ',2,0);
				}
		}
		else
		{
				mileage = (mileage_stepcount * step)/100;//计算里程
				tempMileage = (float)mileage/1000;//转换为千米
				sprintf(display,"%6.3fkm ",tempMileage);
				OLED_ShowStr(48,2,(u8 *)display,2);
		}
}

void DisplaySetValue(void) //显示设置值
{
  if(page==0)
   {
	if(set_value==8||set_value==9)
	{
	 OLED_ShowChar(48,4,heart_rate_Min/100+'0',2,set_value+1-8);//显示
	 OLED_ShowChar(56,4,heart_rate_Min%100/10+'0',2,set_value+1-8);//显示
	 OLED_ShowChar(64,4,heart_rate_Min%10+'0',2,set_value+1-8);//显示
	 OLED_ShowChar(48,6,heart_rate_Max/100+'0',2,set_value+1-9);//显示
	 OLED_ShowChar(56,6,heart_rate_Max%100/10+'0',2,set_value+1-9);//显示
	 OLED_ShowChar(64,6,heart_rate_Max%10+'0',2,set_value+1-9);//显示
	}
	if(set_value==10)
	{
	 OLED_ShowChar(48,4,blood_oxygen_Lowlimit/100+'0',2,set_value+1-10);//显示
	 OLED_ShowChar(56,4,blood_oxygen_Lowlimit%100/10+'0',2,set_value+1-10);//显示
	 OLED_ShowChar(64,4,blood_oxygen_Lowlimit%10+'0',2,set_value+1-10);//显示
	}
	if(set_value==11||set_value==12)
	{
     OLED_ShowChar(48,4,tempMin/100+'0',2,set_value+1-11);//显示
     OLED_ShowChar(56,4,tempMin%100/10+'0',2,set_value+1-11);//显示
     OLED_ShowChar(64,4,'.',2,set_value+1-11);
     OLED_ShowChar(72,4,tempMin%10+'0',2,set_value+1-11);//显示		
     OLED_ShowChar(48,6,tempMax/100+'0',2,set_value+1-12);//显示
     OLED_ShowChar(56,6,tempMax%100/10+'0',2,set_value+1-12);//显示
     OLED_ShowChar(64,6,'.',2,set_value+1-12);
     OLED_ShowChar(72,6,tempMax%10+'0',2,set_value+1-12);//显示
	}
   }
  else
   {
	 if(set_value==4)
	  {
		sprintf((char *)display,"%dcm  ",step);
		OLED_ShowStr(48, 4, (u8*)display, 2);//显示经度
	  }	
     if(set_value==5)
	  {
		sprintf(display,"%6.3fkm ",(float)mileage_alert/1000);
		OLED_ShowStr(35,4,(u8 *)display,2);
	  }					
	}
}

void KeySettings(void)//按键设置函数
{
	  unsigned char keynum = 0,i;
	  static u16 pressCount=0;
	  bool press = 0;
	  u16 bufferCount=0;
	
	  keynum = KEY_Scan(1);//获取按键值
		if(keynum==1)//设置
		{
		 set_value++;
         if(page==1)
		 {
	       if(set_value > 5)
			{
				set_value=0;
				Store_Data();
				OLED_CLS();//清屏
				for(i=0;i<2;i++)OLED_ShowCN(i*16,2,i+44,0);//显示中文：里程
				  OLED_ShowChar(32,2,':',2,0);
			}
		   if(set_value==4)
			{
				OLED_CLS();//清屏
				for(i=0;i<4;i++)
				  OLED_ShowCN(i*16+32,0,i+46,0);//测试显示中文：设置步长
			}
		   if(set_value==5)
			{
				for(i=0;i<6;i++)
				  OLED_ShowCN(i*16+16,0,i+50,0);//测试显示中文：设置提醒里程
			}
		 }
		 else
		 {
		   if(set_value > 12)
			{
                set_value=0;
				OLED_CLS();//清屏
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,4,i+16,1);//测试显示中文：心率
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16+48,4,i+18,1);//测试显示中文：血氧
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16+95,4,i+20,1);//测试显示中文：步数
				OLED_ShowCentigrade(112, 2);    //℃
				Delay_ms(50);
				Store_Data();
			}
				
		   if(set_value==8)
			{
				OLED_CLS();//清屏
				for(i=0;i<4;i++)
				  OLED_ShowCN(i*16+32,0,i+22,0);//测试显示中文：设置心率
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,4,i+26,0);   //测试显示中文：下限
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,6,i+28,0);   //测试显示中文：上限
				OLED_ShowChar(34,4,':',2,0);
				OLED_ShowChar(34,6,':',2,0);
		    }
		   if(set_value==10)
			{
				for(i=0;i<4;i++)
				  OLED_ShowCN(i*16+32,0,i+30,0);//测试显示中文：设置血氧
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,4,i+26,0);   //测试显示中文：下限
				OLED_ShowStr(0,6,"                ", 2);
			}
			if(set_value==11)
			{
				for(i=0;i<4;i++)
				  OLED_ShowCN(i*16+32,0,i+34,0);//测试显示中文：设置温度
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,4,i+26,0);   //测试显示中文：下限
				for(i=0;i<2;i++)
				  OLED_ShowCN(i*16,6,i+28,0);   //测试显示中文：上限
				OLED_ShowChar(34,4,':',2,0);
				OLED_ShowChar(34,6,':',2,0);
			}
		}
				DisplaySetValue();
	   }
		if(keynum==2)//加
		{
		 if(set_value == 0 && page==1)
		 {
			if(pressCount<10)pressCount++;
					  
			if(press==0)
			  {
				press=1;
				BEEP=1;
				Delay_ms(100);
				BEEP=0;
				if(timer_remind_sign==1)  //如果是提醒时间到了，先关闭提醒
				 {
					timer_remind_sign=0;
					buzzer_alarm_flag&=0xEF;
				 }
				else
				 {
					start_timer_sign=!start_timer_sign;
				 }
			   }
			if(pressCount>=5)   //长按进行时间清零
			  {
				start_timer_sign=0;
				timecount_record=0;
				while(bufferCount<1000)
				{
					BEEP=1;
					bufferCount++;
					Delay_ms(1);
				}
				BEEP=0;
				pressCount=0;
			  }
			}
				if(page==1)
				{
						if(set_value == 1)//设置提醒时间的小时
						{
								hour ++;
								if(hour == 100)hour=0;
						}
						if(set_value == 2)//设置提醒时间的分钟
						{
								minute ++;
								if(minute == 60)minute=0;
						}
						if(set_value == 3)//设置提醒时间的秒
						{
								second ++;
								if(second == 60)second=0;
						}
						if(set_value == 4)//设置步长
						{
								if(step<200)step++;
						}
						if(set_value == 5)//设置提醒里程
						{
								if(mileage_alert<20000)mileage_alert+=10;
						}
				}
				else
				{
				  if(set_value == 1)//设置年
					{
						SysDate.year ++;
						if(SysDate.year == 100)SysDate.year=0;
						DS1302_DateSet(&SysDate);//设置时间
					}
						if(set_value == 2)//设置月
						{
								SysDate.mon ++;
								if(SysDate.mon == 13)SysDate.mon = 1;
								if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))
								{
										if(SysDate.day>30)SysDate.day=1;
								}
								else
								{
										if(SysDate.mon==2)
										{
												if(commondYear_leapYear_flag==1)
												{
														if(SysDate.day>29)
																SysDate.day=1;
												}
												else
												{
														if(SysDate.day>28)
																SysDate.day=1;
												}
										}
								}
								DS1302_DateSet(&SysDate);//设置时间
						}
						if(set_value == 3)//设置日
						{
								SysDate.day ++;
								if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))//大月
								{
										if(SysDate.day==32)//最大31天
												SysDate.day=1;//从1开始
								}
								else
								{
										if(SysDate.mon==2)//二月份
										{
												if(commondYear_leapYear_flag==1)//闰年
												{
														if(SysDate.day==30)//最大29天
																SysDate.day=1;
												}
												else
												{
														if(SysDate.day==29)//最大28天
																SysDate.day=1;
												}
										}
										else
										{
												if(SysDate.day==31)//最大30天
														SysDate.day=1;
										}
								}
								DS1302_DateSet(&SysDate);//设置时间
						}
						if(set_value == 4)//设置星期
						{
								SysDate.week ++;
								if(SysDate.week == 8)SysDate.week=1;
								DS1302_DateSet(&SysDate);//设置时间
						}
						if(set_value == 5)//设置时
						{
								SysDate.hour ++;
								if(SysDate.hour == 24)SysDate.hour=0;
								DS1302_DateSet(&SysDate);//设置时间
						}
						if(set_value == 6)//设置分
						{
								SysDate.min ++;
								if(SysDate.min == 60)SysDate.min=0;
								DS1302_DateSet(&SysDate);//设置时间
						}
						if(set_value == 7)//设置秒
						{
								SysDate.sec ++;
								if(SysDate.sec == 60)SysDate.sec=0;
								DS1302_DateSet(&SysDate);//设置时间
						}
						if((set_value==8)&&(heart_rate_Max-heart_rate_Min>1))heart_rate_Min++;
						if((set_value==9)&&(heart_rate_Max<300))heart_rate_Max++;
						if((set_value==10)&&(blood_oxygen_Lowlimit<200))blood_oxygen_Lowlimit++; 
						if((set_value==11)&&(tempMax-tempMin>1))tempMin++; 
						if((set_value==12)&&(tempMax<999))tempMax++; 
				}
				DisplaySetValue();
		}
		else
		{
			 if(press==1)
			 {
					press=0;
				  pressCount=0;
			 }
		}
		if(keynum==3)//减
		{
			  
			  if(page==1)
				{  
						if(set_value == 0)
						{
							  if(mileage_remind_sign==1)   //如果是提醒里程到了，先关闭提醒
							  {
										mileage_remind_sign=0;
									  BEEP=0;
								}
								else if(mileage_remind_sign==0)   //里程清零
								{
										mileage=0;
										mileage_stepcount=0;
										
										Store_Data(); //存储步数
								}
						}
						if(set_value == 1)//设置提醒时间的小时
						{
								if(hour == 0)hour=100;
							  hour --;
						}
						if(set_value == 2)//设置提醒时间的分钟
						{
								if(minute == 0)minute=60;
							  minute --;
						}
						if(set_value == 3)//设置提醒时间的秒
						{
								if(second == 0)second=60;
							  second --;
						}
						if(set_value == 4)//设置步长
						{
								if(step>0)step--;
						}
						if(set_value == 5)//设置提醒里程
						{
								if(mileage_alert>=10)mileage_alert-=10;
						}
				}
			  else
				{
						if(set_value == 1)//设置年
						{
								if(SysDate.year == 0)SysDate.year=100;
								SysDate.year --;
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 2)//设置月
						{
								if(SysDate.mon == 1)SysDate.mon=13;
								SysDate.mon --;
								if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))
								{
										if(SysDate.day>30)
												SysDate.day=1;
								}
								else
								{
										if(SysDate.mon==2)
										{
												if(commondYear_leapYear_flag==1)
												{
														if(SysDate.day>29)
																SysDate.day=1;
												}
												else
												{
														if(SysDate.day>28)
																SysDate.day=1;
												}
										}
								}
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 3)//设置日
						{
								SysDate.day --;
								if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))
								{
										if(SysDate.day==0)
												SysDate.day=31;
								}
								else
								{
										if(SysDate.mon==2)
										{
												if(commondYear_leapYear_flag==1)
												{
														if(SysDate.day==0)
																SysDate.day=29;
												}
												else
												{
														if(SysDate.day==0)
																SysDate.day=28;
												}
										}
										else
										{
												if(SysDate.day==0)
														SysDate.day=30;
										}
								}
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 4)//设置星期
						{
								if(SysDate.week == 1)SysDate.week=8;
								SysDate.week --;
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 5)//设置时
						{
								if(SysDate.hour == 0)SysDate.hour=24;
								SysDate.hour --;
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 6)//设置分
						{
								if(SysDate.min == 0)SysDate.min=60;
								SysDate.min --;
								DS1302_DateSet(&SysDate);
						}
						if(set_value == 7)//设置秒
						{
								if(SysDate.sec == 0)SysDate.sec=60;
								SysDate.sec --;
								DS1302_DateSet(&SysDate);
						}
						if((set_value==8)&&(heart_rate_Min>0))heart_rate_Min--;
						if((set_value==9)&&(heart_rate_Max-heart_rate_Min>1))heart_rate_Max--;
						if((set_value==10)&&(blood_oxygen_Lowlimit>0))blood_oxygen_Lowlimit--; 
						if((set_value==11)&&(tempMin>0))tempMin--; 
						if((set_value==12)&&(tempMax-tempMin>1))tempMax--; 
				}
				DisplaySetValue();
		}
		if(keynum==4&&page==0)//步数清零
		{
			 stepcount = 0;
			 Store_Data();
		}
		if(keynum==5 && set_value == 0)//切换显示界面
		{
			  page=!page;
				OLED_CLS();//清屏
				if(page==0)
				{
						for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+16,1);//测试显示中文：心率
						for(i=0;i<2;i++)OLED_ShowCN(i*16+48,4,i+18,1);//测试显示中文：血氧
						for(i=0;i<2;i++)OLED_ShowCN(i*16+95,4,i+20,1);//测试显示中文：步数
					  OLED_ShowCentigrade(112, 2);    //℃
				}
				else
				{
					  for(i=0;i<2;i++)OLED_ShowCN(i*16,2,i+44,0);//显示中文：里程
					  OLED_ShowChar(32,2,':',2,0);
				}
		}
}


void displayTimeCunt(void)  //显示定时时间
{
	  if(page==1)
		{
			  if(set_value==0)
				{
						OLED_ShowChar(32,0,timecount_record/3600/10+'0',2,0);
						OLED_ShowChar(40,0,timecount_record/3600%10+'0',2,0);
						OLED_ShowChar(48,0,':',2,0);
						OLED_ShowChar(56,0,timecount_record%3600/60/10+'0',2,0);
						OLED_ShowChar(64,0,timecount_record%3600/60%10+'0',2,0);
						OLED_ShowChar(72,0,':',2,0);
						OLED_ShowChar(80,0,timecount_record%3600%60/10+'0',2,0);
						OLED_ShowChar(88,0,timecount_record%3600%60%10+'0',2,0);
				}
				else
				{
					  if(set_value<4)
						{
								OLED_ShowChar(32,0,hour/10+'0',2,set_value+1-1);
								OLED_ShowChar(40,0,hour%10+'0',2,set_value+1-1);
								OLED_ShowChar(48,0,':',2,0);
								OLED_ShowChar(56,0,minute/10+'0',2,set_value+1-2);
								OLED_ShowChar(64,0,minute%10+'0',2,set_value+1-2);
								OLED_ShowChar(72,0,':',2,0);
								OLED_ShowChar(80,0,second/10+'0',2,set_value+1-3);
								OLED_ShowChar(88,0,second%10+'0',2,set_value+1-3);
						}
				}
		}
}
void UsartSendData(void)//串口发送数据
{
	  static u8 flag1=0,flag2=1;
	  float temp=0.0;  
	
	  if(flag1!=SysDate.sec)
		{
			  flag1 = SysDate.sec;
			  flag2 = !flag2;
			  if(flag2==1)//2秒发送一次数据
				{
						temp=(float)temperature/10; 
						printf("体温:%4.1fC  \r\n",temp);         //发送体温
						Delay_ms(10); 
						printf("心率:%d  \r\n",heart_rate);     //发送心率
						Delay_ms(10); 
					    printf("血氧:%d  \r\n",blood_oxygen_concentration);     //发送血氧
						Delay_ms(10); 
						printf("步数:%d     \r\n",stepcount);     //发送步数
						Delay_ms(10);
						printf("\r\n");
				}
		}
}

int main(void)
{
	unsigned char i;
	
	DelayInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	I2C_Configuration(); //IIC初始化
	OLED_Init(); //OLED初始化
	KEY_Init(); //按键初始化
	Delay_ms(200);
	CheckNewMcu();
    OLED_CLS();//清屏
	DS18B20_Init();
	DS1302_Init(&SysDate);
	uart1_Init(9600);   
	Delay_ms(100);
	for(i=0;i<8;i++)OLED_ShowCN(i*16,2,i+8,0);//测试显示中文：欢迎使用使能手环
	DS1302_DateRead(&SysDate);//读时间
	OLED_CLS();//清屏
	for(i=0;i<2;i++)OLED_ShowCN(i*16,4,i+16,1);//测试显示中文：心率
	for(i=0;i<2;i++)OLED_ShowCN(i*16+48,4,i+18,1);//测试显示中文：血氧
	for(i=0;i<2;i++)OLED_ShowCN(i*16+95,4,i+20,1);//测试显示中文：步数
	OLED_ShowCentigrade(112, 2);    //℃
	IIC_init();//IIC初始化
	adxl345_init();//ADXL345初始化
	Init_MAX30102();//MAX30102初始化
	TIM2_Init(99,719);   //定时器初始化，定时1ms
	while(1)
	{
		  flashing=!flashing;
		  KeySettings();
	      DisplayTime();
		  displayTimeCunt();
		  if(set_value == 0)//不在设置状态下，读取相关数据
			{
					DisplayTemperature();
					GetSteps();
					GetHeartRateSpO2();
			}
			UsartSendData();
	}
}

void TIM2_IRQHandler(void)//定时器2中断服务程序，用于记录时间
{ 
	  static u16 timeCount1 = 0;
	  static u16 timeCount2 = 0;
	  static u16 timeCount3 = 0;
	  static u16 timeCount4 = 0;
	
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{ 
				TIM_ClearITPendingBit(TIM2, TIM_IT_Update); //清除中断标志位  
			
			  if(timer_remind_sign==1) //时间提醒
				{
						timeCount1 ++;
					  if(timeCount1==1)BEEP=1;
					  if(timeCount1==100)BEEP=0;
					  if(timeCount1>=1000)
						{
							timeCount1=0;
						}
				}
				else
				{
						timeCount1=0;
				}
			
				timeCount2 ++;
			
				if(timeCount2 >= 1000)   //1s
				{
						timeCount2 = 0;
					  if(start_timer_sign==1)
						{
								if(timecount_record<(98*3600+59*60))
								{
										timecount_record ++;          //时间计时
								}
								
								if(timecount_record==(hour*3600+minute*60+second))//时间到了提醒标志置1
								{
										 timer_remind_sign=1;
									   buzzer_alarm_flag|=0x10;
								}
						}
						sendFlag=1;
				}
				
			  timeCount3 ++;
				if(timeCount3 >= 100) 
				{
						timeCount3=0;

						if(((heart_rate!=0)&&(heart_rate>=heart_rate_Max||heart_rate<=heart_rate_Min))||((blood_oxygen_concentration!=0)&&(blood_oxygen_concentration<=blood_oxygen_Lowlimit))||(temperature>=tempMax||temperature<=tempMin))//不在范围蜂鸣器报警
						{
							BEEP=~BEEP;
							if((!(buzzer_alarm_flag&0x02))&&((heart_rate!=0)&&(heart_rate>=heart_rate_Max||heart_rate<=heart_rate_Min)))//心率报警
							{
									buzzer_alarm_flag|=0x02;
							}
							if((!(buzzer_alarm_flag&0x04))&&((blood_oxygen_concentration!=0)&&(blood_oxygen_concentration<=blood_oxygen_Lowlimit)))//血氧报警
							{
									buzzer_alarm_flag|=0x04;
							}
							if((!(buzzer_alarm_flag&0x08))&&(temperature>=tempMax||temperature<=tempMin))//温度报警
							{
									buzzer_alarm_flag|=0x08;
							}
							
						}else
						{	
							buzzer_alarm_flag&=0xF1;
							
							BEEP=0;
							if(mileage>=mileage_alert)
							{
									if((buzzer_alarm_flag>>5)==0)
									{
											buzzer_alarm_flag|=0x20;
											mileage_remind_sign=1;
									}
							}
							else
							{
									buzzer_alarm_flag&=0xDF;
									mileage_remind_sign=0;
							}
						}
				}
				
				timeCount4 ++;
				if(timeCount4>=50)
				{
					  static u8 bufferCount=0;
					
						timeCount4=0;
					  
						if(mileage_remind_sign==1)   //里程到了蜂鸣器提醒
						{
								bufferCount++;
								if(bufferCount==3)BEEP=1;
								if(bufferCount==6)BEEP=0; 
								if(bufferCount==9)BEEP=1;
								if(bufferCount==12)BEEP=0;
								if(bufferCount==15)BEEP=1;
								if(bufferCount==18)BEEP=0; 
						}
				}
		}
}
