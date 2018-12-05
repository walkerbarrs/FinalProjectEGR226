#include "msp.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#define BUFFERSIZE 100

/*
 * Name:Walker Barrs and Mith Havercroft
 * Date:11/14/2018
 * Professor: Professor Zuidema
 * Class:   EGR226
 * Description: This is a code for an alarm clock that allows the user
 * to set the time and alarm using pushbuttons
 */

uint8_t alarmChange = 1;
uint8_t alarmFlag = 0;
uint8_t alarmSound = 0;
uint8_t speaker = 0;
uint8_t lightcount = 0;
uint8_t tempMinutes = 0;    //variable to hold the temporary Alarm minutes to compare with time
uint8_t tempHours = 0;      //variable to hold the temporary alarm hours to compare with time
uint8_t tempAmpm = 0;       //variable to hold the temporary alarm am_pm to compar with time
float nADC = 0;             //Variable to hold the adjusted voltage from the potentiometer
float potVal = 0;           //variable to hold the clock cycles for the voltage of the potentiometer
float Ftemp = 0;            //variable used to hold the temperature from the temp sensor
uint8_t hours = 12;         //Hours, minutes, seconds for time
uint8_t minutes = 58;
uint8_t seconds = 53;
uint8_t Ahours = 1;         //Hours, minutes, seconds for Alarm
uint8_t Aminutes = 4;
uint8_t Aseconds = 0;
uint32_t Aam_pm = 2;         //%2 = 0 for am %2 = 1 for pm
uint32_t am_pm = 1;          //%2 = 0 for am %2 = 1 for pm
int counting = 0;           //variable to hold the counting flag for the time
int clockChange = 1;        //Variable to tell the program if it should count normal or fast
uint8_t currentState = 0;   //this will tell the handlers when to act and when to not so that weird things arent updated when they are not supposed to
uint8_t setTime = 0;        //this told the program to switch to the set time state
uint8_t blinking = 0;       //this variable allowed the hours, minutes, or seconds to blink while setting the time or the alarm
uint8_t onOffUp = 0;        //this held the value of button that in counting enabled the alarm and in set time and set alarm incremented the blinking variable
uint8_t alarmSet = 0;       //Variable to keep track of whether the alarm is on or off
uint8_t snoozedec = 0;      //this allowed the user when in counting to snooze the alarm for 10 minutes and in set time and set alarm decremented the blinking variable
uint8_t setalarm = 0;       //this variable tells the program when in counting that it needs to go to the set alarm state
uint8_t UARTflag = 0;       //this flag tells the program whether the user has entered something via an external serial
uint8_t uartTime = 0;       //if the user sends send a time through an external serial this flag goes off
uint8_t uartAlarm = 0;      //if the user sends send an alarm time through an external serial this flag goes off
uint8_t wakeup = 0;         //this variable tells the program whether to start increasing the LEDs or not
uint16_t i = 0;             //this holds the counter for all the print functions
uint8_t lights = 0;         //global variable used to hold the interrupt flag
uint8_t LED =  0;           //global variable used to hold the PWM signal of the lighs
char input[BUFFERSIZE];     //Array to hold the user's input

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
void onOffUpBtn();
void snoozeDec();
void PortADC_init(void);
void ADC14_init(void);
void Get_ADC_Convert_Temp(void);
void blinkALRMMinutes();
void blinkALRMHours();
void setAlarm();
void serialInput();
void uartUpdate();
void writeOut();
void PWMlights_init();
void PWMlights_update();
void Lightsintrpt();
void T32_INT2_IRQHandler(void);
void Speaker_update();
void Speaker_intrpt();
void PWMSpeaker_init();
void alarmSetConditions();
void alarmSoundConditions();
void setTimeConditions();
void setAlarmChanges();

void main(void)
{

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    //Initialize custom functions
    ADCpin_init();
    ADC_init();
    Systick_init();
    TimerA_init();
    LCD_init();
    ms_delay(50);
    PWMlights_init();
    serialInput();
    clockChangeInit();
    setTimeInit();
    countIntrpt();
    onOffUpBtn();
    ADC14_init();
    setAlarm();
    snoozeDec();
    PWMSpeaker_init();
    __enable_irq();
    enum states state = COUNTING;    //Default to LED OFF and create state variable of type states

    while(1)
    {
        switch(state)
        {
        case COUNTING:
            if(counting == 1)
            {
                readPotentiometer();           //Read the voltage from the potentiometer and temp
                updateClock();                 //Update the clock
                readPotentiometer();           //Read the voltage from the potentiometer
                checkClockChange();            //Check to see if we want to count normal or fast
                counting  = 0;                 //reset counting
                if(onOffUp == 1)
                {
                    if(alarmSet == 0) alarmSet = 1;
                    else alarmSet = 0;
                    onOffUp = 0;
                }
                if(alarmSet == 1)              //if the alarm is enabled
                {
                    alarmSetConditions();   //custom function to check the inputs while the alarm is set
                }
                else
                {
                    lights = 1;
                    LED = 0;
                    PWMlights_update();
                    lights = 0;
                }
                printVals();                   //print the values to the LCD
                if(alarmSound)
                {
                    alarmSoundConditions(); //custom function to check the inputs while the alarm is sounding
                }

                if(UARTflag)                //If the uart flag goes off update the time, alarm, etc based on inputs and reset the flag and the counter to store the input array
                    {
                        uartUpdate();
                        UARTflag = 0;
                        i = 0;
                    }
                readPotentiometer();           //Read the voltage from the potentiometer and temp
                if(setTime == 1) currentState = 1;
                if(setalarm == 1) currentState = 2;
                if(currentState == 1) state = SET_TIME;     //if current state == 1 set state to set time
                if(currentState == 2) state = SET_ALARM;    //if current state == 2 set state to set alarm
              //  P3->IE |= 0xEC;                             //turn all the interrupts back on
            }
            //P3->IE |= 0xEC;                             //turn all the interrupts back on
            break;

        case SET_TIME:
            if(setTime == 1)    blinkCLKHours();            //if set time is only pressed once blink the hours
            else if(setTime == 2)   blinkCLKMinutes();      //if set time is pressed twice blink the minutes
            checkClockChange();     //check to make sure the fast counting hasnt been pushed
            setTimeConditions();
            if(currentState == 0)           //if current state == 0 reset the setTime flag and go to counting
            {
                setTime = 0;
                setalarm = 0;
                state = COUNTING;
            }
            //P3->IE |=0xEC;          //turn on all interrupts back on
        break;

        case SET_ALARM:
                if(setalarm == 1)   blinkALRMHours();           //if setalarm is 1 blink the hours
                else if(setalarm == 2)  blinkALRMMinutes();     //if setalarm is 2 blink the minutes
                checkClockChange();                             //keep track of whether the clock change is on fast or slow mode
                setAlarmChanges();
                if(counting == 1)
                    {
                        updateClock();
                        counting = 0;
                    }
                if(setalarm == 3) currentState = 0;
                if(currentState == 0)           //if current state is 0 reset setalarm flag and turn alarm set flag on and go to counting state
                {
                    setalarm = 0;
                    setTime = 0;
                    alarmSet = 1;
                    alarmChange = 1;
                    state = COUNTING;
                }
              //  P3->IE |=0xEC;                          //turn the interrupts back on
        break;
        }
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

    ADC14->CTL0 |= 0x00000001;  //start conversion
    while(!ADC14->IFGR0);
    result = ADC14->MEM[0];
    nADC = ((result *3.3)/ 16384);     //result * vref / 2^14 == resolution
    potVal = (nADC / 3.3)*(60000 - 1);        //Get the percentage of potval to vRef and multiply by the Period
    updateTimerA(potVal);                   //Update Timer A with the value of potVal
}
//Custom function to update LED 2.0 and 2.1 with the new intensity
void updateTimerA(float potVal)
{
    TIMER_A2->CCR[4] = potVal; //put the result of potval into the timer controlling the backlight intensity of the LCD
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
    us_delay(500);

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

    LCD_command(0x01);  //clear display
    us_delay(500);
    LCD_command(0x80);  //start on first line

    if((am_pm %2) == 1) //if in am check condition to see which string will be stored into currentTime
    {
        if(seconds < 10 && minutes >= 10)   //if seconds less then 10 and minutes >= 10 store this string
           sprintf(currentTime,"%2d:%d:0%d am      ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds less than 10 and minutes < 10 store this string
            sprintf(currentTime,"%2d:0%d:0%d am      ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if minutes <10 and seconds >= 10 store this string
            sprintf(currentTime,"%2d:0%d:%d am       ", hours, minutes, seconds);
        else    //else just store this one
            sprintf(currentTime,"%2d:%d:%d am        ", hours, minutes, seconds);
    }
    else    //else the clock must be in pm mode
    {
        if(seconds < 10 && minutes >= 10) //if seconds less then 10 and minutes >= 10 store this string
           sprintf(currentTime,"%2d:%d:0%d pm       ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds less than 10 and minutes < 10 store this string
            sprintf(currentTime,"%2d:0%d:0%d pm     ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if minutes <10 and seconds >= 10 store this string
            sprintf(currentTime,"%2d:0%d:%d pm      ", hours, minutes, seconds);
        else    //else just store this one
            sprintf(currentTime,"%2d:%d:%d pm     ", hours, minutes, seconds);
    }
    if((Aam_pm %2) == 1)    //now check the alarm time if in am do this
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds less then 10 and minutes >= 10 store this string
           sprintf(currentAlarm,"%2d:%d:0%d am      ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10) //if seconds less than 10 and minutes < 10 store this string
            sprintf(currentAlarm,"%2d:0%d:0%d am     ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if minutes <10 and seconds >= 10 store this string
            sprintf(currentAlarm,"%2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else    //else just store this one
            sprintf(currentAlarm,"%2d:%d:%d am    ", Ahours, Aminutes, Aseconds);
    }
    else        //if it is not am it must be om so do this
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds less then and minutes >= 10 store this string
           sprintf(currentAlarm,"%2d:%d:0%d pm      ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10) //if seconds less than 10 and minutes < 10 store this string
            sprintf(currentAlarm,"%2d:0%d:0%d pm      ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if minutes <10 and seconds >= 10 store this string
            sprintf(currentAlarm,"%2d:0%d:%d pm     ", Ahours, Aminutes, Aseconds);
        else    //else just store this one
            sprintf(currentAlarm,"%2d:%d:%d pm      ", Ahours, Aminutes, Aseconds);
    }
    if(alarmSet == 1)    //if the alarm is set store alarm enabled into alarmonoff
    {
        sprintf(alarmOnOff, "Alarm  Enabled  ");
    }
    else    //else the alarm is disabled
    {
        sprintf(alarmOnOff, "Alarm Disabled  ");
    }

    sprintf(currentTemp, "F:  %.1f          ", Ftemp);  //Store the current temp reading into the current temp string

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTime[i]);     //send currentTime to LCD
        us_delay(1);
    }

    LCD_command(0xC0);  //go to 2nd line
    for(i=0;i<16;i++)
    {
        LCD_data(alarmOnOff[i]);    //Send alarmonoff to LCD
    }
    LCD_command(0x90);  //go to third line
    for(i=0;i<16;i++)
    {
        LCD_data(currentAlarm[i]);  //send currentalarm to LCD
    }

    LCD_command(0xD0);  //start on fourth line

    for(i=0; i<16; i++)     //run through string length
    {
        LCD_data(currentTemp[i]);     //send currentTemp to the LCD
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
        seconds ++;             //everytime this function is called add one to seconds
        if(seconds == 60)       //if seconds == 60 reset seconds and increment minutes
            {
                seconds = 0;
                minutes++;
            }
        if(minutes == 60)       //if minutes == 60 reset minutes and increment hours
        {
            minutes = 0;
            hours++;
        }
        if(hours == 13)         //if hours == 13 reset hours and increment am_pm
        {
          hours = 1;
          am_pm++;
        }
}
//Custom function to initialize the clockchange button with an interrupt
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
    P3->IE  |= BIT6;            //Enable an interrupt on P3.6
    P3->IFG = 0;                //Initialize interrupt flag to 0
}
//Custom function add one to clockChange when the button is pressed with no output
void PORT3_IRQHandler(void)
{
    if(currentState == 0)   //counting state
    {
        if(P3->IFG & BIT7)          //If the interrupt is on P3.6
        {
            clockChange++;          //add one to clock change
            P3->IFG &= ~ BIT7;      //Turn the flag off
            P3->IE &= ~ BIT7;       //turn off interrupt
        }
        else if(P3->IFG & BIT6)          //If the interrupt is on P3.6
        {
            setTime = 1;            //set settime to 1
            currentState = 1;       //set current state to 1
            P3->IFG &= ~ BIT6;      //Turn the flag off
            P3->IE &= ~ BIT6;       //turn off interrupt
        }
        else if(P3->IFG & BIT5)             //if the interrupt is on 3.5
        {
            if(alarmSet == 0) alarmSet = 1; //toggle alarmset
            else if(alarmSet == 1) alarmSet = 0;
            ms_delay(5);
            P3->IFG &= ~ BIT5;          //turn off the flag
            P3->IE &=~ BIT5;            //turn off the interrupt
        }
        else if(P3->IFG & BIT3)         //if the interrupt is on 3.3
        {
            if(alarmSet == 1)           //if the alarm is set set snooze flag to 1
            snoozedec = 1;
            P3->IFG &= ~BIT3;           //turn off the flag
            P3->IE &= ~ BIT3;           //turn the interrupt off
        }
        else if(P3->IFG & BIT2)         //if the flag is on 3.2
        {
            currentState = 2;           //set current state to 2
            setalarm = 1;               //set set alarm to 1
            P3->IFG &= ~ BIT2;          //turn off the flag
            P3->IE &= ~ BIT2;           //turn off the interrupt
        }
    }
    else if(currentState == 1)  //set time state
    {
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
       {
            setTime ++;          //increment set time
            if(setTime == 3)
                currentState = 0;       //if set time is 3 set the current state to 0 (counting)
            P3->IFG &= ~ BIT6;      //Turn the flag off
            P3->IE &= ~ BIT6;       //turn off the interrupt
       }
        else if(P3->IFG & BIT5)             //if the interrupt is on 3.5
       {
           onOffUp = 1;                     //set up to 1
           P3->IFG &= ~BIT5;                //turn off the flag
           P3->IE &= ~BIT5;                 //turn the interrupt off
       }
        else if(P3->IFG & BIT7)             //if the interrupt is on 3.7
       {
           clockChange++;          //add one to clock change
           P3->IFG &= ~BIT7;                //turn off the flag
           P3->IE &= ~BIT7;                 //turn off the interrupt
       }
        else if(P3->IFG & BIT3)             //if the interrupt is on 3.3
        {
            snoozedec = 1;                  //set decrement to 1
            P3->IFG &= ~ BIT3;              //turn off the flag
            P3->IE &= ~BIT3;                //turn off the interrupt
        }
        else if(P3->IFG & BIT2)          //If the interrupt is on P3.6
       {
            P3->IFG &= ~ BIT2;       //Turn the flag off
            P3->IE &= ~ BIT2;        //turn the interrupt off
       }
    }
    else if(currentState == 2)  //if in the set alarm state
    {
        if(P3->IFG & BIT6)          //If the interrupt is on P3.6
       {
            setTime ++;          //increment set time
            if(setTime == 3) currentState = 0; // if set time is 3 set the current state back to 0
            P3->IFG &= ~ BIT6;      //Turn the flag off
            P3->IE &= ~ BIT6;
       }
        else if(P3->IFG & BIT5)         //if the interrupt is on 3.5
       {
           onOffUp = 1;                 //set up to 1
           P3->IFG &= ~BIT5;            //turn off the flag
           P3->IE &= ~BIT5;             //turn off the interrupt
       }
        else if(P3->IFG & BIT7)         //if the interrupt is on 3.7
       {
           clockChange++;          //add one to clock change
           P3->IFG &= ~BIT7;            //turn off the interrupt flag
           P3->IE &= ~BIT7;             //tunr off the interrupt
       }
        else if(P3->IFG & BIT3)         //if the interrupt is on 3.3
        {
            snoozedec = 1;              //set decrement to 1
            P3->IFG &= ~ BIT3;          //turn off the flag
            P3->IE &= ~BIT3;            //turn off the interrupt
        }
        else if(P3->IFG & BIT2)          //If the interrupt is on P3.6
       {
            setalarm++;                     //increment set alarm
            if(setalarm == 3) currentState = 0; //if set alarm is 3 set current state to 0
            P3->IFG &= ~ BIT2;      //Turn the flag off
            P3->IE &= ~ BIT2;       //turn off the interrupt
       }
    }
    ms_delay(10);
}
//Custom function to check whether or not the clock speed button has been pressed and then changes the spped respectively
void checkClockChange()
{
    if((clockChange %2) == 0)
        {
            TIMER32_1->LOAD = 20000;         //Load .0167 seconds into the timer = 3000000
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
    if(blinking == 0)   //if blkining is 0 print the hours, minutes, and seconds to the LCD
    {
    if((am_pm %2) == 1) //if am do this
    {
        if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
           sprintf(currentTime,"  %2d:%d:0%d am   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:0%d am   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:%d am   ", hours, minutes, seconds);
        else    //else store this string
            sprintf(currentTime,"  %2d:%d:%d am   ", hours, minutes, seconds);
    }
    else    //else it must be pm
    {
        if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
           sprintf(currentTime,"  %2d:%d:0%d pm   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:0%d pm   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:%d pm   ", hours, minutes, seconds);
        else    //Else store this string
            sprintf(currentTime,"  %2d:%d:%d pm   ", hours, minutes, seconds);
    }
    }
    else if(blinking == 1)  //if blinking is one print only the minutes and seconds to the LCD
    {
        if((am_pm %2) == 1) //if alarm is am do this
        {
            if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
               sprintf(currentTime,"    :%d:0%d am   ", minutes, seconds);
            else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
                sprintf(currentTime,"    :0%d:0%d am   ", minutes, seconds);
            else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentTime,"    :0%d:%d am   ", minutes, seconds);
            else    //else store this string
                sprintf(currentTime,"    :%d:%d am   ", minutes, seconds);
        }
        else    //else it must be in pm so do this
        {
            if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
               sprintf(currentTime,"    :%d:0%d pm   ", minutes, seconds);
            else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
                sprintf(currentTime,"    :0%d:0%d pm   ", minutes, seconds);
            else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentTime,"    :0%d:%d pm   ", minutes, seconds);
            else    //else just store this string
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
    if (blinking) blinking = 0; //toggle blinking evertime this function is called
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkCLKMinutes()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentTime[17];
    if(blinking == 0)   //if blinking is 0 print the hours, minutes, and seconds to the LCD
    {
    if((am_pm %2) == 1) //If in am do this
    {
        if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
           sprintf(currentTime,"  %2d:%d:0%d am   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:0%d am   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:%d am   ", hours, minutes, seconds);
        else    //else just store this string
            sprintf(currentTime,"  %2d:%d:%d am   ", hours, minutes, seconds);
    }
    else    //else it must be in pm mode
    {
        if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
           sprintf(currentTime,"  %2d:%d:0%d pm   ", hours, minutes, seconds);
        else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:0%d pm   ", hours, minutes, seconds);
        else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentTime,"  %2d:0%d:%d pm   ", hours, minutes, seconds);
        else    //else just print this string
            sprintf(currentTime,"  %2d:%d:%d pm   ", hours, minutes, seconds);
    }
    }
    else if(blinking == 1)  //if blinking is 1 dont print the minutes to the LCD
    {
        if((am_pm %2) == 1) //if in am mode do this
        {
            if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
               sprintf(currentTime,"  %2d:  :0%d am   ", hours, seconds);
            else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
                sprintf(currentTime,"  %2d:  :0%d am   ", hours, seconds);
            else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentTime,"  %2d:  :%d am   ", hours, seconds);
            else    //else store this string
                sprintf(currentTime,"  %2d:  :%d am   ", hours, seconds);
        }
        else    //else it must be in pm mode
        {
            if(seconds < 10 && minutes >= 10)   //if seconds <10 and minutes >= 10 store this string
               sprintf(currentTime,"  %2d:  :0%d pm   ", hours, seconds);
            else if(seconds < 10 && minutes < 10)   //if seconds <10 and minutes < 10 store this string
                sprintf(currentTime,"  %2d:  :0%d pm   ", hours, seconds);
            else if(minutes < 10 && seconds >=10)   //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentTime,"  %2d:  :%d pm   ", hours, seconds);
            else    //else store this string
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
    if (blinking) blinking = 0; //toggle blinking everytime the function is called
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
    P3->IE  |= BIT5;            //Enable an interrupt on P3.5
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
    P3->IE  |= BIT3;            //Enable an interrupt on P3.3
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
    P3->IE  |= BIT2;            //Enable an interrupt on P3.2
    P3->IFG = 0;                //Initialize interrupt flag to 0
    NVIC->ISER[1] = 1 <<((PORT3_IRQn)& 31);         //This initializes which interrupt is going to be used
}
//custom function to initialize ADC for the temp sensor
void ADC14_init(void)
    {
    P5->SEL0 |= BIT4;                           //Configure pin 5.4 to A0 Input
    P5->SEL1 |= BIT4;

    ADC14->CTL0 &=~ ADC14_CTL0_ENC;         // disable ADC converter during initialization (BIT1 goes to zero)
    ADC14->CTL0 |= 0x04200210;              // S/H pulse mode, SMCLK, 16 sample clocks
    ADC14->CTL1 = 0x00000030;               // 14 bit resolution
    ADC14->CTL1 |= 0x00000000;              // convert for mem0 register
    ADC14->MCTL[1] = 0x00000001;            // ADC14INCHx = 0 for mem[1]
    ADC14->CTL0 |= ADC14_CTL0_ENC;          // Enable ADC14ENC, starts the ADC after configuration (BIT1 goes to 1)
    }
//custom function to convert the reading into a temperature
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
    if(blinking == 0)   //if blinking is 0 print the hours, minutes, and seconds to the LCD
    {
    if((Aam_pm %2) == 1)    //if it is in am mode
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
           sprintf(currentAlarm,"  %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else    //else store this string
            sprintf(currentAlarm,"  %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
    else    //else it must be in pm mode
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
           sprintf(currentAlarm,"  %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else    //else store this string
            sprintf(currentAlarm,"  %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
    }
    else if(blinking == 1)  //if blinking is 1 only print the minutes and seconds to the LCD
    {
        if((Aam_pm %2) == 1)    //if in am mode
        {
            if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
               sprintf(currentAlarm,"    :%d:0%d am   ", Aminutes, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
                sprintf(currentAlarm,"    :0%d:0%d am   ", Aminutes, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentAlarm,"    :0%d:%d am   ", Aminutes, Aseconds);
            else    //else just store this string
                sprintf(currentAlarm,"    :%d:%d am   ", Aminutes, Aseconds);
        }
        else    //else it must be in pm mode
        {
            if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
               sprintf(currentAlarm,"    :%d:0%d pm   ", Aminutes, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
                sprintf(currentAlarm,"    :0%d:0%d pm   ", Aminutes, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentAlarm,"    :0%d:%d pm   ", Aminutes, Aseconds);
            else    //else store this string
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
    if (blinking) blinking = 0;     //toggle blinking everytime the function is called
    else blinking = 1;
}
//This is a custom function that will blink the hours, minutes or seconds depending on what the user wants to change
void blinkALRMMinutes()
{
    int i = 0;//initialize i at zero for the for loop counter
    //Store the strings that need to sent to the LCD into four strings
    char currentAlarm[17];
    if(blinking == 0)   //if blinking is 0
    {
    if((Aam_pm %2) == 1)    //if in am mode
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
           sprintf(currentAlarm,"  %2d:%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:0%d am   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:%d am   ", Ahours, Aminutes, Aseconds);
        else    //else store this string
            sprintf(currentAlarm,"  %2d:%d:%d am   ", Ahours, Aminutes, Aseconds);
    }
    else    //else it must be in pm mode
    {
        if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
           sprintf(currentAlarm,"  %2d:%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aseconds < 10 && Aminutes < 10)//if seconds <10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:0%d pm   ", Ahours, Aminutes, Aseconds);
        else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
            sprintf(currentAlarm,"  %2d:0%d:%d pm   ", Ahours, Aminutes, Aseconds);
        else //else store this string
            sprintf(currentAlarm,"  %2d:%d:%d pm   ", Ahours, Aminutes, Aseconds);
    }
    }
    else if(blinking == 1)  //if blinking is one only print the hours and seconds to the LCD
    {
        if((Aam_pm %2) == 1)    //if in am mode
        {
            if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
               sprintf(currentAlarm,"  %2d:  :0%d am   ", Ahours, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
                sprintf(currentAlarm,"  %2d:  :0%d am   ", Ahours, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentAlarm,"  %2d:  :%d am   ", Ahours, Aseconds);
            else    //store this string
                sprintf(currentAlarm,"  %2d:  :%d am   ", Ahours, Aseconds);
        }
        else    //else it must be in pm mode
        {
            if(Aseconds < 10 && Aminutes >= 10) //if seconds <10 and minutes >= 10 store this string
               sprintf(currentAlarm,"  %2d:  :0%d pm   ", Ahours, Aseconds);
            else if(Aseconds < 10 && Aminutes < 10) //if seconds <10 and minutes < 10 store this string
                sprintf(currentAlarm,"  %2d:  :0%d pm   ", Ahours, Aseconds);
            else if(Aminutes < 10 && Aseconds >=10) //if seconds >= 10 and minutes < 10 store this string
                sprintf(currentAlarm,"  %2d:  :%d pm   ", Ahours, Aseconds);
            else    //store this string
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
    if (blinking) blinking = 0; //toggle blinking everytime this funciton is called
    else blinking = 1;
}
//Custom function to initialize the serial for UART with no outputs
void serialInput()
{
    EUSCI_A0 -> CTLW0 |= 1;     //set the EUSCI in reset mode
    EUSCI_A0->MCTLW = 0XAA81;   //BRSx = 0xAA BRFx = 0x8 OS16 = 0x1
    EUSCI_A0 -> CTLW0 = 0X00C1; //Set ctlw0 for SMCLK
    EUSCI_A0->BRW = 19;         // == (clock freq / baud rate) / 16 == (3000000 / 9600)/ 16 = 19
    P1-> SEL0 |= 0X0C;          //Initialize 1.2 and 1.3 for uart
    P1-> SEL1 &= ~0X0C;
    EUSCI_A0 -> CTLW0 &= ~1;    //diable reset mode
    EUSCI_A0->IFG &= ~BIT0;    // Clear interrupt
    EUSCI_A0->IE |= BIT0;      // Enable interrupt
    NVIC->ISER[0] = 1 <<((EUSCIA0_IRQn)& 31);      //This initializes which interrupt is going to be used
}
//Custom function to store the user's input into an array until \n is pressed
void EUSCIA0_IRQHandler(void)
{
    if(EUSCI_A0->IFG & BIT0)  // Interrupt on the receive line
    {
        //Strore character into array
        input[i] = EUSCI_A0->RXBUF;
        //If the input is \n set flag
        if(input[i] == '\n')
            {
                UARTflag = 1;
                input[i] = '\0';
            }
        //else i is decremented
        else    i++;
        //If i is 0 restart
        if(i == BUFFERSIZE - 1)  i = 0;
        //Clear interrupt flag
        EUSCI_A0->IFG = 0;
    }
}
//Custom function to check the input from uart and convert to output
void uartUpdate()
{
    //initialize variables
    char *uartcommand;  //holds the first string once tokenized
    char *uarthour;     //holds the second string once tokenized
    char *uartminutes;  //holds the third string once tokenized
    char *uartseconds;  //holds the fourth string once tokenized
    int temphour, tempmin, tempsec; //holds temporary values for hours, minutes, and seconds
    char del1[50] = " ";    //1st deliminator string
    char del2[50] = ":";    //2nd deliminator string
    char del3[50] = "\0";   //3rd deliminator string

    uartcommand = strtok(input, del1);  //tokenize till the 1st delim string and store result in uart command

    uarthour = strtok(NULL, del2);      //tokenize till the 2nd delim string and store result in uarthour
    uartminutes = strtok(NULL, del2);   //tokenize till the 2nd delim string and store result in uartmin
    uartseconds = strtok(NULL, del3);   //tokenize till the 3rd delim string and store result in uartseconds
    if(!(strcmp(uartcommand,"SETTIME")))    //if the user wants to set the time
    {
        temphour = atoi(uarthour);      //convert uart hours, minutes, and seconds into a decimal number
        tempmin = atoi(uartminutes);
        tempsec = atoi(uartseconds);
        if((temphour <= 24) && (tempmin <= 59) && (tempsec <= 59))  //if they are all valid numbers store them into the global hours, minutes, seconds
        {
            hours = temphour;
            minutes = tempmin;
            seconds = tempsec;
            if(hours > 12)              //because the user enters military time if the hours is greater then 12. - 12 and increment am_pm only if it is in the wrong state
            {
                hours = hours - 12;
                if((am_pm %2) == 1) am_pm++;
            }
            else                            //else just check am_pm to see if it is in the right state if not increment it
            {
                if((am_pm %2) == 0) am_pm++;
            }
        }

    }
    else if(!(strcmp(uartcommand,"SETALARM")))  //else if the user wants to set the alarm
    {
         temphour = atoi(uarthour);     //store uart hour and minutes into temporary variables
         tempmin = atoi(uartminutes);
        if((temphour <= 24) &&(tempmin <= 59))  //if the temporary variables are valid then store them into the global alarm hours and minutes
        {
            Ahours = temphour;
            Aminutes = tempmin;
            if(Ahours > 12)         //if the alarm hours is greater than 12. -12 and then check to make sure Aam_pm is in pm mode if not increment Aam_pm
            {
                alarmSet = 1;
                Ahours = Ahours - 12;
                if((Aam_pm %2) == 1) Aam_pm++;
            }
            else                    //else check to make sure Aam_pm is in am mode if not increment am_pm
            {
                alarmSet = 1;
                if((Aam_pm %2) == 0) Aam_pm++;
            }
            alarmChange = 1;
        }
    }
    else if(!(strcmp(uartcommand,"READTIME")))  //if the user wants to know the time set uart time to 1 and call write out
    {
        uartTime = 1;
        writeOut();
    }
    else if(!(strcmp(uartcommand,"READALARM"))) //if the user wants to know the alarm time set uart time to 1 and call write out
    {
        uartAlarm = 1;
        writeOut();
    }
}
//Custom function to print alarm and time to serial
void writeOut()
{
    //initialize variables
    char time[9];   //array to hold the current time
    char alarm[5];  //array to hold the current alarm time
    int hour_24 = 0;    //integer to hold the hours in military time
    int min = 0;        //integer to hold the minutes
    int sec = 0;        //integer to hold he seconds
    int n = 0;          //counter used to send the string to the external serial

    if(uartTime)    //if the user wants the time
    {
        hour_24 = hours;    //store the hours into hour_24
        if(((am_pm %2) == 0)) hour_24 = hour_24 + 12;   //if the time is pm add 12
        min = minutes;  //store the minutes into min
        sec = seconds;  //store the seconds into sec
        if(sec < 10 && min >= 10)   //if the sec < 10 and min >= 10 store this string
            sprintf(time,"%2d:%d:0%d\n", hour_24, min, sec);
        else if(sec < 10 && min < 10)   //if the sec < 10 and min < 10 store this string
            sprintf(time,"%2d:0%d:0%d\n", hour_24, min, sec);
        else if(min < 10 && sec >=10)   //if the sec >= 10 and min < 10 store this string
            sprintf(time,"%2d:0%d:%d\n", hour_24, min, sec);
        else    //store this string
            sprintf(time,"%2d:%d:%d\n", hour_24, min, sec);
        //Print the time onto the serial
        for(n = 0; n <9; n++)
        {
            EUSCI_A0->TXBUF = time[n];
            while(!(EUSCI_A0->IFG & BIT1));
        }
    }
    else if(uartAlarm)  //if the user wants to know the alarm time
    {
        hour_24 = Ahours;   //store the hours into hour_24
        if(((Aam_pm%2) == 0)) hour_24 = hour_24 + 12;   //if the alarm is in pm mode add 12 to hour_24
        min = Aminutes; //store minutes into min
        sec = Aseconds; //store seconds into sec
        if(min<10)  //if the minutes is less than 10 store this string
            sprintf(alarm, "%2d:0%d\n", hour_24, min);
        else    //store this string
            sprintf(alarm, "%2d:%d\n", hour_24, min);
        //Print the alarm time the serial
        for(n = 0; n <5; n++)
        {
            EUSCI_A0->TXBUF = alarm[n];
            while(!(EUSCI_A0->IFG & BIT1));
        }
    }
}
//custom function to initialize the PWM lights with no output
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
    if(lights)
    {
        if(LED == 0)
        {
            TIMER_A1->CCR[1] = 0;    //RED LED
            TIMER_A1->CCR[2] = 0;  //Green LED
            TIMER_A1->CCR[3] = 0;    //Blue LED
        }
        else
        {
            TIMER_A1->CCR[1] = ((LED * 600) -1);    //RED LED
            TIMER_A1->CCR[2] = ((LED * 600) -1);  //Green LED
            TIMER_A1->CCR[3] = ((LED * 600) -1);    //Blue LED
        }
            TIMER32_2->LOAD = 3000000;
            lights = 0; //Reset timer 32 interrupt flag

    }
}
//Custom function to initialize the screensaver with an interrupt with no output
void Lightsintrpt()
{
    //initialization for timer 32 interrupt
    TIMER32_2->CONTROL = 0XC2;          //initialize timer 32 for periodic mode, no prescaler, 32-bit timer
    TIMER32_2->LOAD = 3000000;         //Load 3 seconds seconds into the timer = 3000000 * 3
    TIMER32_2->INTCLR = 0;              //Clear interupt flag
    TIMER32_2->CONTROL |= 0X20;         //enable interrupts
    NVIC->ISER[0] = 1 <<((T32_INT2_IRQn)& 31);      //This initializes which interrupt is going to be used

}
//custom functions to check the states of various inputs when in the set time state with no outputs
void setTimeConditions()
{
    if(onOffUp == 1)                //if the up button was presseed increment either the hours, minutes, or seconds based on which one is currently blinking
                {
                   if(setTime == 1)
                   {
                       hours= hours + counter;
                       if(hours >= 13)          //if the hours reach 13 reset them to 1 and increment am_pm
                           {
                               am_pm++;
                               if(counter != 0)
                               hours = (hours - 12);
                               else hours = 1;
                           }
                       counter = 0;
                   }
                   else
                   {
                       minutes = minutes + counter;
                       if(minutes >= 60)        //if the hours reach 60 reset them back to 0
                           {
                               if(counter != 0)
                               minutes = minutes - 60;
                               else minutes = 0;
                           }
                       counter = 0;
                   }
                   onOffUp = 0;
                }
                if(snoozedec == 1)              //if the decrement button is pressed decrement the hours or minutes based on which is blinking
                {
                    if(setTime == 1)
                    {
                        hours = hours - counter;
                        if(hours <= 0)          //if the hours are 0 reset them to 12 and increment am_pm
                            {
                                am_pm++;
                                if(counter != 0)
                                hours = 12 + hours;
                                else hours = 12;

                            }
                        counter = 0;
                    }
                    else
                    {
                        minutes= minutes -counter;
                        if(minutes < 0)      //because minutes is uint after 0 it will go back to 255 so if it goes to 255 reset it to 59
                            {
                                if(counter != 0)
                                minutes = 59 + (minutes + 1);
                                else minutes = 59;
                            }
                        counter = 0;
                    }
                    snoozedec = 0;              //reset the snooze flag
                }
}
//custom functions to check the states of various inputs when in the set alarm state with no outputs
void setAlarmChanges()
{
    if(onOffUp == 1)                                //if up was pressed increment the hours or minutes based on what is blinking
    {
       if(setalarm == 1)
       {
           Ahours = Ahours + counter;
           if(Ahours >= 13)         //if hours - 13 reset to 1 and increment Aam_pm
               {
                   Aam_pm++;
                   if(counter!=0)
                   Ahours = Ahours - 12;
                   else  Ahours = 1;

               }
           counter = 0;
       }
       else if(setalarm == 2)
       {

           Aminutes = Aminutes + counter;
           if(Aminutes >= 60)       //if minutes is 60 reset minutes to 0
               {
                   if(counter!= 0)
                   Aminutes = Aminutes - 60;
                   else  Aminutes = 0;
               }
           counter = 0;
       }
       onOffUp = 0;                 //reset the up flag
    }
    if(snoozedec == 1)              //if decrement was pressed based on what is blinking decrement hours or minutes
    {
        if(setalarm == 1)
        {
            Ahours = Ahours - counter;
            if(Ahours <= 0)         //if hours is 0 reset to 12 and increment Aam_pm
                {
                    Aam_pm++;
                    if(counter != 0)
                    Ahours = 12 + Ahours;
                    else    Ahours = 12;
                }
            counter = 0;
        }
        else if(setalarm == 2)
        {
            Aminutes = Aminutes - counter;
            if(Aminutes < 0)     //if minutes = 255, 0 - 1 = 255, then reset minutes to 59
                {   if(counter != 0)
                    Aminutes = 59 + (Aminutes + 1);

                    else
                        Aminutes = 59;
                }
            counter = 0;
        }
        snoozedec = 0;              //reset the flag back to 0
    }
}

