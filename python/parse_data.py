import h5py
import numpy as np
import capnp
import sys
import os.path
import glob

sys.path.append('./../src/capnp')
import CapnpGame_capnp


def write_in_dataset(dataset, raw_data_folder, boardsize=9, val_prob=10,
    force_full_write=True):
    """
    TODO: make code nice and readable
    val_prob: every val_probth game will be chosen for validation
    force_full_write: True will make sure that everything will be overwritten
    once by duplicating some datapoints
    """
    train_x = dataset['train_x']
    train_y = dataset['train_y']
    val_x = dataset['val_x']
    val_y = dataset['val_y']
    print(sorted(glob.glob(raw_data_folder)))
    paths = sorted(glob.glob(raw_data_folder))[:-1]
    paths.reverse()
    print("Writing data from ", paths)

    total_written=[0,0]


    #latest_id_read = dataset.attrs["latest_id_read"]
    #
    #f = h5py.File(paths[0],'r')
    #game_msg = f['game_record'][f['game_record'].attrs['count_id']-1].tostring()
    #g = CapnpGame_capnp.Game.from_bytes(game_msg)
    #dataset.attrs["latest_id_read"] = g.id
    #f.close()
    while True:
        for raw_data_path in paths:
            print(raw_data_path)
            raw_data = h5py.File(raw_data_path,'r')
            for i in range(raw_data['game_record'].attrs['count_id']-1,-1,-1):
                game_msg = raw_data['game_record'][i].tostring()
                g = CapnpGame_capnp.Game.from_bytes(game_msg)
                #print(g.id)
                #if g.id == latest_id_read:
                #    # found the first entry that is already in the dataset
                #    raw_data.close()
                #    return

                if total_written[0] >= train_x.shape[0] and total_written[1] >= val_x.shape[0]:
                    raw_data.close()
                    return

                if i % val_prob != 0: #: #np.random.sample(1)[0] > val_prob
                    dest_x, dest_y, j = train_x, train_y, 0
                else:
                    dest_x, dest_y, j = val_x, val_y, 1

                for i in range(len(g.stateprobs)):
                    # copy all states (if there is space) from the game
                    if total_written[j] > dest_x.shape[0]-1:
                        #print("total_written exeeded dest.shape")
                        break
                    offset = total_written[j]
                    #offset = dest_x.attrs["next_i_to_overwrite"]
                    dest_x[offset] = np.array(g.stateprobs[i].state).reshape((12 , boardsize, boardsize))

                    probs = np.array(g.stateprobs[i].probs)
                    winner = np.asarray([int(g.result)])

                    y = np.concatenate((probs.flatten(), winner), axis=0)

                    dest_y[offset] = y.reshape((1,boardsize*boardsize+2))

                    #dest_x.attrs.modify("next_i_to_overwrite",(offset+1)%dest_x.shape[0])

                    total_written[j] += 1


            # save to disk after every game(?)
            dataset.flush()
            raw_data.close()

        if not force_full_write:
            break

    return

def update_dataset(raw_data_folder, dataset_path, boardsize=9, val_prob=10,
                    num_states=125000):
    """
    raw_data_folder: path to raw_data with .h5 file with cpnp messages
    dataset_path: path to dataset
    boardsize: _
    val_prob: every val_probth game will be chosen for validation
    num_states: if dataset file has to be created, states the new dataset file can
    hold"""
    if os.path.isfile(dataset_path):
        dataset = h5py.File(dataset_path, 'r+', libver='latest')
        write_in_dataset(dataset, raw_data_folder, val_prob=val_prob,
            boardsize=boardsize, force_full_write=True)
    else:
        print("dataset not found at specified path, creating new dataset")
        val_prob = (1/val_prob)
        # create file for dataset
        dataset = h5py.File(dataset_path, 'w', libver='latest')

        #dataset.swmr_mode = True # not sure if we want that
        train_x = dataset.create_dataset("train_x", shape=(
            int(num_states*(1-val_prob)),12,boardsize,boardsize), dtype='int8')
        train_y = dataset.create_dataset("train_y", shape=(
            int(num_states*(1-val_prob)),boardsize*boardsize+2))
        val_x = dataset.create_dataset("val_x", shape=(
            int(num_states*val_prob),12,boardsize,boardsize), dtype='int8')
        val_y = dataset.create_dataset("val_y", shape=(
            int(num_states*val_prob),boardsize*boardsize+2))

        # # save where to (over-)write next in dataset
        # train_x.attrs.create("next_i_to_overwrite",0)
        # val_x.attrs.create("next_i_to_overwrite",0)
        #
        # # save the newest file that was read from
        # dataset.attrs.create("latest_p_read", b"dummy")
        # # save the index of newest entry that was copied
        # dataset.attrs.create("latest_i_read", 0)

        dataset.flush()
        print("created dataset")
        write_in_dataset(dataset, raw_data_folder, val_prob=val_prob,
            boardsize=boardsize, force_full_write=True)
    dataset.close()
    print("Dataset updated")
