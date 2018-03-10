import capnp
import h5py
import os
import sys
import uuid
import time
from datetime import datetime
import AGZtraining
sys.path.append('./Model_Evaluation')
from matchsystem import MatchSystem
sys.path.append('./Dynamic_Thresholding')
from dynamic_thresholding import get_games_from_hdf5, compute_threshold

# initialisation
hd5_folder_path = "/home/argos/argos-zero/raw_data/"

server_path = '/var/www/html/argos/'

path_to_gtp = ['docker', 'exec', '-i', 'peaceful_noether', '/root/argos-build/src/gtp']

dataset_path = '/home/argos/argos-zero/train_val.h5'

file_used_for_last_training = "game_record-0000.h5"
threshold = 100 #games that we need to trigger training

path_to_schema = "/home/argos/argos-zero/src/capnp/CapnpGame.capnp"
schema = capnp.load(path_to_schema).Game

def new_is_better(old_network, new_network, numMatches=400):
    return True # TODO: FIX
    match = MatchSystem(firstPlayer = old_network, secondPlayer = new_network)
    match_result = match.runMatches(numMatches)
    return match_result['playerOne'] <= sum(match_result.items()) * 0.45


while True:
    # searches for all hdf5 files in the path
    file_list = []
    for file_name in os.listdir(hd5_folder_path):
        if file_name.endswith(".h5"):
            file_list.append(file_name)

    # sort file list according to contained number in filename
    file_list.sort()

    num_of_games = 0
    training_list = []
    # the last file is still in progress, new games are appended.
    # we therefore use the before-last game
    for file_name in file_list[-2::-1]:
        if file_name == file_used_for_last_training:
            break
        path = hd5_folder_path + file_name
        f = h5py.File(path)
        num = f["game_record"].attrs["count_id"]
        num_of_games += num
        training_list.append(path)

    if num_of_games > threshold:
        # training returns a uuid
        new_network = AGZtraining.train(server_path, training_list, dataset_path,
                boardsize=9, val_prob=10, num_states=100000)

        file_used_for_last_training = file_list[-2]

        # collect uuid of the current best network
        uuid_old_network = open(server_path + 'best-weights', 'r')
        old_network_with_threshold = uuid_old_network.readline()
        old_network = old_network_with_threshold.split('; ')[0]
        uuid_old_network.close()

        # the inputs for function new_is_better are two lists containing 
        # path to gtp, uuid of network and the engine total time
        old_network_params = path_to_gtp + [server_path + old_network]
        new_network_params = path_to_gtp + [server_path + new_network]
        
        if new_is_better(old_network_params, new_network_params):
            # compute the resign threshold
            games = get_games_from_hdf5(training_list, schema)
            resign_threshold = compute_threshold(games, 0.05)
            print('Resign threshold: ', resign_threshold)
            
            # change the uuid in the best-weights-file
            best_weights = open(server_path + 'best-weights', 'w')
            best_weights.write(new_network + '; ' + str(resign_threshold))
            best_weights.close()
            print('New best network: ', new_network)

    else:
        # in order not to have it running the whole time
        time.sleep(30)
