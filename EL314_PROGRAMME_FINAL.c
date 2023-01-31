/* COUNTING DOWN TIMER ON NXP FRDM K64F & Multi-function Shield*/

#include "MK64F12.h"


void delayMs(int n);
void shift(uint8_t val);
void segment_send(uint8_t val, uint8_t seg);
const uint8_t seg_val[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
const uint8_t seg_pos[4] = {0xF1, 0xF2, 0xF4, 0xF8};
const uint8_t LED[4] = {0x1, 0x4, 0x8, 0x2};
int tempo_minima = 0;  
int deux = 0; 

/* Main Function*/
int main(void)
{
    SIM_SCGC5 |= 0x3E00;                                   //CLOCK Using port A-B
    SIM_SCGC6 |= 1 << SIM_SCGC6_FTM0_SHIFT;               //FTM Clock configuration

    PORTA->PCR[1] = 0x100;                             // THE BUZZER
    PORTB->PCR[3] = 0x100;                            //S1-A1 SWITCH
    PORTB->PCR[10] = 0x100;                          //S2-A2 SWITCH
    PORTB->PCR[11] = 0x100;                         //S3-A3 SWITCH
    PTB->PDDR &= ~((1UL << 3) | (1UL << 10) | (1UL << 11));

    PORTB->PCR[23] = 0x100;                        // pin Latch
    PORTC->PCR[3] = 0x100;                        //CLOCK pin
    PORTC->PCR[12] = 0x100;                      //DATA pin
    PTB->PDDR |= (1UL << 23);
    PTC->PDDR |= ((1UL << 3) | (1UL << 12));     //set B23,C3,C12 as output 

    PORTD->PCR[1] = 0x100; //D1
    PORTD->PCR[3] = 0x100; //D2
    PORTD->PCR[2] = 0x100; //D3
    PORTD->PCR[0] = 0x100; //D4
    PTD->PDDR |= ((1UL << 1) | (1UL << 3) | (1UL << 2) | (1UL << 0)); //set input or output 
    PTD->PDOR |= ((1UL << 1) | (1UL << 3) | (1UL << 2) | (1UL << 0)); //set bit or clear bit 

    // Flex Timer Module
    FTM0->SC = 0x0;              // to disable the Timer 
    FTM0->CNTIN = 0x00;         //Start the FTM counter 
    FTM0->MOD = 0xA4;          //config the Maxima of the MOD //164
    FTM0->SC |= 0x07;         //pre-scale = 128 
    FTM0->SC |= 0x08;        //Enable THE FTM

    
    __disable_irq();
    PORTB->PCR[3] &= ~0xF0000;  //S1-Interrupt Enable
    PORTB->PCR[10] &= ~0xF0000; //S2-Interrupt 
    PORTB->PCR[11] &= ~0xF0000; //S3-Interrupt

    PORTB->PCR[3] |= 0xA0000;  //SW1_Falling Edge
    PORTB->PCR[10] |= 0xA0000; //SW2_Falling Edge
    PORTB->PCR[11] |= 0xA0000; //SW3_Falling Edge

    NVIC->ISER[1] |= ((1UL << 28)); // Enable interrupt for port B
    __enable_irq();

    while (1)
    {
        segment_send(seg_val[tempo_minima / 10], seg_pos[0]);
        segment_send(seg_val[tempo_minima % 10] & ~(1 << 7), seg_pos[1]);
        segment_send(seg_val[deux / 10], seg_pos[2]);
        segment_send(seg_val[deux % 10], seg_pos[3]);
    }
}

// The 7 Segment Display
void segment_send(uint8_t val, uint8_t seg)
{
    PTB->PDOR &= ~(1UL << 23);
    shift(val);
    shift(seg);
    PTB->PDOR |= (1UL << 23);
}

//SWITCH AS AN INTERRUPT
void PORTB_IRQHandler(void)
{
    int i, j; //change all these 
    
    if ((PTB->PDIR & (1UL << 3)) == 0)   //Press SW1_to Increase the Number
    {
        deux += 20;
        if (deux >= 60)
        {
            deux = deux % 60;
            tempo_minima += 1;
        }
    }

    if ((PTB->PDIR & (1UL << 10)) == 0) // Press SW2_to Decrease the Number //The Button is at B10
    {
        deux -= 10;
        if ((deux< 0) & (tempo_minima >= 1))
        {
            tempo_minima -= 1;
            deux = 60 + deux;
        }
    }

    if ((PTB->PDIR & (1UL << 11)) == 0) // Press SW3_to Start the Timer
    {
        int i, j = 0;
        while ((deux >= 0) | (tempo_minima > 0)) // LED Setting 
        {
           //DISPLAY SEGMENT 1second
            SysTick->LOAD = 0x1F4000 - 1;
            SysTick->CTRL = 5;
            for (i = 0; i < 10; i++)
            {
                while ((SysTick->CTRL & 0x10000) == 0)
                {
                    segment_send(seg_val[tempo_minima / 10], seg_pos[0]);
                    segment_send(seg_val[tempo_minima % 10] & ~(1UL << 7), seg_pos[1]);
                    segment_send(seg_val[deux / 10], seg_pos[2]);
                    segment_send(seg_val[deux % 10], seg_pos[3]);
                }
            }
            deux--;
            PTD->PDOR = ~(LED[j % 4]);
            if ((deux< 0) & (tempo_minima > 0))
            {
                deux= 60 + deux;
                tempo_minima--;
            }
            j++;
        }
        while (1)
        {
            PTA->PDDR |= (1UL << 1); //THE BUZZER TONE IS BEING SET HERE 
            PTD->PDOR = (0xF);
            delayMs(55);
            PTA->PDDR &= ~(1UL << 1);
            PTD->PDOR = (0x0);
            delayMs(50);
        }
    }
    delayMs(300); //Space Daley
    PORTB->ISFR |= ((1UL << 3) | (1UL << 10) | (1UL << 11));
}

void shift(uint8_t val)   //Shifting out 
{
    int i;
    for (i = 0; i < 8; i++)
    {
        if ((val & 128) != 0)
        {
            PTC->PDOR |= (1UL << 12);
        }
        else
        {
            PTC->PDOR &= ~(1UL << 12);
        }
        val <<= 1;
        PTC->PDOR |= (1UL << 3);
        PTC->PDOR &= ~(1UL << 3);
    }
}

void delayMs(int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        while ((FTM0->SC & 0x80) == 0)
        {
            segment_send(0xBF, seg_pos[0]);
            segment_send(0xBF, seg_pos[1]);
            segment_send(0xBF, seg_pos[2]);
            segment_send(0xBF, seg_pos[3]);
        }
        FTM0->SC &= ~(0x80);
    }
}

/* COUNTING DOWN TIMER ON NXP FRDM K64F & Multi-function Shield*/