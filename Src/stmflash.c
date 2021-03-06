#include "stmflash.h"
#include "delay.h"
#include "usart.h"
#include "mpu6050.h"
#include "control.h"
#include "ANO_DT.h"
#include "HAL.h"
#include "nrf24l01.h"

#define EE_6050_ACC_X_OFFSET_ADDR		0
#define EE_6050_ACC_Y_OFFSET_ADDR		1
#define EE_6050_ACC_Z_OFFSET_ADDR		2
#define EE_6050_GYRO_X_OFFSET_ADDR		3
#define EE_6050_GYRO_Y_OFFSET_ADDR		4
#define EE_6050_GYRO_Z_OFFSET_ADDR		5
#define EE_PID_RATE_ROLL_P				6
#define EE_PID_RATE_ROLL_I				7
#define EE_PID_RATE_ROLL_D				8
#define EE_PID_RATE_PIT_P				9
#define EE_PID_RATE_PIT_I				10
#define EE_PID_RATE_PIT_D				11
#define EE_PID_RATE_YAW_P				12
#define EE_PID_RATE_YAW_I				13
#define EE_PID_RATE_YAW_D				14
#define EE_PID_ANGLE_ROLL_P				15
#define EE_PID_ANGLE_ROLL_I				16
#define EE_PID_ANGLE_ROLL_D				17
#define EE_PID_ANGLE_PIT_P				18
#define EE_PID_ANGLE_PIT_I				19
#define EE_PID_ANGLE_PIT_D				20
#define EE_PID_ANGLE_YAW_P				21
#define EE_PID_ANGLE_YAW_I				22
#define EE_PID_ANGLE_YAW_D				23
#define EE_MAG_X_OFFSET_ADDR			24
#define EE_MAG_Y_OFFSET_ADDR			25
#define EE_MAG_Z_OFFSET_ADDR			26
#define EE_RC_ARRD_AND_MATCHED			27
//#define EE_RC_MATCHED_SHOW_IN_PID18_D	28

u32 VirtAddVarTab[] = {
		0x1EE00, 0x1EE02, 0x1EE04,	//acc_offset(XYZ)
		0x1EE06, 0x1EE08, 0x1EE0A,	//gyro_offset(XYZ)
		0x1EE1C, 0x1EE1E, 0x1EE20,	//rate_rol(P I D)
		0x1EE22, 0x1EE24, 0x1EE26,	//rate_pit(P I D)
		0x1EE28, 0x1EE2A, 0x1EE2C,	//rate_yaw(P I D)
		0x1EE2E, 0x1EE30, 0x1EE32,	//ang_rol(P I D)
		0x1EE34, 0x1EE36, 0x1EE38,	//ang_pit(P I D)
		0x1EE3A, 0x1EE3C, 0x1EE3E,	//ang_yaw(P I D)
		0x1EE40, 0x1EE42, 0x1EE44,	//mag_offset(XYZ)
		0x1EE46,					//高位是rc_mached,低位是RX_ARRD[4]
};

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//STM32 FLASH 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2013/7/27
//版本：V1.1
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////	
//********************************************************************************
//V1.1修改说明
//修正了STMFLASH_Write函数地址偏移的一个bug.
//////////////////////////////////////////////////////////////////////////////////

																		 
/*******************************************************************************
*FLASH as EEPROM
*Fuction
*
*
********************************************************************************/

void Para_ResetToFactorySetup()
{
	roll_rate_PID.P  = 0.68; roll_rate_PID.I  = 0.15; roll_rate_PID.D  = 0.010;

	pitch_rate_PID.P = 0.68 ;pitch_rate_PID.I = 0.15; pitch_rate_PID.D = 0.010;

	yaw_rate_PID.P   = 8.0; yaw_rate_PID.I   = 0.8; yaw_rate_PID.D   = 0;

	roll_angle_PID.P  = 3.5; roll_angle_PID.I  = 0.00; roll_angle_PID.D  = 0.098;

	pitch_angle_PID.P = 3.5 ;pitch_angle_PID.I = 0.00; pitch_angle_PID.D = 0.098;

	yaw_angle_PID.P   = 0; yaw_angle_PID.I   = 0; yaw_angle_PID.D   = 0;


	EE_SAVE_PID();
	f.send_pid1 = 1;
	f.send_pid2 = 1;
//	f.send_pid3 = 1;
//	f.send_pid4 = 1;
//	f.send_pid5 = 1;
	f.send_pid6 = 1;

}


void EE_SAVE_RC_ADDR_AND_MATCHED()
{
	int16_t temp;

	temp = ((int16_t)rc_matched << 8) | RX_ADDRESS[4];
	EE_WriteVariable(VirtAddVarTab[EE_RC_ARRD_AND_MATCHED], temp);
}

void EE_READ_RC_ADDR_AND_MATCHED()
{
	int16_t temp;

	EE_ReadVariable(VirtAddVarTab[EE_RC_ARRD_AND_MATCHED], &temp);
	rc_matched = (uint8_t)(temp >> 8);
	RX_ADDRESS[4] = (uint8_t)(temp & 0xff);
}

void Param_SavePID()
{
	EE_SAVE_PID();
}

void EE_SAVE_ACC_OFFSET(void)
{
	EE_WriteVariable(VirtAddVarTab[EE_6050_ACC_X_OFFSET_ADDR], (int16_t)(imu.accOffset[X] * 1000));
	EE_WriteVariable(VirtAddVarTab[EE_6050_ACC_Y_OFFSET_ADDR], (int16_t)(imu.accOffset[Y] * 1000));
	EE_WriteVariable(VirtAddVarTab[EE_6050_ACC_Z_OFFSET_ADDR], (int16_t)(imu.accOffset[Z] * 1000));
}
void EE_READ_ACC_OFFSET(void)
{
	int16_t acc_offset[3];

	EE_ReadVariable(VirtAddVarTab[EE_6050_ACC_X_OFFSET_ADDR], &acc_offset[0]);
	EE_ReadVariable(VirtAddVarTab[EE_6050_ACC_Y_OFFSET_ADDR], &acc_offset[1]);
	EE_ReadVariable(VirtAddVarTab[EE_6050_ACC_Z_OFFSET_ADDR], &acc_offset[2]);

	imu.accOffset[X] = acc_offset[0] / 1000.f;
	imu.accOffset[Y] = acc_offset[1] / 1000.f;
	imu.accOffset[Z] = acc_offset[2] / 1000.f;
}
void EE_SAVE_GYRO_OFFSET(void)
{
	EE_WriteVariable(VirtAddVarTab[EE_6050_GYRO_X_OFFSET_ADDR], (int16_t)(imu.gyroOffset[X] * 1000));
	EE_WriteVariable(VirtAddVarTab[EE_6050_GYRO_Y_OFFSET_ADDR], (int16_t)(imu.gyroOffset[Y] * 1000));
	EE_WriteVariable(VirtAddVarTab[EE_6050_GYRO_Z_OFFSET_ADDR], (int16_t)(imu.gyroOffset[Z] * 1000));
}
void EE_READ_GYRO_OFFSET(void)
{
	int16_t gyro_offset[3];

	EE_ReadVariable(VirtAddVarTab[EE_6050_GYRO_X_OFFSET_ADDR], &gyro_offset[0]);
	EE_ReadVariable(VirtAddVarTab[EE_6050_GYRO_Y_OFFSET_ADDR], &gyro_offset[1]);
	EE_ReadVariable(VirtAddVarTab[EE_6050_GYRO_Z_OFFSET_ADDR], &gyro_offset[2]);

	imu.gyroOffset[X] = gyro_offset[0] / 1000.f;
	imu.gyroOffset[Y] = gyro_offset[1] / 1000.f;
	imu.gyroOffset[Z] = gyro_offset[2] / 1000.f;
}

void EE_SAVE_MAG_OFFSET(void)
{
	EE_WriteVariable(VirtAddVarTab[EE_MAG_X_OFFSET_ADDR], imu.magOffset[X]);
	EE_WriteVariable(VirtAddVarTab[EE_MAG_Y_OFFSET_ADDR], imu.magOffset[Y]);
	EE_WriteVariable(VirtAddVarTab[EE_MAG_Z_OFFSET_ADDR], imu.magOffset[Z]);

}

void EE_READ_MAG_OFFSET(void)
{
	EE_ReadVariable(VirtAddVarTab[EE_MAG_X_OFFSET_ADDR], &imu.magOffset[X]);
	EE_ReadVariable(VirtAddVarTab[EE_MAG_Y_OFFSET_ADDR], &imu.magOffset[Y]);
	EE_ReadVariable(VirtAddVarTab[EE_MAG_Z_OFFSET_ADDR], &imu.magOffset[Z]);
}


void EE_SAVE_PID(void)
{
	u16 _temp;
	_temp = (uint16_t)(roll_rate_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_ROLL_P],_temp);
	_temp = (uint16_t)(roll_rate_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_ROLL_I],_temp);
	_temp = (uint16_t)(roll_rate_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_ROLL_D],_temp);
	_temp = (uint16_t)(pitch_rate_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_PIT_P],_temp);
	_temp = (uint16_t)(pitch_rate_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_PIT_I],_temp);
	_temp = (uint16_t)(pitch_rate_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_PIT_D],_temp);
	_temp = (uint16_t)(yaw_rate_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_YAW_P],_temp);
	_temp = (uint16_t)(yaw_rate_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_YAW_I],_temp);
	_temp = (uint16_t)(yaw_rate_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_RATE_YAW_D],_temp);

	_temp = (uint16_t)(roll_angle_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_P],_temp);
	_temp = (uint16_t)(roll_angle_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_I],_temp);
	_temp = (uint16_t)(roll_angle_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_D],_temp);
	_temp = (uint16_t)(pitch_angle_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_P],_temp);
	_temp = (uint16_t)(pitch_angle_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_I],_temp);
	_temp = (uint16_t)(pitch_angle_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_D],_temp);
	_temp = (uint16_t)(yaw_angle_PID.P * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_P],_temp);
	_temp = (uint16_t)(yaw_angle_PID.I * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_I],_temp);
	_temp = (uint16_t)(yaw_angle_PID.D * 1000);
	EE_WriteVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_D],_temp);
}
void EE_READ_PID(void)
{
	u16 _temp;

	roll_rate_PID.iLimit = 300;
	pitch_rate_PID.iLimit = 300;
	yaw_rate_PID.iLimit = 300;
	roll_angle_PID.iLimit = 300;
	pitch_angle_PID.iLimit = 300;
	yaw_angle_PID.iLimit = 300;

	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_ROLL_P],&_temp);
	roll_rate_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_ROLL_I],&_temp);
	roll_rate_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_ROLL_D],&_temp);
	roll_rate_PID.D = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_PIT_P],&_temp);
	pitch_rate_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_PIT_I],&_temp);
	pitch_rate_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_PIT_D],&_temp);
	pitch_rate_PID.D = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_YAW_P],&_temp);
	yaw_rate_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_YAW_I],&_temp);
	yaw_rate_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_RATE_YAW_D],&_temp);
	yaw_rate_PID.D = (float)_temp / 1000;

	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_P],&_temp);
	roll_angle_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_I],&_temp);
	roll_angle_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_ROLL_D],&_temp);
	roll_angle_PID.D = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_P],&_temp);
	pitch_angle_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_I],&_temp);
	pitch_angle_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_PIT_D],&_temp);
	pitch_angle_PID.D = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_P],&_temp);
	yaw_angle_PID.P = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_I],&_temp);
	yaw_angle_PID.I = (float)_temp / 1000;
	EE_ReadVariable(VirtAddVarTab[EE_PID_ANGLE_YAW_D],&_temp);
	yaw_angle_PID.D = (float)_temp / 1000;

	f.send_pid1 = 1;
	f.send_pid2 = 1;
}

u8 EE_WriteVariable(u32 addr,u16 data)
{
	STMFLASH_Write(STM32_FLASH_BASE+(u32)addr,&data,2);
	return 0;
}

u8 EE_ReadVariable(u32 addr,u16* data)
{
	STMFLASH_Read(STM32_FLASH_BASE+(u32)addr,data,2);
	return 0;
}

//解锁STM32的FLASH
void STMFLASH_Unlock(void)
{
  FLASH->KEYR=MY_FLASH_KEY1;//写入解锁序列.
  FLASH->KEYR=MY_FLASH_KEY2;
}
//flash上锁
void STMFLASH_Lock(void)
{
  FLASH->CR|=1<<7;//上锁
}
//得到FLASH状态
u8 STMFLASH_GetStatus(void)
{	
	u32 res;		
	res=FLASH->SR; 
	if(res&(1<<0))return 1;		    //忙
	else if(res&(1<<2))return 2;	//编程错误
	else if(res&(1<<4))return 3;	//写保护错误
	return 0;						//操作完成
}
//等待操作完成
//time:要延时的长短
//返回值:状态.
u8 STMFLASH_WaitDone(u16 time)
{
	u8 res;
	do
	{
		res=STMFLASH_GetStatus();
		if(res!=1)break;//非忙,无需等待了,直接退出.
		delay_us(1);
		time--;
	 }while(time);
	 if(time==0)res=0xff;//TIMEOUT
	 return res;
}
//擦除页
//paddr:页地址
//返回值:执行情况
u8 STMFLASH_ErasePage(u32 paddr)
{
	u8 res=0;
	res=STMFLASH_WaitDone(0X5FFF);//等待上次操作结束,>20ms    
	if(res==0)
	{ 
		FLASH->CR|=1<<1;//页擦除
		FLASH->AR=paddr;//设置页地址 
		FLASH->CR|=1<<6;//开始擦除		  
		res=STMFLASH_WaitDone(0X5FFF);//等待操作结束,>20ms  
		if(res!=1)//非忙
		{
			FLASH->CR&=~(1<<1);//清除页擦除标志.
		}
	}
	return res;
}
//在FLASH指定地址写入半字
//faddr:指定地址(此地址必须为2的倍数!!)
//dat:要写入的数据
//返回值:写入的情况
u8 STMFLASH_WriteHalfWord(u32 faddr, u16 dat)
{
	u8 res;	   	    
	res=STMFLASH_WaitDone(0XFF);	 
	if(res==0)//OK
	{
		FLASH->CR|=1<<0;//编程使能
		*(vu16*)faddr=dat;//写入数据
		res=STMFLASH_WaitDone(0XFF);//等待操作完成
		if(res!=1)//操作成功
		{
			FLASH->CR&=~(1<<0);//清除PG位.
		}
	} 
	return res;
} 
//读取指定地址的半字(16位数据) 
//faddr:读地址 
//返回值:对应数据.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}
#if STM32_FLASH_WREN	//如果使能了写   
//不检查的写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数   
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i=0;i<NumToWrite;i++)
	{
		STMFLASH_WriteHalfWord(WriteAddr,pBuffer[i]);
	    WriteAddr+=2;//地址增加2.
	}  
} 
//从指定地址开始写入指定长度的数据
//WriteAddr:起始地址(此地址必须为2的倍数!!)
//pBuffer:数据指针
//NumToWrite:半字(16位)数(就是要写入的16位数据的个数.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //字节
#else 
#define STM_SECTOR_SIZE	2048
#endif		 
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//最多是2K字节
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)	
{
	u32 secpos;	   //扇区地址
	u16 secoff;	   //扇区内偏移地址(16位字计算)
	u16 secremain; //扇区内剩余地址(16位字计算)	   
 	u16 i;    
	u32 offaddr;   //去掉0X08000000后的地址
	if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE))){
		SendChar("error flash\r\n");
		return;//非法地址
	}
	STMFLASH_Unlock();						//解锁
	offaddr=WriteAddr-STM32_FLASH_BASE;		//实际偏移地址.
	secpos=offaddr/STM_SECTOR_SIZE;			//扇区地址  0~127 for STM32F103RBT6
	secoff=(offaddr%STM_SECTOR_SIZE)/2;		//在扇区内的偏移(2个字节为基本单位.)
	secremain=STM_SECTOR_SIZE/2-secoff;		//扇区剩余空间大小   
	if(NumToWrite<=secremain)secremain=NumToWrite;//不大于该扇区范围
	while(1) 
	{	
		STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			STMFLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//擦除这个扇区
			for(i=0;i<secremain;i++)//复制
			{
				STMFLASH_BUF[i+secoff]=pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//写入整个扇区  
		}else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;				//扇区地址增1
			secoff=0;				//偏移位置为0 	 
		   	pBuffer+=secremain;  	//指针偏移
			WriteAddr+=secremain*2;	//写地址偏移(16位数据地址,需要*2)	   
		   	NumToWrite-=secremain;	//字节(16位)数递减
			if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//下一个扇区还是写不完
			else secremain=NumToWrite;//下一个扇区可以写完了
		}	 
	};	
	STMFLASH_Lock();//上锁
}
#endif

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

//////////////////////////////////////////测试用///////////////////////////////////////////
//WriteAddr:起始地址
//WriteData:要写入的数据
void Test_Write(u32 WriteAddr,u16 WriteData)   	
{
	STMFLASH_Write(WriteAddr,&WriteData,1);//写入一个字 
}
















