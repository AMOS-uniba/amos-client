#include <cstdio>
#include "serial.h"
#include "protocol.h"
#include "logger.h"

using namespace std;

SerialPort::SerialPort() {
  BAUDRATE = 9600;
  TIMEOUT = 300;
  REPEATS = 3;
}

SerialPort::~SerialPort() {
  close();
}

bool SerialPort::open(char * ID) {
  hComm = CreateFile(ID, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

  if (!isOpen())
    return(false);

  DCB dcb;
  dcb.DCBlength = sizeof(DCB);
  GetCommState(hComm, &dcb);
  dcb.BaudRate = BAUDRATE;				//baud rate
  dcb.Parity = NOPARITY;				//parity
  dcb.ByteSize = 8;						//data bits
  dcb.StopBits = ONESTOPBIT;			//stopbits
  dcb.fDsrSensitivity = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  SetCommState(hComm, &dcb);
  GetCommMask(hComm, &dwOldMask);
  SetCommMask(hComm, EV_TXEMPTY);

  GetCommTimeouts(hComm, &timeouts);
  timeouts.WriteTotalTimeoutConstant = TIMEOUT;

  timeouts.ReadIntervalTimeout = MAXDWORD;			//Specify time-out between characters for receiving.
  timeouts.ReadTotalTimeoutMultiplier = 0;			//Specify value that is multiplied by the requested number of bytes to be read.
  timeouts.ReadTotalTimeoutConstant = TIMEOUT;	//Specify value is added to the product of the ReadTotalTimeoutMultiplier member
  SetCommTimeouts(hComm, &timeouts);

  return(true);
}

bool SerialPort::close() {
  SetCommMask(hComm, dwOldMask);
  FlushFileBuffers(hComm);
  CloseHandle(hComm);
  hComm = NULL;
  return(!isOpen());
}

bool SerialPort::isOpen() {
  return(hComm!=NULL && hComm!=INVALID_HANDLE_VALUE);
}

bool SerialPort::getChar(char * telegram) {
  DWORD dwBytesRead = 0;
  if (!ReadFile(hComm, telegram, 1, &dwBytesRead, NULL))
    log_err.output << log_err.timestamp() << "Problem so seriovou linkou" << endl;
  if (dwBytesRead)
    return(TRUE);
  else
    return(FALSE);
}

int SerialPort::getTelegram(char * telegram) {
  int offset = 0;
  while (getChar(telegram + offset)) {
    if (*(telegram + offset) == Protocol::END_BYTE) {
      return(offset + 1);
    } else {
      offset++;
    }
  }
  PurgeComm(hComm, PURGE_RXABORT);
  return(0);
}

void SerialPort::sendTelegram(char * telegram, int bytesTosend) {
  for (int i = 0; i < bytesTosend; i++) {
    TransmitCommChar(hComm, telegram[i]);
    WaitCommEvent(hComm, &dwMask, NULL);
  }
}

int SerialPort::sendAndReceive(char * send, int bytesToSend, char * receive) {
  int bytesReceived = 0;

  Sleep(1);

  for (int i = 0; i <= REPEATS; i++) {
    sendTelegram(send, bytesToSend);

    if ((bytesReceived = getTelegram(receive)) > 0 && Protocol::isValidAnswer(Protocol::getAddress(send), receive)) {
      if (i > 0)
        log_err.output << log_err.timestamp() << "Telegram opakovany " << i << "x" << endl;
      return(bytesReceived);
    }
    Sleep(1);
  }
  log_err.output << log_err.timestamp() << "Telegram bez odpovede" << endl;
  *receive = Protocol::END_BYTE;
  return(1);
}
