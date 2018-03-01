#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// TCP socket network:
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "argos/Config.h"
#include "argos/TimeControl.h"
#include "argos/Tree.h"
#include "argos/Util.h"

//capnp
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "capnp/Message.h"

int main() {

    // if network variable set: collect information
    // if yes, where to send them (IP Adress:port)
    bool collect_data = 0;

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    string server_name;

    Tree tree;
    tree.setKomi(7.5);
    std::cout << tree.rootBoard().ToAsciiArt() << std::endl;

    NatMap<Player, BasicTimeControl> tc;
    tc[Player::Black()] = BasicTimeControl(config::engine::totalTime);
    tc[Player::White()] = BasicTimeControl(config::engine::totalTime);


    // initialize capnp message if data should be collected
    std::vector<std::array<double, 362>> probabilities;
    std::vector<NetworkFeatures::Planes> states;


    while ((!tree.rootBoard().BothPlayerPass())) {
        const Player actPl = tree.rootBoard().ActPlayer();
        const auto time = tc[actPl].getTimeForMove(tree.rootBoard());
        std::cout << "Remaining time: " << format_duration(tc[actPl].getRemainingTime())
                  << std::endl;

        std::cout << "Evaluating in " << format_duration(time) << std::endl;
        tc[actPl].timedAction([&]() { tree.evaluate(time); });

        std::cout << "Best move: " << tree.bestMove().ToGtpString() << std::endl;
        const auto winrate = tree.rootNode()->winrate(tree.rootBoard().ActPlayer());
        if (winrate < .1f) {
            std::cout << tree.rootBoard().ActPlayer().ToGtpString() << " resigns." << std::endl;
            break;
        }
        const Vertex move = tree.bestMove();

        printTree(tree.rootNode().get(), tree.rootBoard().ActPlayer());

        // fill capnp struct statprob for the move
        // TODO

        if (collect_data){

            std::array<double, 362> node_probs; // an allen Stellen mit 0 initialisieren
            std::int8_t pos, row, col, board_size; //where in the array to write the probability

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
            probabilities.push_back(node_probs);

            // states from the board
            states.push_back(tree.rootBoard().getFeatures().getPlanes());
        }


        tree.playMove(move);

        std::cout << tree.rootBoard().ToAsciiArt(move) << std::endl;
    }

    // convert what we collected to capnp messages
    // TODO

    // only possible here and not before because we do not know the number of moves in advance

    if (collect_data){

        //num_moves = len(x);

        ::capnp::MallocMessageBuilder message;
        Game::Builder game = message.initRoot<Game>(2);




        // fill the game struct with all information collected
        // open TCP socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                error("ERROR opening socket");

        server = gethostbyname(server_name);

        if (server == NULL) {
                fprintf(stderr,"ERROR, no such host\n");
                exit(0);
            }
        serv_addr.sin_family = AF_INET;

        bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);

        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
                error("ERROR connecting");

        // write capnp message to socket
        writePackedMessageToFd(sockfd, message);

        //close socket
        close(sockfd);
    }

}
