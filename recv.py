
# coding: utf-8

# In[1]:

import h5py
import socket
import capnp
import sys
import struct
from os.path import exists
import json


# In[2]:

get_ipython().system(u'cat /home/franziska/Documents/Master/sem1/argos-zero/src/capnp/CapnpGame.capnp')


# In[3]:

def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = b''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return data
        data += packet
    return data


# In[ ]:

class GameLogger:
    def __init__(self, port, schema_path, filename, limited_state_num=1000000):
        self.limited_state_num = limited_state_num
        
        self._init_socket(port)
        self._init_proto(schema_path)
        
        self.file_i = 1
        self.filename = filename
        self._init_h5()
        
    def _init_socket(self, port):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #self.server.bind((socket.gethostname(), PORT))
        self.server.bind(("localhost", PORT))
        self.server.listen(50)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
    def _init_proto(self, schema_path):
        self.schema = capnp.load(schema_path)
        
    def _init_h5(self):
        file = "%s-%.4d.h5" % (self.filename, self.file_i)
        self.f = h5py.File(file, 'a')
        
        # define explicit unicode vlen type
        dt = h5py.special_dtype(vlen=str)
        
        if "json_game_record" not in self.f.keys():
            gamerecord_dataset = self.f.create_dataset('json_game_record', (self.limited_state_num,) ,dtype=dt)
            gamerecord_dataset.attrs['count_id'] = 0
        
    def _write_h5(self, msg):
        gamerecord_dataset = self.f["json_game_record"]
        
        # convert to msg to game infos
        game_infos = self.schema.Game.from_bytes(msg)
        
        game_dict = game_infos.to_dict()
        
        # iteratively take stateprob from stateprobs
        for stateprob in game_infos.stateprobs:
            gamerecord_dataset = self.f["json_game_record"]
            game_dict['stateprobs'] = stateprob.to_dict()
            current_id = gamerecord_dataset.attrs['count_id']
            gamerecord_dataset[current_id] = json.dumps(game_dict)
            gamerecord_dataset.attrs['count_id'] += 1
            
            if self.limited_state_num <= gamerecord_dataset.attrs['count_id']:
                self.file_i += 1
                self.f.close()
                self._init_h5()
        
    def listen(self):
        try:
            sys.stdout.flush()

            while True:
                client, address = self.server.accept()
                msg = recvall(client, 4096 * 100)
                #print(len(msg))
                #file = open("testfile",'wb') 
                #file.write(msg) 
                #file.close()
                #print("file written")
                #game =self.schema.Game.read(file)
 
                game = self.schema.Game.from_bytes(msg)
                #print(self.schema.Game.from_bytes(msg))

                self._write_h5(msg)
                print(self.f["json_game_record"].attrs['count_id'])
                
        finally:
            self.server.close()
            self.f.close()
            print("closed")


# In[ ]:

PORT = 8006
logger = GameLogger(port=PORT, schema_path='/home/franziska/Documents/Master/sem1/argos-zero/src/capnp/CapnpGame.capnp', filename='game_record')
logger.listen()


# In[ ]:

msg.to_dict()


# In[ ]:

logger.server.close()


# ## HDF5

# In[ ]:

f = h5py.File("game_record.h5")


# In[ ]:

f["json_game_record"][0]


# In[ ]:

# todo
# new file if old one is full


#done: 
# append multiple games: check
# take multiple connections: queue
# open file correctly also if already exists

# remote connection(ben!)


# In[ ]:

f = h5py.File("game_record1.h5", 'a')


# In[ ]:



