#include "Collector.h" //copied from selfplay
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Config.h"
#include "TimeControl.h"
#include "Tree.h"
#include "Util.h"
#include "Collector.h" //copy end

// TCP socket network:
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//capnp
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "../capnp/CapnpGame.h"

#include <ctime>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>


Collector::Collector(string server, int port){
    //initialize all the variables
    _server = server;
    _port = port;

    //    int sockfd, portno, n;
    //    struct sockaddr_in serv_addr;
    //    string server_name;

    // initialize capnp message if data should be collected
    std::vector<std::array<double, 362>> _probabilities;
    std::vector<NetworkFeatures::Planes> _states;

}

void Collector::collectMove(const Tree& tree){

    std::array<double, 362> node_probs; // an allen Stellen mit 0 initialisieren
    std::int16_t pos, row, col, board_size; //where in the array to write the probability

    // iterate over all node. only legal moves do exist.
    for (auto node : tree.rootNode()->children().get())
    {
        //is vertex a pass
        if(node->parentMove().GetRaw() == Vertex::Pass().GetRaw()){
            pos = 361;
        }else{
            row = node->parentMove().GetRow();
            col = node->parentMove().GetColumn();
            board_size = BOARDSIZE;

            pos = row * board_size + col;
        }

        // write the statistics to the array
        node_probs[pos] = node->statistics().num_evaluations.load();


        // normalize visits by dividing by the sum of all
        // get the sum
        int a, sum = 0;
        for (a=0; a<362; a++)
            {
                sum+=node_probs[a];
            }

        // now divide
        for (a=0; a<362; a++)
            {
                node_probs[a] = node_probs[a]/sum;
            }


    }
    // probabilities
    _probabilities.push_back(node_probs);

    // states from the board
    _states.push_back(tree.rootBoard().getFeatures().getPlanes());
}

void Collector::sendData(const Tree& tree){
    int sockfd;
    // get the Socket fd for server at port
    sockfd = connectToServer(this->_server, this->_port);

    // serialize collected info into capnp Message
    std::uint16_t a, b, i, j, k;
    std::uint16_t num_moves = _states.size();

    std::uint16_t num_flattened_features = board_size * board_size * 8; // try to replace hardcoding of 8!

    ::capnp::MallocMessageBuilder message;
    Game::Builder game = message.initRoot<Game>();
    ::capnp::List<StateProb>::Builder stateprobs = game.initStateprobs(num_moves);


    // fill the StateProb structs with the information from the vectors
    for (a=0;a<num_moves;a++){
        StateProb:: Builder stateprob = stateprobs[a];
        stateprob.setIdx(a);

        // fill probs
        ::capnp::List<float>::Builder probs =  stateprob.initProbs(362);
        for (b=0;b<362;b++){
            probs.set(b, _probabilities[a][b]);
        }

        //fill states
        ::capnp::List<float>::Builder cstates =  stateprob.initState(num_flattened_features);
        for (i=0;i<board_size;i++){
            for (j=0;j<board_size;j++){
                for (k=0;k<8;k++){
                    int ind = board_size * 8 * i + 8 * j + k;
                    cstates.set(ind, _states[a][i][j][k]);
                }
            }
        }
    }

    // now fill the game
    boost::uuids::uuid id(boost::uuids::random_generator());
    game.setId(boost::lexical_cast<std::string>(id));

    std::time_t time = std::time(nullptr);
    game.setTimestamp(time);

    game.setBoardsize(board_size);

    game.setNetwork1(0); // to be initialized with the correct network idea
    game.setNetwork2(0);

    int result = tree.rootBoard().PlayoutWinner().ToScore(); // ToScore (Black()) == 1, ToScore (White()) == -1
    int binarized_result = (result + 1)/2;
    bool res = binarized_result;
    game.setResult(res);



    // open TCP socket
//        sockfd = socket(AF_INET, SOCK_STREAM, 0);
//        if (sockfd < 0)
//                error("ERROR opening socket");

//        server = gethostbyname(server_name);

//        if (server == NULL) {
//                fprintf(stderr,"ERROR, no such host\n");
//                exit(0);
//            }
//        serv_addr.sin_family = AF_INET;

//        bcopy((char *)server->h_addr,
//                 (char *)&serv_addr.sin_addr.s_addr,
//                 server->h_length);

//        serv_addr.sin_port = htons(portno);

//        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
//                error("ERROR connecting");

//        // write capnp message to socket
//        writePackedMessageToFd(sockfd, message);

//        //close socket
//        close(sockfd);

    //send
}