#ifndef SDCard_h
#define SDCard_h

#include <SD.h>
#include <SPI.h>

class SDCard
{
public:
  SDCard(uint8_t, HardwareSerial&);  
  bool   Initialize();
  bool   CreateFile(String);
  bool   OpenFileForReading(String);
  bool   OpenFileForWriting(String);
  bool   CloseFile();
  bool   ClearFile(String);
  int    FileByteLength();
  int    WriteToFile(String);
  String ReadLineFromFile();
private:
  char* ConvertStringToArray(String);  
  File file;
  HardwareSerial* printer;
  uint8_t csPin;
};

#endif
