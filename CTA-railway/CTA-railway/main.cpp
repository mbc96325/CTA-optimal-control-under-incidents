#include "Simulation.hpp"
#include "util.hpp"

int main() {
	Simulation myFirstSim;
	myFirstSim.init();
	cout << "Simulation initialized!\n" << "Start running...\n";

	Report report = myFirstSim.run();
	report.show();
	return 0;
}

// python API
extern "C" {
	Simulation Sim;
	Report report;

	// initialize the simulator, just need once
	_declspec(dllexport) void initSim() {
		Sim.init();
	}

	// reset the simulator
	_declspec(dllexport) void resetSim() {
		Sim.reset();
	}

	// start the simulator, and it will stop at the suspend point
	// the user set in the simulation events or when the simulation
	// reaches its end.
	// the report data will be printed.
	_declspec(dllexport) void runSim() {
		report = Sim.run();
		//report.show();
	}

	// functions to get the data from last report point when the
	// simulation is suspended or finished.
	_declspec(dllexport) bool SimIsFinished() {
		return report.isFinished;
	}

	_declspec(dllexport) double getTotalTravelTime() {
		return report.totalTravelTime;
	}

	_declspec(dllexport) double getTotalDelay() {
		return report.totalDelay;
	}

	_declspec(dllexport) double getStationDelay(int stationID, int direction) {
		return Sim.getStationDelay(stationID, direction);
	}

	_declspec(dllexport) int getStationPass(int stationID, int direction) {
		return Sim.getStationPass(stationID, direction);
	}

	_declspec(dllexport) int getStationWaitingPassengers(int stationID, int direction) {
		return Sim.getStationWaitingPassengers(stationID, direction);
	}

	_declspec(dllexport) void addSuspend(double suspendTime) {
		Event newSuspend(suspendTime, SUSPEND);
		Sim.addEvent(newSuspend);
	}

	_declspec(dllexport) double getTime() {
		return Sim.getTime();
	}

	_declspec(dllexport) void addOD(double time, int from, int to, int num) {
		Event newODEvent(time, NEW_OD, false);
		newODEvent.from = from;
		newODEvent.to = to;
		newODEvent.num = num;
		Sim.addEvent(newODEvent);
	}
}