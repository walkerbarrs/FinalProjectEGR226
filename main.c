#include "msp.h"
#include <stdio.h>
 /*
 * Name:Walker Barrs and Mith Havercroft
 * Date:11/14/2018
 * Professor: Professor Zuidema
 * Class:   EGR226
 * Description: This is a code for an alarm clock that allows the user
 * to set the time and alarm using pushbuttons
 */
 float Ftemp = 0;
uint8_t hours = 12;     //Hours, minutes, seconds for time
uint8_t minutes = 55;
uint8_t seconds = 0;
uint8_t Ahours = 12;    //Hours, minutes, seconds for Alarm
uint8_t Aminutes = 55;
uint8_t Aseconds = 0;
uint8_t Aam_pm = 1;
uint8_t am_pm = 1; //% = 0 for am % = 1 for pm
int counting = 0;
int clockChange = 1;
uint8_t currentState = 0;  //this will tell the handlers when to act and when to not so that weird things arent updated when they are not supposed to
uint8_t setTime = 0;
uint8_t blinking = 0;
uint8_t onOffUp = 0;
uint8_t alarmSet = 0; //Variable to keep track of whether the alarm is on or off
uint8_t snoozedec = 0;
uint8_t setalarm = 0;
uint8_t lights = 0; //global variable used to hold the interrupt flag
uint8_t LED =  0;   //global variable used to hold the PWM signal of the lighs
 enum states{
    COUNTING,    //This is the first state where the switch has not yet been pressed and LED is off
    SET_TIME,   //This is the state where the time is set
    SET_ALARM,  //this is the state where the alarm time is set
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
void setTimeInit();
void blinkCLKHours();
void blinkCLKMinutes();
void blinkCLKSeconds();
void onOffUpBtn();
void snoozeDec();
void PortADC_init(void);
void ADC14_init(void);
void Get_ADC_Convert_Temp(void);
void blinkALRMSeconds();
void blinkALRMMinutes();
void blinkALRMHours();
void setAlarm();
void PWMlights_init();
void PWMlights_update();
void Lightsintrpt();
void T32_INT1_IRQHandler(void);
void setTimeConditions();
 /*
 * main.c
 */
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
    setTimeInit();
    countIntrpt();
    onOffUpBtn();
    ADC14_init();
    setAlarm();
    snoozeDec();
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
                Get_ADC_Convert_Temp();
                updateClock();
                printVals();
                checkClockChange();
                counting  = 0;
                if(alarmSet == 1)
                {
                    if((hours == Ahours) && (minutes == Aminutes) && (am_pm == Aam_pm)) printf("Fuck Yeah\n");
                    if(snoozedec == 1)
                    {
                        Aminutes = Aminutes + 10;
                        if(Aminutes > 59)
                            {
                                Aminutes = Aminutes - 60;
                                Aam_pm++;
                                Ahours ++;
                                if(Ahours >12) Ahours = 1;
                            }
                        snoozedec = 0;
                    }
                }
                if(currentState == 1) state = SET_TIME;
                if(currentState == 2) state = SET_ALARM;
            }
            P3->IE |= 0xEC;
            break;
         case SET_TIME:
            if(setTime == 1)
            {
                blinkCLKHours();
            }
            else if(setTime == 2)
            {
                blinkCLKMinutes();
            }
            else if(setTime == 3)
            {
                blinkCLKSeconds();
            }
            readPotentiometer();
            Get_ADC_Convert_Temp();
            checkClockChange();
            setTimeConditions();
            if(onOffUp == 1)
            {
               us_delay(100);
               if(setTime == 1)
               {
                   hours++;
                   if(hours == 13)
                       {
                           am_pm++;
                           hours = 1;
                        }
               }
               else if(setTime == 2)
               {
                   minutes++;
                   if(minutes == 60)
                       {
                           am_pm++;
                           minutes = 0;
                       }
               }
               onOffUp = 0;
            }
            if(snoozedec == 1)
            {
                if(setTime == 1)
                {
                    hours--;
                    if(hours == 0)
                        {
                            am_pm++;
                            hours = 12;
                        }
                }
                else if(setTime == 2)
                {
                    minutes--;
                    if(minutes == 255)
                        {
                            am_pm++;
                            minutes = 59;
                        }
                }
                snoozedec = 0;
            }
            if(currentState == 0)
            {
                setTime = 0;
                state = COUNTING;
            }
        P3->IE |=0xEC;
        break;
         case SET_ALARM:
                if(setalarm == 1)
                {
                    blinkALRMHours();
                }
                else if(setalarm == 2)
                {
                    blinkALRMMinutes();
                }
                else if(setalarm == 3)
                {
                    blinkALRMSeconds();
                }
                readPotentiometer();
                Get_ADC_Convert_Temp();
                checkClockChange();
                if(onOffUp == 1)
                {
                   if(setalarm == 1)
                   {
                       Ahours++;
                       if(Ahours == 13)
                           {
                               Aam_pm++;
                               Ahours = 1;
                            }
                   }
                   else if(setalarm == 2)
                   {
                        Aminutes++;
                       if(Aminutes == 60)
                           {
                               Aam_pm++;
                               Aminutes = 0;
                           }
                   }
                   onOffUp = 0;
                }
                if(snoozedec == 1)
                {
                    if(setalarm == 1)
                    {
                        Ahours--;
                        if(Ahours == 0)
                            {
                                Aam_pm++;
                                Ahours = 12;
                            }
                    }
                    else if(setalarm == 2)
                    {
                        Aminutes--;
                        if(Aminutes == 255)
                            {
                                Aam_pm++;
                                Aminutes = 59;
                            }
                    }
                    snoozedec = 0;
                }
                if(currentState == 0)
                {
                    setalarm = 0;
                    alarmSet = 1;
                    state = COUNTING;
                }
        P3->IE |=0xEC;
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
    char alarmOnOff[17];
    char currentAlarm[17];
    char currentTemp[17];
     if((am_pm %2) == 1)
    {
        if(seconds < 10 && minutes >= 10)
           sprintf(currentTime,"   %2d:%d:0%d am   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)
            sprintf(currentTime,"   %2d:0%d:0%d am   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)
            sprintf(currentTime,"   %2d:0%d:%d am   ", hours, minutes, seconds);
        else
            sprintf(currentTime,"   %2d:%d:%d am   ", hours, minutes, seconds);
    }
    else
    {
        if(seconds < 10 && minutes >= 10)
           sprintf(currentTime,"   %2d:%d:0%d pm   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)
            sprintf(currentTime,"   %2d:0%d:0%d pm   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)
            sprintf(currentTime,"   %2d:0%d:%d pm   ", hours, minutes, seconds);
        else
            sprintf(currentTime,"   %2d:%d:%d pm   ", hours, minutes, seconds);
    }
    if((Aam_pm %2) == 1)
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"   %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"   %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"   %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"   %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
     else
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"   %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"   %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"   %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"   %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
     if(alarmSet == 1)
    {
        sprintf(alarmOnOff, " Alarm  Enabled ");
    }
    else
    {
        sprintf(alarmOnOff, " Alarm Disabled ");
    }
    ms_delay(1);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(1);
     for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send LCD_data 1 character at a time
    }
     LCD_command(0xC0);
    ms_delay(1);
    for(i=0;i<16;i++)
    {
        LCD_data(alarmOnOff[i]);
    }
     LCD_command(0x90);
    ms_delay(1);
    for(i=0;i<16;i++)
    {
        LCD_data(currentAlarm[i]);
    }
     sprintf(currentTemp, "     F:  %.1f     ", Ftemp);
    LCD_command(0xD0);  //start on fourth line
    ms_delay(1);
     for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTemp[i]);     //send LCD_data 1 character at a time
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
                ms_delay(1);
            }
        if(minutes == 60)
        {
            minutes = 0;
            hours++;
            ms_delay(1);
        }
        if(hours == 13)
        {
          hours = 1;
          am_pm++;
          ms_delay(1);
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
//custom function to initialize the setTime button
void setTimeInit()
{
    P3->SEL0 &= ~BIT6;          //Set as GPIO
    P3->SEL1 &= ~BIT6;
    P3->DIR &= ~BIT6;           //Set as input
    P3->REN |= BIT6;            //Enable resistor
    P3->OUT |= BIT6;            //Pull- up the resistor
    P3->IES |= BIT6;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT6;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
}
//Custom function add one to clockChange when the button is pressed with no output
void PORT3_IRQHandler(void)
{
    if(currentState == 0)   //counting state
    {
        if(P3->IFG & BIT7)          //If the interrupt is on P3.6
        {
                    clockChange++;
                    P3->IFG &= ~ BIT7;      //Turn the flag off
                    P3->IE &= ~ BIT7;
        }
        else if(P3->IFG & BIT6)          //If the interrupt is on P3.6
        {
                    setTime = 1;
                    currentState = 1;
                    P3->IFG &= ~ BIT6;      //Turn the flag off
                    P3->IE &= ~ BIT6;
        }
        else if(P3->IFG & BIT5)
        {
            if(alarmSet == 0) alarmSet = 1;
            else alarmSet = 0;
            P3->IFG &= ~ BIT5;
            P3->IE &=~ BIT5;
        }
        else if(P3->IFG & BIT3)
        {
            if(alarmSet == 1)
            snoozedec = 1;
            P3->IFG &= ~BIT3;
            P3->IE &= ~ BIT3;
        }
        else if(P3->IFG & BIT2)
        {
            currentState = 2;
            setalarm = 1;
            P3->IFG &= ~ BIT2;
            P3->IE &= ~ BIT2;
        }
    }
    else if(currentState == 1)  //set time state
    {
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
       {
                   setTime ++;
                   if(setTime == 3) currentState = 0;
                   P3->IFG &= ~ BIT6;      //Turn the flag off
                   P3->IE &= ~ BIT6;
       }
        else if(P3->IFG & BIT5)
       {
           onOffUp = 1;
           P3->IFG &= ~BIT5;
           P3->IE &= ~BIT5;
       }
        else if(P3->IFG & BIT7)
       {
           P3->IFG &= ~BIT7;
       }
        else if(P3->IFG & BIT3)
        {
            snoozedec = 1;
            P3->IFG &= ~ BIT3;
            P3->IE &= ~BIT3;
        }
        else if(P3->IFG & BIT2)          //If the interrupt is on P3.6
       {
            P3->IFG &= ~ BIT2;      //Turn the flag off
            P3->IE &= ~ BIT2;
       }
    }
    else if(currentState == 2)
    {
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
       {
                   setTime ++;
                   if(setTime == 3) currentState = 0;
                   P3->IFG &= ~ BIT6;      //Turn the flag off
                   P3->IE &= ~ BIT6;
       }
        else if(P3->IFG & BIT5)
       {
           onOffUp = 1;
           P3->IFG &= ~BIT5;
           P3->IE &= ~BIT5;
       }
        else if(P3->IFG & BIT7)
       {
           P3->IFG &= ~BIT7;
       }
        else if(P3->IFG & BIT3)
        {
            snoozedec = 1;
            P3->IFG &= ~ BIT3;
            P3->IE &= ~BIT3;
        }
        else if(P3->IFG & BIT2)          //If the interrupt is on P3.6
       {
            setalarm++;
            if(setalarm == 3) currentState = 0;
            P3->IFG &= ~ BIT2;      //Turn the flag off
            P3->IE &= ~ BIT2;
       }
    }
}
//Custom function to check whether or not the clock speed button has been pressed and then changes the spped respectively
void checkClockChange()
{
    if((clockChange %2) == 0)
        {
            TIMER32_1->LOAD = 30000;         //Load .0167 seconds into the timer = 3000000
        }
    else
    {
        TIMER32_1->LOAD = 3000000;         //Load 1 seconds into the timer = 3000000
    }
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkCLKHours()
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
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkCLKMinutes()
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
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkCLKSeconds()
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
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//Custom function to initialize the button for on/off/up
void onOffUpBtn()
{
    P3->SEL0 &= ~BIT5;          //Set as GPIO
    P3->SEL1 &= ~BIT5;
    P3->DIR &= ~BIT5;           //Set as input
    P3->REN |= BIT5;            //Enable resistor
    P3->OUT |= BIT5;            //Pull- up the resistor
    P3->IES |= BIT5;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT5;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
    NVIC->ISER[1] = 1 <<((PORT3_IRQn)& 31);         //This initializes which interrupt is going to be used
}
//Custom function to initialize the button for snooze/decrement
void snoozeDec()
{
    P3->SEL0 &= ~BIT3;          //Set as GPIO
    P3->SEL1 &= ~BIT3;
    P3->DIR &= ~BIT3;           //Set as input
    P3->REN |= BIT3;            //Enable resistor
    P3->OUT |= BIT3;            //Pull- up the resistor
    P3->IES |= BIT3;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT3;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
    NVIC->ISER[1] = 1 <<((PORT3_IRQn)& 31);         //This initializes which interrupt is going to be used
}
//Custom function to initialize the button for setAlarm
void setAlarm()
{
    P3->SEL0 &= ~BIT2;          //Set as GPIO
    P3->SEL1 &= ~BIT2;
    P3->DIR &= ~BIT2;           //Set as input
    P3->REN |= BIT2;            //Enable resistor
    P3->OUT |= BIT2;            //Pull- up the resistor
    P3->IES |= BIT2;            //Set the interrupt flag to be set when the button goes from 1 to 0
    P3->IE  |= BIT2;            //Enable an interrupt on P3.7
    P3->IFG = 0;                //Initialize interrupt flag to 0
    NVIC->ISER[1] = 1 <<((PORT3_IRQn)& 31);         //This initializes which interrupt is going to be used
}
 void ADC14_init(void)
    {
    P5->SEL0 |= BIT4;                           //Configure pin 5.5 to A0 Input
    P5->SEL1 |= BIT4;
     ADC14->CTL0 &=~ ADC14_CTL0_ENC;         // disable ADC converter during initialization (BIT1 goes to zero)
    ADC14->CTL0 |= 0x04200210;              // S/H pulse mode, SMCLK, 16 sample clocks
    ADC14->CTL1 = 0x00000030;               // 14 bit resolution
    ADC14->CTL1 |= 0x00000000;              // convert for mem0 register
    ADC14->MCTL[1] = 0x00000001;            // ADC14INCHx = 0 for mem[1]
    ADC14->CTL0 |= ADC14_CTL0_ENC;          // Enable ADC14ENC, starts the ADC after configuration (BIT1 goes to 1)
    }
void Get_ADC_Convert_Temp(void)
    {
    static volatile uint16_t result;
    float voltage=0;
    float Ctemp = 0;
     ADC14->CTL0 |= ADC14_CTL0_SC;                       // Start Conversation (Sets BIT0 to 1)
    while(!ADC14->IFGR0 & BIT0);                        // Wait for conversation to complete (interrupt flag)
    result = ADC14->MEM[1];                             // Get value from ADC and store it as result
    voltage = (result*3.3)/16384;                       // Calculate voltage
    Ctemp = (voltage-.5)/.01;                           //Convert voltage reading to degrees celcius
    Ftemp = (1.8*Ctemp)+32;                             //Convert Celcius to Fahrenheit
    }
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkALRMHours()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentAlarm[17];
    if(blinking == 0)
    {
    if((am_pm %2) == 1)
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
    else
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
    }
    else if(blinking == 1)
    {
        if((am_pm %2) == 1)
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"    :%d:0%d am   ", Aminutes, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"    :0%d:0%d am   ", Aminutes, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"    :0%d:%d am   ", Aminutes, Aseconds);
            else
                sprintf(currentAlarm,"    :%d:%d am   ", Aminutes, Aseconds);
        }
        else
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"    :%d:0%d pm   ", Aminutes, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"    :0%d:0%d pm   ", Aminutes, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"    :0%d:%d pm   ", Aminutes, Aseconds);
            else
                sprintf(currentAlarm,"    :%d:%d pm   ", Aminutes, Aseconds);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);
     for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentAlarm[i]);     //send LCD_data 1 character at a Alarm
    }
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkALRMMinutes()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentAlarm[17];
    if(blinking == 0)
    {
    if((Aam_pm %2) == 1)
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
    else
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
    }
    else if(blinking == 1)
    {
        if((Aam_pm %2) == 1)
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"  %2d:  :0%d am   ", Ahours, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"  %2d:  :0%d am   ", Ahours, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"  %2d:  :%d am   ", Ahours, Aseconds);
            else
                sprintf(currentAlarm,"  %2d:  :%d am   ", Ahours, Aseconds);
        }
        else
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"  %2d:  :0%d pm   ", Ahours, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"  %2d:  :0%d pm   ", Ahours, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"  %2d:  :%d pm   ", Ahours, Aseconds);
            else
                sprintf(currentAlarm,"  %2d:  :%d pm   ",Ahours, Aseconds);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);
     for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentAlarm[i]);     //send LCD_data 1 character at a time
    }
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkALRMSeconds()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentAlarm[17];
    if(blinking == 0)
    {
    if((Aam_pm %2) == 1)
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
    else
    {
        if(Aseconds < 10 && Aminutes >= 10)
           sprintf(currentAlarm,"  %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)
            sprintf(currentAlarm,"  %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10)
            sprintf(currentAlarm,"  %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else
            sprintf(currentAlarm,"  %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
    }
    else if(blinking == 1)
    {
        if((Aam_pm %2) == 1)
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"  %2d:%d:   am   ", Ahours, Aminutes);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"  %2d:0%d:   am   ", Ahours, Aminutes);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"  %2d:0%d:   am   ", Ahours, Aminutes);
            else
                sprintf(currentAlarm,"  %2d:%d:   am   ", Ahours, Aminutes);
        }
        else
        {
            if(Aseconds < 10 && Aminutes >= 10)
               sprintf(currentAlarm,"  %2d:%d:   pm   ", Ahours, Aminutes);
            else if(Aseconds < 10 && Aminutes < 10)
                sprintf(currentAlarm,"  %2d:0%d:   pm   ", Ahours, Aminutes);
            else if(Aminutes < 10 && Aseconds >=10)
                sprintf(currentAlarm,"  %2d:0%d:   pm   ", Ahours, Aminutes);
            else
                sprintf(currentAlarm,"  %2d:%d:   pm   ", Ahours, Aminutes);
        }
    }
    ms_delay(5);
    LCD_command(0x01);  //clear display
    LCD_command(0x80);  //start on first line
    ms_delay(5);
     for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentAlarm[i]);     //send LCD_data 1 character at a Alarm
    }
    ms_delay(100);
    if (blinking) blinking = 0;
    else blinking = 1;
}
void PWMlights_init()
{
     P7->SEL0 |= (BIT5|BIT6|BIT7);       //initialize P7.5-7.7 for Timer A
    P7->SEL1 &= ~(BIT5|BIT6|BIT7);
    P7->DIR |= (BIT5|BIT6|BIT7);        //Set direction as output
    P7->OUT &= ~(BIT5|BIT6|BIT7);       //Start with all lights off
 
    TIMER_A1->CCR[0] = 60000 -1;                        //Initialize Timer A
    TIMER_A1->CCTL[1] = TIMER_A_CCTLN_OUTMOD_7;
    TIMER_A1->CCR[1] = 0;   //RED
    TIMER_A1->CCTL[2] = TIMER_A_CCTLN_OUTMOD_7;
    TIMER_A1->CCR[2] = 0;   //GREEN
    TIMER_A1->CCTL[3] = TIMER_A_CCTLN_OUTMOD_7;
    TIMER_A1->CCR[3] = 0;   //BLUE
    TIMER_A1->CTL = TASSEL_2| MC_1| TACLR;
}
//Custom function to update the intensity of the LED's with an input of what light is to be changed
void PWMlights_update()
{   
    //if light is one the user wants to update the LEDs
    if(lights == 1)
    {
        {
            TIMER_A1->CCR[1] = ((LED * 600) -1);    //RED LED
            TIMER_A1->CCR[2] = ((LED * 600) -1);  //Green LED
            TIMER_A1->CCR[3] = ((LED * 600) -1);    //Blue LED
            
            TIMER32_1->LOAD = 9000000;
            lights = 0; //Reset timer 32 interrupt flag
        }
    }
}
//Custom function to initialize the screensaver with an interrupt with no output
 void Lightsintrpt()
{
    //initialization for timer 32 interrupt
    TIMER32_1->CONTROL = 0XC2;          //initialize timer 32 for periodic mode, no prescaler, 32-bit timer
    TIMER32_1->LOAD = 9000000;         //Load 3 seconds seconds into the timer = 3000000 * 3
    TIMER32_1->INTCLR = 0;              //Clear interupt flag
    TIMER32_1->CONTROL |= 0X20;         //enable interrupts
 }
void T32_INT1_IRQHandler(void)
 {
    //set the global flag true and then clear timer 32 interrrupt flag
    lights = 1;
    TIMER32_1->INTCLR = 0;
    if(LED < 100)
    {
        LED++;
    }
    else
    {
        LED = 100;
    }
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
}
void setTimeConditions()
{
    if(onOffUp == 1)                //if the up button was presseed increment either the hours, minutes, or seconds based on which one is currently blinking
    {
       if(setTime == 1)
       {
           hours++;
           if(hours == 13)          //if the hours reach 13 reset them to 1 and increment am_pm
               {
                   am_pm++;
                   hours = 1;
               }
       }
       else
       {
           minutes++;
           if(minutes == 60)        //if the hours reach 60 reset them back to 0
               {
                   minutes = 0;
               }
       }
       onOffUp = 0;
    }
    if(snoozedec == 1)              //if the decrement button is pressed decrement the hours or minutes based on which is blinking
    {
        if(setTime == 1)
        {
            hours--;
            if(hours == 0)          //if the hours are 0 reset them to 12 and increment am_pm
                {
                    am_pm++;
                    hours = 12;
                }
        }
        else
        {
            minutes--;
            if(minutes == 255)      //because minutes is uint after 0 it will go back to 255 so if it goes to 255 reset it to 59
                {
                    minutes = 59;
                }
        }
        snoozedec = 0;              //reset the snooze flag
    }
}
