//launches scripts start.ps1/stop.ps1 when sun is at predefined height  (c) Jozef Vilagi

#include <cmath>
#include <ctime>
#include <windows.h>
#include "APC\APC_Math.h"
#include "APC\APC_VecMat3D.h"
#include "APC\APC_Spheric.h"
#include "APC\APC_Time.h"
#include "APC\APC_Sun.h"
#include "APC\APC_Const.h"
#include "util\logger.h"

using namespace std;

const int MJD_OFFSET = 40587;
const short MAXCHARS = 255;

double deltaT = 67.28 / 86400.0;			//delta t for year 2014
int PAUSE_OSINIT = 30 * 1000;				//wait for 30sec while (OS) services initialize
const int PAUSE_REFRESH = 5 * 1000;		    //sun height poll interval

char path_launcher[MAXCHARS] = "C:\\Program Files\\Launcher\\";			//directory path for launcher files

char stationID[MAXCHARS] = "NORTH_POLE";
double lat = pi / 2;
double lon = 0;
double h_dark = -10.0;

bool fileExists(const char* filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool isBlocked() {
	return(fileExists("block"));
}

void readCfg(char * filename) {//station initialization
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
	PAUSE_OSINIT = atoi(strtok(NULL, " "))*1000;

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
	deltaT = atof(strtok(NULL, " ")) / 86400.0;

	if (fabs(h_dark + 9.5) > 8.5) {
		log_0.output << log_0.timestamp() << "Vyska slnka mimo rozsah" << endl;
		exit(2);
	}

	if (fabs(lat) > pi / 2) {
		log_0.output << log_0.timestamp() << "Zemep. sirka mimo rozsah" << endl;
		exit(3);
	}

	if (fabs(lon) > pi) {
		log_0.output << log_0.timestamp() << "Zemep. dlzka mimo rozsah" << endl;
		exit(4);
	}

	log_0.output << log_0.timestamp() << "Stanica " << stationID << " bezi" << endl;
}

void startupScript() {
	if (fileExists("start.ps1")) {
		if (!isBlocked()) {
			log_0.output << log_0.timestamp() << "Spusta sa start.ps1" << endl;
			system("powershell -ExecutionPolicy Unrestricted -File start.ps1");
		}
		else
			log_0.output << log_0.timestamp() << "Spustenie start.ps1 blokovane" << endl;
	}
	else {
		log_0.output << log_0.timestamp() << "start.ps1 sa nenasiel v " << path_launcher << endl;
	}
	SetCurrentDirectory(path_launcher);
}

void stopScript() {
	if (fileExists("stop.ps1")) {
		log_0.output << log_0.timestamp() << "Spusta sa stop.ps1" << endl;
		system("powershell -ExecutionPolicy Unrestricted -File stop.ps1");
	}
	else {
		log_0.output << log_0.timestamp() << "stop.ps1 sa nenasiel v " << path_launcher << endl;
	}
	SetCurrentDirectory(path_launcher);
}

void shutdownScript() {
	if (fileExists("shutdown.ps1")) {
		log_0.output << log_0.timestamp() << "Spusta sa shutdown.ps1" << endl;
		system("powershell -ExecutionPolicy Unrestricted -File shutdown.ps1");
	}
	else {
		log_0.output << log_0.timestamp() << "shutdown.ps1 sa nenasiel v " << path_launcher << endl;
	}
	SetCurrentDirectory(path_launcher);
}

double hSun() {
	double h, A, ra, de, mjd_utc = 40587 + time(NULL) / 86400.0;
	double lmst = GMST(mjd_utc) + lon;
	MiniSun((mjd_utc + deltaT - MJD_J2000) / 36525.0, ra, de);
	Equ2Hor(de, lmst - ra, lat, h, A);
	return(h*Deg);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	GetCurrentDirectory(MAXCHARS, path_launcher);
	readCfg("launcher.ini");
	Sleep(PAUSE_OSINIT);

	double dH, dH0 = hSun() - h_dark;
	if (dH0 <= 0) {
		startupScript();
	}
	else {
		stopScript();
	}


	while (true) {
		Sleep(PAUSE_REFRESH);
		if (dH0*(dH = hSun() - h_dark) <= 0) {
			if (dH <= 0) {
				startupScript();
			}
			else {
				stopScript();
				log_0.rotate();
				shutdownScript();				
			}
		}
		dH0 = dH;
	}

	return 0;
}

