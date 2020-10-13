#pragma once
#include <ctime>
#include "protocol.h"
#include "serial.h"

const int MAXCHARS = 255;


class AMOSKO {
private:
	void parseStatus();
	void logErrors();
	void logAction(const char* a);


public:
	AMOSKO();
	~AMOSKO();

	static const unsigned char ADDRESS = 0x99;
	static const int REFRESH = 500;

	enum class LidDirection {
		CLOSING = 0,
		OPENING = 2
	};

	enum class FanMode {
		FAN_AUTO = 0,
		FAN_ON = 1,
		FAN_OFF = 2
	};

	char stationID[MAXCHARS];					//north pole
	char comID[6];
	char command[MAXCHARS];
	double lat = 90;
	double lon = 0;
	double h_dark = -9.0;
	float hum_max = 60;
	FanMode fmode = FanMode::FAN_AUTO;                   //fan_mode
	double deltaT = 67.28 / 86400.0;			//delta_t for year 2014 [d]

	char error;
	bool open, closed, moving, peeking;
	LidDirection lid_direction;

	bool ii, fan;
	bool camHeat, lensHeat;
	bool rain, light, pc_power;

  bool uptodate, uptodate2, uptodate3;

  unsigned long timeAlive;
  time_t lastUpdate = time(NULL);
  time_t lastLog = lastUpdate;

  short int T_lens, T_cpu, T, humidity, motor_position;

  char recvTel[Protocol::MAX_TELEGRAM_LENGTH];
  char sendTel[Protocol::MAX_TELEGRAM_LENGTH];
  char data[15];

  bool override = true;
  SerialPort comport;

  bool openCOM();
  void closeCOM();

  //just for testing, not used
  bool nullTel();

  //basic sensors and error messages
  bool updateStatus();

  //temperatures and humidity sensors
  bool updateStatus2();

  //motor position ...
  bool updateStatus3();

  //    a  : akcia
  //"\x00" : nic sa neurobi(len pre testovanie)
  //"\x01" : Otvori sa kryt(motor sa vypne automaticky po zaktivovani snimaca otvorenia)
  //"\x02" : Zatvori sa kryt(motor sa vypne automaticky po zaktivovani snimacov zatvorenia)
 
  //"\x05" : Zapne sa VENTILATOR
  //"\x06" : vypne sa VENTILATOR
  //"\x07" : zapne sa ZJO
  //"\x08" : vypne sa ZJO

  //"\x0b" : poziadavka na SW reset(cez Watchdog) nasledne mozno naviazat na Bootloader
  bool action(const char * a);

  //Obsadenie pamati EEPROM :
  //
  //ADRESA            OBSAH             POCET BYTOV                         VYZNAM
  //
  //0x0             0x1234              2                 na testovanie
  //0x2             CAS_MEDZIPOLOHA     2                 unsigned int: urcuje cas  zastavky krytu v medzipolohe
  //0x4             ZDVIH_MEDZIPOLOHA   2                 unsigned int: urcuje o kolko krokov premennej ZDVIH sa ma otvorit kry zo zatvoreneho stavu
  //0x6             TimeOutSens         2                 unsigned int: urcuje cas  po signale zo snimaca dazda resp. teploty po ktorom Slave zareaguje
  //0x8             ZT_LENS             2                 deciint: teplota o ktoru ma byt objektiv teplejsi ako okolie
  //0xA             ZT_CAM              2                 deciint: ziadana teplota kamery
  //0xC             ZT_PRAH             2                 deciint: prahova vonkajsia teplota od ktorej sa staruje regulacia teploty objektivu

  //Prepinace v EEPROM(zatial neimplementovane)
  //ADRESA       PREPINAC
  //0xE0     PREP_VYP_KOM

  bool readEEPROM(short int addr, short int & content);
  bool writeEEPROM(short int addr, short int & content);

  void readCfg(const char * filename);
  double hSun();
  bool isDark();
  void logStats();
  void rotateLogs();

  bool isAutomatic();
  void setAutomatic(bool ottomatic);

  void snapImage(const char* filename);
  int restoreCameraProperties(const char * snapshot);

};

bool fileExists(const char* filePath);
