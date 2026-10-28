#include "TimePanel.h"
#include <Arduino.h>

TimePanel::TimePanel(int clockPin, int latchPin, int dataPin, int enablePin, int pwmChannel, int colonPin, int colonChannel)
: m_ClockPin(clockPin), m_LatchPin(latchPin), m_DataPin(dataPin), m_EnablePin(enablePin), m_PWMChannel(pwmChannel), m_ColonPin(colonPin), m_ColonChannel(colonChannel)
{

}

void TimePanel::Init()
{
    pinMode(m_ClockPin, OUTPUT);
    pinMode(m_LatchPin, OUTPUT);
    pinMode(m_DataPin, OUTPUT);

    ledcSetup(m_PWMChannel, 5000, 8);
    ledcAttachPin(m_EnablePin, m_PWMChannel);
    ledcSetup(m_ColonChannel, 5000, 8);
    ledcAttachPin(m_ColonPin, m_ColonChannel);

}

const static unsigned char STC = 1 << 5; //top center
const static unsigned char STL = 1 << 6; //top left
const static unsigned char STR = 1 << 4; //top right
const static unsigned char SMC = 1 << 2; //middle center
const static unsigned char SBL = 1 << 7; //bottom left
const static unsigned char SBR = 1 << 1; //bottom right
const static unsigned char SBC = 1 << 3; //bottom center
const static unsigned char DOT = 1 << 0; //decimal point

const static unsigned char NumberMap[10] =
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
STC | STL | STR | SMC | SBR | SBC        //9
};

void TimePanel::ShiftDigit(unsigned char digit)
{
    shiftOut(m_DataPin, m_ClockPin, MSBFIRST, NumberMap[digit]);
}

void TimePanel::SetTime(int hours, int minutes)
{
    digitalWrite(m_ClockPin, LOW);
    digitalWrite(m_LatchPin, LOW);

    //these go left to right
    ShiftDigit(hours / 10);
    ShiftDigit(hours % 10);
    ShiftDigit(minutes / 10);
    ShiftDigit(minutes % 10);

    // Explicitly pull clock low after shifting finishes, then latch
    digitalWrite(m_ClockPin, LOW);
    digitalWrite(m_LatchPin, HIGH);
}