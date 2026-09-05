#include <Arduino.h>

#define DATA_PIN 1
#define CLOCK_PIN 2

const static unsigned char STC = 1 << 0; //top center
const static unsigned char STL = 1 << 1; //top left
const static unsigned char STR = 1 << 2; //top right
const static unsigned char SMC = 1 << 3; //middle center
const static unsigned char SBL = 1 << 4; //bottom left
const static unsigned char SBR = 1 << 5; //bottom right
const static unsigned char SBC = 1 << 6; //bottom center
const static unsigned char DOT = 1 << 7; //decimal point

const static unsigned char NumberMap[10] =
{
STC | STL | STR | SBL | SBR | SBC,       //0
STL | SBR,                               //1
STC | STR | SMC | SBL | SBC,             //2
STC | STR | SMC | SBR | SBC,             //3
STL | STR | SMC | SBR,                   //4
STC | STL | SMC | SBR | SBC,             //5
STC | STL | SMC | SBL | SBR | SBC,       //6
STC | STR | SBR,                         //7
STC | STL | STR | SMC | SBL | SBR | SBC, //8
STC | STL | STR | SMC | SBL | SBC        //9
};

unsigned char digit = 0;
void setup()
{
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  digit = 0;
}

void loop()
{
  unsigned char dots = digit % 2 == 0 ? DOT : 0;
  shiftIn(DATA_PIN, CLOCK_PIN, NumberMap[digit] | dots);
  shiftIn(DATA_PIN, CLOCK_PIN, NumberMap[digit] | dots);

  digit++;
  if (digit > 9)
    digit = 0;

    delay(1000);
}
