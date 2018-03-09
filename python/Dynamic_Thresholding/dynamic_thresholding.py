
# coding: utf-8

# In[1]:


import h5py
import capnp
import os
import numpy as np


# In[2]:


path_to_schema = "../../src/capnp/CapnpGame.capnp"
path_to_game_folder = "/Users/florian/Desktop/games2/"



# In[4]:


def get_games_from_hdf5(list_of_files, schema):
    """ list_of_files: absolute path of hdf5 files"""
    games = []

    for file in list_of_files:
        if file.endswith(".h5"):
            f = h5py.File(file, "r")
            dataset = f["game_record"]

            for i in range(dataset.attrs["count_id"]):
                game = schema.from_bytes(dataset[i].tostring())
                games.append(game)
                
    return games

# In[7]:


def get_games_without_res(games):
    """filter games such that we only have games with no resignation"""
    return [game for game in games if game.noresignmode]


# In[30]:


def get_winrates_for_player(games, player):
    """Returns the winrates for only the given player"""
    winrate_lists = [[sp.winrate for sp in game.stateprobs] for game in games]    
    filtered_winrates_list = []
    
    for winrates in winrate_lists:
        filtered_winrates = [winrate for i, winrate in enumerate(winrates) if i % 2 == player]
        filtered_winrates_list.append(filtered_winrates)
        
    return filtered_winrates_list


# In[31]:


def get_games_won_by_player(games, player):
    return [game for game in games if game.result == player]


# In[32]:


def get_winrates_for_games_won_by_player(games, player):
    """Get the winrates only for this player in the games won by the player"""
    games_won_by_player = get_games_won_by_player(games, player)
    only_player_winrates = get_winrates_for_player(games_won_by_player, player)
    
    return only_player_winrates


# In[35]:


def find_fn_count(threshold, winrate_lists):
    """
    threshold: float, below what win rate should the player resign?
    winrates: list of lists of floats, win rates of player in several games
    """
    return sum([any([1 - winrate <= threshold for winrate in winrates]) for winrates in winrate_lists])


# In[64]:


def compute_threshold(games, max_fn_rate):
    #games = get_games_without_res(games)
    black = get_winrates_for_games_won_by_player(games, 1)
    white = get_winrates_for_games_won_by_player(games, 0)
    dataset = black + white

    thresholds = np.linspace(0, 0.25, num=250)
        
    results = np.array([find_fn_count(threshold, dataset) for threshold in thresholds])
    results = results / len(dataset)

    
    best_i = np.where(results <= max_fn_rate)[0][-1]
    return thresholds[best_i]


# In[ ]:

# path_to_schema = "../../src/capnp/CapnpGame.capnp"
# path_to_game_folder = "/Users/florian/Desktop/games2/"
# 
# schema = capnp.load(path_to_schema).Game
# games = get_games_from_hdf5(training_list, schema)
# 
# compute_threshold(games, 0.05)
# 
