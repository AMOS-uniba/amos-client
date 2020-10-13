#pragma once
#include <windows.h>

class SerialPort {

public:
  int BAUDRATE;
  int TIMEOUT;
  int REPEATS;

  SerialPort();
  ~SerialPort();

  bool open(char * ID);
  bool close();
  bool isOpen();
  int sendAndReceive(char * send, int bytesToSend, char * receive);
  int getTelegram(char * telegram);

private:
  bool getChar(char * telegram);
  void sendTelegram(char * telegram, int bytesToSend);

  DWORD dwOldMask, dwMask;
  COMMTIMEOUTS timeouts;
  HANDLE hComm;
};