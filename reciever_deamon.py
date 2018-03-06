
# coding: utf-8

# In[1]:

import h5py
import socket
import capnp
import sys
import struct
from os.path import exists
import json


get_ipython().system('cat ./src/capnp/CapnpGame.capnp')


def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = b''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return data
        data += packet
    return data


def show_hd5_content():
    f = h5py.File("game_record.h5")
    print(f["json_game_record"][0])


class GameLogger:
    def __init__(self, port, schema_path, filename):
        self._init_socket(port)
        self._init_proto(schema_path)
        self._init_h5(filename)
        
    def _init_socket(self, port):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #self.server.bind((socket.gethostname(), PORT))
        self.server.bind(("localhost", PORT))
        self.server.listen(50)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
    def _init_proto(self, schema_path):
        self.schema = capnp.load(schema_path)
        
    def _init_h5(self, filename):
        file = filename + '.h5'
        
        if exists(file):
            self.f = h5py.File(file, 'a')
        else:
            self.f = h5py.File(file, 'a')
        
    def _write_h5(self, msg):
        limited_state_num = 1000

        # define explicit unicode vlen type
        dt = h5py.special_dtype(vlen=str)

        # create gamerecord dataset with name "json_game_record"
        if "json_game_record" in self.f.keys():
            gamerecord_dataset = self.f["json_game_record"]
        else:
            gamerecord_dataset = self.f.create_dataset('json_game_record', (limited_state_num,) ,dtype=dt)
            gamerecord_dataset.attrs['count_id'] = 0
        
        
        # convert to msg to game infos
        game_infos = self.schema.Game.from_bytes(msg)
        
        game_dict = game_infos.to_dict()
        
        # iteratively take stateprob from stateprobs
        for stateprob in game_infos.stateprobs:
            game_dict['stateprobs'] = stateprob.to_dict()
            current_id = gamerecord_dataset.attrs['count_id']
            gamerecord_dataset[current_id] = json.dumps(game_dict)
            gamerecord_dataset.attrs['count_id'] += 1
            
            # if limited_state_num == current_id:
                
                
    def listen(self):
        try:
            sys.stdout.flush()
            

            while True:
                client, address = self.server.accept()
                msg = recvall(client, 4096 * 100)
                # print(len(msg))
                # file = open("testfile",'wb') 
                # file.write(msg) 
                # file.close()
                # print("file written")
                # game =self.schema.Game.read(file)
 
                game = self.schema.Game.from_bytes(msg)
                # print(self.schema.Game.from_bytes(msg))

                self._write_h5(msg)
                # print(self.f["json_game_record"].attrs['count_id'])
                
        finally:
            self.server.close()
            self.f.close()
            print("closed")

# Main function
PORT = 8000
logger = GameLogger(port=PORT, schema_path='./src/capnp/CapnpGame.capnp', filename = 'game_record')
logger.listen()




