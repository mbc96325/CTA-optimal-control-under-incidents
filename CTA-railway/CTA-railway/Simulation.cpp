#pragma once
//Header Files
#include "util.hpp"
#include "Simulation.hpp"


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

// Run Simulation
Report Simulation::run() {

	do {
		if (EventQueue.empty()) {
			std::cout << "Empty Queue!" << std::endl;
			return report();
		}
		else {
			Event nextevent = EventQueue.top();
			EventQueue.pop();
			time = nextevent.time;

			if (nextevent.type == ARRIVAL) {

				Train* train = nextevent.train;
				int trainID = train->trainID;
				int station = train->arrivingStation;
				int direction = train->direction;
				int& capacity = train->capacity;
				int* destination = train->destination;
				int& passengerNum = train->passengerNum;
				int lineID = train->lineID;

				// calculate travel time and passenger get off
				totalTravelTime += passengerNum * (time - train->lastTime);
				passengerNum -= destination[station];
				capacity += destination[station];
				num_arrived += destination[station];
				destination[station] = 0;

				// if it's a transfer station, do the transfer (add new OD to the stations)
				if (stations[station].isTransfer) {
					// iterate the destination table of the train
					// ################# can be improved here ##################
					// 1.table iteration is slow, other structures can be considered
					// 2.all the transfer OD can be put into the queue at once! 
					for (int dest_station = 0; dest_station < TOTAL_STATIONS; dest_station++) {
						if (destination[dest_station] > 0) {
							// first, find the passengers whose trip is finished ( not at this station,
							// but at its transfer station ), finish them!
							if (transferTime[station][dest_station] >= 0.0) {		// try to use transfer time to do it...
								int off_num = destination[dest_station];
								passengerNum -= destination[dest_station];
								capacity += destination[dest_station];
								num_arrived += destination[dest_station];
								destination[dest_station] = 0;
								// if need to count the time they go to their desired exit, do it here
								// ############### ADD NEW CODE HERE ###############
								totalTravelTime += transferTime[station][dest_station] * off_num;

								// skip the transfer check
								continue;
							}

							// a redundant transfer_direction is calculated here.
							// ####### can be improved here #######
							Policy transfer_policy = getPolicy(station, dest_station, lineID);

							if (transfer_policy.direction == -1) { // meaning the passenger should transfer

								// get off the train
								int num_transfer = destination[dest_station];
								passengerNum -= num_transfer;
								capacity += num_transfer;

								///// note that the future OD should be an event,
								///// we can only add OD for the current time.
								int transfer_stationID = transfer_policy.transferStation;
								if (transferTime[station][transfer_stationID] == 0) {
									// for transfer on the same platform
									addPassengers(transfer_stationID, dest_station, num_transfer);
								}
								else {
									// if transfer time need to be count, do it here
									totalTravelTime += transferTime[station][transfer_stationID] * num_transfer;
									// set up a new event for a future OD pair
									Event newEvent(time + transferTime[station][transfer_stationID], TRANSFER);
									/*int** ODpair = new int*;
									ODpair[0] = new int[3];
									ODpair[0][0] = transfer_stationID;
									ODpair[0][1] = dest_station;
									ODpair[0][2] = num_transfer;
									newEvent.OD = ODpair;*/
									newEvent.from = transfer_stationID;
									newEvent.to = dest_station;
									newEvent.num = num_transfer;
									EventQueue.push(newEvent);
								}

							}
						}
					}
				}
				// if not terminal, new passengers board and calculate delay and travel time
				if (!stations[station].isTerminal[direction]) {
					Q* passengerQueue = &stations[station].queue[direction];

					// calculate delay and total travel time
					double delta_time = (time - stations[station].avg_inStationTime[direction])\
						* (double)passengerQueue->size();
					totalDelay += delta_time;
					totalTravelTime += delta_time;
					stations[station].avg_inStationTime[direction] = time;

					// if there is space on the train, get the passengers onto the train
					while (!passengerQueue->empty()) {
						WaitingPassengers* passengers = &passengerQueue->front();

						if (passengers->numPassengers <= capacity) {
							// all this destination group get on the train
							capacity -= passengers->numPassengers;
							passengerNum += passengers->numPassengers;
							destination[passengers->destination] += passengers->numPassengers;
							passengerQueue->pop();
						}
						else {
							// part of this destination group get on the train
							passengers->numPassengers -= capacity;
							passengerNum += capacity;
							destination[passengers->destination] += capacity;
							capacity = 0;
						}
					}

					// set up a new arrival event
					// rebuild with a new function to get the next arrival time and next station!!!!!!!!!!
					nextevent.time = getNextArrivalTime(trainID);
					train->arrivingStation = getNextArrivalStationID(trainID);
					EventQueue.push(nextevent);

					// if the station is the first station, start a new train
					// if (stations[station].isTerminal[1 - direction]) {
					// 	Event newEvent(time + headway[station], ARRIVAL);
					// 	Train* newTrain = new Train(lineID, direction, station);
					// 	// if the trains' capacity is different in different lines, it can be set here.
					// 	newEvent.train = newTrain;
					// 	EventQueue.push(newEvent);
					// }				// now we use a table to init all the trains at once

				}
				// if is terminal, delete the train
				// (debug) report the passenger numbers on the train
				if (stations[station].isTerminal[direction]) {
					if (passengerNum > 0)
						throw "passenger not cleared at terminal!";
					delete train;
					delete& nextevent;
				}

			}
			else if (nextevent.type == SUSPEND) {
				// return immediate cost for the RL model to make decision
				delete& nextevent;

				return report();
			}
			else if (nextevent.type == NEW_OD) {
				// very slow!!!!!!!!!!!!!!!!! better using more compact way
				// add new ODs to the corresponding stations' queues
				int** OD = nextevent.OD;
				for (int from = 0; from < TOTAL_STATIONS; from++) {
					for (int to = 0; to < TOTAL_STATIONS; to++) {
						// we add all these OD pairs all at the same time
						if (OD[from][to] != 0) {
							addPassengers(from, to, OD[from][to]);
						}
					}
				}
				/////////also need to delete the OD!!!!!!!!!!!!!!
				delete& nextevent;
			}
			else if (nextevent.type == TRANSFER) {
				// add transfer OD pairs
				/*int from = nextevent.OD[0][0];
				int to = nextevent.OD[0][1];
				int num = nextevent.OD[0][2];*/
				addPassengers(nextevent.from, nextevent.to, nextevent.num);
				/////////also need to delete the OD!!!!!!!!!!!!!!...??
				delete& nextevent;
			}

		}

	} while (time < TOTAL_SIMULATION_TIME);

	// when time is up
	return report();
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
			if (lineIDOfStation[nextStation] == lineID) {
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
	if (dir != -1) {
		Policy result = { dir, -1, -1 };
		return result;
	}
	else {
		dir = directions[nextStation][to];
		Policy result = { -1, nextStation, dir };
		return result;
	}
}

// the function to add new OD pairs to the queues of stations
void Simulation::addPassengers(int from, int to, int num) {
	// check if need to transfer to another line
	// if need to do so, calculate the transfer time !!!!!!!
	Policy policy = getPolicy(from, to, -1);
	int direction = policy.direction;
	if (direction == -1) {	// if need to transfer
		// calculate the transfer time for the new OD put into the system
		totalTravelTime += transferTime[from][policy.transferStation] * num;

		from = policy.transferStation;
		direction = policy.transferDirection;
	}

	Station* station = &stations[from];

	// push the passengers into the queue, update avg_inStationTime
	double queue_len = (double)station->queue[direction].size();
	double new_len = queue_len + (double)num;
	station->avg_inStationTime[direction] = (queue_len * station->avg_inStationTime[direction]\
		+ (double)num * time) / new_len;
	WaitingPassengers passengers;

	//passengers.arrivingTime = time;
	passengers.destination = to;
	passengers.numPassengers = num;
	station->queue[direction].push(passengers);

	// update the num of departed passengers
	num_departed += num;
}

// this is the function to load the data and initalize the Simulation 
void Simulation::init() {
	// first load the data
	// arrivalStationID
	str_mat str_ASID =			readcsv("simple_data/arrivalStationID.csv");	// arrivalStationID
	str_mat str_AT =			readcsv("simple_data/arrivalTime.csv");			// arrivalTime
	str_mat str_directions =	readcsv("simple_data/directions.csv");			// directions
	str_mat str_policy =		readcsv("simple_data/policy.csv");				// policy
	str_mat str_policy_num =	readcsv("simple_data/policy_num.csv");			// policy_num
	str_mat str_STI =			readcsv("simple_data/startTrainInfo.csv");		// startTrainInfo
	str_mat str_stations =		readcsv("simple_data/stations.csv");			// stations
	str_mat str_TT =			readcsv("simple_data/transferTime.csv");		// transferTime
	str_mat str_fixedOD =		readcsv("simple_data/fixedOD.csv");				// fixedOD

	// then load the data for each variable in turn
	// str_ASID to arrivalStationID: str_mat to vector of vector of int
	for (auto iter_row = str_ASID.cbegin(); iter_row != str_ASID.cend(); iter_row++)
	{
		vector<int> newASID;
		for (auto iter_col = (*iter_row).cbegin(); iter_col != (*iter_row).cend(); iter_col++)
		{
			newASID.push_back(atoi((*iter_col).c_str()));
		}
		arrivalStationID.push_back(newASID);
	}

	// str_AT to arrivalTime: str_mat to vector of vector of double
	for (auto iter_row = str_AT.cbegin(); iter_row != str_AT.cend(); iter_row++)
	{
		vector<double> newAT;
		for (auto iter_col = (*iter_row).cbegin(); iter_col != (*iter_row).cend(); iter_col++)
		{
			newAT.push_back(atof((*iter_col).c_str()));
		}
		arrivalTime.push_back(newAT);
	}

	// str_directions to directions: str_mat (compact format) to 2-d array
	for (auto iter_row = str_directions.cbegin(); iter_row != str_directions.cend(); iter_row++)
	{
		int from = atoi((*iter_row)[0].c_str());
		int to = atoi((*iter_row)[1].c_str());
		int direction = atoi((*iter_row)[2].c_str());
		directions[from][to] = direction;
	}

	// str_policy to policy: str_mat (compact format) to 3-d array
	for (auto iter_row = str_policy.cbegin(); iter_row != str_policy.cend(); iter_row++)
	{
		int from = atoi((*iter_row)[0].c_str());
		int to = atoi((*iter_row)[1].c_str());
		int index = 0;
		auto iter_col = (*iter_row).cbegin();
		iter_col++;
		iter_col++;
		while (iter_col != (*iter_row).cend()) {
			policy[from][to][index] = atoi((*iter_col).c_str());
			index++;
			iter_col++;
		}
	}

	// str_policy_num to policy_num & str_TT to transferTime
	for (int row = 0; row < TOTAL_STATIONS; row++) {
		for (int col = 0; col < TOTAL_STATIONS; col++) {
			policy_num[row][col] = atoi((str_policy_num[row][col]).c_str());
			transferTime[row][col] = atoi((str_TT[row][col]).c_str());
		}
	}
	// str_STI to startTrainInfo
	for (auto iter_row = str_STI.cbegin(); iter_row != str_STI.cend(); iter_row++)
	{
		vector<int> newTrain;
		for (auto iter_col = (*iter_row).cbegin(); iter_col != (*iter_row).cend(); iter_col++)
		{
			newTrain.push_back(atoi((*iter_col).c_str()));
		}
		startTrainInfo.push_back(newTrain);
	}

	// str_stations to stations
	for (auto iter_row = str_stations.cbegin(); iter_row != str_stations.cend(); iter_row++)
	{
		int stationID = atoi((*iter_row)[0].c_str());
		int lineID = atoi((*iter_row)[1].c_str());
		bool isTerminal0 = str2bool((*iter_row)[2]);
		bool isTerminal1 = str2bool((*iter_row)[3]);
		bool isTransfer = str2bool((*iter_row)[4]);

		Station newStation(stationID, lineID, isTerminal0, isTerminal0, isTransfer);
		stations.push_back(newStation);
	}

	// str_fixedOD to fixedOD
	for (auto iter_row = str_fixedOD.cbegin(); iter_row != str_fixedOD.cend(); iter_row++)
	{
		vector<int> newOD;
		for (auto iter_col = (*iter_row).cbegin(); iter_col != (*iter_row).cend(); iter_col++)
		{
			newOD.push_back(atoi((*iter_col).c_str()));
		}
		fixedOD.push_back(newOD);
	}

	// init the iterators
	totalTrainNum = str_STI.size();	// use startTranInfo to get the train number
	time_iter = new int[totalTrainNum];
	stationID_iter = new int[totalTrainNum];

	// reset using the loaded data
	reset();
}

// reset/init the simulation state using loaded data.
void Simulation::reset() {
	time = 0.0;
	totalTravelTime = 0.0;
	totalDelay = 0.0;
	num_departed = 0;
	num_arrived = 0;

	// #################################
	// clear the events, maybe very slow
	// #################################
	while (!EventQueue.empty())
		EventQueue.pop();

	// reset the iterators
	for (int i = 0; i < totalTrainNum; i++) {
		time_iter[i] = 0;
		stationID_iter[i] = 0;
	}

	// renew the start out trains
	for (int i = 0; i < totalTrainNum; i++) {
		int trainID = startTrainInfo[i][0];
		int startingStationID = startTrainInfo[i][1];
		int lineID = startTrainInfo[i][2];
		int direction = startTrainInfo[i][3];
		int capacity = startTrainInfo[i][4];
		double startTime = startTrainInfo[i][5];

		Train* newTrain = new Train(trainID, lineID, direction, startingStationID, startTime, capacity);
		Event newEvent(startTime, ARRIVAL);
		newEvent.train = newTrain;
		EventQueue.push(newEvent);
	}

	// renew the fixed OD pairs
	for (auto iter_row = fixedOD.cbegin(); iter_row != fixedOD.cend(); iter_row++)
	{
		int O = (*iter_row)[0];
		int D = (*iter_row)[1];
		int number = (*iter_row)[2];
		int time = (*iter_row)[3];
		// #########################################################################
		// TRANSFER type is more suitable for compact format. Will be changed later.
		// #########################################################################
		Event newODEvent(double(time), TRANSFER);
		newODEvent.from = O;
		newODEvent.to = D;
		newODEvent.num = number;
		EventQueue.push(newODEvent);
	}

	// renew the stations (queues)
	for (int i = 0; i < TOTAL_STATIONS; i++) {
		// reset the queues, maybe a bit slow
		while (!stations[i].queue[0].empty())
			stations[i].queue[0].pop();
		while (!stations[i].queue[1].empty())
			stations[i].queue[1].pop();

		// reset avg_inStationTime
		stations[i].avg_inStationTime[0] = 0.0;
		stations[i].avg_inStationTime[1] = 0.0;
	}
}