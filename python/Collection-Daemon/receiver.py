import h5py
import socket
import capnp
import sys
import struct
from os.path import exists
import numpy as np


def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = b''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return data
        data += packet
    return data


class GameLogger:
    def __init__(self, port, filename, limited_game_num=1000):
        self.count_key = "count_id"
        self.dataset_key = "game_record"
        self.limited_game_num = limited_game_num
        
        self._init_socket(port)
        
        self.file_i = 1
        self.filename = filename
        self._init_h5()

    def _init_socket(self, port):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.bind(("localhost", PORT))
        self.server.listen(50)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
    def _init_h5(self):
        file = "%s-%.4d.h5" % (self.filename, self.file_i)
        self.f = h5py.File(file, "a")
                
        if self.dataset_key not in self.f.keys():
             # find the datatype of the np.void(msg) for creating dataset in the next step
            dt = np.dtype("V195880")
            # create dataset according to the datasetkey and set limit for num of games/ num of records
            gamerecord_dataset = self.f.create_dataset(self.dataset_key, (self.limited_game_num,) ,dtype=dt)
            # initialize the attribute count_id
            gamerecord_dataset.attrs[self.count_key] = 0
        
    def _write_h5(self, msg):                
        gamerecord_dataset = self.f[self.dataset_key]
        
        # the capnp msg will be directly stored in hdf5
        current_id = gamerecord_dataset.attrs[self.count_key]
        gamerecord_dataset[current_id] = msg
        gamerecord_dataset.attrs[self.count_key] += 1

        self.f.flush()
        
        if self.limited_game_num <= gamerecord_dataset.attrs[self.count_key]:
            self.file_i += 1
            self.f.close()
            self._init_h5()
        
    def listen(self):
        try:
            sys.stdout.flush()
            
            while True:
                client, address = self.server.accept()
                msg = recvall(client, 4096 * 100)
                print('Message from {}'.format(address))
                self._write_h5(msg)
                
        finally:
            self.server.close()
            self.f.close()
            print("closed")


PORT = 18000
logger = GameLogger(port=PORT, filename='game_record')
logger.listen()
