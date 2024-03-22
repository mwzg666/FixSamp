#include "main.h"

#ifdef IRDA_FUN

#define IRDA_IO_SIM   // IOģ�����

BYTE IrDAStart = 0;

WORD IrDACounter = 0;
BYTE  IrDABitCnt = 0;
WORD IrDATimer = 0;
u8  RX2_Cnt;    //���ռ���
bit B_TX2_Busy; //����æ��־


#define Baudrate2   9600UL
#define UART2_BUF_LENGTH    128


u8  RX2_Buffer[MAX_LENGTH]; //���ջ���


void IrDASend(u8 *buf, u8 len)
{
    u8 i;

    #ifdef IRDA_IO_SIM
    // ���ⷢ�͵�ʱ��رս���
    P1INTE &= ~1;
    DelayUs(100);
    #endif
    
    for (i=0; i<len;  i++)     
    {
        #ifndef IRDA_IO_SIM
        S2BUF = buf[i];
        B_TX2_Busy = 1;
        while(B_TX2_Busy);
        #else
        IrDASendByte(buf[i]);
        #endif
    }

    #ifdef IRDA_IO_SIM
    DelayUs(100);
    P1INTE |= 1;
    #endif
}

#ifndef IRDA_IO_SIM

void SetTimer2Baudraye(u32 dat)
{
    AUXR &= ~(1<<4);    //Timer stop
    AUXR &= ~(1<<3);    //Timer2 set As Timer
    AUXR |=  (1<<2);    //Timer2 set as 1T mode
    T2H = (u8)(dat / 256);
    T2L = (u8)(dat % 256);
    IE2  &= ~(1<<2);    //��ֹ�ж�
    AUXR |=  (1<<4);    //Timer2 run enable
}

void IrDAConfig()    
{
    SetTimer2Baudraye(65536UL - (MAIN_Fosc / 4) / Baudrate2);
    
    P_SW2 &= ~0x01; 
    //P_SW2 |= 1;       //UART2 switch to: 0: P1.0 P1.1,  1: P4.6 P4.7

    //UART2ģʽ, 0x00: ͬ����λ���,               0x40: 8λ����,�ɱ䲨����, 
    //           0x80: 9λ����,�̶�������, 0xc0: 9λ����,�ɱ䲨����
    S2CON = (S2CON & 0x3f) | 0x40;   
    S2CON |= (1<<4);    //��������

    //USART2CR2 |= (1<<7) | (1<<6); // ʹ��IrDA 
    //USART2CR3 = 72;
    //USART2BRL = 72;      // IrDA������
    //USART2BRH = 0;
    
    B_TX2_Busy = 0;
    RX2_Cnt = 0;

    
    IE2   |= 1;         //�����ж�
}


void UART2_int (void) interrupt 8
{
    if((S2CON & 1) != 0)
    {
        S2CON &= ~1;    //Clear Rx flag
        RX2_Buffer[RX2_Cnt] = S2BUF;
        if(++RX2_Cnt >= UART2_BUF_LENGTH)   RX2_Cnt = 0;

        IrDATimer = 0;
    }

    if((S2CON & 2) != 0)
    {
        S2CON &= ~2;    //Clear Tx flag
        B_TX2_Busy = 0;
    }
}
#endif

void ClearIrDABuf()
{
    IrDATimer = 0;
    memset(RX2_Buffer, 0xFF, UART2_BUF_LENGTH);
    RX2_Cnt = 0;
}

#ifdef IRDA_IO_SIM
void DelayUs(WORD t)
{
    while(t--)
    {
        ;
    }
}


void IrDA_0()
{
    IRDA_TX(1);
    DelayUs(20);
    IRDA_TX(0);
    DelayUs(84);
}


void IrDA_1()
{
    IRDA_TX(0);
    DelayUs(104);
}


void IrDASendByte(BYTE dat)
{
    BYTE i;
    
    IrDA_0();
    for (i=0;i<8;i++)
    {
        if (dat & (1<<i))
        {
            IrDA_1();
        }
        else
        {
            IrDA_0();
        }
    }
    IrDA_1();
}




void IrDARec()
{
    if (IrDAStart == 1)
    {
        if (IrDATimer > 20)
        {
            IrDAStart = 0;
            IrDATimer = 0;

            StopTimer3();

            #if 0
            memset(RX2_Buffer,0,UART2_BUF_LENGTH);
            sprintf(RX2_Buffer,"%lu", IrDACounter);
            Rs485Send(RX2_Buffer, strlen(RX2_Buffer));
            #else
            RX2_Cnt++; // ���һ���ֽ�û����ʼλ 

            //Rs485Send(RX2_Buffer, RX2_Cnt);

            // ���ֻ��һ���ֽڣ�һ���Ǹ��ţ��ֽڶ���
            if (RX2_Cnt > 1)
            {
                SendCmd(CMD_DATA_TT, RX2_Buffer, RX2_Cnt);
            }
            #endif
        }
        
    }
    
}



void IrDA_Init()
{
    IRDA_TX(0);
    IRDA_RX(1);
    
    // P1.0 -- �½�Ե�ж�
    P1IM0 = 0;
    P1IM1 = 0;

    // ���ȼ����
    PIN_IP  |= (1<<1);
    PIN_IPH |= (1<<1);

    // �����ж�
    P1INTE |= 1;
}


WORD RstTimer3()
{
    WORD ret = 0;
    T4T3M &= ~((1<<3) | (1<<2) | (1<<1) | (1<<0)); //ֹͣ / ��ʱ��
    //T4T3M |= (1<<1); // 1T ģʽ
    TM3PS = 0;
    
    ret = T3H;
    ret <<= 8;
    ret |= T3L;
    
    T3H = 0;
    T3L = 0;
    T4T3M |= (1<<3);  //����

    return ret;
}

void StopTimer3()
{
    T4T3M &= ~(1<<3);
}


void PushBit()
{
    BYTE cnt;
    
    WORD Counter = RstTimer3();
    
    cnt = (BYTE)(Counter / 90);
    IrDABitCnt += cnt;

    if (IrDABitCnt >= 1)
    {
        RX2_Buffer[RX2_Cnt] &= ~(1<<(IrDABitCnt-1));
    }

    if (IrDABitCnt >= 10)
    {
        IrDABitCnt = 0;
        RX2_Cnt++;
    }    
}


void IrDA_RxInt()
{
    if (IrDAStart == 0)
    {
        RstTimer3();
        
        // ��һ����ʼλ
        IrDAStart = 1;
        IrDABitCnt = 0;

        ClearIrDABuf();
    }
    else
    {
        PushBit();
    }

    IrDATimer = 0;
}
#endif

#endif
