// Copyright 2014 asarcar Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//
#include <cstring>  // memcpy
#include <iostream>
#include <sstream>  // ostringstream
#include <thread>   // sleep_for    
#include <zmq.hpp>

using namespace std;

constexpr int msglen = 16;

int main () {
  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind ("tcp://*:5555");

  for(int i=0; true; ++i) {
    zmq::message_t request;

    //  Wait for next request from client
    socket.recv (&request);
    cout << "CLT: Received: " 
         << static_cast<char *>(request.data()) << endl;

    //  Do some 'work'
    this_thread::sleep_for(chrono::seconds{1});

    //  Send reply back to client
    zmq::message_t reply (msglen);
    ostringstream oss;
    oss << "World " << i;
    memcpy ((void *) reply.data(), oss.str().data(), oss.str().size());
    cout << "CLT: Sending: " 
         << static_cast<char *>(reply.data()) << endl;
    socket.send (reply);
  }
  return 0;
}
