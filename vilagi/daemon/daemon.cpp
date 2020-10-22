//amos v3 daemon program (c) Jozef Vilagi

#include <io.h>
#include <fcntl.h>

#include <windows.h>
#include <winuser.h>
#include <process.h>

#include "..\util\logger.h"
#include "..\util\amosko.h"

using namespace std;

char root[MAXCHARS];										   //installation root directory
char path_ufo[] = "C:\\Program Files (x86)\\UFO2\\";		   //directory path for UFO installation
char ufo_executable[] = "UFO2.exe";							   //UFO executable
const int PAUSE_OSINIT = 15 * 1000;				                   //wait for 15 sec while (OS) services initialize

AMOSKO cok;

bool isUFOrunning(HWND &frame) {
	frame = FindWindow(NULL, "UFOCapture");
	return(frame != NULL);
}

void startUFO() {
	HWND frame;
	GetCurrentDirectory(MAXCHARS, root);
	SetCurrentDirectory(path_ufo);									//setting correct working directory!!!

	if (!fileExists(ufo_executable)) {
		log_0.output << log_0.timestamp() << ufo_executable << " sa nepodarilo najst/otvorit" << endl;
		exit(5);
	}

	if (!isUFOrunning(frame)) {
		if ((INT_PTR)ShellExecute(NULL, NULL, ufo_executable, NULL, NULL, SW_SHOWMINIMIZED) <= 32) {
			log_0.output << log_0.timestamp() << ufo_executable << " sa nepodarilo spustit" << endl;
			exit(6);
		}
		else {
			log_0.output << log_0.timestamp() << "D: UFO bezi" << endl;
		}
	}

	SetCurrentDirectory(root);
}

void closeUFO() {
	HWND frame, child;

	if (!isUFOrunning(frame))
		return;

	while (isUFOrunning(frame)) {
		SendNotifyMessage(frame, WM_SYSCOMMAND, SC_CLOSE, 0);
		Sleep(500);
		while ((child = GetLastActivePopup(frame)) != NULL) {
			SetActiveWindow(child);
			SendDlgItemMessage(child, 1, BM_CLICK, 0, 0);
		}
		Sleep(500);
	}
	log_0.output << log_0.timestamp() << "D: UFO zastavene" << endl;
}

bool isAutomatic() {
	return(!fileExists("manual"));
}

bool isOverriden() {
	return(fileExists("override"));
}

//zobrazenie konzoly pre odladovanie
void showConsole() {
	AllocConsole();

	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((INT_PTR)handle_out, _O_TEXT);
	FILE* hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((INT_PTR)handle_in, _O_TEXT);
	FILE* hf_in = _fdopen(hCrt, "r");
	setvbuf(hf_in, NULL, _IONBF, 128);
	*stdin = *hf_in;
}

bool alreadyRunning() {
	HANDLE hMutexOneInstance = ::CreateMutexA(NULL, TRUE, "DAEMON-58154947-7476-44B8-A49B-DDF33E9F0C5A");
	return(GetLastError() == ERROR_ALREADY_EXISTS);
}

void remapND() {//because of windows 7 bug - network disks don't get connected at startup sometimes!
	system(cok.command);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWND frame;



	if (alreadyRunning()) {
		log_0.output << log_0.timestamp() << "D: Jeden demon uz bezi, dalsi nie je ziaduci" << endl;
		exit(7);
	}

	cok.readCfg("amos.ini");
	DeleteFile("override");//cleanup at startup, just in case
	cok.override = false;

	//showConsole(); //console for debugging, uncomment if needed

	log_0.output << endl;

	Sleep(PAUSE_OSINIT);    //wait for OS startup properly prepare usb3
	_spawnl(_P_DETACH, "Usb3CamDeviceReset.exe", "Usb3CamDeviceReset.exe", "/silent", NULL);//driver bug workaround
	Sleep(10000);    //unpredictable delay

	remapND();//win7 bug - remap network disk
	log_0.output << log_0.timestamp() << "D: Demon bezi, automaticky rezim " << (isAutomatic() ? "zapnuty" : "vypnuty") << endl;

	int attempt = 0;
	while (true) {
		if (!isOverriden()) {
			if (cok.openCOM()) {

				cok.updateStatus();
				cok.updateStatus2();
				cok.updateStatus3();

				cok.logStats();

				if (cok.uptodate && cok.open && (cok.hSun() > cok.h_dark + 1) && cok.action("\x02")) {
					Sleep(2000);// kazdopadne zatvor aspon kryt, aj ked je vypnuta automatika
				}

				if (cok.uptodate && cok.ii && (cok.hSun() > cok.h_dark + 1) && cok.action("\x08")) {
					Sleep(2000);// kazdopadne vypni aj ZJO, aj ked je vypnuta automatika
				}


				if (isAutomatic() && cok.uptodate /*&& cok.uptodate2*/) {

					if (!cok.fan && cok.fmode == AMOSKO::FanMode::FAN_ON)
						cok.action("\x05");
					if (cok.fan && cok.fmode == AMOSKO::FanMode::FAN_OFF)
						cok.action("\x06");

					if (cok.isDark()) {

						if (cok.closed && !cok.rain && cok.pc_power && (cok.humidity < cok.hum_max * 10)) {
							cok.action("\x01");
							Sleep(2000);
						}

						if (cok.open) {
							if (!cok.ii)
								cok.action("\x07");
							if (!cok.fan && cok.fmode == AMOSKO::FanMode::FAN_AUTO)
								cok.action("\x05");
							if (!isUFOrunning(frame)) {
								Sleep(2000);
								cok.snapImage("start.bmp");
								startUFO();
							}
						}

					}
					else {
						closeUFO();

						if (cok.open) {
							cok.snapImage("end.bmp");
							cok.action("\x02");
							Sleep(2000);
						}

						if (cok.fan && cok.fmode == AMOSKO::FanMode::FAN_AUTO)
							cok.action("\x06");
						if (cok.ii) {
							cok.action("\x08");
							cok.rotateLogs();
						}

						/*if (cok.closed && cok.moving && ++attempt >= 5) {									//Paniri Caur badness!!!
								cok.action("\x0b");															//reset slave if fails to open
								cok.closeCOM();																//
								cok.uptodate3 = cok.uptodate2 = false;										//
								log_0.output << log_0.timestamp() << "D: Sluha sa restartuje..." << endl;   //
								Sleep(10000);																//
								attempt = 0;																//
						}*/
					}
				}
			}
			else {
				log_0.output << log_0.timestamp() << "D: " << cok.comID << " sa nepodarilo otvorit." << endl;
			}

			//here are critical notifications checked and handled (beter use another checker thread... or watchchdog application)
			//
			//is commport in automatic mode and open?
			//is commport in automatic mode and communicating?
			//is cok closed during daylight?
			//cok is open during night-time?
			//is ufo running during night-time?
			//is enough diskspace?
			//is daemon running

		}
		else {
			cok.closeCOM();
			cok.rotateLogs();
		}

		Sleep(AMOSKO::REFRESH);
	}

	return 0;
}
