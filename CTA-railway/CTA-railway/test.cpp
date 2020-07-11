#include "Simulation.hpp"
#include "util.hpp"

int main() {
	Simulation myFirstSim;
	myFirstSim.init();
	Report report = myFirstSim.run();
	report.show();

	return 0;
}

//extern "C" {
//	Simulation myFirstSim;
//	_declspec(dllexport) void initSim() {
//		myFirstSim.init();
//	}
//	_declspec(dllexport) void runSim() {
//		Report report = myFirstSim.run();
//		report.show();
//	}
//}