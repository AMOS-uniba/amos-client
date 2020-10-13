#pragma once

class Protocol {

public:
  enum {
    START_BYTE_SLAVE = 0x5a,
    END_BYTE = 0x0d,
    MAX_TELEGRAM_LENGTH = 100,
    START_BYTE_MASTER = 0x55
  };

  static int getAddress(char * telegram);
  static bool isValidAnswer(int address, char * telegram);

  static int makeTel(char address, const char* data, int len, char *telegram);
  static int tel2Dat(char * telegram,char * data);



private:
  Protocol();
  ~Protocol();
  static char lsbToAscii(int i);
  static char msbToAscii(int i);
  static int asciiToByte(char high, char low);
};

