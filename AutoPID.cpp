#include "AutoPID.h"

#ifdef ARDUINO
#include <Arduino.h>
#define sys_millis(x) millis(x)
#else

#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#include <ctime>

long sys_millis() {
  using namespace std;
  timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

#endif

#define notZero(x) (abs(x) > 0.00001)


AutoPID::AutoPID(double *input, double *setpoint, double *output, double outputMin, double outputMax,
                 double Kp, double Ki, double Kd) {
  _input = input;
  _setpoint = setpoint;
  _output = output;
  _outputMin = outputMin;
  _outputMax = outputMax;
  setGains(Kp, Ki, Kd);
  _timeStep = 1000;
  _stopped = true;
}//AutoPID::AutoPID

void AutoPID::setGains(double Kp, double Ki, double Kd) {
  _Kp = Kp;
  _Ki = Ki;
  _Kd = Kd;
}//AutoPID::setControllerParams

void AutoPID::setBangBang(double bangOn, double bangOff) {
  _bangOn = bangOn;
  _bangOff = bangOff;
}//void AutoPID::setBangBang

void AutoPID::setBangBang(double bangRange) {
  setBangBang(bangRange, bangRange);
}//void AutoPID::setBangBang

void AutoPID::setOutputRange(double outputMin, double outputMax) {
  _outputMin = outputMin;
  _outputMax = outputMax;
}//void AutoPID::setOutputRange

void AutoPID::setTimeStep(unsigned long timeStep){
  _timeStep = timeStep;
}


bool AutoPID::atSetPoint(double threshold) {
  return abs(*_setpoint - *_input) <= threshold;
}//bool AutoPID::atSetPoint

void AutoPID::run() {
  if (_stopped) {
    return;
  }
  //if bang thresholds are defined and we're outside of them, use bang-bang control
  if (notZero(_bangOn) && ((*_setpoint - *_input) > _bangOn)) {
    *_output = _outputMax;
    _lastStep = sys_millis();
  } else if (notZero(_bangOff) && ((*_input - *_setpoint) > _bangOff)) {
    *_output = _outputMin;
    _lastStep = sys_millis();
  } else {                                    //otherwise use PID control
    unsigned long _dT = sys_millis() - _lastStep;   //calculate time since last update
    if (_dT >= _timeStep) {                     //if long enough, do PID calculations
      _lastStep = sys_millis();
      double _error = *_setpoint - *_input;
      _integral += (_error + _previousError) / 2 * _dT / 1000.0;   //Riemann sum integral
      //_integral = constrain(_integral, _outputMin/_Ki, _outputMax/_Ki);
      double _dError = (_error - _previousError) / _dT / 1000.0;   //derivative
      _previousError = _error;
      double PID = (_Kp * _error) + (_Ki * _integral) + (_Kd * _dError);
      //*_output = _outputMin + (constrain(PID, 0, 1) * (_outputMax - _outputMin));
      *_output = constrain(PID, _outputMin, _outputMax);
    }
  }
}//void AutoPID::run

void AutoPID::start() {
  reset();
  _stopped = false;
}

void AutoPID::stop() {
  _stopped = true;
  reset();
}

void AutoPID::reset() {
  _lastStep = sys_millis();
  _integral = 0;
  _previousError = 0;
  *_output = 0;
}

bool AutoPID::isStopped(){
  return _stopped;
}

double AutoPID::getIntegral(){
  return _integral;
}

void AutoPID::setIntegral(double integral){
  _integral = integral;
}

void AutoPIDRelay::reset() {
  AutoPID::reset();
  _lastPulseTime = sys_millis();
}

void AutoPIDRelay::run() {
  AutoPID::run();
  while ((sys_millis() - _lastPulseTime) > _pulseWidth) _lastPulseTime += _pulseWidth;
  *_relayState = ((sys_millis() - _lastPulseTime) < (_pulseValue * _pulseWidth));
}


double AutoPIDRelay::getPulseValue(){
  return (isStopped()?0:_pulseValue);
}


