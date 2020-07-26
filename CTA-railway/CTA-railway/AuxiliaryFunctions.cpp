#pragma once
//Header Files
#include "util.hpp"
#include "Simulation.hpp"

// if it is the OD just put into the system, lineID should be -1
int Simulation::getNextStation(int from, int to, int lineID) {
	// for robustness.
	if (from == to) {
		return from;
	}

	// choose the peak/off-peak hour matrix
	int** _policy_num = policy_num;
	int*** _policy = policy_offpeak;
	if ((time >= 19080 && time <= 33900) || (time >= 51900 && time <= 66600))	// peak hour
		_policy = policy;

	int num = _policy_num[from][to];
	int nextStation;
	bool transfer = true;
	if (num == 1) {
		nextStation = _policy[from][to][0];
	}
	else {
		for (int i = 0; i < num; i++) {
			nextStation = _policy[from][to][i];
			// if there is an optimal solution on the same line, abandon the transfer
			if (stations[nextStation].lineID == lineID) {
				transfer = false;
				break;
			}
		}
		if (transfer) {
			// randomly choose a station to transfer to
			nextStation = _policy[from][to][rand() % num];
		}
	}
	return nextStation;
}

// the function to add new OD pairs to the queues of stations
// NOTE: check the real station before use this function!
// the 'from' station must be the real station to get on the train
void Simulation::addPassengers(int from, int to, int num) {
	// check if the passenger can take the train, will cost some time.
	double _temp;
	int _real_station = getRealStation(from, to, _temp);
	if (_real_station != from) {
		// if not satisfy the requirement...
		throw "Not real station!";
	}

	int _next_station = getNextStation(from, to, -1);
	int direction = directions[from][_next_station];
	Station* station = &stations[from];

	// push the passengers into the queue, update avg_inStationTime
	double queue_len = (double)station->queueSize[direction];
	double new_len = queue_len + double(num);
	station->avg_inStationTime[direction] = (queue_len * station->avg_inStationTime[direction] + double(num) * time) / new_len;
	// debug
	if (station->avg_inStationTime[direction] > time)
		cout << "ERROR: time error!\n";
	WaitingPassengers passengers;

	passengers.destination = to;
	passengers.numPassengers = num;
	station->queue[direction].push(passengers);
	station->queueSize[direction] += num;
	station->numPass[direction] += num;
}

// Return the real station to get on the train after transfer, the transfer time will be returned by reference _transfer_time
// Note that the result station can be the destination, meaning that the destination can be achieved only by transfering...
// If the real station == from, it means that the passenger doesn't need to transfer.
int Simulation::getRealStation(int from, int to, double& _transfer_time) {
	_transfer_time = 0.0;
	int nextStation = getNextStation(from, to, -1);
	while (transferTime[from][nextStation] != -1) {
		// meaning the two stations are transfer stations to each other.
		_transfer_time += transferTime[from][nextStation];
		from = nextStation;
		nextStation = getNextStation(from, to, -1);
	}
	return from;
}

double Simulation::getNextArrivalTime(int trainID) {
	double NAT = arrivalTime[trainID][time_iter[trainID]];
	time_iter[trainID]++;
	return NAT;
}

int Simulation::getNextArrivalStationID(int trainID) {
	int NASID = arrivalStationID[trainID][stationID_iter[trainID]];
	stationID_iter[trainID]++;
	return NASID;
}

bool Simulation::trainEnd(int trainID) {
	if (time_iter[trainID] >= arrivalTime[trainID].size())
		return true;
	else
		return false;
}

// an inner function to arrange all the information needed in the RL model
Report Simulation::report() {
	Report result;
	if (time < SIMULATION_END_TIME)
		result.isFinished = false;
	else
		result.isFinished = true;
	result.totalDelay = totalDelay;
	result.totalTravelTime = totalTravelTime;
	result.numArrived = num_arrived;
	result.numDeparted = num_departed;

	return result;
}

// get the delay contributed by a station (a direction)
double Simulation::getStationDelay(int stationID, int direction) {
	return stations[stationID].delay[direction];
}

int Simulation::getStationPass(int stationID, int direction) {
	return stations[stationID].numPass[direction];
}

int Simulation::getStationWaitingPassengers(int stationID, int direction) {
	return stations[stationID].queueSize[direction];
}

double Simulation::getTime() {
	return time;
}

// the function of Report
void Report::show() {
	if (isFinished)
		cout << "isFinished:\t\t\t" << "TRUE" << "\n";
	else
		cout << "isFinished:\t\t\t" << "FALSE" << "\n";
	cout << "totalTravelTime (h):\t\t" << totalTravelTime/3600.0 << "\n";
	cout << "totalDelay (h):\t\t\t" << totalDelay/3600.0 << "\n";
	cout << "# passenger departed:\t\t" << numDeparted << "\n";
	cout << "# passenger arrived:\t\t" << numArrived << "\n";
	cout << "average travel time (min):\t" << totalTravelTime / double(numDeparted * 60) << endl;
}

