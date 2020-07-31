import numpy as np
from ctypes import *
import csv
import copy
from queue import PriorityQueue
import random

# properties of agent entities
class Agent(object):
    def __init__(self):
        self.station_id = None
        self.queueSize = 0
        self.future_queue = None
        self.current_queue = None
        # self.temp_queue = None
        self.waitingTime = 0

# Passengers waiting outside the stations
class Passenger(object):
    def __init__(self, O=-1, D=-1, num=0, time=-1):
        self.O = O
        self.D = D
        self.num = num
        self.time = time

class World(object):
    """
    The simulation class.
    The simulation will start at 5:00 a.m. and the control will start at 15:15 and end at 18:00.
    After using .make_world() or .reset(), the time will be 15:15 and there will be passengers waiting in queue outside the controlled stations--36, 37, 41

    """
    def __init__(self):
        # list of agents and entities (can change at execution-time!)
        self.agents = [] # controlled stations
        self.dim_state = 2
        self.dim_action = 11
        self.current_time = 0
        self.control_start_time = 54900 # 15:15
        self.control_end_time = 64800   # 18:00
        self.time_interval = 15 * 60    # the time interval for observation & decision

        # load the Simulator
        self.Sim = WinDLL("CTA-railway.dll")
        initAPI(self.Sim)   # activate the APIs

        # here to load the OD for the controlled stations and store in the memory
        self.Queue36 = loadODQueue("./data/station36.csv")
        self.Queue37 = loadODQueue("./data/station37.csv")
        self.Queue41 = loadODQueue("./data/station41.csv")

        # some parameters
        self.using_bus_prop = 0.2
        self.using_taxi_prop = 0.2
        self.in_vehicle_time_factor = 0.121
        self.waiting_on_platform_factor = 0.299
        self.platform_crowding_risk_factor = 1.0
        self.waiting_outside_factor = 0.299 * 1.5
        self.using_bus_penalty = 1.321
        self.using_taxi_penalty = 2.0

    def make_world(self):
        # simulator
        self.Sim.initSim()

        # the agents
        self.agents = [Agent() for i in range(3)]   # 是否需要对两个方向分别控制？
        self.agents[0].station_id = 36
        self.agents[1].station_id = 37
        self.agents[2].station_id = 41

        self.reset_world()

    def reset_world(self):
        # simulator
        self.current_time = 0
        self.Sim.resetSim()
        # add the suspend points
        for t in range(self.control_start_time, self.control_end_time, self.time_interval):
            self.Sim.addSuspend(float(t))
        self.Sim.runSim()   # run to 15:15

        # load the OD from memory, avoid reading files frequently
        self.agents[0].future_queue = copy.deepcopy(self.Queue36)
        self.agents[1].future_queue = copy.deepcopy(self.Queue37)
        self.agents[2].future_queue = copy.deepcopy(self.Queue41)
        for i in range(3):
            self.agents[i].waitingTime = 0
            self.agents[i].temp_queue = PriorityQueue()
            self.agents[i].current_queue = PriorityQueue()
        
        # have the first 15 min's passengers put into the queue
        self.current_time = self.Sim.getTime()    # synchronize the time
        for i in range(3):
            while not self.agents[i].future_queue.empty():
                first_passenger = self.agents[i].future_queue.get() # this is a tuple of (int: arrivalTime, Passenger: p)
                if first_passenger[0] <= self.current_time:
                    self.agents[i].current_queue.put(first_passenger)
                    self.agents[i].queueSize += first_passenger[1].num
                else:
                    self.agents[i].future_queue.put(first_passenger)

    def observation(self, agent):
        # 这里要看哪些？三个站的站外排队人数，蓝线上所有站的双向排队人数.排队人数怎么放到这个array里也讲究的。

        self_demand = agent.current_demand
        other_demand = 0
        for other in self.agents:
            if other is not agent:
                other_demand = other.current_demand
        return np.array([self_demand, other_demand])

    # update state of the world
    def step(self, action_n):
        # 1. let some passengers get into the station, some will leave
        for i, action in enumerate(action_n):# 0 - 1
            action_prop = np.argmax(action)/10
            num_LetIn = int(action_prop * self.agents[i].queueSize)

            # put the first num_LetIn passengers into the station
            while num_LetIn > 0:
                passenger = self.agents[i].current_queue.get()[1]
                self.Sim.addOD(passenger.time, passenger.O, passenger.D, passenger.num) # here consider OD group will travel together
                num_LetIn -= passenger.num
                self.agents[i].queueSize -= passenger.num

            if not self.agents[i].current_queue.empty():
                # if there are passengers left, have some of them take a bus or a taxi
                temp_queue = PriorityQueue()
                while not self.agents[i].current_queue.empty():
                    p = self.agents[i].current_queue.get()
                    r = random.random()
                    if r <= self.using_bus_prop:
                        # check if can take a bus
                        # if can, turn to the bus

                        self.agents[i].queueSize -= p[1].num

                        # else, put into the temp_queue

                    elif r <= self.using_bus_prop + self.using_taxi_prop:
                        # take a taxi


                        self.agents[i].queueSize -= p[1].num

                    else:
                        # put into the temp_queue
                        temp_queue.put(p)
                
                while not temp_queue.empty():
                    self.agents[i].current_queue.put(temp_queue.get())
                    
        # 2. run the simulator to the next control point
        self.Sim.runSim()
        self.current_time = self.Sim.getTime()

        # 3. add new passengers coming during the interval
        for i in range(len(self.agents)):
            while not self.agents[i].future_queue.empty():
                first_passenger = self.agents[i].future_queue.get()
                if first_passenger[0] <= self.current_time:
                    self.agents[i].current_queue.put(first_passenger)
                    self.agents[i].queueSize += first_passenger[1].num
                else:
                    self.agents[i].future_queue.put(first_passenger)

    def get_cost(self):
        waiting1 = self.agents[0].current_demand - self.agents[0].allowed_demand
        waiting2 = self.agents[1].current_demand +  waiting1 - self.agents[1].allowed_demand
        travel_time = (self.agents[0].allowed_demand + self.agents[1].allowed_demand)**2 + 10
        return (travel_time + waiting1*100 + waiting2*10)*1e-5
            
    def if_done(self):
        return self.Sim.SimIsFinished()

def initAPI(dll):
    """
    function to init the API of the dll for python calls
    """
    dll.initSim.restype = c_void_p
    dll.resetSim.restype = c_void_p
    dll.runSim.restype = c_void_p
    dll.SimIsFinished.restype = c_bool
    dll.getTotalTravelTime.restype = c_double
    dll.getTotalDelay.restype = c_double
    dll.getTime.restype = c_double

    dll.getStationDelay.argtypes = [c_int, c_int]
    dll.getStationDelay.restype = c_double

    dll.getStationPass.argtypes = [c_int, c_int]
    dll.getStationPass.restype = c_int

    dll.getStationWaitingPassengers.argtypes = [c_int, c_int]
    dll.getStationWaitingPassengers.restype = c_int

    dll.addSuspend.argtypes = [c_double]
    dll.addSuspend.restype = c_void_p

    dll.addOD.argtypes = [c_double, c_int, c_int, c_int] # time, from, to, num
    dll.addOD.restype = c_void_p

def loadODQueue(_file):
    """
    load the OD for the controlled stations, return a PriorityQueue of OD of the station
    """
    PQ = PriorityQueue()

    with open(_file, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            O = int(row[0])
            D = int(row[1])
            num = int(row[2])
            time = int(row[3])
            newPassenger = Passenger(O, D, num, time)
            PQ.put((time, newPassenger))
    
    return PQ

