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
#include <cstring>
#include <iostream>
#include <sstream>  // ostringstream
#include <zmq.hpp>

using namespace std;

constexpr int msglen = 16;

int main (void)
{
  cout << "Connecting to hello world server…" << endl;
  void *context = zmq_ctx_new ();
  void *requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect (requester, "tcp://localhost:5555");

  int request_nbr;
  for (request_nbr = 0; request_nbr != 10; ++request_nbr) {
    char buffer [msglen];
    ostringstream oss;
    oss << "Hello " << request_nbr;
    cout << "SVR: Sending: " << oss.str() << endl;
    zmq_send (requester, oss.str().data(), oss.str().size(), 0);
    zmq_recv (requester, buffer, sizeof(buffer), 0);
    cout << "SVR: Received: " << buffer << endl;
  }
  zmq_close (requester);
  zmq_ctx_destroy (context);
  return 0;
}
