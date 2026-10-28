#include "TemperaturePanel.h"
#include <Arduino.h>

TemperaturePanel::TemperaturePanel(int clockPin, int latchPin, int dataPin, int enablePin, int pwmChannel)
: m_ClockPin(clockPin), m_LatchPin(latchPin), m_DataPin(dataPin), m_EnablePin(enablePin), m_PWMChannel(pwmChannel),
m_High(-100), m_Current(-100), m_Low(-100)
{

}
void TemperaturePanel::Init()
{
    pinMode(m_ClockPin, OUTPUT);
    pinMode(m_LatchPin, OUTPUT);
    pinMode(m_DataPin, OUTPUT);

    ledcSetup(m_PWMChannel, 5000, 8);
    ledcAttachPin(m_EnablePin, m_PWMChannel);
}

void TemperaturePanel::SetHighLow(int high, int low)
{
    m_High = high;
    m_Low = low;
    UpdateDisplay();
}

void TemperaturePanel::SetCurrent(int temp)
{
    m_Current = temp;
    UpdateDisplay();
}

const static unsigned char STC = 1 << 1; //top center
const static unsigned char STL = 1 << 0; //top left
const static unsigned char STR = 1 << 2; //top right
const static unsigned char SMC = 1 << 3; //middle center
const static unsigned char SBL = 1 << 7; //bottom left
const static unsigned char SBR = 1 << 4; //bottom right
const static unsigned char SBC = 1 << 6; //bottom center
const static unsigned char DOT = 1 << 5; //decimal point

const static unsigned char NumberMap[12] =
{
    STC | STL | STR | SBL | SBR | SBC,       //0
    STR | SBR,                               //1
    STC | STR | SMC | SBL | SBC,             //2
    STC | STR | SMC | SBR | SBC,             //3
    STL | STR | SMC | SBR,                   //4
    STC | STL | SMC | SBR | SBC,             //5
    STC | STL | SMC | SBL | SBR | SBC,       //6
    STC | STR | SBR,                         //7
    STC | STL | STR | SMC | SBL | SBR | SBC, //8
    STC | STL | STR | SMC | SBR | SBC,       //9
    0,   //10 = blank
    SMC, //11 = minus
};

void TemperaturePanel::UpdateDisplay()
{
    digitalWrite(m_ClockPin, LOW);
    digitalWrite(m_LatchPin, LOW);

    //first high, then current, then low
    ShiftTemperature(m_High);
    ShiftTemperature(m_Current);
    ShiftTemperature(m_Low);

    // Explicitly pull clock low after shifting finishes, then latch
    digitalWrite(m_ClockPin, LOW);
    digitalWrite(m_LatchPin, HIGH);
}

void TemperaturePanel::ShiftTemperature(int temp)
{
    //these go right to left
    if (temp > -100)
    {
        ShiftDigit(abs(temp) % 10);
        int middle = (abs(temp) / 10) % 10;
        if (temp > -10 && temp < 10)
            ShiftDigit(10); //blank
        else
            ShiftDigit((unsigned char)middle);
        if (temp < 0)
            ShiftDigit(11); //negative
        else if (temp >= 100)
            ShiftDigit((abs(temp) / 100) % 10);
        else
            ShiftDigit(10); //blank
    }
    else
    {
        //blank it out
        ShiftDigit(10);
        ShiftDigit(10);
        ShiftDigit(10);
    }
}

void TemperaturePanel::ShiftDigit(unsigned char digit)
{
    shiftOut(m_DataPin, m_ClockPin, MSBFIRST, NumberMap[digit]);
}
