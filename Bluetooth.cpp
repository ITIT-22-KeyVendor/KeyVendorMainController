#include "Bluetooth.h"

Bluetooth::Bluetooth(HardwareSerial &serial) {
  this->serial = &serial;
}
String Bluetooth::Read() {
  String data = "";

  if (serial == NULL) return data;
  
  if (serial->available())
    while (serial->available())
      data += char(serial->read());

  return data;
}
bool   Bluetooth::Write(String data) {
  if (serial == NULL) return false;

  for (int i = 0; i < data.length(); i++)
    serial->write(data[i]);
 
  return true;
}
