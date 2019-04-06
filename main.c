#include <msp430.h> 
#include <stdint.h>

#define     ADS_ADDR        0x40
#define     CMD_RESET       0x06
#define     CMD_START       0x08
#define     CMD_PD          0x02
#define     CMD_RDATA       0x10
#define     CMD_RREG        0x20
#define     CMD_WREG        0x40

/*
 * @fn:     void InitSystemClock(void)
 * @brief:  初始化系统时钟
 * @para:   none
 * @return: none
 * @comment:初始化系统时钟
 */
void InitSystemClock(void)
{
    /*配置DCO为1MHz*/
    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    /*配置SMCLK的时钟源为DCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK的分频系数置为1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
#if 0
    /*配置LFXT晶振电容*/
    BCSCTL3 |= XCAP_3;
    /*延时一段时间等待振荡稳定*/
    __delay_cycles(50000);
    /*清除晶振错误标志*/
    BCSCTL3 &= ~LFXT1OF;
    IFG1 &= ~OFIFG;
    /*错误标志仍然存在？*/
    while(LFXT1OF&BCSCTL3);
    while(IFG1 & OFIFG);

    /*默认ACLK就是LFXT1*/

    /*输出ACLK*/
    //P1SEL |= BIT0;
    //P1DIR |= BIT0;
#endif
}
/*
 * @fn:     void initI2C(void)
 * @brief:  初始化i2c bus
 * @para:   none
 * @return: none
 * @comment:none
 */
void initI2C(void)
{
    /*设置IO复用*/
    P1SEL |= BIT6 | BIT7;
    P1SEL2 |= BIT6 | BIT7;
    /*禁能USCI模块*/
    UCB0CTL1 |= UCSWRST;
    /*I2C Master*/
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;
    /*clock from SMCLK*/
    UCB0CTL1 = UCSWRST + UCSSEL_2;
    /*set tx speed*/
    UCB0BR0 = 32;
    UCB0BR1 = 0;
    /*设置slave 地址*/
    UCB0I2CSA = ADS_ADDR;
    /*使能I2C*/
    UCB0CTL1 &= ~UCSWRST;
}
/*
 * @fn: uint8_t i2cSend(uint8_t slaveaddr,uint8_t *ptr,uint8_t byteno)
 * @brief:  none
 * @para:   none
 * @return: none
 * @comment:none
 */
uint8_t i2cSend(uint8_t slaveaddr,uint8_t *ptr,uint8_t byteno)
{
    uint16_t timeout = 50000;

    do{
        timeout --;//在规定的超时时间内等待总线空闲
    }while((UCB0STAT & UCBBUSY) & (timeout != 0));

    if(timeout == 0){//超时返回
        while(1);
    }
    UCB0I2CSA = slaveaddr;//设置slave地址

    UCB0CTL1 |= UCTR;//transmitter模式

    UCB0CTL1 |= UCTXSTT;//发送start

    do{//连续发送数据
        while(!(IFG2 & UCB0TXIFG));//上一Byte是否发送完成？
        UCB0TXBUF = *ptr;
        ptr ++;
    }while(--byteno);
    while(!(IFG2 & UCB0TXIFG));//等待最后一个Byte发送完成

    UCB0CTL1 |= UCTXSTP;//发送stop

    __delay_cycles(500);//适当延时，可有可无

    return 0;
}

/*
 * @fn: uint8_t i2cRead(uint8_t slaveaddr,uint8_t *tx_ptr,uint8_t *rx_ptr,uint8_t tx_no,uint8_t rx_no)
 * @brief:  利用I2C Bus读取数据
 * @para:   slaveaddr：从机地址
 *          tx_ptr：读之前需要发送的命令，例如被读从机的寄存器地址
 *          rx_ptr：读出数据缓冲区
 *          tx_no：发送命令的长度
 *          rx_no：接收数据的长度
 * @return: 读成功返回0，否则不为0
 * @comment:none
 */
uint8_t i2cRead(uint8_t slaveaddr,uint8_t *tx_ptr,uint8_t *rx_ptr,uint8_t tx_no,uint8_t rx_no)
{
    uint16_t timeout = 50000;

    do{
        timeout --;//在规定的超时时间内等待总线空闲
    }while((UCB0STAT & UCBBUSY) & (timeout != 0));

    if(timeout == 0){//超时返回
        return 1;
    }

    UCB0I2CSA = slaveaddr;//设置slave地址

    UCB0CTL1 |= UCTR | UCTXSTT;//transmitter模式，发送start
    do{//连续发送数据
        while(!(IFG2 & UCB0TXIFG));//上一Byte是否发送完成？
        UCB0TXBUF = *tx_ptr;
        tx_ptr ++;
    }while(--tx_no);
    while(!(IFG2 & UCB0TXIFG));//等待最后一个Byte发送完成

    __delay_cycles(10);

    UCB0CTL1 &= ~UCTR;//receiver模式

    UCB0CTL1 |= UCTXSTT;//发送start

    do{//连续接收数据
        if(rx_no == 1){//如果是最后一个数据，要在此处发送STOP，NACK也会自动发送
            while(UCB0CTL1 & UCTXSTT);
            UCB0CTL1 |= UCTXSTP;
        }
        while(!(IFG2 & UCB0RXIFG));//完成一次接收？
        *rx_ptr = UCB0RXBUF;
        rx_ptr ++;
    }while(--rx_no);

    __delay_cycles(1000);//适当延时，可有可无
    return 0;
}
/*
 * @fn:     void adsSoftwareRST(void)
 * @brief:  软件复位ADS
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsSoftwareRST(void)
{
    uint8_t t_cmd[1] = {CMD_RESET};
    i2cSend(ADS_ADDR,t_cmd,1);
}
/*
 * @fn:     void adsStartCONV(void)
 * @brief:  开始转换
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsStartCONV(void)
{
    uint8_t t_cmd[1] = {CMD_START};
    i2cSend(ADS_ADDR,t_cmd,1);
}
/*
 * @fn:     void adsPowerDown(void)
 * @brief:  进入掉电模式
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsPowerDown(void)
{
    uint8_t t_cmd[1] = {CMD_PD};
    i2cSend(ADS_ADDR,t_cmd,1);
}
/*
 * @fn:     void adsReadDATA(uint8_t *ptr)
 * @brief:  读转换结果
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsReadDATA(uint8_t *ptr)
{
    uint8_t t_cmd[1] = {CMD_RDATA};
    i2cRead(ADS_ADDR,t_cmd,ptr,1,2);
}
/*
 * @fn:     void adsReadREG(uint8_t reg_addr,uint8_t *ptr)
 * @brief:  读寄存器值
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsReadREG(uint8_t reg_addr,uint8_t *ptr)
{
    uint8_t t_cmd[1] = {CMD_RREG};
    t_cmd[0] |= (reg_addr << 2);
    i2cRead(ADS_ADDR,t_cmd,ptr,1,1);
}
/*
 * @fn:     void adsWriteREG(uint8_t reg_addr,uint8_t data)
 * @brief:  写寄存器值
 * @para:   none
 * @return: none
 * @comment:none
 */
void adsWriteREG(uint8_t reg_addr,uint8_t data)
{
    uint8_t t_cmd[2] = {CMD_WREG,0x00};
    t_cmd[0] |= (reg_addr << 2);
    t_cmd[1] = data;
    i2cSend(ADS_ADDR,t_cmd,2);
}
/*
 * @fn:     void hardwareResetADS(void)
 * @brief:  硬件复位ADS
 * @para:   none
 * @return: none
 * @comment:none
 */
void hardwareResetADS(void)
{
    /*拉低RESET进行复位*/
    P1OUT &= ~BIT4;
    /*延时一段时间*/
    __delay_cycles(10000);
    /*释放RESET，复位完成*/
    P1OUT |= BIT4;
}
/*
 * @fn:     void initADS(void)
 * @brief:  初始化ADS
 * @para:   none
 * @return: none
 * @comment:none
 */
void initADS(void)
{
    /*设置RESET所连端口P1.4为输出*/
    P1DIR |= BIT4;
    /*硬件复位ADS*/
    hardwareResetADS();
    /*软件复位ADS*/
    adsSoftwareRST();
    //__delay_cycles(1000);
    /*设置寄存器00*/
    adsWriteREG(0x00,0xD1);
    /*设置寄存器01*/
    adsWriteREG(0x01,0xD8);
    /*设置寄存器02*/
    //adsWriteREG(0x02,0x00);
}
/**
 * main.c
 */
int main(void)
{
    uint8_t data[2] = {0,0};
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	InitSystemClock();
	initI2C();
	initADS();
	__delay_cycles(3000);
	adsReadREG(0x00,data);//读刚才设置的寄存器，确认写入成功
	adsReadREG(0x01,data);//读刚才设置的寄存器，确认写入成功
	__delay_cycles(3000);
	adsStartCONV();//开始转换
	__delay_cycles(10000);
	adsReadREG(0x02,data);//读此寄存器可以判断数据是否转换完毕
	adsReadREG(0x02,data);//读此寄存器可以判断数据是否转换完毕
	adsReadREG(0x02,data);//读此寄存器可以判断数据是否转换完毕
	adsReadREG(0x02,data);//读此寄存器可以判断数据是否转换完毕
	adsReadDATA(data);//读数据
	while(1)
	{
	    __delay_cycles(1000000);
	}
	return 0;
}
