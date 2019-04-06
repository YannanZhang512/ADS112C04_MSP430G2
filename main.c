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
 * @brief:  ��ʼ��ϵͳʱ��
 * @para:   none
 * @return: none
 * @comment:��ʼ��ϵͳʱ��
 */
void InitSystemClock(void)
{
    /*����DCOΪ1MHz*/
    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    /*����SMCLK��ʱ��ԴΪDCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK�ķ�Ƶϵ����Ϊ1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
#if 0
    /*����LFXT�������*/
    BCSCTL3 |= XCAP_3;
    /*��ʱһ��ʱ��ȴ����ȶ�*/
    __delay_cycles(50000);
    /*�����������־*/
    BCSCTL3 &= ~LFXT1OF;
    IFG1 &= ~OFIFG;
    /*�����־��Ȼ���ڣ�*/
    while(LFXT1OF&BCSCTL3);
    while(IFG1 & OFIFG);

    /*Ĭ��ACLK����LFXT1*/

    /*���ACLK*/
    //P1SEL |= BIT0;
    //P1DIR |= BIT0;
#endif
}
/*
 * @fn:     void initI2C(void)
 * @brief:  ��ʼ��i2c bus
 * @para:   none
 * @return: none
 * @comment:none
 */
void initI2C(void)
{
    /*����IO����*/
    P1SEL |= BIT6 | BIT7;
    P1SEL2 |= BIT6 | BIT7;
    /*����USCIģ��*/
    UCB0CTL1 |= UCSWRST;
    /*I2C Master*/
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;
    /*clock from SMCLK*/
    UCB0CTL1 = UCSWRST + UCSSEL_2;
    /*set tx speed*/
    UCB0BR0 = 32;
    UCB0BR1 = 0;
    /*����slave ��ַ*/
    UCB0I2CSA = ADS_ADDR;
    /*ʹ��I2C*/
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
        timeout --;//�ڹ涨�ĳ�ʱʱ���ڵȴ����߿���
    }while((UCB0STAT & UCBBUSY) & (timeout != 0));

    if(timeout == 0){//��ʱ����
        while(1);
    }
    UCB0I2CSA = slaveaddr;//����slave��ַ

    UCB0CTL1 |= UCTR;//transmitterģʽ

    UCB0CTL1 |= UCTXSTT;//����start

    do{//������������
        while(!(IFG2 & UCB0TXIFG));//��һByte�Ƿ�����ɣ�
        UCB0TXBUF = *ptr;
        ptr ++;
    }while(--byteno);
    while(!(IFG2 & UCB0TXIFG));//�ȴ����һ��Byte�������

    UCB0CTL1 |= UCTXSTP;//����stop

    __delay_cycles(500);//�ʵ���ʱ�����п���

    return 0;
}

/*
 * @fn: uint8_t i2cRead(uint8_t slaveaddr,uint8_t *tx_ptr,uint8_t *rx_ptr,uint8_t tx_no,uint8_t rx_no)
 * @brief:  ����I2C Bus��ȡ����
 * @para:   slaveaddr���ӻ���ַ
 *          tx_ptr����֮ǰ��Ҫ���͵�������类���ӻ��ļĴ�����ַ
 *          rx_ptr���������ݻ�����
 *          tx_no����������ĳ���
 *          rx_no���������ݵĳ���
 * @return: ���ɹ�����0������Ϊ0
 * @comment:none
 */
uint8_t i2cRead(uint8_t slaveaddr,uint8_t *tx_ptr,uint8_t *rx_ptr,uint8_t tx_no,uint8_t rx_no)
{
    uint16_t timeout = 50000;

    do{
        timeout --;//�ڹ涨�ĳ�ʱʱ���ڵȴ����߿���
    }while((UCB0STAT & UCBBUSY) & (timeout != 0));

    if(timeout == 0){//��ʱ����
        return 1;
    }

    UCB0I2CSA = slaveaddr;//����slave��ַ

    UCB0CTL1 |= UCTR | UCTXSTT;//transmitterģʽ������start
    do{//������������
        while(!(IFG2 & UCB0TXIFG));//��һByte�Ƿ�����ɣ�
        UCB0TXBUF = *tx_ptr;
        tx_ptr ++;
    }while(--tx_no);
    while(!(IFG2 & UCB0TXIFG));//�ȴ����һ��Byte�������

    __delay_cycles(10);

    UCB0CTL1 &= ~UCTR;//receiverģʽ

    UCB0CTL1 |= UCTXSTT;//����start

    do{//������������
        if(rx_no == 1){//��������һ�����ݣ�Ҫ�ڴ˴�����STOP��NACKҲ���Զ�����
            while(UCB0CTL1 & UCTXSTT);
            UCB0CTL1 |= UCTXSTP;
        }
        while(!(IFG2 & UCB0RXIFG));//���һ�ν��գ�
        *rx_ptr = UCB0RXBUF;
        rx_ptr ++;
    }while(--rx_no);

    __delay_cycles(1000);//�ʵ���ʱ�����п���
    return 0;
}
/*
 * @fn:     void adsSoftwareRST(void)
 * @brief:  �����λADS
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
 * @brief:  ��ʼת��
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
 * @brief:  �������ģʽ
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
 * @brief:  ��ת�����
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
 * @brief:  ���Ĵ���ֵ
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
 * @brief:  д�Ĵ���ֵ
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
 * @brief:  Ӳ����λADS
 * @para:   none
 * @return: none
 * @comment:none
 */
void hardwareResetADS(void)
{
    /*����RESET���и�λ*/
    P1OUT &= ~BIT4;
    /*��ʱһ��ʱ��*/
    __delay_cycles(10000);
    /*�ͷ�RESET����λ���*/
    P1OUT |= BIT4;
}
/*
 * @fn:     void initADS(void)
 * @brief:  ��ʼ��ADS
 * @para:   none
 * @return: none
 * @comment:none
 */
void initADS(void)
{
    /*����RESET�����˿�P1.4Ϊ���*/
    P1DIR |= BIT4;
    /*Ӳ����λADS*/
    hardwareResetADS();
    /*�����λADS*/
    adsSoftwareRST();
    //__delay_cycles(1000);
    /*���üĴ���00*/
    adsWriteREG(0x00,0xD1);
    /*���üĴ���01*/
    adsWriteREG(0x01,0xD8);
    /*���üĴ���02*/
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
	adsReadREG(0x00,data);//���ղ����õļĴ�����ȷ��д��ɹ�
	adsReadREG(0x01,data);//���ղ����õļĴ�����ȷ��д��ɹ�
	__delay_cycles(3000);
	adsStartCONV();//��ʼת��
	__delay_cycles(10000);
	adsReadREG(0x02,data);//���˼Ĵ��������ж������Ƿ�ת�����
	adsReadREG(0x02,data);//���˼Ĵ��������ж������Ƿ�ת�����
	adsReadREG(0x02,data);//���˼Ĵ��������ж������Ƿ�ת�����
	adsReadREG(0x02,data);//���˼Ĵ��������ж������Ƿ�ת�����
	adsReadDATA(data);//������
	while(1)
	{
	    __delay_cycles(1000000);
	}
	return 0;
}
