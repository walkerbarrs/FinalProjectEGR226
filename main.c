#include "msp.h"
#include <stdio.h>

int hours = 4;
int minutes = 15;
int seconds = 0;
int am_pm = 1; //% = 0 for am % = 1 for pm
int counting = 0;
enum states{
    COUNTING,    //This is the first state where the switch has not yet been pressed and LED is off
};
//Declare Custom Functions
void LCD_init();
void LCD_command(uint8_t command);
void LCD_data(uint8_t data);
void pulseEnable();
void pushNibble(uint8_t nibble);
void pushByte(uint8_t byte);
void printVals();
void ADCpin_init();
void ADC_init();
void TimerA_init();
void ms_delay(int delay);
void us_delay(int delay);
void Systick_init();
void countIntrpt();
void T32_INT1_IRQHandler(void);
void readPotentiometer();
void updateTimerA(float potVal);
void updateClock();

void main(void)
{


    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    //Initialize custom functions
    ADCpin_init();
    ADC_init();
    Systick_init();
    TimerA_init();
    LCD_init();
    countIntrpt();
    __enable_irq();
    enum states state = COUNTING;    //Default to LED OFF and create state variable of type states



    while(1)
    {
        switch(state)
        {
        case COUNTING:
            if(counting == 1)
            {
                readPotentiometer();           //Read the voltage from the potentiometer
                updateClock();
                printVals();
                counting  = 0;
            }
            break;
        }
    }

}

//Custom function to initialize P2.0 and P2.1 for the LEDS and P5.5 for ADC
void ADCpin_init()
{
    //Initialize Port 5 ADC
    P5->SEL0 |= BIT5;
    P5->SEL0 |= BIT5;
    P5->DIR &= ~BIT5;

}

//Custom function to initialize the ADC
void ADC_init()
{
    ADC14->CTL0 &=~ 0x00000002; //diable ENC for configuration
    ADC14->CTL0 |= 0x04400110;  //S/H pulse Mode,SMCLK, 16 sample checks
    ADC14->CTL1 = 0x00000030;   //14 bit resolution
    ADC14->CTL1 |= 0x00000000;  //selecting mem0 register
    ADC14->MCTL[0] = 0x00000000; //1st zero for mem0
    ADC14->CTL0 |= 0x00000002; //enable ENC for configuration
}

//Custom funciton to initalize Timer A on P7.4 and P6.7 with no output
void TimerA_init()
{

    //Timer A output pin
    P6->SEL0 |= BIT7;
    P6->SEL1 &= ~BIT7;
    P6->DIR |= BIT7;

    //Timer A initialization
    TIMER_A2->CCR[0] = 60000 - 1;                   //Load the period
    TIMER_A2->CCTL[4] = TIMER_A_CCTLN_OUTMOD_7;
    TIMER_A2->CCR[4] = 0;                   //Initialize the duty cycle
    TIMER_A2->CTL = TASSEL_2| MC_1| TACLR;
}
//Custom function to initialize the systic timer for delays
void Systick_init()
{
    SysTick->CTRL = 0;  //Disable SysTick during step
    SysTick->LOAD = 0x00FFFFFF; //max reload value
    SysTick->VAL = 0;   //any write to current clears it
    SysTick->CTRL = 0x00000005; //enable systic, 3mHz, no interrupts
}

//custom function to read the potentiometer and return the adjusted voltage
void readPotentiometer()
{
    //initialize variables
    float result;
    float nADC;
    float potVal = 0;

    ADC14->CTL0 |= 0x00000001;  //start conversion
    while(!ADC14->IFGR0);
    result = ADC14->MEM[0];
    nADC = ((result *3.3)/ 16384);     //result * vref / 2^14 == resolution
    potVal = (nADC / 3.3) * (60000-1);        //Get the percentage of potval to vRef and multiply by the Period
    updateTimerA(potVal);                   //Update Timer A with the value of potVal
}

//Custom function to update LED 2.0 and 2.1 with the new intensity
void updateTimerA(float potVal)
{
    TIMER_A2->CCR[4] = potVal;
}
//Custom function to delay the program for a set number of milliseconds
void ms_delay(int delay)
{
    SysTick->LOAD = ((delay * 3000)-1);  //delay for one millisecond per delay
    SysTick->VAL = 0;    //1ms countdown to 0
    while((SysTick->CTRL &0x10000) == 0) {}; //Wait for flag to be set (timeout happened)
}
//Custom function to delay the program for a set number of microseconds
void us_delay(int delay)
{
    SysTick->LOAD = ((delay * 3)-1);  //delay for one microsend per delay
    SysTick->VAL = 0;    //1ms countdown to 0
    while((SysTick->CTRL &0x10000) == 0) {}; //Wait for flag to be set (timeout happened)
}
//custom function to initialize LCD
void LCD_init()
{
    P2->SEL0 &= ~(0xF0);
    P2->SEL1 &= ~(0xF0);
    P2->DIR |= (0xF0);
    P5->SEL0 &= ~(0x03);
    P5->SEL1 &= ~(0x03);
    P5->DIR |= (0x03);

    //this portion of the code came from the lab document
    LCD_command(0x03);     //initialize reset sequence
    ms_delay(100);
    LCD_command(0x03);
    us_delay(200);
    LCD_command(0x03);
    ms_delay(100);

    LCD_command(0x02);     //Set the 4 bit mode
    ms_delay(100);
    LCD_command(0x02);
    us_delay(100);

    LCD_command(0x08);  //set up 2 lines 5x7 format
    us_delay(100);

    LCD_command(0x0F);  //set up display on, cursor on, blinking
    us_delay(100);

    LCD_command(0x01);  //Clear display and move curser to home position
    us_delay(100);

    LCD_command(0x06);  //set up to increment curser
    us_delay(100);
}
//Custom function to tell the LCD you want to give it a command
void LCD_command(uint8_t command)
{
   P5->OUT &= ~(BIT1);  //Set rs to 0 for command
   pushByte(command);   //Send command to pushbyte

}

//Custom function to send the LED data you want printed to the screen
void LCD_data(uint8_t data)
{
    P5->OUT |= (BIT1);  //Set rs to 1 for data
    pushByte(data);     //Send data to pushbyte
}

//Custom function that allows LCD to read in data
void pulseEnable()
{
    P5->OUT &= ~BIT0;   //Set enable low for 10us
    us_delay(10);
    P5->OUT |= BIT0;    //set enable high for 10us
    us_delay(10);
    P5->OUT &= ~BIT0;   //set enable low for 10us
    us_delay(10);
}

//Custom function to give the input to the LCD
void pushNibble(uint8_t nibble)
{
    P2->OUT &= ~0xF0;   //Clear bits in use
    P2->OUT |= nibble;  //Turn that nibble on
    pulseEnable();      //enable pulse for LCD to recieve info
}

//custom function to send pushnibble the Most significant nibble then the least significant
void pushByte(uint8_t byte)
{
    pushNibble(byte & 0xF0);    //Send nibble only first four bits of the byte(8bits)
    pushNibble(byte << 4);      //Send nibble the last four bits of the byte(8bits)
}

//custom function to print names on lines of the lcd
void printVals()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentTime[17];

    if((am_pm %2) == 1)
    {
        if(seconds < 10 && minutes >= 10)
           sprintf(currentTime,"  %2d:%d:0%d am   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)
            sprintf(currentTime,"  %2d:0%d:0%d am   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)
            sprintf(currentTime,"  %2d:0%d:%d am   ", hours, minutes, seconds);
        else
            sprintf(currentTime,"  %2d:%d:%d am   ", hours, minutes, seconds);
    }
    else
    {
        if(seconds < 10 && minutes >= 10)
           sprintf(currentTime,"  %2d:%d:0%d pm   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)
            sprintf(currentTime,"  %2d:0%d:0%d pm   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)
            sprintf(currentTime,"  %2d:0%d:%d pm   ", hours, minutes, seconds);
        else
            sprintf(currentTime,"  %2d:%d:%d pm   ", hours, minutes, seconds);
    }
    ms_delay(75);
    LCD_command(0x01);  //clear display
    ms_delay(20);
    LCD_command(0x80);  //start on first line
    ms_delay(20);

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send LCD_data 1 character at a time
    }

}
//Custom function to initialize the screensaver with an interrupt with no output
void countIntrpt()
{
    //initialization for timer 32 interrupt
    TIMER32_1->CONTROL = 0XC2;          //initialize timer 32 for periodic mode, no prescaler, 32-bit timer
    TIMER32_1->INTCLR = 0;              //Clear interupt flag
    TIMER32_1->LOAD = 3000000;         //Load 10 seconds into the timer = 3000000
    TIMER32_1->CONTROL |= 0X20;         //enable interrupts
    NVIC->ISER[0] = 1 <<((T32_INT1_IRQn)& 31);      //This initializes which interrupt is going to be used
    NVIC_SetPriority(T32_INT1_IRQn, 3);

}
//Custom function add one to count when timer32 counts 1 second
void T32_INT1_IRQHandler(void)
{
    //set the global flag true and then clear timer 32 interrrupt flag
    counting = 1;
    TIMER32_1->INTCLR = 0;
}

void updateClock()
{
        seconds ++;
        if(seconds == 60)
            {
                seconds = 0;
                minutes++;
            }
        ms_delay(20);
        if(minutes == 60)
        {
            minutes = 0;
            hours++;
        }
        ms_delay(20);
        if(hours == 13)
        {
          hours = 1;
          am_pm++;
        }
        ms_delay(20);
}
