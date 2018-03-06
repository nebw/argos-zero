#include "Collector.h"

Collector::Collector(string server, int port){
    //initialize all the variables
    _server = server;
    _port = port;

}

void Collector::collectMove(const Tree::Tree& tree){

}

void Collector::sendData(){
    int sockfd;
    // get the Socket fd for server at port
    sockfd = connectToServer(this->_server, this->_port);

    // serialize collected info into capnp Message

    //send
}
