import h5py
import os
import sys
import uuid
from datetime import datetime
import AGZtraining
sys.path.append('./Model Evaluation')
from matchsystem import MatchSystem


# initialisation
hd5_folder_path = "./testhd5"

server_path = '/var/www/html/argos/'

path_to_gtp = '/home/argos/build/argos/src/gtp'

file_used_for_last_training = "game_record-0000.h5"
threshold = 25000 #games that we need to trigger training


def new_is_better(old_network, new_network, numMatches=400):
    match = MatchSystem(playerOne = old_network, playerTwo = new_network)
    match_result = match.runMatches(numMatches)
    return match_result['playerOne'] <= sum(match_result.items())*0.45

# searches for all hdf5 files in the path
file_list = []
for file_name in os.listdir(hd5_folder_path):
    if file_name.endswith(".h5"):
        file_list.append(file_name)

# sort file list according to contained number in filename
file_list.sort()


while True:
    num_of_games = 0
    # the last file is still in progress, new games are appended.
    # we therefore use the before-last game
    for file_name in file_list[-2::-1]:
        if file_name == file_used_for_last_training:
            break
        f = h5py.File(hd5_folder_path+file_name)
        num = f["game_record"].attrs["count_id"]
        num_of_games += num

    if num_of_games > threshold:

        # training returns a uuid
        new_network = AGZtraining.train(export_path = server_path)

        file_used_for_last_training = file_list[-2]

        # collect uuid of the current best network
        uuid_old_network = open(server_path + 'best-weights', 'r')
        old_network_with_threshold = uuid_old_network.readline()
        old_network_with_threshold.split(';')
        old_network = old_network_with_threshold[0]
        uuid_old_network.close()

        # the inputs for function new_is_better are two lists containing 
        # path to gtp, uuid of network and the engine total time
        old_network_params = [path_to_gtp, old_network, '5']
        new_network_params = [path_to_gtp, new_network, '5']
        
        if new_is_better(old_network_params, new_network_params):
            # change the uuid in the best-weights-file
            best_weights = open(server_path + 'best-weights', 'w')
            best_weights.write(new_network + ';' + resign_threshold)
            best_weights.close()

    else:
        # in order not to have it running the whole time
        time.sleep(30)
