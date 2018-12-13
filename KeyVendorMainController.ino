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
  else if (command.type == USER_CONFIRM ) UserConfirmCommand(command);
  else if (command.type == USER_DENY    ) UserDenyCommand(command);
  else if (command.type == USER_BAN     ) UserBanCommand(command);
  else if (command.type == USER_UNBAN   ) UserUnbanCommand(command);
  else if (command.type == USER_PROMOTE ) UserPromoteCommand(command);
  else if (command.type == USER_DEMOTE  ) UserDemoteCommand(command);
  else if (command.type == GET_USER_LIST) GetUserListCommand(command);
  else if (command.type == GET_KEY_LIST ) GetKeyListCommand(command);
  else if (command.type == SET_KEY_LIST ) SetKeyListCommand(command);
  else if (command.type == GET_KEY      ) GetKeyCommand(command);
  else if (command.type == GET_LOG      ) GetLogCommand(command);
  else if (command.type == CLEAR_LOG    ) ClearLogCommand(command);
  else if (command.type == ADMIN_CHECK  ) AdminCheckCommand(command);
  else if (command.type == UPDATE_INFO  ) UpdateInfoCommand(command);
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
  if (IsUUIDInFile(bansFileName, command.uuid)) {
    answer = GenerateAnswerString(command, ACCESS_DENIED);
    WriteToLog(command, "-", ACCESS_DENIED);
  }
  else if (IsUUIDInFile(usersFileName, command.uuid)) {
    answer = GenerateAnswerString(command, SUCCESS);
    WriteToLog(command, "-", SUCCESS);
  }
  else {
    answer = GenerateAnswerString(command, FAILURE);
    WriteToLog(command, "-", FAILURE);
  }  
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

    WriteToLog(command, "-", SUCCESS);
  }

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserConfirmCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    if (IsUUIDInFile(requestsFileName, command.data)) {
      String line = "";
      sdCard.OpenFileForReading(requestsFileName);
      do {
        line = sdCard.ReadLineFromFile();
        if (line != "" && line.startsWith(command.data)) 
          break;
      } while (line != "");
      sdCard.CloseFile();
      sdCard.OpenFileForWriting(usersFileName);
      sdCard.WriteToFile(line);
      sdCard.CloseFile();
      sdCard.DeleteLineFromFile(requestsFileName, command.data);
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserDenyCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    if (IsUUIDInFile(requestsFileName, command.data)) {
      sdCard.DeleteLineFromFile(requestsFileName, command.data);
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserBanCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    if (IsUUIDInFile(usersFileName, command.data)) {
      String line = "";
      sdCard.OpenFileForReading(usersFileName);
      do {
        line = sdCard.ReadLineFromFile();
        if (line != "" && line.startsWith(command.data)) 
          break;
      } while (line != "");
      sdCard.CloseFile();
      sdCard.OpenFileForWriting(bansFileName);
      sdCard.WriteToFile(line);
      sdCard.CloseFile();
      sdCard.DeleteLineFromFile(usersFileName, command.data);
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserUnbanCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    if (IsUUIDInFile(bansFileName, command.data)) {
      String line = "";
      sdCard.OpenFileForReading(bansFileName);
      do {
        line = sdCard.ReadLineFromFile();
        if (line != "" && line.startsWith(command.data)) 
          break;
      } while (line != "");
      sdCard.CloseFile();
      sdCard.OpenFileForWriting(usersFileName);
      sdCard.WriteToFile(line);
      sdCard.CloseFile();
      sdCard.DeleteLineFromFile(bansFileName, command.data);
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserPromoteCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    if (IsUUIDInFile(usersFileName, command.data)) {
      sdCard.OpenFileForWriting(adminsFileName);
      sdCard.WriteToFile(command.data);
      sdCard.CloseFile();
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void UserDemoteCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid) && command.uuid != command.data) {
    if (IsUUIDInFile(adminsFileName, command.data)) {
      sdCard.DeleteLineFromFile(adminsFileName, command.data);
      answer = GenerateAnswerString(command, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE);
    }
  }
  else if (IsUUIDInFile(usersFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else
    answer = GenerateAnswerString(command, FAILURE);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void GetUserListCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    int indexer = command.data.toInt();
    String data = "";
    String line = "";
    int i = 0;
    sdCard.OpenFileForReading(usersFileName);
    do
    {
      line = sdCard.ReadLineFromFile();
      if (line != "" && i >= indexer * 10) {
        if (i > indexer * 10) data += "@";
        data += line;
      }
      i++;
    } while (line != "" && i < (indexer+1) * 10);

    sdCard.CloseFile();
    answer = GenerateAnswerString(command, SUCCESS, data);
  }
  else
    answer = GenerateAnswerString(command, ACCESS_DENIED);

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
void SetKeyListCommand(Command command) {
  String answer = "";

  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    sdCard.ClearFile(keysFileName);
    sdCard.OpenFileForWriting(keysFileName);
    String key = "";

    for(int i=0; i < command.data.length(); i++) {
      if (command.data[i] == '@') {
        sdCard.WriteToFile(key);
        key = "";
      }
      else if (i == command.data.length() - 1) {
        key += command.data[i];
        sdCard.WriteToFile(key);
        key = "";
      }
      else
        key += command.data[i];
    }

    sdCard.CloseFile();

    answer = GenerateAnswerString(command, SUCCESS);
  }
  else
    answer = GenerateAnswerString(command, ACCESS_DENIED);

  Serial.println(answer);
  int bytesSend = bluetooth.Write(answer);
  Serial.println(bytesSend);
}
void GetKeyCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED);
  else if (IsUUIDInFile(usersFileName, command.uuid)) {
    int keyPosition = 0;
    String line = "";
    bool isInFile = false;
    sdCard.OpenFileForReading(keysFileName);
    do {
      keyPosition++;
      line = sdCard.ReadLineFromFile();
      if (line != "" && line.startsWith(command.data)) {
        isInFile = true;
        break;
      }
    } while (line != "");
    sdCard.CloseFile();
    if (isInFile) {
      Serial2.println(keyPosition);
      answer = GenerateAnswerString(command, SUCCESS); 
      WriteToLog(command, command.data, SUCCESS);
    } else {
      answer = GenerateAnswerString(command, FAILURE); 
      WriteToLog(command, command.data, FAILURE);
    }
  }
  else 
    answer = GenerateAnswerString(command, FAILURE);
  
  Serial.println(answer);
  bluetooth.Write(answer);
}
void GetLogCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    int indexer = command.data.toInt();
    String data = "";
    String line = "";
    sdCard.OpenFileForReading(logFileName);
    
    int i = 0;
    do
    {
      line = sdCard.ReadLineFromFile();
      if (line != "" && i >= indexer * 10) {
        if (i > indexer * 10) data += "@";
        data += line;
      }
      i++;
    } while (line != "" && i < (indexer+1) * 10);

    sdCard.CloseFile();
    answer = GenerateAnswerString(command, SUCCESS, data);
  }
  else
    answer = GenerateAnswerString(command, ACCESS_DENIED);

  Serial.println(answer);
  bluetooth.Write(answer);
}
void ClearLogCommand(Command command) {
  String answer = "";

  if (IsUUIDInFile(adminsFileName, command.uuid)) {
    sdCard.ClearFile(logFileName);
    answer = GenerateAnswerString(command, SUCCESS);
  }
  else
    answer = GenerateAnswerString(command, ACCESS_DENIED);

  Serial.println(answer);
  int bytesSend = bluetooth.Write(answer);
  Serial.println(bytesSend);
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
void UpdateInfoCommand(Command command) {
  String answer = "";
  if (IsUUIDInFile(bansFileName, command.uuid))
    answer = GenerateAnswerString(command, ACCESS_DENIED); 
  else if (IsUUIDInFile(usersFileName, command.uuid)) {
    String newLine = command.uuid + " " + command.data;
    sdCard.EditLineFromFile(usersFileName, command.uuid, newLine);
    answer = GenerateAnswerString(command, SUCCESS); 
    WriteToLog(command, "-", SUCCESS);
  }
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
String GetUserName (String uuid) {
  sdCard.OpenFileForReading(usersFileName);
  String line = "";
  String userName = "";
  bool isInFile = false;
  
  do {
    line = sdCard.ReadLineFromFile();
    if (line != "" && line.startsWith(uuid)) {
      isInFile = true;
      break;
    }
  } while (line != "");
  sdCard.CloseFile();

  if (isInFile) {
    bool nameStarted = false;
    for (int i = 0; i < line.length(); i++) {
      if (!nameStarted && line[i] == ' ')
        nameStarted = true;
      else if (nameStarted && line[i] == '@')
        break;
      else if (nameStarted)
        userName += line[i];
    }
  }
  
  return userName;
}
void WriteToLog (Command command, String data, String answer) {
  String logText = command.uuid + '@' + command.timeStamp + '@' + command.type + '@' + data + '@' + answer + "@";
  if (IsUUIDInFile(usersFileName, command.uuid))
    logText += GetUserName(command.uuid);
  else 
    logText += "-";
  sdCard.OpenFileForWriting(logFileName);
  sdCard.WriteToFile(logText);
  sdCard.CloseFile();
}
