#ifndef Command_h
#define Command_h

const String SUCCESS =         "0";
const String FAILURE =         "1";
const String INVALID_COMMAND = "2";
const String ACCESS_DENIED =   "3";

const String USER_LOGIN =    "0";  
const String USER_REGISTER = "1";  
const String USER_CONFIRM =  "2";
const String USER_DENY =     "3";
const String USER_BAN =      "4";
const String USER_UNBAN =    "5";
const String USER_PROMOTE =  "6";
const String USER_DEMOTE =   "7";
const String GET_USER_LIST = "8";
const String GET_KEY_LIST =  "9";
const String SET_KEY_LIST =  "10";
const String GET_KEY =       "11";
const String GET_LOG =       "12";
const String CLEAR_LOG =     "13";
const String ADMIN_CHECK =   "14";
const String UPDATE_INFO =   "15";

const String* COMMANDS[] = { &USER_LOGIN,     &USER_REGISTER,  &USER_CONFIRM,   &USER_DENY,      
                             &USER_BAN,       &USER_UNBAN,     &USER_PROMOTE,   &USER_DEMOTE,  
                             &GET_USER_LIST,  &GET_KEY_LIST,   &SET_KEY_LIST,   &GET_KEY,        
                             &GET_LOG,        &CLEAR_LOG,      &ADMIN_CHECK,    &UPDATE_INFO };
const int COMMANDS_ARRAY_SIZE = 16;

struct Command { 
  String uuid = "";
  String timeStamp = "";
  String type = "";
  String data = "";
  bool isValid = true;
  bool isComplete = true;
};

Command GenerateCommand(String commandString) {
  Command command;
  int commandLength = commandString.length();

  if (commandLength < 1 || commandString[0] != '$')
    command.isValid = false;
  else if (commandLength < 3 || commandString[commandLength - 1] != '$')
    command.isComplete = false;
  else {
    String uuid = "";
    String timeStamp = "";
    String type = "";
    String data = "";

    int i = 1;
    while(i < commandLength - 1 && commandString[i] != '@') {
      uuid += commandString[i];
      i++;    }
    if (commandString[i] == '@') i++;
    while(i < commandLength - 1 && commandString[i] != '@') {
      timeStamp += commandString[i];
      i++;    }
    if (commandString[i] == '@') i++;
    while(i < commandLength - 1 && commandString[i] != '@') {
      type += commandString[i];
      i++;    }
    if (commandString[i] == '@') i++;
    while(i < commandLength - 1) {
      data += commandString[i];
      i++;    }

    if (uuid == "" || timeStamp == "" || type == "")
      command.isValid = false;
    else {
      bool existing = false;
      for (int i = 0; i < COMMANDS_ARRAY_SIZE; i++)
        if (*COMMANDS[i] == type) {
          command.type = *COMMANDS[i];
          command.uuid = uuid;
          command.timeStamp = timeStamp;
          command.data = data;
          existing = true;
          break;
        }
        
      if (!existing) command.isValid = false;
    }
  }
  
  return command;
}
String GenerateAnswerString(Command command, String answerType, String data = "") {
  String answer = "$" + command.type + "@" + answerType;
  if (data != "") answer += "@" + data;
  answer += "$\n";
  return answer;
}

#endif
