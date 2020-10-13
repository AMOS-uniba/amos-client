#include <windows.h>

class PerformanceTimer {

		public:

		PerformanceTimer();
		~PerformanceTimer();
		void start();
		void stop();
		int getElapsedTime();
		void wait(int ms);

	private:
		LARGE_INTEGER counterFrequency;				//[cycles/s]
		LARGE_INTEGER t0, t;						//cycles

};

extern PerformanceTimer timer;