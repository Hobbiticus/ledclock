#pragma once

class TemperaturePanel
{
public:
    TemperaturePanel(int clockPin, int latchPin, int dataPin, int enablePin, int pwmChannel);
    void Init();

    void SetHighLow(int high, int low);
    void SetCurrent(int temp);

    enum
    {
        ROW_HIGH = 1 << 0,
        ROW_CURR = 1 << 1,
        ROW_LOW = 1 << 2
    };
    void EnableAnimation(unsigned char whichRows) { m_AnimateRows |= whichRows; }
    void StepAnimation();

private:
    int m_ClockPin;
    int m_LatchPin;
    int m_DataPin;
    int m_EnablePin;
    int m_PWMChannel;

    int m_High;
    int m_Current;
    int m_Low;

    unsigned char m_AnimateRows;
    unsigned char m_AnimationStep;

    void UpdateDisplay();
    void ShiftTemperature(int temp);
    void ShiftDigit(unsigned char digit);
};
