import h5py
import numpy as np
import capnp
import sys
import os.path
import glob

sys.path.append('./../src/capnp')
import CapnpGame_capnp


def write_in_dataset(dataset, raw_data_folder, boardsize=9, val_prob=0.05,
    force_full_write=False,total_written=[0,0]):
    """
    TODO: make code nice and readable
    val_prob: probability of a specific game is chosen for the validation set
    force_full_write: True will make sure that there will be no empty entries
    in the dataset by duplicating some datapoints
    total_written: states written in [train_set,val_set]

    """
    train_x = dataset['train_x']
    train_y = dataset['train_y']
    val_x = dataset['val_x']
    val_y = dataset['val_y']

    paths = glob.glob(raw_data_folder)[:-1]
    paths.reverse()

    latest_p_read = dataset.attrs["latest_p_read"]
    latest_i_read = dataset.attrs["latest_i_read"]

    dataset.attrs["latest_p_read"] = paths[0]
    f = h5py.File(paths[0],'r')
    dataset.attrs["latest_i_read"] = f['game_record'].attrs['count_id']-1
    f.close()

    for raw_data_path in paths:
        raw_data = h5py.File(raw_data_path,'r')
        for i in range(raw_data['game_record'].attrs['count_id']-1,-1,-1):
            if i == latest_i_read and raw_data_path == latest_p_read:
                print("found the first entry that is already in the dataset")
                raw_data.close()
                return
            print(i)
            game_msg = raw_data['game_record'][i].tostring()
            g = CapnpGame_capnp.Game.from_bytes(game_msg)

            if np.random.sample(1)[0] > val_prob and total_written[0] < train_x.shape[0]:
                dest_x, dest_y, j = train_x, train_y, 0
            elif total_written[1] < val_x.shape[0]:
                dest_x, dest_y, j = val_x, val_y, 1
            else:
                raw_data.close()
                return

            for i in range(len(g.stateprobs)):
                offset = dest_x.attrs["next_i_to_overwrite"]
                dest_x[offset] = np.array(g.stateprobs[i].state).reshape(
                                                    (12, boardsize, boardsize))

                probs = np.array(g.stateprobs[i].probs)
                winner = np.asarray([int(g.result)])

                y = np.concatenate((probs.flatten(), winner), axis=0)

                dest_y[offset] = y.reshape((1,83))

                dest_x.attrs.modify("next_i_to_overwrite",
                    (offset+1)%dest_x.shape[0])

                total_written[j] += 1
                if total_written[j] > dest_x.shape[0]:
                    break

            # save to disk after every iteration(?)
            dataset.flush()
        raw_data.close()
    if force_full_write:
        write_in_dataset(dataset, raw_data_folder, force_full_write=True,
            total_written=total_written)
    return

def update_dataset(raw_data_folder, dataset_path, boardsize=9, val_prob=0.05,
                    samples=25000):
    """
    raw_data_folder: path to raw_data with .h5 file with cpnp messages
    dataset_path: path to dataset
    boardsize: _
    val_prob: probability of a specific game is chosen for the validation set
    samples: if dataset file has to be created, states the new dataset file can
    hold"""
    if os.path.isfile(dataset_path):
        dataset = h5py.File(dataset_path, 'r+', libver='latest')
        write_in_dataset(dataset, raw_data_folder)
    else:
        print("dataset not found at specified path, creating new dataset")
        # create file for dataset
        dataset = h5py.File(dataset_path, 'w', libver='latest')

        #dataset.swmr_mode = True # not sure if we want that
        train_x = dataset.create_dataset("train_x", shape=(
            int(samples*(1-val_prob)),12,boardsize,boardsize), dtype='int8')
        train_y = dataset.create_dataset("train_y", shape=(
            int(samples*(1-val_prob)),83))
        val_x = dataset.create_dataset("val_x", shape=(
            int(samples*val_prob),12,boardsize,boardsize), dtype='int8')
        val_y = dataset.create_dataset("val_y", shape=(
            int(samples*val_prob),83))

        # save where to (over-)write next in dataset
        train_x.attrs.create("next_i_to_overwrite",0)
        val_x.attrs.create("next_i_to_overwrite",0)

        # save the newest file that was read from
        dataset.attrs.create("latest_p_read", b"dummy")
        # save the index of newest entry that was copied
        dataset.attrs.create("latest_i_read", 0)
        dataset.flush()
        print("created dataset")
        write_in_dataset(dataset, raw_data_folder, force_full_write=True)
    dataset.close()
    print("Dataset updated")
