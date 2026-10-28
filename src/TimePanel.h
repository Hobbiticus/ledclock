#pragma once

class TimePanel
{
public:
    TimePanel(int clockPin, int latchPin, int dataPin, int enablePin, int pwmChannel, int colonPin, int colonChannel);
    void Init();

    void SetTime(int hours, int minutes);

private:
    int m_ClockPin;
    int m_LatchPin;
    int m_DataPin;
    int m_EnablePin;
    int m_PWMChannel;
    int m_ColonPin;
    int m_ColonChannel;

    void ShiftDigit(unsigned char digit);
};
