#include "Bluetooth.h"
#include "SDCard.h"
#include "Command.h"

#define SERIAL_BUFFER_SIZE 256

const String logFileName      = "log.txt";
const String requestsFileName = "requests.txt";
const String usersFileName    = "users.txt";
const String adminsFileName   = "admins.txt";
const String bansFileName     = "bans.txt";
const String keysFileName     = "keys.txt";

Bluetooth bluetooth(Serial3);
SDCard sdCard(53, Serial);

long startTime = 0;
long currentTime = 0;
int delayTime = 100;
int incompleteCycle = 0;
int maxIncompleteCycle = 50;
String commandText = "";

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);
  delay(300);
  
  sdCard.Initialize();
  delay(500);

  sdCard.CreateFile(logFileName);
  sdCard.CreateFile(requestsFileName);
  sdCard.CreateFile(usersFileName);
  sdCard.CreateFile(adminsFileName);
  sdCard.CreateFile(bansFileName);
  sdCard.CreateFile(keysFileName);

  startTime = millis();
  Serial.println("KeyVendor is ready");
}
void loop() {
  currentTime = millis();
  
  if ((currentTime - startTime) < delayTime) return;
  else startTime = millis();
  
  commandText += bluetooth.Read();  
  if (commandText == "") return;
  int commandLength = commandText.length();

  Serial.print("COMMAND: ");      Serial.println(commandText);
  Serial.print("COMMAND SIZE: "); Serial.println(commandLength);
  Command command = GenerateCommand(commandText);
  Serial.print("uuid: ");         Serial.println(command.uuid);
  Serial.print("timeStamp: ");    Serial.println(command.timeStamp);
  Serial.print("type: ");         Serial.println(command.type);
  Serial.print("data: ");         Serial.println(command.data);
  Serial.print("isComplete: ");   Serial.println(command.isComplete);
  Serial.print("isValid: ");      Serial.println(command.isValid);
  Serial.println();

  if (command.isComplete) {
    commandText = "";
    incompleteCycle = 0;
  }
 
	if      (!command.isValid             ) InvalidCommand(command);
  else if (!command.isComplete          ) IncompleteCommand(command);
  else if (command.type == USER_LOGIN   ) UserLoginCommand(command);
  else if (command.type == USER_REGISTER) UserRegisterCommand(command);
  else if (command.type == USER_CONFIRM ) return;
  else if (command.type == USER_DENY    ) return;
  else if (command.type == USER_BAN     ) return;
  else if (command.type == USER_UNBAN   ) return;
  else if (command.type == USER_PROMOTE ) return;
  else if (command.type == USER_DEMOTE  ) return;
  else if (command.type == GET_USER_LIST) return;
  else if (command.type == GET_KEY_LIST ) GetKeyListCommand(command);
  else if (command.type == SET_KEY_LIST ) return;
  else if (command.type == GET_KEY      ) GetKeyCommand(command);
  else if (command.type == GET_LOG      ) return;
  else if (command.type == CLEAR_LOG    ) return;
  else if (command.type == ADMIN_CHECK  ) AdminCheckCommand(command);
}

void InvalidCommand(Command command) {
  String answer = GenerateAnswerString(command, INVALID_COMMAND);
  Serial.println(answer);
  bluetooth.Write(answer);
}
void IncompleteCommand(Command command) {
  incompleteCycle++;
  Serial.print("INCOMPLETE CYCLE NUMBER: ");
  Serial.println(incompleteCycle);
  if (incompleteCycle >= maxIncompleteCycle) {
    incompleteCycle = 0;
    commandText = "";
    InvalidCommand(command);
  }
}
void UserLoginCommand(Command command) {
  String answer = "";
  
  if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, SUCCESS);
  else if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else 
    answer = GenerateAnswerString(command, FAILURE);
  
  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserRegisterCommand(Command command) {
  String answer = "";

  if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, FAILURE);
  else if (IsUUIDInFile(requestsFileName, command.uuid))
    answer = GenerateAnswerString(command, FAILURE);
  else if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else {
    sdCard.OpenFileForReading(adminsFileName);
    String line = sdCard.ReadLineFromFile();
    bool noAdmins = line == "";
    sdCard.CloseFile();

    if (noAdmins) {
      sdCard.OpenFileForWriting(adminsFileName);
      sdCard.WriteToFile(command.uuid);
      sdCard.CloseFile();

      String userInfo = command.uuid + " " + command.data;
      sdCard.OpenFileForWriting(usersFileName);
      sdCard.WriteToFile(userInfo);
      sdCard.CloseFile();
      answer = GenerateAnswerString(command, SUCCESS);
    }
    else {
      String userInfo = command.uuid + " " + command.data;
      sdCard.OpenFileForWriting(requestsFileName);
      sdCard.WriteToFile(userInfo);
      sdCard.CloseFile();
      answer = GenerateAnswerString(command, SUCCESS);
    }
  }

  Serial.println(answer);
  bluetooth.Write(answer);
}
void GetKeyListCommand(Command command) {
  String answer = "";

  if (IsUUIDInFile(usersFileName, command.uuid)) {
    String data = "";

    sdCard.OpenFileForReading(keysFileName);
    String line = "";

    int i = 0;
    do
    {
      line = sdCard.ReadLineFromFile();
      if (line != "") {
        if (i > 0) data += "@";
        data += line;
      }
      i++;
    } while (line != "");

    sdCard.CloseFile();

    answer = GenerateAnswerString(command, SUCCESS, data);
  }
  else if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else 
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  int bytesSend = bluetooth.Write(answer);
  Serial.println(bytesSend);
}
void GetKeyCommand(Command command) {
  String answer = "";

  if (IsUUIDInFile(usersFileName, command.uuid)) {
    Serial2.println(command.data);
    answer = GenerateAnswerString(command, SUCCESS); 
  }    
  else if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else 
    answer = GenerateAnswerString(command, FAILURE);
  
  Serial.println(answer);
  bluetooth.Write(answer);
}
void AdminCheckCommand(Command command) {
  bool isAdmin = IsUUIDInFile(adminsFileName, command.uuid);
  String answer = "";
  
  if (isAdmin) 
    answer = GenerateAnswerString(command, SUCCESS);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}

bool IsUUIDInFile (String file, String uuid) {
  sdCard.OpenFileForReading(file);
  String line = "";
  bool isInFile = false;
  do {
    line = sdCard.ReadLineFromFile();
    if (line != "" && line.startsWith(uuid)) {
      isInFile = true;
      break;
    }
  } while (line != "");
  sdCard.CloseFile();
  return isInFile;
}
