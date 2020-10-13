#include <cstdio>
#include <iomanip>

#include "APC\APC_Math.h"
#include "APC\APC_VecMat3D.h"
#include "APC\APC_Spheric.h"
#include "APC\APC_Time.h"
#include "APC\APC_Sun.h"
#include "APC\APC_Const.h"

#include "logger.h"
#include "amosko.h"
#include "C:\Users\Sesquideus\Documents\IC Imaging Control 3.5\classlib\include\tisudshl.h"

using namespace DShowLib;
using namespace std;


FileIO log_err("err");
FileIO log_st("stat");

AMOSKO::AMOSKO(): T(0), T_cpu(0), T_lens(0),
	camHeat(false), lensHeat(false), closed(false)
{
	strcpy(stationID, "North pole");	//default values
	strcpy(comID, "COM1");				//default values
	InitLibrary();                      //load camera library
}

AMOSKO::~AMOSKO() {
	ExitLibrary();                      //unload camera library
}

void AMOSKO::readCfg(const char * filename) {//station initialization
	char input[MAXCHARS];

	if (!fileExists(filename)) {
		log_0.output << log_0.timestamp() << filename << " sa nepodarilo najst/nacitat" << endl;
		exit(1);
	}

	ifstream fin(filename);

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	strcpy(stationID, strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	strcpy(comID + 3, strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	lat = atof(strtok(NULL, " "))*Rad;

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	lon = atof(strtok(NULL, " "))*Rad;

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	h_dark = atof(strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	hum_max = (float)atof(strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	fmode = (AMOSKO::FanMode) atoi(strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	deltaT = atof(strtok(NULL, " ")) / 86400.0;

	fin.getline(input, MAXCHARS);
	strcpy(command, input);

	if (fabs(h_dark + 9.5) > 8.5) {
		log_0.output << log_0.timestamp() << "Vyska slnka mimo rozsah." << endl;
		exit(2);
	}

	if (fabs(lat) > pi / 2) {
		log_0.output << log_0.timestamp() << "Zemep. sirka mimo rozsah." << endl;
		exit(3);
	}

	if (fabs(lon) > pi) {
		log_0.output << log_0.timestamp() << "Zemep. dlzka mimo rozsah." << endl;
		exit(4);
	}

	if (fabs(hum_max - 50) > 50) {
		log_0.output << log_0.timestamp() << "Max. povolena vlhkost mimo rozsah." << endl;
		exit(5);
	}
}




bool AMOSKO::openCOM() {
	if (!comport.isOpen()) {
		Sleep((int) (REFRESH * 1.6)); //wait for closing comport  by other party properly!!! (22.7.2015)
		comport.open(comID);
		//log0->output << log0->timestamp() << (override ? "C: " : "D: ") << comID << " " << (comport.open(comID) ? "open" : "failed to open") << endl;
	}
	return(comport.isOpen());
}

void AMOSKO::closeCOM() {
	if (comport.isOpen()) {
		comport.close();
	}
}

bool AMOSKO::nullTel() {
	return(comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, "", 0, sendTel), recvTel) != 1);
}

void AMOSKO::parseStatus() {
	this->moving = data[1] & 0x1;
	this->lid_direction = (AMOSKO::LidDirection) (data[1] & 0x2);
	open = data[1] & 0x4;
	closed = data[1] & 0x8;
	lensHeat = data[1] & 0x10;
	camHeat = data[1] & 0x20;
	ii = data[1] & 0x40;
	fan = data[1] & 0x80;

	rain = data[2] & 0x1;
	light = data[2] & 0x2;
	pc_power = data[2] & 0x4;
	peeking = data[2] & 0x20;

	memcpy(&timeAlive, data + 4, 4);
}

void AMOSKO::logErrors() {
	error = data[3];
	if (error & 0x1)
		log_err.output << log_err.timestamp() << "Chyba merania DS1820 (Tlens)" << endl;
	if (error & 0x2)
		log_err.output << log_err.timestamp() << "Chyba merania s SHT31" << endl;
	if (error & 0x4)
		log_err.output << log_err.timestamp() << "Nudzove uzavretie veka (svetlo)" << endl;
	if (error & 0x8)
		log_err.output << log_err.timestamp() << "Watchdog reset" << endl;
	if (error & 0x10)
		log_err.output << log_err.timestamp() << "Brownout reset" << endl;
	if (error & 0x20)
		log_err.output << log_err.timestamp() << "Vypadok napajanie majstra" << endl;
	if (error & 0x40)
		log_err.output << log_err.timestamp() << "Chyba merania DS1820 (Tcpu)" << endl;
	if (error & 0x80)
		log_err.output << log_err.timestamp() << "Nudzove uzavretie veka (dazd)" << endl;


}

bool AMOSKO::updateStatus() {
	bool success = comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, "S", 1, sendTel), recvTel) != 1;
	if (success) {
		Protocol::tel2Dat(recvTel, data);
		parseStatus();
		logErrors();
		lastUpdate = time(NULL);
	}
	
	return(uptodate = success);
}
bool AMOSKO::updateStatus2() {
	bool success = comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, "T", 1, sendTel), recvTel) != 1;
	if (success) {
		Protocol::tel2Dat(recvTel, data);
		memcpy(&T_lens, data + 1, 2);
		memcpy(&T_cpu, data + 3, 2);
		memcpy(&T, data + 5, 2);
		memcpy(&humidity, data + 7, 2);

		if (abs(T_lens) > 999 || abs(T_cpu) > 999 || abs(T) > 999 || abs(humidity) > 1000)
			this->uptodate2 = false;

		lastUpdate = time(NULL);
	}
	this->uptodate2 = success;
	return this->uptodate2;
}

bool AMOSKO::updateStatus3() {
	bool success = comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, "Z", 1, sendTel), recvTel) != 1;
	if (success) {
		Protocol::tel2Dat(recvTel, data);

		memcpy(&motor_position, data + 1, 2);

		if (abs(motor_position) > 999)
			this->uptodate3 = false;

		lastUpdate = time(NULL);
	}
	this->uptodate3 = success;

	return this->uptodate3;
}

void AMOSKO::logAction(const char * a) {
	switch (a[0]) {
	case 0x1:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na otvorenie veka" << endl;
		break;
	case 0x2:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na zatvorenie veka" << endl;
		break;
	case 0x3:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na zapnutie pomocneho kontaktu" << endl;
		break;
	case 0x4:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na vypnutie pomocneho kontaktu" << endl;
		break;
	case 0x5:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na zapnutie ventilatora" << endl;
		break;
	case 0x6:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na vypnutie ventilatora" << endl;
		break;
	case 0x7:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na zapnutie ZJO" << endl;
		break;
	case 0x8:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na vypnutie ZJO" << endl;
		break;
	case 0x9:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na zapnutie hotwire" << endl;
		break;
	case 0xa:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Prikaz na vypnutie hotwire" << endl;
		break;
	case 0xb:
		break; //nerealizuje sa, sluha na reset-telegram neodpoveda
	default:
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Nedefinovany prikaz: " << a[0] << endl;
	}
}

void AMOSKO::logStats() {

	time_t now = time(NULL);
	if (uptodate /*&& uptodate2 && uptodate3*/ && now - lastLog > 59) {//log cca every minute, only good data
		lastLog = now;
		log_st.output << log_0.timestamp() << fixed << setprecision(5) << (40587 + now / 86400.0);
		log_st.output << setprecision(1) << setw(6) << (abs(humidity) > 1000 ? -99.9 : humidity / 10.0) << setw(6) << (abs(T) > 999 ? -99.9 : T / 10.0);
		log_st.output << setw(6) << (abs(T_lens) > 999 ? -99.9 : T_lens / 10.0) << setw(6) << (abs(T_cpu) > 999 ? -99.9 : T_cpu / 10.0);
		log_st.output << " " << (open ? "O" : (closed ? "Z" : ".")) << (ii ? "I" : "-") << (fan ? "V" : "-") << (rain ? "D" : "-") << (light ? "S" : "-") << (!pc_power ? "N" : "-") << endl;
	}
}

void AMOSKO::rotateLogs() {
	log_0.rotate();
	log_err.rotate();
	log_st.rotate();
}

bool AMOSKO::action(const char * a) {
	strcpy(data, "C");
	strcat(data, a);

	bool success = comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, data, 2, sendTel), recvTel) != 1;
	if (success) {
		Protocol::tel2Dat(recvTel, data);
		parseStatus();
		logErrors();
		logAction(a);
		lastUpdate = time(NULL);
	}

	uptodate = success;
	return(success);
}

bool AMOSKO::readEEPROM(short int addr, short int &content) {
	strcpy(data, "R");
	memcpy(data + 1, &addr, 2);

	bool success = (bool)(comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, data, 3, sendTel), recvTel) != 1);
	if (success) {
		Protocol::tel2Dat(recvTel, data);
		memcpy(&content, data + 1, 2);
	}
	return(success);
}

bool AMOSKO::writeEEPROM(short int addr, short int &content) {
	strcpy(data, "E");
	memcpy(data + 1, &addr, 2);
	memcpy(data + 3, &content, 2);

	bool success = comport.sendAndReceive(sendTel, Protocol::makeTel(ADDRESS, data, 5, sendTel), recvTel) != 1;
	if (success) {
		Protocol::tel2Dat(recvTel, data);
		memcpy(&content, data + 1, 2);
	}
	return(success);
}

double AMOSKO::hSun() {
	double h, A, ra, de, mjd_utc = 40587 + time(NULL) / 86400.0;
	double lmst = GMST(mjd_utc) + lon;
	MiniSun((mjd_utc + deltaT - MJD_J2000) / 36525.0, ra, de);
	Equ2Hor(de, lmst - ra, lat, h, A);
	return(h*Deg);
}

bool AMOSKO::isDark() {
	return(hSun() < h_dark);
}

bool AMOSKO::isAutomatic() {
	return(!fileExists("manual"));
}

void AMOSKO::setAutomatic(bool ottomatic) {
	if (ottomatic)
		DeleteFile("manual");
	else
		CloseHandle(CreateFile("manual", FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL));
}

int AMOSKO::restoreCameraProperties(const char * snapshot) {
	if (!fileExists("camera.xml")) {
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Subor camera.xml sa nepodarilo najst/nacitat" << endl;
		return 2;
	}

	int res = 0;
	Grabber grabber;
	FrameHandlerSink::tFHSPtr sink = FrameHandlerSink::create(eY800, 5);
	sink->setSnapMode(true);
	grabber.setSinkType(sink);
	grabber.loadDeviceStateFromFile("camera.xml");

	if (grabber.isDevValid()) {
		grabber.startLive(false);
		Sleep(200);
		sink->snapImages(1, 1000);

		if (grabber.getLastError().isError()) {
			log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Nepodarilo sa urobit kontrolnu snimku" << endl;
			res = 3;
		}
		else {
			if (snapshot != NULL)
				saveToFileBMP(*sink->getLastAcqMemBuffer(), snapshot);
			log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Kontrolna snimka " << snapshot << " ulozena" << endl;
		}
	}
	else {
		log_0.output << log_0.timestamp() << (override ? "C: " : "D: ") << "Grabber je neplatny" << endl;
		res = 4;
	}

	grabber.closeDev();

	return res;
}

void AMOSKO::snapImage(const char* filename) {
	for (int i = 0; i < 5 && restoreCameraProperties(filename) > 0; i++) {
		Sleep(5000);
	}
}


bool fileExists(const char* filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
