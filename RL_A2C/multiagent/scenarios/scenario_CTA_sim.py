import numpy as np

# properties of agent entities
class Agent(object):
    def __init__(self):
        self.station_id = None
        self.state = None
        self.demand = None
        self.allowed_demand = 0
        self.current_demand = 0

class World(object):
    # simulation class
    def __init__(self):
        # list of agents and entities (can change at execution-time!)
        self.agents = [] # controlled stations
        self.dim_state = 2
        self.dim_action = 11
        self.current_time = 0
        self.sim_time = [0, 100]
        self.total_allowed_demand = [0]*len(self.agents)


    def make_world(self):
        self.agents = [Agent() for i in range(2)]
        for i, agent in enumerate(self.agents):
            agent.station_id = i
            agent.demand = 100 + i*50
        # add landmarks
        self.reset_world()

    # update state of the world
    def step(self, action_n):
        self.current_time += 1
        for i, action in enumerate(action_n):# 0 - 1
            # print(action)
            action_prop = np.argmax(action)/10
            self.agents[i].current_demand = self.agents[i].demand + self.current_time * 10
            self.agents[i].allowed_demand = self.agents[i].current_demand * action_prop
            a=1


    def if_done(self):
        return self.current_time >= self.sim_time[1]

    def observation(self, agent):
        self_demand = agent.current_demand
        other_demand = 0
        for other in self.agents:
            if other is not agent:
                other_demand = other.current_demand
        return np.array([self_demand, other_demand])

    def reset_world(self):
        self.current_time = 0
        action_n = [1, 1]
        for i, action in enumerate(action_n):# 0 - 10
            self.agents[i].current_demand = self.agents[i].demand + self.current_time * 10
            self.agents[i].allowed_demand = self.agents[i].current_demand*action
            a=1

    def get_cost(self):
        waiting1 = self.agents[0].current_demand - self.agents[0].allowed_demand
        waiting2 = self.agents[1].current_demand +  waiting1 - self.agents[1].allowed_demand
        travel_time = (self.agents[0].allowed_demand + self.agents[1].allowed_demand)**2 + 10
        return (travel_time + waiting1*100 + waiting2*10)*1e-5





