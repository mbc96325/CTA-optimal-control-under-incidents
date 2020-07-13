#pragma once
//Header Files
#include "util.hpp"
#include "Simulation.hpp"

// this is the function to load the data and initalize the Simulation 
void Simulation::init() {
	// first init the variables
	policy_num = new int* [TOTAL_STATIONS];
	for (int i = 0; i < TOTAL_STATIONS; i++) {
		policy_num[i] = new int[TOTAL_STATIONS];
		for (int j = 0; j < TOTAL_STATIONS; j++) {
			policy_num[i][j] = 0;
		}
	}

	policy = new int** [TOTAL_STATIONS];
	for (int i = 0; i < TOTAL_STATIONS; i++) {
		policy[i] = new int* [TOTAL_STATIONS];
		for (int j = 0; j < TOTAL_STATIONS; j++) {
			policy[i][j] = new int[MAX_POLICY_NUM];
			for (int k = 0; k < MAX_POLICY_NUM; k++) {
				policy[i][j][k] = -1;
			}
		}
	}

	directions = new int* [TOTAL_STATIONS];
	for (int i = 0; i < TOTAL_STATIONS; i++) {
		directions[i] = new int[TOTAL_STATIONS];
		for (int j = 0; j < TOTAL_STATIONS; j++) {
			directions[i][j] = -1;
		}
	}

	transferTime = new double* [TOTAL_STATIONS];
	for (int i = 0; i < TOTAL_STATIONS; i++) {
		transferTime[i] = new double[TOTAL_STATIONS];
		for (int j = 0; j < TOTAL_STATIONS; j++) {
			transferTime[i][j] = -1.0;
		}
	}

	// then load the data
	cout << "Reading disk";
	// arrivalStationID
	str_mat str_ASID =			readcsv("data/arrivalStationID.csv");	cout << ".";
	str_mat str_AT =			readcsv("data/arrivalTime.csv");		cout << ".";
	str_mat str_directions =	readcsv("data/directions.csv");			cout << ".";
	str_mat str_policy =		readcsv("data/policy.csv");				cout << ".";
	str_mat str_policy_num =	readcsv("data/policy_num.csv");			cout << ".";
	str_mat str_STI =			readcsv("data/startTrainInfo.csv");		cout << ".";
	str_mat str_stations =		readcsv("data/stations.csv");			cout << ".";
	str_mat str_TT =			readcsv("data/transferTime.csv");		cout << ".";
	str_mat str_fixedOD =		readcsv("data/fixedOD.csv");			cout << ".";
	cout << "done\nLoading data";

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

		Station newStation(stationID, lineID, isTerminal0, isTerminal1, isTransfer);
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
		// filter the od between the transfer stations...
		if (transferTime[newOD[0]][newOD[1]] == -1)
			fixedOD.push_back(newOD);
	}

	// init the iterators
	totalTrainNum = str_STI.size();	// use startTranInfo to get the train number
	time_iter = new int[totalTrainNum];
	stationID_iter = new int[totalTrainNum];
	cout << "done\nStart initializing the simulator...";

	// reset using the loaded data
	reset();
}

// reset/init the simulation state using loaded data.
void Simulation::reset() {
	time = 0.0;
	_last_time = 0.0;
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
		Event newODEvent(double(time), NEW_OD, false);
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
		stations[i].queueSize[0] = 0;
		stations[i].queueSize[1] = 0;
		stations[i].avg_inStationTime[0] = 0.0;
		stations[i].avg_inStationTime[1] = 0.0;
		stations[i].delay[0] = 0.0;
		stations[i].delay[1] = 0.0;
	}
}