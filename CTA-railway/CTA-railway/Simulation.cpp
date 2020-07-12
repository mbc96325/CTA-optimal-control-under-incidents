#pragma once
//Header Files
#include "util.hpp"
#include "Simulation.hpp"

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
							if (transferTime[station][dest_station] >= 0.0) { 
								// meaning that passengers can transfer to that station
								int off_num = destination[dest_station];
								passengerNum -= destination[dest_station];
								capacity += destination[dest_station];
								num_arrived += destination[dest_station];
								destination[dest_station] = 0;
								totalTravelTime += transferTime[station][dest_station] * off_num;

								// skip the transfer check
								continue;
							}

							// a redundant transfer_direction is calculated here.
							// ####### can be improved here #######
							Policy transfer_policy = getPolicy(station, dest_station, lineID);
							if (station == dest_station)
								throw "same station!";

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
									if (transfer_stationID == dest_station)
										cout << "tt=0 error!";
									addPassengers(transfer_stationID, dest_station, num_transfer);
									cout << "s";
								}
								else {
									// if transfer time need to be count, do it here
									totalTravelTime += transferTime[station][transfer_stationID] * num_transfer;
									// set up a new event for a future OD pair
									Event newEvent(time + transferTime[station][transfer_stationID], NEW_OD);
									newEvent.from = transfer_stationID;
									newEvent.to = dest_station;
									newEvent.num = num_transfer;
									EventQueue.push(newEvent);

									if (station == transfer_stationID) {
										cout << "transfer station error!\n";
									}
								}

							}
						}
					}
				}

				// if not terminal, new passengers board and calculate delay and travel time
				if (!stations[station].isTerminal[direction] && !trainEnd(trainID)) {
					Q* passengerQueue = &stations[station].queue[direction];

					// calculate delay and total travel time
					double delta_time = (time - stations[station].avg_inStationTime[direction])\
						* (double)stations[station].queueSize[direction];
					totalDelay += delta_time;
					totalTravelTime += delta_time;
					stations[station].avg_inStationTime[direction] = time;

					// if there is space on the train, get the passengers (if existing) onto the train
					while (!passengerQueue->empty() && capacity > 0) {
						WaitingPassengers* passengers = &passengerQueue->front();

						if (passengers->numPassengers <= capacity) {
							// all this destination group get on the train, update the passenger num on and off the train
							capacity -= passengers->numPassengers;
							passengerNum += passengers->numPassengers;
							destination[passengers->destination] += passengers->numPassengers;
							stations[station].queueSize[direction] -= passengers->numPassengers;
							passengerQueue->pop();
						}
						else {
							// part of this destination group get on the train, update the passenger num on and off the train
							passengers->numPassengers -= capacity;
							passengerNum += capacity;
							destination[passengers->destination] += capacity;
							stations[station].queueSize[direction] -= capacity;
							capacity = 0;
							break;
						}
					}

					// set up a new arrival event
					nextevent.time = getNextArrivalTime(trainID);
					train->arrivingStation = getNextArrivalStationID(trainID);
					train->lastTime = time;
					EventQueue.push(nextevent);
				}

				// if is terminal, delete the train
				// (debug) report the passenger numbers on the train
				else{
					if (passengerNum > 0) {
						cout << passengerNum << " passengers not cleared at temperary terminal " << station << "!\n";
					}
					// Here to deal with the passengers whose trip is not yet finished, if exist.
					// These people are neither transfering nor arriving at the destination,
					// thus, we only need to add them back to the queues.
					for (int dest_station = 0; dest_station < TOTAL_STATIONS; dest_station++) {
						if (destination[dest_station] > 0) {
							if (station == dest_station) {
								cout << "temp terminal error!";
							}
							// here doesn't exist transfer
							addPassengers(station, dest_station, destination[dest_station]);
							//cout << "!!";
						}
					}
					delete train;
				}

			}
			else if (nextevent.type == SUSPEND) {
				// return immediate cost for the RL model to make decision
				return report();
			}
			else if (nextevent.type == NEW_OD) {
				// add new OD pairs
				if (nextevent.from == nextevent.to) {
					cout << "illegal OD pair from " << nextevent.from << " to " << nextevent.to << " at time " << time << "!\n";
				}
				else {
					addPassengers(nextevent.from, nextevent.to, nextevent.num);
					//cout << "new od\n";
				}
			}

		}

	} while (time < TOTAL_SIMULATION_TIME);

	// when time is up
	return report();
}