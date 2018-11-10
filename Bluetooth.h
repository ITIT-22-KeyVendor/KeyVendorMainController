#ifndef Bluetooth_h
#define Bluetooth_h

#include "HardwareSerial.h"

class Bluetooth
{
public:
  Bluetooth(HardwareSerial &serial);
  
  String Read();
  bool Write(String data);  
    
private:
  HardwareSerial* serial;
};

#endif
