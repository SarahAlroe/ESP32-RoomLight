#ifndef Type_h
#define Type_h

#include "Arduino.h"
#include "NeoPixelAnimator.h"

class Type
{
  public:
    Type();
    Type(AnimUpdateCallback animFunc, String desc);
    
    void setFunction(AnimUpdateCallback animFunc);
    void setDesc(String desc);
    void setSettingsHTML(String settingsHTML);
    
    AnimUpdateCallback getFunction();
    String getDesc();
    String getSettingsHTML();

  private:
    AnimUpdateCallback _animFunc;
    String _desc;
    String _settingsHTML;
};

#endif
