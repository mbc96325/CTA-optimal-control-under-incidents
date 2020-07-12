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

	int num = policy_num[from][to];
	int nextStation;
	bool transfer = true;
	if (num == 1) {
		nextStation = policy[from][to][0];
	}
	else {
		for (int i = 0; i < num; i++) {
			nextStation = policy[from][to][i];
			// if there is an optimal solution on the same line, abandon the transfer
			if (stations[nextStation].lineID == lineID) {
				transfer = false;
				break;
			}
		}
		if (transfer) {
			// randomly choose a station to transfer to
			nextStation = policy[from][to][rand() % num];
		}
	}
	return nextStation;
}

// if it is the OD just put into the system, lineID should be -1
Policy Simulation::getPolicy(int from, int to, int lineID) {
	int num = policy_num[from][to];
	int nextStation;
	int dir;
	bool transfer = true;
	if (num == 1) {
		nextStation = policy[from][to][0];
	}
	else {
		for (int i = 0; i < num; i++) {
			nextStation = policy[from][to][i];
			// if there is an optimal solution on the same line, abandon the transfer
			if (stations[nextStation].lineID == lineID) {
				transfer = false;
				break;
			}
		}
		if (transfer) {
			// randomly choose a station to transfer to
			nextStation = policy[from][to][rand() % num];
		}
	}
	dir = directions[from][nextStation];
	if (dir != -1) { // no need to transfer
		Policy result = { dir, -1, -1 };
		return result;
	}
	else {	// need to transfer
		// here we assume a passenger won't transfer twice continuously
		// also assume there's only one shortest path after transfer
		int nextNextStation = policy[nextStation][to][0];
		dir = directions[nextStation][nextNextStation];
		if (dir == -1) {
			cout << "continuous transfer at station " << nextStation << "!\n";
		}
		Policy result = { -1, nextStation, dir };
		return result;
	}
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
	double new_len = queue_len + (double)num;
	station->avg_inStationTime[direction] = (queue_len * station->avg_inStationTime[direction]\
		+ (double)num * time) / new_len;
	WaitingPassengers passengers;

	//passengers.arrivingTime = time;
	passengers.destination = to;
	passengers.numPassengers = num;
	station->queue[direction].push(passengers);
	station->queueSize[direction] += num;
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
	if (time < TOTAL_SIMULATION_TIME)
		result.isFinished = false;
	else
		result.isFinished = true;
	result.totalDelay = totalDelay;
	result.totalTravelTime = totalTravelTime;

	return result;
}

// the function of Report
void Report::show() {
	if (isFinished)
		std::cout << "isFinished:\t" << "TRUE" << "\n";
	else
		std::cout << "isFinished:\t" << "FALSE" << "\n";
	std::cout << "totalTravelTime:" << totalTravelTime << "\n";
	std::cout << "totalDelay:\t" << totalDelay << "\n";
}