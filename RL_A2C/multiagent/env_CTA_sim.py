import gym
from gym import spaces
from gym.envs.registration import EnvSpec
import numpy as np
from multiagent.multi_discrete import MultiDiscrete
import copy

# environment for all agents in the multiagent world
# currently code assumes that no agents will be created/destroyed at runtime!
class MultiAgentEnv(gym.Env):
    metadata = {
        'render.modes' : ['human', 'rgb_array']
    }

    def __init__(self, world):

        self.world = world
        self.agents = self.world.agents
        # set required vectorized gym env property
        self.n = len(world.agents)
        # scenario callbacks

        # if true, every agent has the same reward
        self.shared_reward = True
        self.time = 0

        # configure spaces
        self.action_space = []
        self.observation_space = []
        for agent in self.agents:
            total_action_space = []
            action_space = spaces.Discrete(world.dim_action)
            total_action_space.append(action_space)
            act_space = MultiDiscrete([[0, act_space.n - 1] for act_space in total_action_space])
            self.action_space.append(act_space)
            # observation space
            obs_dim = len(world.observation(agent))
            self.observation_space.append(spaces.Box(low=-np.inf, high=+np.inf, shape=(obs_dim,), dtype=np.float32))


    def step(self, action_n):
        obs_n = []
        reward_n = []
        self.world.step(action_n)

        benchmark_world = copy.deepcopy(self.world)
        action_n_benchmark = np.zeros((self.n, self.world.dim_action))
        action_n_benchmark[:,-1] = 1
        benchmark_world.step(action_n_benchmark)

        reward = benchmark_world.get_cost() - self.world.get_cost()
        # reward = -self.world.get_cost()

        for agent in self.agents:
            obs_n.append(self.world.observation(agent))

        done_n = [self.world.if_done()] * self.n
        if self.shared_reward:
            reward_n = [reward] * self.n

        return obs_n, reward_n, done_n

    def reset(self):
        # reset world
        self.world.reset_world()
        # record observations for each agent
        obs_n = []
        for agent in self.agents:
            obs_n.append(self.world.observation(agent))
        return obs_n



