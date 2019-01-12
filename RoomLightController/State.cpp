#include "Arduino.h"
#include "State.h"

State::State()
{
  _type = 0;
  for (int i = 0; i < colorCount; i++) {
    _colors[i] = RgbwColor(0, 0, 0, 0);
  }
  _dayTemp = 550;
  _nightTemp = 200;
  _dayTime = 7 * 60 + 30;
  _nightTime = 22 * 60;
  _transTime = 60;
  _animDelay = 5000;
}

void State::setType(int type) {
  _type = type;
}

void State::setColor(int i, RgbwColor color) {
  _colors[i] = color;
}

void State::setDynTemp(unsigned int dayTemp, unsigned int nightTemp, unsigned int dayTime, unsigned int nightTime, unsigned int transTime) {
  _dayTemp = dayTemp;
  _nightTemp = nightTemp;
  _dayTime = dayTime;
  _nightTime = nightTime;
  _transTime = transTime;
}

void State::setAnimationDelay(unsigned int animDelay) {
  _animDelay = animDelay;
}

void State::setGammaCorrect(bool gc) {
  _gammaCorrect = gc;
}

int State::getType() {
  return _type;
}

RgbwColor State::getColor(int i) {
  return _colors[i];
}

unsigned int State::getDayTemp() {
  return _dayTemp;
}

unsigned int State::getNightTemp() {
  return _nightTemp;
}

unsigned int State::getDayTime() {
  return _dayTime;
}

unsigned int State::getNightTime() {
  return _nightTime;
}

unsigned int State::getTransTime() {
  return _transTime;
}

unsigned int State::getAnimationDelay() {
  return _animDelay;
}

bool State::getGammaCorrect() {
  return _gammaCorrect;
}
