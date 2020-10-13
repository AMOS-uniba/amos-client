#include <cstring>
#include "protocol.h"


Protocol::Protocol() {
}


Protocol::~Protocol() {
}

int Protocol::getAddress(char * telegram) {
  return(asciiToByte(telegram[1], telegram[2]));
}

bool Protocol::isValidAnswer(int address, char * telegram) {

  int rlength;
  for(rlength=0; rlength<MAX_TELEGRAM_LENGTH && telegram[rlength]!=END_BYTE; rlength++) {
    ;
  }
  rlength++;

  //ci sedi dlzka telegramu
  if(telegram[0]==START_BYTE_SLAVE && asciiToByte(telegram[3], telegram[4])==(rlength-8)/2 && telegram[rlength-1]==END_BYTE) {
    //ci sedi cislo modulu
    int addressReceived=getAddress(telegram);
    if(address==addressReceived) {
      //ci sedi kontrolna suma
      int crc=0;
      for(int i=0; i<rlength-3; i++) {
        crc+=telegram[i];
      }
      crc%=256;
      if(msbToAscii(crc)==telegram[rlength-3] && lsbToAscii(crc)==telegram[rlength-2]) {
        return (true);
      } else {
        return (false);
      }
    } else {
      return (false);
    }
  } else {
    return (false);
  }
}

int Protocol::asciiToByte(char high, char low) {
  high-=(char)48;
  low-=(char)48;

  if(high>9) {
    high-=(char)7;
  }
  if(low>(char)9) {
    low-=(char)7;

  }
  int result=(high << 4)+low;

  return (result);
}

char Protocol::lsbToAscii(int i) {
  char low=(char) ((i & 15)+48);
  if(low>57) {
    low+=(char)7;
  }
  return (low);
}

char Protocol::msbToAscii(int i) {
  char high=(char) (((i & 240) >> 4)+48);//unsigned right shift??
  if(high>57) {
    high+=(char)7;
  }
  return (high);
}

int Protocol::makeTel(const char address, const char * data, int len, char* telegram) {
//zostavi telegram CPU->Slave s adresou adresa,dlzkou dat.pola dlzka_dat
//data zoberie z pola *data , pole telegramu zapise do *telegram
//vrati dlzku telegramu

  char crc=0;
  int i;

  crc += telegram[0] = START_BYTE_MASTER;
  crc += telegram[1] = msbToAscii(address);
  crc += telegram[2] = lsbToAscii(address);
  crc += telegram[3] = msbToAscii(len);
  crc += telegram[4] = lsbToAscii(len);
  for (i = 0; i < len; i++) {
    crc += telegram[5+2*i] = msbToAscii(data[i]);
    crc += telegram[6+2*i] = lsbToAscii(data[i]);
  }

  i=5+2*len;
  telegram[i++]=msbToAscii(crc);
  telegram[i++]=lsbToAscii(crc);
  telegram[i++]=END_BYTE;
  return(i);
}

//str2dat, desifruje data zo vstupneho pola telegram
//vystup zapise do pola data
//vrati: dlzku pola data
int Protocol::tel2Dat(char * telegram, char * data) {
  int len = asciiToByte(telegram[3], telegram[4]);
  for (int i = 0; i < len; i++)	{
    data[i] = asciiToByte(telegram[5 + 2 * i], telegram[6 + 2 * i]);
  }
  return(len);

}


