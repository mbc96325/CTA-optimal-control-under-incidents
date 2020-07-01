#include "Simulation.hpp"
#include "util.hpp"

int main() {
	Simulation myFirstSim;
	myFirstSim.init();
	Report report = myFirstSim.run();
	report.show();

	return 0;
}