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
			
			//// debug
			//cout << time;
			//if (nextevent.type == ARRIVAL)
			//	cout << "\tArrival\n";
			//else
			//	cout << "\tNew OD\n";

			if (nextevent.type == ARRIVAL) {

				Train* train = nextevent.train;
				int trainID = train->trainID;
				int station = train->arrivingStation;
				int direction = train->direction;
				int& capacity = train->capacity;
				int* destination = train->destination;
				int& passengerNum = train->passengerNum;
				int lineID = train->lineID;

				// debug
				if (_last_time > time) {
					cout << "ERROR: time error!\n";
					cout << "last time: " << _last_time << "\n";
					cout << "current time: " << time << "\n";
				}

				// calculate travel time and passenger get off
				totalTravelTime += passengerNum * (time - train->lastTime);
				passengerNum -= destination[station];
				capacity += destination[station];
				num_arrived += destination[station];
				destination[station] = 0;

				// if it's a transfer station, do the transfer (add new OD to the stations)
				if (stations[station].isTransfer) {
					for (int dest_station = 0; dest_station < TOTAL_STATIONS; dest_station++) {
						if (destination[dest_station] > 0) {
							// first, find the passengers whose trip is finished ( not at this station,
							// but at its transfer station ), finish them!
							double transfer_time = 0.0;
							int real_station = getRealStation(station, dest_station, transfer_time);

							// arriving the destination
							if (real_station == dest_station) { 
								// meaning that passengers can transfer to the destination without taking a train
								int off_num = destination[dest_station];
								passengerNum -= destination[dest_station];
								capacity += destination[dest_station];
								num_arrived += destination[dest_station];	// consider them as arriving the dest
								destination[dest_station] = 0;
								totalTravelTime += transfer_time * off_num;

								// skip the transfer check
								continue;
							}

							// really need a transfer
							if (transferTime[station][getNextStation(station, dest_station, lineID)] != -1) {
								// meaning the passenger should transfer,
								// assume the passengers directly go to the real station

								// 1. get off the train
								int num_transfer = destination[dest_station];
								passengerNum -= num_transfer;
								capacity += num_transfer;
								destination[dest_station] = 0;

								// 2. count the time they walk to the transfer station
								totalTravelTime += transfer_time * num_transfer;

								// 3. create a new OD event for these passengers
								Event newEvent(time + transfer_time, NEW_OD, true);
								newEvent.from = real_station;
								newEvent.to = dest_station;
								newEvent.num = num_transfer;
								EventQueue.push(newEvent);
							}
						}
					}
				}

				// if not terminal, new passengers board and calculate delay and travel time
				// !stations[station].isTerminal[direction] && 
				if (!trainEnd(trainID)) {
					Q* passengerQueue = &stations[station].queue[direction];

					// calculate delay and total travel time
					double delta_time = (time - stations[station].avg_inStationTime[direction]) * (double)stations[station].queueSize[direction];
					/*if (delta_time < 0)
						cout << "ERROR: negative delay!\n";*/
					totalDelay += delta_time;
					totalTravelTime += delta_time;
					stations[station].avg_inStationTime[direction] = time;
					stations[station].delay[direction] += delta_time;		// count the delay contributed by the station

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

				// if is terminal (maybe because of the incident), delete the train
				// (debug) report the passenger numbers on the train
				else{
					if (passengerNum > 0) {
						if (!stations[station].isTerminal[direction])
							cout << passengerNum << " passengers not cleared at temperary terminal " << station << "!\n";
						else
							cout << "ERROR: " << passengerNum << " passengers not cleared at fixed terminal station " << station << "!\n";
					}
					// Here to deal with the passengers whose trip is not yet finished, if exist.
					// These people are neither transfering nor arriving at the destination,
					// thus, we only need to add them back to the queues.
					for (int dest_station = 0; dest_station < TOTAL_STATIONS; dest_station++) {
						if (destination[dest_station] > 0) {
							// directly add new OD pairs
							Event newODEvent(time, NEW_OD, true);
							newODEvent.from = station;
							newODEvent.to = dest_station;
							newODEvent.num = destination[dest_station];
							EventQueue.push(newODEvent);
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
					// check the real station
					double transfer_time = 0.0;
					int real_station = getRealStation(nextevent.from, nextevent.to, transfer_time);

					// a. directly add to the queue
					if (real_station == nextevent.from) {
						addPassengers(nextevent.from, nextevent.to, nextevent.num);
						if (!nextevent.isTransfer)
							num_departed += nextevent.num;
					}

					// b. can transfer to the destination
					else if (real_station == nextevent.to) {
						totalTravelTime += transfer_time;
					}

					// c. still need a transfer
					else {
						totalTravelTime += transfer_time;
						nextevent.from = real_station;
						nextevent.time = time + transfer_time;
						EventQueue.push(nextevent);
					}
				}
			}

		}

		_last_time = time;

	} while (time < TOTAL_SIMULATION_TIME);

	// when time is up
	return report();
}