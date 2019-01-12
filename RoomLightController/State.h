#ifndef State_h
#define State_h

#include "Arduino.h"
#include <NeoPixelBus.h>

class State
{
  public:
    State();
    
    void setType(int type);
    void setColor(int i, RgbwColor color);
    void setDynTemp(unsigned int dayTemp, unsigned int nightTemp, unsigned int dayTime, unsigned int nightTime, unsigned int transTime);
    void setAnimationDelay(unsigned int animDelay);
    void setGammaCorrect(bool gc);

    int getType();
    RgbwColor getColor(int i);
    unsigned int getDayTemp();
    unsigned int getNightTemp();
    unsigned int getDayTime();
    unsigned int getNightTime();
    unsigned int getTransTime();
    unsigned int getAnimationDelay();
    bool getGammaCorrect();

    const int colorCount = 12;

  private:
    int _type;
    RgbwColor _colors[12];
    unsigned int _dayTemp;
    unsigned int _nightTemp;
    unsigned int _dayTime;
    unsigned int _nightTime;
    unsigned int _transTime;
    unsigned int _animDelay;
    bool _gammaCorrect;
};

#endif
