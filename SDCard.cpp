#include "SDCard.h"

SDCard::SDCard(uint8_t csPin, HardwareSerial &serial) : csPin(csPin) {
  this->printer = &serial;
}
bool   SDCard::Initialize() {
  pinMode(csPin, OUTPUT);
  if (!SD.begin(csPin))
    return false;
  return true;
}
bool   SDCard::CreateFile(String fileName) {
  file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    printer->println("Could not create file: " + fileName);
    printer->println();
    return false;
  }
  file.close();
  printer->println("File created: " + fileName);
  printer->println();
  return true;
}
bool   SDCard::OpenFileForReading(String fileName) {
  file = SD.open(fileName, FILE_READ);
  if (!file) {
    printer->println("Could not open file for reading " + fileName);
    printer->println();
    return false;
  }
  printer->println("File opened for reading: " + fileName);
  printer->println();
  return true;
}
bool   SDCard::OpenFileForWriting(String fileName) {
  file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    printer->println("Could not open file for writing " + fileName);
    printer->println();
    return false;
  }
  printer->println("File opened for writing: " + fileName);
  printer->println();
  return true;
}
bool   SDCard::CloseFile() {
  if (!file) return false;
  file.close();
  return true;
}
bool   SDCard::ClearFile(String fileName) {
  SD.remove(fileName);
  bool result = CreateFile(fileName);
  return result;
}
int    SDCard::FileByteLength() {
  return file.available();
}
int    SDCard::WriteToFile(String data) {
  if (file)
  {
    int bytesWritten = file.println(data);
    printer->println("Bytes written: " + bytesWritten);
    printer->println(data);
    printer->println();
    return true;
  }

  return false;
}
String SDCard::ReadLineFromFile() {
  String data = "";
  char symbol;
  while (file.available()) {
    symbol = file.read();
    if (symbol == '\n')
      break;
    data += symbol;
  }
  return String(data);
}
bool   SDCard::EditLineFromFile (String fileName, String lineStartingText, String replacementText) {
  if(!OpenFileForReading(fileName))
    return false;
    
  File tempFile = SD.open("temp.txt", FILE_WRITE);
  String line = "";
  
  do {
    line = ReadLineFromFile();
    if (line != "" && line.startsWith(lineStartingText))
      line = replacementText;
    if (line != "")
      tempFile.println(line);
  } while (line != "");
  
  tempFile.close();
  CloseFile();
  
  ClearFile(fileName);
  OpenFileForWriting(fileName);
  tempFile = SD.open("temp.txt", FILE_READ);

  do {
    line = "";
    char symbol;
    while (tempFile.available()) {
      symbol = tempFile.read();
      if (symbol == '\n')
        break;
      line += symbol;
    }
    WriteToFile(line);
  } while (line != "");

  CloseFile();
  tempFile.close();
  SD.remove("temp.txt");
  return true;
}
bool   SDCard::DeleteLineFromFile (String fileName, String lineStartingText) {
  if (!OpenFileForReading(fileName))
    return false;
  
  File tempFile = SD.open("temp.txt", FILE_WRITE);
  String line = "";
  
  do {
    line = ReadLineFromFile();
    if (line != "") {
      if (line.startsWith(lineStartingText))
        continue;
      else
        tempFile.println(line);
    }      
  } while (line != "");
  
  tempFile.close();
  CloseFile();
  
  ClearFile(fileName);
  OpenFileForWriting(fileName);
  tempFile = SD.open("temp.txt", FILE_READ);

  do {
    line = "";
    char symbol;
    while (tempFile.available()) {
      symbol = tempFile.read();
      if (symbol == '\n')
        break;
      line += symbol;
    }
    WriteToFile(line);
  } while (line != "");

  CloseFile();
  tempFile.close();
  SD.remove("temp.txt");
  return true;
}

