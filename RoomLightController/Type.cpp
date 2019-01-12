#include "Arduino.h"
#include "Type.h"


Type::Type() {}
Type::Type(AnimUpdateCallback animFunc, String desc)
{
  _animFunc = animFunc;
  _desc = desc;
}

void Type::setFunction(AnimUpdateCallback animFunc) {
  _animFunc = animFunc;
}

void Type::setDesc(String desc) {
  _desc = desc;
}

void Type::setSettingsHTML(String settingsHTML) {
  _settingsHTML = settingsHTML;
}

AnimUpdateCallback Type::getFunction() {
  return _animFunc;
}

String Type::getDesc() {
  return _desc;
}

String Type::getSettingsHTML() {
  return _settingsHTML;
}
