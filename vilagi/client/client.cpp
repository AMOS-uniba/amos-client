//amos v3 client program  (c) Jozef Vilagi

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <conio.h>
#include "util\amosko.h"
#include "util\logger.h"

HANDLE wHnd;
AMOSKO cok;
bool serviceMode = false;
bool closing = false;

void initConsole(SHORT w, SHORT h) {
	wHnd = GetStdHandle(STD_OUTPUT_HANDLE);

	//Set the cursor visibility
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(wHnd, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(wHnd, &cursorInfo);
	// Set up the required window size:
	SMALL_RECT windowSize = { 0, 0, w - 1, h - 1 };
	// Change the console window size:
	SetConsoleWindowInfo(wHnd, TRUE, &windowSize);
	// Create a COORD to hold the buffer size:
	COORD bufferSize = { w, h };
	// Change the internal buffer size:
	SetConsoleScreenBufferSize(wHnd, bufferSize);
}

void gotoXY(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(wHnd, coord);
}

void printHelp(int x, int y) {
	gotoXY(x, y);
	printf(" o  : otvori kryt");
	gotoXY(x, ++y);
	printf(" z  : zatvori kryt");
	gotoXY(x, ++y);
	printf(" 1  : ovlada ZJO");
	gotoXY(x, ++y);
	printf(" 2  : ovlada ventilator");
	
	gotoXY(x, y += 3);
	printf(" R  : RESET sluhu   ");
	gotoXY(x, ++y);
	printf(" S  : kontrolna snimka  ");
	gotoXY(x, ++y);
	printf(" A  : automaticky rezim ");
	gotoXY(x, ++y);
	printf(" M  : manualny rezim    ");

}

void printSection1(int x, int y) {
	char stav[20];
	strcpy(stav, "NEVIE SA...    ");

	if (cok.moving) {
		if (cok.lid_direction == AMOSKO::LidDirection::OPENING)
			strcpy(stav, "OTVARA SA...    ");
		else
			strcpy(stav, "ZATVARA SA...   ");
	}
	else {
		if (cok.peeking) {
			strcpy(stav, "V MEDZIPOLOHE...");
		}
		else {
			if (cok.open) {
				strcpy(stav, "OTVORENY        ");
			}
			if (cok.closed) {
				strcpy(stav, "ZATVORENY       ");
			}
		}
	}

	gotoXY(x, y);
	printf("Kryt komory:       %s", cok.uptodate ? stav : "?               ");
	gotoXY(x, ++y);
	printf("_________________________________________________");
	gotoXY(x, y += 2);
	printf("Zosilnovac jasu:   %s", cok.uptodate ? (cok.ii ? "Zapnuty" : "       ") : "?         ");
	gotoXY(x, ++y);
	printf("Ventilator:        %s", cok.uptodate ? (cok.fan ? "Zapnuty" : "       ") : "?         ");
	
	gotoXY(x, y += 2);
	printf("SNIMAC DAZDA:      %s", cok.uptodate ? (cok.rain ? "Aktivny!!!" : "OK        ") : "?         ");
	gotoXY(x, ++y);
	printf("SNIMAC OSVETLENIA: %s", cok.uptodate ? (cok.light ? "Aktivny!!!" : "OK        ") : "?         ");
	gotoXY(x, ++y);
	printf("NAPAJANIE MAJSTRA: %s\n", cok.uptodate ? (!cok.pc_power ? "Vypadok!!!" : "OK        ") : "?         ");
	printf(" ______________________________________________________________________________");
}

void printSection2(int x, int y) {
	gotoXY(x, y);
	if (cok.uptodate3 && abs(cok.motor_position) < 999)
		printf(" Hriadel: %03d  ", cok.motor_position);
	else
		printf(" Hriadel: ?    ");

	gotoXY(x, y += 3);
	if (cok.uptodate2 && abs(cok.T)< 999)
		printf(" Teplota:%5.1f\370", cok.T / 10.0);
	else
		printf(" Teplota: ?      ");

	gotoXY(x, ++y);
	if (cok.uptodate2 && abs(cok.humidity)<=1000)
		printf(" Vlhkost: %4.1f%%", cok.humidity / 10.0);
	else
		printf(" Vlhkost: ?     ");	
	
	gotoXY(x, y += 2);
	if (cok.uptodate2 && abs(cok.T_lens)< 999)
		printf("%cT_obj : %5.1f\370", cok.lensHeat ? '+' : ' ', cok.T_lens / 10.0);
	else
		printf(" T_obj :  ?      ");
	
	gotoXY(x, ++y);
	if (cok.uptodate2 && abs(cok.T_cpu)< 999)
		printf(" T_cpu:  %5.1f\370", cok.T_cpu / 10.0);
	else
		printf(" T_cpu:   ?      ");


}

void printTime() {
	time_t mytime = time(NULL);
	gotoXY(1, 1);
	printf("UTC: %s", ctime(&mytime));
	printf(" ______________________________________________________________________________");
	gotoXY(37, 1);
	printf("Uptime: ");
	if (cok.uptodate)
		printf("%.5fd", cok.timeAlive/75.0/86400.0);
	else
		printf("?          ");
	gotoXY(66, 1);
	printf("Slnko: %5.1f\370", cok.hSun());
}

void printTitle() {
	char title[MAXCHARS];
	strcpy(title, cok.stationID);
	strcat(title, cok.isAutomatic() ? ":   AUTOMATIKA ZAPNUTA  " : ":   AUTOMATIKA VYPNUTA!  ");
	strcat(title, serviceMode ? " (SERVISNY REZIM!!!)":" ");
	SetConsoleTitle(title);
}

void printMessage(const char* message, short duration) {
	gotoXY(1, 16);
	printf(message);
	Sleep(duration);
	gotoXY(1, 16);
	printf("                                                                               ");
}

BOOL WINAPI consoleHandler(DWORD CEvent) {
	closing = true;
	Sleep(10000);
	return(true);
}

int main(int argc, char *argv[]) {

	if (argc>1)
		serviceMode = !strcmp(argv[1], "-s");

	char klavesa = 0, akcia = 0;
	char msg[100];

	initConsole(80, 18);
	if (fileExists("override")) {
		printf("Klient uz je asi spusteny...\n");
		system("pause");
		exit(1);
	}

	cok.readCfg("amos.ini");

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE);

	CloseHandle(CreateFile("override", FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL));
	Sleep((int) (AMOSKO::REFRESH * 1.6));//delay after daemon closes COM

	printTitle();
	printHelp(56, 5);

	log_0.output << log_0.timestamp() << "C: Riadenie prebera klient" << endl;

	while (!closing) {
		if (cok.openCOM()) {

			cok.updateStatus();
			cok.updateStatus2();
			cok.updateStatus3();

			cok.logStats();

			if (!serviceMode && cok.uptodate && cok.open && cok.hSun() > -6 && cok.action("\x02"))//zatvor, ak nahodou niekto zabudol zavriet klienta
				printMessage("Komora sa zatvara, Slnko je prilis vysoko!!!!!                                 ", 2000);

			if (!serviceMode && cok.uptodate && cok.ii && cok.hSun() > -6 && cok.action("\x08"))//vypni aj ZJO, ak nahodou niekto zabudol zavriet klienta
				printMessage("ZJO sa vypina, Slnko je prilis vysoko!!!!!                                     ", 2000);


			printTitle();
			printTime();
			printSection1(1, 5);
			printSection2(36, 5);

			if (_kbhit())
				klavesa = (char)_getch();
			if (klavesa == 27)
				break;

			switch (klavesa) {
			case('o'):
				if (serviceMode || cok.hSun() < -6)
					cok.action("\x01");
				else
					printMessage("Otvorenie je blokovane, Slnko je prilis vysoko!!!!!                            ", 2000);
				break;
			case('z'):
				cok.action("\x02");
				break;
			case('1'):
				if (cok.ii)
					cok.action("\x08");
				else {
					if (serviceMode || cok.hSun() < -6)
						cok.action("\x07");
					else
						printMessage("ZJO je blokovany, Slnko je prilis vysoko!!!!!                                  ", 2000);
				}
				break;
			case('2'):
				if (cok.fan)
					cok.action("\x06");
				else
					cok.action("\x05");
				break;

			case('R'):
				cok.action("\x0b");
				cok.closeCOM();
				cok.uptodate3 = cok.uptodate2 = false;
				log_0.output << log_0.timestamp() << "C: Sluha sa restartuje..." << endl;
				printTime();
				printSection1(1, 5);
				printSection2(36, 5);
				for (int i = 8; i > 0 && !closing; i--) {
					sprintf(msg, "Sluha sa restartuje... %ds                                                    ", i);
					printMessage(msg, 1000);
				}
				break;
			case('S'):
				if (cok.restoreCameraProperties("check.bmp") == 0)
					printMessage("Snimka check.bmp ulozena.                                                     ", 2000);
				else
					printMessage("Snimku sa nepodarilo urobit (kamera pripojena? UFO vypnute?)                  ", 2000);
				break;
			case('A'):
				cok.setAutomatic(true);
				printTitle();
				log_0.output << log_0.timestamp() << "C: Automaticky rezim demona zapnuty" << endl;
				break;
			case('M'):
				cok.setAutomatic(false);
				printTitle();
				log_0.output << log_0.timestamp() << "C: Automaticky rezim demona vypnuty" << endl;
				break;
			default:
				break;
			}
			akcia = klavesa = 0;

		}
		else {
			log_0.output << log_0.timestamp() << "C: " << cok.comID << " sa nepodarilo otvorit." << endl;
		}

		Sleep(AMOSKO::REFRESH);
	}

	log_0.output << log_0.timestamp() << "C: Klient ukonceny" << endl;
	cok.closeCOM();
	DeleteFile("override");

	return(0);
}










