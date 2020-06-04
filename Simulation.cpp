#pragma once
//Header Files
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <queue>
#include <vector>

#define TOTAL_STATIONS 200
#define DEFAULT_CAPACITY 300
#define WARMUP_PERIOD 3600
#define TOTAL_SIMULATION_TIME 7200
#define MAX_TRANSFER 8
#define MAX_POLICY_NUM 4	// the largest possible num of optimal policy from station i to station j

// declaration
struct Report;				// the struct to report to the RL model
struct WaitingPassengers;	// the struct to store the information of waiting passenegers in a queue
struct Train;				// the struct to store the information of a train

//Vector of Queues for passengers
typedef std::queue<WaitingPassengers> Q;
//typedef std::vector<Q> vecQ;
//typedef std::vector<int> transfer_list;

enum EventType {
	// only three types of events are considered in the simulation
	// ARRIVAL is the arrival of a train
	// SUSPEND is to suspend the program to wait for the RL model's new input
	// NEW_OD is to add new passengers (including the transfer passengers) to 
	//			the queues of the stations.
	// TRANSFER is to add future OD of transfer passengers
	ARRIVAL,
	SUSPEND,
	NEW_OD,
	TRANSFER
};

struct WaitingPassengers {
	//int arrivingTime;
	int numPassengers;
	int destination;
};

// the struct to depict the decision a passenger will make at a situation.
struct Policy {
	int direction;
	int transferStation;
	int transferDirection;
};

// Event Class
class Event {
	// the base class for events
public:
	const EventType type;	// the type of the event
	double time;			// the happening time of the event, in the unit of sec
	int** OD = NULL;		// if is the global OD, it will be a matrix
							// if is a transfer OD, OD[0] will be an array containing
							// [int from, int to, int num]
	Train* train = NULL;

	Event(double t, EventType type = ARRIVAL) : time(t), type(type) { }
};

//Compare events for Priority Queue
struct EventCompare {
	bool operator() (const Event left, const Event right) const {
		return left.time > right.time;
	}
};

class Station {
	// Each station in the system has a unique ID. For the transfer stations, consider there are 
	// several independent stations in each line, which have different IDs.
public:
	int ID;					// station ID
	int lineID;				// line ID
	int nextStationID[2];	// next station's ID in both directions. if this is terminal, next is -1
	int travelTime_in[2];	// travel time from the previous station to this one.
	int travelTime_out[2];	// travel time from this station to the next one.
	bool isTerminal[2];		// if the station is the terminal station
	bool isTransfer;		// if the station is a transfer station
	//transfer_list transferList;	// store the stations share the same location but have different IDs.
	Q queue[2];				// passenger queues for both directions
	double avg_inStationTime[2];	//avg arriving time of passengers in the queue, used for delay calculation

	Station(int ID, int lineID, int nextID0, int nextID1, int travelTime_in0, int travelTime_in1,\
	 	int travelTime_out0, int travelTime_out1, bool isTerminalInDir0, bool isTerminalInDir1,\
		int terminalDirection = 0) : ID(ID), lineID(lineID), isTransfer(false) {
		nextStationID[0] = nextID0;
		nextStationID[1] = nextID1;
		travelTime_in[0] = travelTime_in0;
		travelTime_in[1] = travelTime_in1;
		travelTime_out[0] = travelTime_out0;
		travelTime_out[1] = travelTime_out1;
		isTerminal[0] = isTerminalInDir0;
		isTerminal[1] = isTerminalInDir1;
	}

	// void setTransfer() {
	// 	isTransfer = true;
	// }

	int getQueueNum(int direction) {
		return queue[direction].size();
	}
};

struct Train {
	// the information about the train
	int lineID;				// the ID of the line the train in on
	int arrivingStation;	// the ID of the station the train arrives in the event
	int direction;			// 0 or 1, consistent with the map info
	int capacity;			// the remaining space on the train
	int destination[TOTAL_STATIONS] = { 0 };// numbers of passengers heading for each station
	int passengerNum;		// total number of passengers on the train
	Train(int lineID, int direction, int nextStation) : lineID(lineID), direction(direction), passengerNum(0),\
		arrivingStation(nextStation), capacity(DEFAULT_CAPACITY) {}
};

//Simulation Class
class Simulation {
public:
	double time;			// the system time in the unit of sec
	double totalTravelTime;	// including on- and off- train time
	double totalDelay;		// the off-train delay, namely the waiting time in the station queue
	int num_departed;		// number of passengers put into the system
	int num_arrived;		// number of passengers arrived at the destination
	
	int policy_num[TOTAL_STATIONS][TOTAL_STATIONS];
	// the matrix stores the num of the optimal paths from station i to station j
	int policy[TOTAL_STATIONS][TOTAL_STATIONS][MAX_POLICY_NUM];
	// the matrix stores the optimal policy--what is the next station to go if the passenger is traveling
	// from station i to station j
	// considering that there may be several optimal solutions between two stations, at most MAX_POLICY_NUM 
	// policies can be stored here
	int direction[TOTAL_STATIONS][TOTAL_STATIONS];
	// return the direction from station i to station j
	// if they are not on the same line, return -1
	double transferTime[TOTAL_STATIONS][TOTAL_STATIONS];
	// this matrix stores the transfer time between two transfer stations.
	// if loop, value is 0
	double headway[TOTAL_STATIONS];
	// if the headway is fixed for each station, it will be stored here. Namely, the frequency of new 
	// trains setting off.
	int lineIDOfStation[TOTAL_STATIONS];
	// an array to store the ID of the line that the station belongs to
	bool isSameStation[TOTAL_STATIONS][TOTAL_STATIONS];
	// used to check if a passenger has arrived at his/her destination
	// input: arriving station, destination
	// output: true if the passeneger can transfer between the two stations or they are just the same station.
	Station* stations;
	// an array to store all the stations

	Simulation() : time(0), totalTravelTime(0), totalDelay(0), num_departed(0), num_arrived(0), EventQueue() {
		srand((unsigned int)(std::time(NULL)));
	}

	// to start work from here
	void init(int** policy_direction, int** policy_change, double** transfer_time, double* Headway);
		// load the initial state
	Report run();	// return a pointer of several doubles,
					// including time, totalTravelTime and totalDelay.
	void reset();	// reload the initial state
	void addPassengers(int from, int to, int num);	// add passengers right now
	void addEvent(Event newevent) {
		EventQueue.push(newevent);
	}
	

protected:
	//Priority Queue for the events
	std::priority_queue < Event, std::vector<Event, std::allocator<Event> >, EventCompare > EventQueue;
	
	Report report();	// return the system information
	bool isOnSameLine(int station, int line);	// return true if the station is in that line 
												// or the station's transfer station is in the line
	Policy getPolicy(int from, int to, int lineID);	// return the optimal traveling policy

};

// defination of the Report structure
struct Report {
	bool isFinished;
	double totalTravelTime;
	double totalDelay;
};

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

// ################## abandoned ####################
// // return if the destination station is in the same line of the train, which means no transfer needed
// bool Simulation::isOnSameLine(int station, int line){
// 	int i = 0;
// 	while (lineIDOfStation[station][i] >= 0){
// 		if (lineIDOfStation[station][i] == line)
// 			return true;
// 		i++;
// 	}
// 	return false;
// }

// Run Simulation
Report Simulation::run() {

	do {
		if (EventQueue.empty()) {
			std::cout << "Empty Queue!" << std::endl;
			return report();
		}else {
			Event nextevent = EventQueue.top();
			EventQueue.pop();
			time = nextevent.time;

			if (nextevent.type == ARRIVAL) {

				Train* train = nextevent.train;
				int station = train->arrivingStation;
				int direction = train->direction;
				int& capacity = train->capacity;
				int* destination = train->destination;
				int& passengerNum = train->passengerNum;
				int lineID = train->lineID;

				// calculate travel time and passenger get off
				totalTravelTime += passengerNum * stations[station].travelTime_in[direction];
				passengerNum -= destination[station];
				capacity += destination[station];
				num_arrived += destination[station];
				destination[station] = 0;

				// if it's a transfer station, do the transfer (add new OD to the stations)
				if (stations[station].isTransfer) {
					// iterate the destination table of the train
					// ################# can be improved here ##################
					// table iteration is slow, other structures can be considered 
					for (int dest_station = 0; dest_station < TOTAL_STATIONS; dest_station++) {
						if (destination[dest_station] > 0) {
							// first, find the passengers whose trip is finished ( not at this station,
							// but at its transfer station ), finish them!
							if (isSameStation[station][dest_station]){
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
									int** ODpair = new int*;
									ODpair[0] = new int[3];
									ODpair[0][0] = transfer_stationID;
									ODpair[0][1] = dest_station;
									ODpair[0][2] = num_transfer;
									newEvent.OD = ODpair;
									EventQueue.push(newEvent);
								}
								

								//######## abandoned ###########
								//// get which station to go to and which direction to take
								//int transfer_stationID = stationPolicy_transfer[station][dest_station];
								//int transfer_direction = stationPolicy_direction[transfer_stationID][dest_station];
								//Station* transfer_station = &stations[transfer_stationID];

								//// push the transfer passengers into the queue, update avg_inStationTime
								//double queue_len = (double)transfer_station->queue[transfer_direction].size;
								//double new_len = queue_len + (double)num_transfer;
								//transfer_station->avg_inStationTime[transfer_direction]\
								//	= (queue_len * transfer_station->avg_inStationTime[transfer_direction]\
								//	+ (double)num_transfer * time) / new_len;
								//// ######### now here we don't consider the transfer time between stations
								//WaitingPassengers passengers;

								////passengers.arrivingTime = time;
								//passengers.destination = dest_station;
								//passengers.numPassengers = num_transfer;
								//transfer_station->queue[transfer_direction].push(passengers);
								//###################################
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
					nextevent.time = time + stations[station].travelTime_out[direction];
					train->arrivingStation = stations[station].nextStationID[direction];
					EventQueue.push(nextevent);

					// if the station is the first station, start a new train
					if (stations[station].isTerminal[1 - direction]) {
						Event newEvent(time + headway[station], ARRIVAL);
						Train* newTrain = new Train(lineID, direction, station);
						// if the trains' capacity is different in different lines, it can be set here.
						newEvent.train = newTrain;
						EventQueue.push(newEvent);
					}

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
				delete &nextevent;

				return report();
			}
			else if (nextevent.type == NEW_OD) {
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

				delete &nextevent;
			}
			else if (nextevent.type == TRANSFER) {
				// add transfer OD pairs
				int from = nextevent.OD[0][0];
				int to   = nextevent.OD[0][1];
				int num  = nextevent.OD[0][2];
				addPassengers(from, to, num);

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
	if (num == 1){
		nextStation = policy[from][to][0];
	}
	else {
		for (int i = 0; i < num; i++){
			nextStation = policy[from][to][i];
			// if there is an optimal solution on the same line, abandon the transfer
			if (lineIDOfStation[nextStation] == lineID){
				transfer = false;
				break;
			}
		}
		if (transfer) {
			// randomly choose a station to transfer to
			nextStation = policy[from][to][rand() % num];
		}
	}
	dir = direction[from][nextStation];
	if (dir != -1) {
		Policy result = {dir, -1, -1};
		return result;
	}
	else {
		dir = direction[nextStation][to];
		Policy result = {-1, nextStation, dir};
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

// this is the function to initalize the Simulation and load the data
void Simulation::init(int** policy_direction, int** policy_change, double** transfer_time, double* Headway) {

}