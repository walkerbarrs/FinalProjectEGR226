#include "msp.h"
#include <stdio.h>

//mitchel was here

uint8_t hours = 12;
uint8_t minutes = 55;
uint8_t seconds = 0;
uint8_t am_pm = 1; //% = 0 for am % = 1 for pm
int counting = 0;
int clockChange = 1;
uint8_t currentState = 0;  //this will tell the handlers when to act and when to not so that weird things arent updated when they are not supposed to
uint8_t setAlarm = 0;
uint8_t blinking = 0;

enum states{
    COUNTING,    //This is the first state where the switch has not yet been pressed and LED is off
    SET_TIME,   //This is the state where the time is set
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
void PORT3_IRQHandler(void);
void clockChangeInit();
void checkClockChange();
void setAlarmInit();
void blinkHours();
void blinkMinutes();
void blinkSeconds();


void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    //Initialize custom functions
    ADCpin_init();
    ADC_init();
    Systick_init();
    TimerA_init();
    LCD_init();
    ms_delay(10);
    clockChangeInit();
    setAlarmInit();
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
                checkClockChange();
                counting  = 0;
                if(currentState == 1) state = SET_TIME;
            }
            break;

        case SET_TIME:
            if(setAlarm == 1) blinkHours();
            else if(setAlarm == 2) blinkMinutes();
            else if(setAlarm == 3) blinkSeconds();
            readPotentiometer();
            checkClockChange();
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
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);

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

}
//Custom function add one to count when timer32 counts 1 second
void T32_INT1_IRQHandler(void)
{
    //set the global flag true and then clear timer 32 interrrupt flag
    counting = 1;
    TIMER32_1->INTCLR = 0;
}
//Custom function to update seconds, minutes, and hours
void updateClock()
{
        seconds ++;
        if(seconds == 60)
            {
                seconds = 0;
                minutes++;
                ms_delay(10);
            }
        if(minutes == 60)
        {
            minutes = 0;
            hours++;
            ms_delay(10);
        }
        if(hours == 13)
        {
          hours = 1;
          am_pm++;
          ms_delay(10);
        }

}
//Custom function to initialize the lightswitch with an interrupt
void clockChangeInit()
{
    P3->SEL0 &= ~BIT7;          //Set as GPIO
    P3->SEL1 &= ~BIT7;
    P3->DIR &= ~BIT7;           //Set as input
    P3->REN |= BIT7;            //Enable resistor
    P3->OUT |= BIT7;            //Pull- up the resistor
    P3->IES |= BIT7;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT7;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
}
//custom function to initialize the setAlarm button
void setAlarmInit()
{
    P3->SEL0 &= ~BIT6;          //Set as GPIO
    P3->SEL1 &= ~BIT6;
    P3->DIR &= ~BIT6;           //Set as input
    P3->REN |= BIT6;            //Enable resistor
    P3->OUT |= BIT6;            //Pull- up the resistor
    P3->IES |= BIT6;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT6;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
    NVIC->ISER[1] = 1 <<((PORT3_IRQn)& 31);         //This initializes which interrupt is going to be used
}
//Custom function add one to clockChange when the button is pressed with no output
void PORT3_IRQHandler(void)
{
    if(currentState == 0)
    {
        if(P3->IFG & BIT7)          //If the interrupt is on P3.6
        {
            ms_delay(30);
            if(P3->IFG & BIT7)
            {
                    clockChange++;
                    P3->IFG &= ~ BIT7;      //Turn the flag off
            }
        }
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
        {
            ms_delay(30);
            if(P3->IFG & BIT6)
            {
                    setAlarm = 1;
                    currentState = 1;
                    P3->IFG &= ~ BIT6;      //Turn the flag off
            }
        }
    }
    else if(currentState == 1)
    {
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
               {
                       ms_delay(30);
                       if(P3->IFG & BIT6)
                       {
                           setAlarm ++;
                           if(setAlarm == 4) setAlarm = 1;
                           P3->IFG &= ~ BIT6;      //Turn the flag off
                       }
               }
    }

}
//Custom function to check whether or not the clock speed button has been pressed and then changes the spped respectively
void checkClockChange()
{
    if((clockChange %2) == 0) TIMER32_1->LOAD = 30000;         //Load .0167 seconds into the timer = 3000000
    else    TIMER32_1->LOAD = 3000000;         //Load 1 seconds into the timer = 3000000
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkHours()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentTime[17];
    if(blinking == 0)
    {
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
    }
    else if(blinking == 1)
    {
        if((am_pm %2) == 1)
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"    :%d:0%d am   ", minutes, seconds);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"    :0%d:0%d am   ", minutes, seconds);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"    :0%d:%d am   ", minutes, seconds);
            else
                sprintf(currentTime,"    :%d:%d am   ", minutes, seconds);
        }
        else
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"    :%d:0%d pm   ", minutes, seconds);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"    :0%d:0%d pm   ", minutes, seconds);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"    :0%d:%d pm   ", minutes, seconds);
            else
                sprintf(currentTime,"    :%d:%d pm   ", minutes, seconds);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send LCD_data 1 character at a time
    }
    ms_delay(250);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkMinutes()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentTime[17];
    if(blinking == 0)
    {
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
    }
    else if(blinking == 1)
    {
        if((am_pm %2) == 1)
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"  %2d:  :0%d am   ", hours, seconds);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"  %2d:  :0%d am   ", hours, seconds);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"  %2d:  :%d am   ", hours, seconds);
            else
                sprintf(currentTime,"  %2d:  :%d am   ", hours, seconds);
        }
        else
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"  %2d:  :0%d pm   ", hours, seconds);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"  %2d:  :0%d pm   ", hours, seconds);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"  %2d:  :%d pm   ", hours, seconds);
            else
                sprintf(currentTime,"  %2d:  :%d pm   ",hours, seconds);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send LCD_data 1 character at a time
    }
    ms_delay(250);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkSeconds()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentTime[17];
    if(blinking == 0)
    {
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
    }
    else if(blinking == 1)
    {
        if((am_pm %2) == 1)
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"  %2d:%d:   am   ", hours, minutes);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"  %2d:0%d:   am   ", hours, minutes);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"  %2d:0%d:   am   ", hours, minutes);
            else
                sprintf(currentTime,"  %2d:%d:   am   ", hours, minutes);
        }
        else
        {
            if(seconds < 10 && minutes >= 10)
               sprintf(currentTime,"  %2d:%d:   pm   ", hours, minutes);
            else if(seconds < 10 && minutes < 10)
                sprintf(currentTime,"  %2d:0%d:   pm   ", hours, minutes);
            else if(minutes < 10 && seconds >=10)
                sprintf(currentTime,"  %2d:0%d:   pm   ", hours, minutes);
            else
                sprintf(currentTime,"  %2d:%d:   pm   ", hours, minutes);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send LCD_data 1 character at a time
    }
    ms_delay(250);
    if (blinking) blinking = 0;
    else blinking = 1;
}
