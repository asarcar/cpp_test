// Copyright 2016 asarcar Inc.
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

/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Standard C++ Headers
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
#include <grpc/grpc.h>
#include <grpc/support/log.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
// Local Headers
#include "experiment_test/grpc_test/db/db_read_json.h"
#include "experiment_test/grpc_test/protos/route_guide.grpc.pb.h"
#include "utils/basic/basictypes.h"
#include "utils/basic/clock.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::CompletionQueue;
using grpc::Status;

using asarcar::routeguide::Point;
using asarcar::routeguide::Feature;
using asarcar::routeguide::Rectangle;
using asarcar::routeguide::RouteSummary;
using asarcar::routeguide::RouteNote;
using asarcar::routeguide::RouteGuide;

using namespace std;
using namespace asarcar;
using namespace asarcar::routeguide;

Point MakePoint(long latitude, long longitude) {
  Point p;
  p.set_latitude(latitude);
  p.set_longitude(longitude);
  return p;
}

Feature MakeFeature(const std::string& name,
                    long latitude, long longitude) {
  Feature f;
  f.set_name(name);
  f.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return f;
}

RouteNote MakeRouteNote(const std::string& message,
                        long latitude, long longitude) {
  RouteNote n;
  n.set_message(message);
  n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return n;
}

// Pure Virtual Class used to process RPC replies
struct RxRPC {
  explicit RxRPC(const string& rpc_name): name{rpc_name} {}
  virtual ~RxRPC() {}
  virtual bool ProcessReply(bool over) = 0;
  virtual void ProcessFinish() {};
  const string name;
};

static constexpr float kCoordFactor = 10000000.0;

struct RxRPCFinish : public RxRPC {
  explicit RxRPCFinish(const string& rpc_name, RxRPC *p): 
      RxRPC{rpc_name + "-Finish"}, par_p{p}{}
  bool ProcessReply(bool over) override {
    DCHECK(status.ok()) << name << " RPC failed"
                        << ": code=" << status.error_code()
                        << ": message=" << status.error_message();
    par_p->ProcessFinish();
    delete par_p; // also free RxRPC structs embeddeded in parent
    return true;
  }
  RxRPC             *par_p;
  Status            status;
};

struct RxRPCGetFeature : public RxRPC {
  RxRPCGetFeature(RouteGuide::Stub* stub_p, 
                  CompletionQueue *cq_p,
                  const Point& point) :
      RxRPC{"GetFeature"}, 
    rpc{stub_p->AsyncGetFeature(&context, point, cq_p)} {
    rpc->Finish(&feature, &status, this);
  }
  bool ProcessReply(bool over) override {
    DCHECK(status.ok()) << "GetFeature RPC failed"
                        << ": code=" << status.error_code()
                        << ": message=" << status.error_message();
    DLOG(INFO) << "-------------- GetFeature --------------";
    DCHECK(feature.has_location()) << "Server returns incomplete feature";
    DLOG(INFO) << "Found Feature \"" << feature.name()  << "\" at ("
               << feature.location().latitude()/kCoordFactor << ","
               << feature.location().longitude()/kCoordFactor << ")";
    // Once the reply is fully processed deallocate ourselves
    delete this;
    return true;
  }
  // Per RPC call context: deadline, authentication context, ...
  ClientContext context;
  Feature       feature;
  Status        status;
  // Asynchronous API: hold on to the "rpc" instance to rc updates 
  // on the RPC and not trigger destroying the RPC until reply processed
  unique_ptr<ClientAsyncResponseReader<Feature>> rpc; 
};

struct RxRPCReadFeatures : public RxRPC {
  RxRPCReadFeatures(const vector<Feature> *feature_list_p,
                    RxRPC *rx_rpc_p) : 
      RxRPC{"ReadFeatures"}, num_features_{feature_list_p->size()}, 
      finish_p(new RxRPCFinish{"ListFeatures", rx_rpc_p}) {}
  void InitRead(void) {
    GPR_ASSERT(state_ == State::INVALID);
    state_ = State::READING;
    read_stream->Read(&feature_, this);
  }
  bool ProcessReply(bool over) override {
    GPR_ASSERT(state_ == State::READING);
    if (over) {
      // server closed stream (i.e. call WritesDone)
      read_stream->Finish(&finish_p->status, finish_p.get());
      return false;
    }
    flist_.push_back(feature_);
    read_stream->Read(&feature_, this);
    return false;
  }
  void ProcessFinish() override {
    state_ = State::FINISHED;
    DLOG(INFO) << "ListFeatures ReadStream Finished";
    DLOG(INFO) << "-------------- ListFeatures --------------";
    // for(const Feature& f: flist_) 
    //   DLOG(INFO) << "Found Feature \"" << f.name()  << "\" at ("
    //              << f.location().latitude()/kCoordFactor << ","
    //              << f.location().longitude()/kCoordFactor << ")";
    DLOG(INFO) << "#Features returned with matching criteria " 
               << flist_.size();
  }
  // Asynchronous API: hold on to the "rpc" instance to rc updates 
  // on the RPC and not trigger destroying the RPC until reply processed
  unique_ptr<ClientAsyncReader<Feature>> read_stream;
 private:
  const size_t num_features_;
  enum class State {INVALID, READING, FINISHED};
  State           state_{State::INVALID};
  Feature         feature_;
  vector<Feature> flist_;
  unique_ptr<RxRPCFinish>       finish_p;
};

struct RxRPCListFeatures : public RxRPC {
  RxRPCListFeatures(RouteGuide::Stub* stub_p, 
                    CompletionQueue *cq_p,
                    const Rectangle& rect,
                    const vector<Feature> *flist_p) : 
      RxRPC{"ListFeatures"}
  {
    rx_read_features_p.reset(new RxRPCReadFeatures{flist_p, this});
    rx_read_features_p->read_stream = 
        stub_p->AsyncListFeatures(&context, rect, cq_p, this);
  }
  bool ProcessReply(bool over) override {
    rx_read_features_p->InitRead();
    return false;
  }
  void ProcessFinish() override {
    rx_read_features_p->ProcessFinish();
  }
  unique_ptr<RxRPCReadFeatures> rx_read_features_p{nullptr};
  ClientContext context;
};

struct RxRPCWritePoints : public RxRPC {
  explicit RxRPCWritePoints(const vector<Feature> *flist_p, 
                            RxRPC *rx_rpc_p) : 
      RxRPC{"RecordRoute"}, flist_p_{flist_p}, 
    finish_p(new RxRPCFinish{"RecordRoute", rx_rpc_p}) {}
  void InitWrite() {
    GPR_ASSERT(state_ == State::INVALID);
    state_ = State::WRITING;
    ProcessReply(false);
  }
  bool ProcessReply(bool over) override {
    switch(state_) {
      case State::WRITING:
        const Feature *feature_p;
        size_t  pos;
        // Stop writing when nothing to write or when 
        // kPoints have been successfully written
        if (flist_p_->size() == 0 || cur_idx_ >= kPoints_) {
          state_ = State::WRITING_DONE;
          write_stream->WritesDone(this);
          break;
        }
        pos = IdxArray_[cur_idx_] % flist_p_->size();
        ++cur_idx_;
        feature_p = &flist_p_->at(pos);
        // introduce delay to validate elapsed time
        this_thread::sleep_for(chrono::milliseconds(kMSecs_));
        write_stream->Write(feature_p->location(), this);
        break;
      default:
        // RxRPCRecordRoute::ProcessReply: 
        // never sees INVALID or FINISHED state. 
        GPR_ASSERT(state_ == State::WRITING_DONE);
        state_ = State::FINISHED;
        write_stream->Finish(&finish_p->status, finish_p.get());
        break;
    }
    return false;
  }
  void ProcessFinish() override {
    GPR_ASSERT(state_ == State::FINISHED);
    DLOG(INFO) << "RecordRoute WriteStream Finished";
    DLOG(INFO) << "-------------- RecordRoute --------------";
    DLOG(INFO) << "Finished trip with " 
               << route_summary.point_count() << " points"
               << ": Passed " << route_summary.feature_count() << " features"
               << ": Travelled " << route_summary.distance() << " meters"
               << ": Time Taken " << route_summary.elapsed_time() << " milliseconds";

  }
  RouteSummary  route_summary;
  // Asynchronous API: hold on to the "rpc" instance to rc updates 
  // on the RPC and not trigger destroying the RPC until reply processed
  unique_ptr<ClientAsyncWriter<Point>> write_stream; 
 private:
  enum class State {INVALID, WRITING, WRITING_DONE, FINISHED};
  State            state_{State::INVALID};  
  size_t           cur_idx_{0};
  const vector<Feature>* flist_p_;
  static constexpr Clock::TimeMSecs kMSecs_{10};
  static constexpr size_t kPoints_{5};
  static constexpr array<int, kPoints_> IdxArray_{{2, 3, 5, 7, 11}};          
  unique_ptr<RxRPCFinish>      finish_p;
};

constexpr Clock::TimeMSecs RxRPCWritePoints::kMSecs_;
constexpr size_t RxRPCWritePoints::kPoints_;
constexpr array<int, RxRPCWritePoints::kPoints_> 
RxRPCWritePoints::IdxArray_;

struct RxRPCRecordRoute : public RxRPC {
  RxRPCRecordRoute(RouteGuide::Stub* stub_p, 
                   CompletionQueue *cq_p,
                   const vector<Feature> *flist_p) : 
      RxRPC{"RecordRoute"}
  {
    rx_write_points_p.reset(new RxRPCWritePoints{flist_p, this});
    rx_write_points_p->write_stream = 
        stub_p->AsyncRecordRoute(&context, 
                                 &rx_write_points_p->route_summary, 
                                 cq_p, this);
  }
  bool ProcessReply(bool over) override {
    rx_write_points_p->InitWrite();
    return false;
  }
  void ProcessFinish() override {  
    rx_write_points_p->ProcessFinish();
  }
  unique_ptr<RxRPCWritePoints> rx_write_points_p;
  ClientContext context;
};

struct RxRPCChatWriter : public RxRPC {
  RxRPCChatWriter() : 
      RxRPC{"RouteChatWriter"},
    client_notes_{{
        MakeRouteNote("Message-0", 0, 0),
        MakeRouteNote("Message-1", 0, 10000000),
        MakeRouteNote("Message-2", 10000000, 0),
        MakeRouteNote("Message-3", 10000000, 10000000),
        MakeRouteNote("Message-4", 20000000, 0),
        MakeRouteNote("Message-5", 10000000, 0),
        MakeRouteNote("Message-6", 0, 0),
        MakeRouteNote("Message-7", 0, 20000000)
            }} {}
  void InitWrite() {
    GPR_ASSERT(state_ == State::CREATED);
    state_ = State::WRITING;
    ProcessReply(false);
  }
  bool ProcessReply(bool over) {
    switch (state_) {
      case State::WRITING:
        RouteNote* note_p;
        // Stop writing when kPoints have been successfully written
        if (idx_ >= kNotes_) {
          state_ = State::WRITING_DONE;
          stream->WritesDone(this);
          break;
        }
        note_p = &client_notes_.at(idx_); 
        ++idx_;
        stream->Write(*note_p, this);
        // DLOG(INFO) << "Tx Note msg " << note_p->message()
        //            << " at (" 
        //            << note_p->location().latitude()/kCoordFactor << ","
        //            << note_p->location().longitude()/kCoordFactor << ")";
        break;
      default:
        // RxRPCRecordRoute::ProcessReply: never sees FINISHED state. 
        // RxRPCFinish::ProcessReply: handles FINISHED state...
        GPR_ASSERT(state_ == State::WRITING_DONE);
        state_ = State::FINISHED;
        break;
    }
    return false;
  }
  void ProcessFinish() {
    GPR_ASSERT(state_ == State::FINISHED);
    DLOG(INFO) << "ChatWriter WriteStream Finished";
  }
  // Asynch API: hold on to the "stream" instance to note updates on the RPC
  // and not trigger destroying the RPC until all replies processed
  shared_ptr<ClientAsyncReaderWriter<RouteNote,RouteNote>> stream;
 private:
  enum class State {CREATED, WRITING, WRITING_DONE, FINISHED};
  State             state_{State::CREATED};
  size_t            idx_{0};
  static constexpr size_t kNotes_{8};
  array<RouteNote, kNotes_> client_notes_;
};

constexpr size_t RxRPCChatWriter::kNotes_;

struct RxRPCChatReader : public RxRPC { 
  RxRPCChatReader(RxRPC *rx_rpc_p) : 
      RxRPC{"ChatReader"},
    finish_p(new RxRPCFinish{"RouteChat", rx_rpc_p}) {}
  void InitRead(void) {
    GPR_ASSERT(state_ == State::INVALID);
    state_ = State::READING;
    stream->Read(&note_, this);
  }
  bool ProcessReply(bool over) override {
    GPR_ASSERT(state_ == State::READING);
    if (over) {
      // server closed stream (i.e. call WritesDone)
      // Bug?: Shouldn't we call Finish only after Read & Write is over? 
      stream->Finish(&finish_p->status, finish_p.get());
      return false;
    }
    nlist_.push_back(note_);
    // Bug: Last unconsumed Read message causes memory leak or assertion
    // iomgr.c:77: LEAKED OBJECT: tcp-client:ipv4:127.0.0.1:50051 0x16301f8    
    // The unconsumed READ returns with a failed status on cq_.Next
    // as the referenced tag is already destroyed!
    // if (nlist_.size() < kNotesReturned_) 
    stream->Read(&note_, this);
    return false;
  }
  void ProcessFinish() override {
    state_ = State::FINISHED;
    DLOG(INFO) << "ChatReader ReadStream Finished";
    DLOG(INFO) << "--------------- RouteChat ---------------";
    for(const RouteNote& n: nlist_) 
      DLOG(INFO) << "Found Note \"" << n.message()  << "\" at ("
                 << n.location().latitude()/kCoordFactor << ","
                 << n.location().longitude()/kCoordFactor << ")";
    DLOG(INFO) << "#RouteNotes returned with matching criteria " 
               << nlist_.size();
  }
  // Asynchronous API: hold on to the "rpc" instance to rc updates 
  // on the RPC and not trigger destroying the RPC until reply processed
  shared_ptr<ClientAsyncReaderWriter<RouteNote,RouteNote>> stream;
 private:
  enum class State {INVALID, READING, FINISHED};
  State           state_{State::INVALID};
  RouteNote       note_;
  vector<RouteNote> nlist_;
  unique_ptr<RxRPCFinish>     finish_p;
  static constexpr size_t kNotesReturned_{2};
};

constexpr size_t RxRPCChatReader::kNotesReturned_;

// Concurrency Examples:
// test/cpp/thread_stress_test.cc
// test/cpp/end2end/thread_stress_test.cc
// test/cpp/qps/*async.cc
struct RxRPCRouteChat : public RxRPC {
  explicit RxRPCRouteChat(RouteGuide::Stub* stub_p, 
                          CompletionQueue *cq_p) :
      RxRPC{"RouteChat"} {
    writer_p.reset(new RxRPCChatWriter{});
    reader_p.reset(new RxRPCChatReader{this});
    unique_ptr<ClientAsyncReaderWriter<RouteNote, RouteNote>>
               stream = stub_p->AsyncRouteChat(&context, cq_p, this);
    reader_p->stream.reset(stream.release());
    writer_p->stream = reader_p->stream;
  }
  bool ProcessReply(bool over) override {
    writer_p->InitWrite(); // run in a separate thread
    reader_p->InitRead();
    return false;
  }
  void ProcessFinish() override {  
    writer_p->ProcessFinish();
    reader_p->ProcessFinish();
  }
  unique_ptr<RxRPCChatWriter> writer_p;
  unique_ptr<RxRPCChatReader> reader_p;
  ClientContext context;
};

class RouteGuideClient {
 public:
  RouteGuideClient(std::shared_ptr<Channel> channel, const string& db_path)
      : stub_(RouteGuide::NewStub(channel)) {
    DbReadJSON dbrj{DbReadJSON::ReadJSONFile(db_path)};
    dbrj.Parse(&feature_list_);
    DLOG(INFO) << "Features Read: " << feature_list_.size();
  }
  ~RouteGuideClient() {
    // Shutdown the completion queue
    cq_.Shutdown();
    // Drain all events still queued up
    void *ignored_tag = nullptr;
    bool ignored_ok = false;
    while (cq_.Next(&ignored_tag, &ignored_ok));
  }

  // One way to multi-thread run initiation of TxRPCs in any thread
  void TxRPCs(void) {
    TxRPCGetFeature();
    TxRPCListFeatures();
    TxRPCRecordRoute();
    TxRPCRouteChat();
  }

  // One way to multi-thread: Run each RPC callback in any thread 
  void RxRPCs(void) {
    constexpr int kNumTxRPCs = 4;
    for (int i=0; i<kNumTxRPCs; ++i) {
      void *tag   = nullptr; // unique RPC call identifier
      bool ok     = false;   // has the RPC Finish call executed successfully
      bool finish = false;
      while (!finish) {
        // Block until the next result is available in the completion queue "cq".
        cq_.Next(&tag, &ok);
        RxRPC *rpc_struct_p = static_cast<RxRPC *>(tag);
        // DLOG(INFO) << "RPC " << rpc_struct_p->name << " event-Q";
        // ok false for read on closed stream
        finish = rpc_struct_p->ProcessReply(!ok); 
      }
    }
    return;
  }

 private:
  unique_ptr<RouteGuide::Stub> stub_;
  vector<Feature> feature_list_;
  // producer-consumer Q used to communicate with gRPC runtime.
  CompletionQueue cq_;

  void TxRPCGetFeature() {
    Point point = MakePoint(409146138, -746188906);
    DLOG(INFO) << "Looking for feature for point: (" 
               << point.latitude() << ","
               << point.longitude() << ")";
    new RxRPCGetFeature{stub_.get(), &cq_, point};
  }

  void TxRPCListFeatures() {
    routeguide::Rectangle rect;
    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    DLOG(INFO) << "Looking for features between ("
               << rect.lo().latitude() << ","
               << rect.lo().longitude() << ") and ("
               << rect.hi().latitude() << ","
               << rect.hi().longitude() << ")";
    new RxRPCListFeatures{stub_.get(), &cq_, rect, &feature_list_};
  }

  void TxRPCRecordRoute() {
    new RxRPCRecordRoute{stub_.get(), &cq_, &feature_list_};
  }

  void TxRPCRouteChat() {
    new RxRPCRouteChat{stub_.get(), &cq_};
  }
};

// Flag Declarations
// --auto_test=[false]
DECLARE_bool(auto_test);
// --db_path=path/to/route_guide_db.json.
DECLARE_string(db_path);

int main(int argc, char** argv) {
  Init::InitEnv(&argc, &argv);

  RouteGuideClient guide(
      grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()),
      FLAGS_db_path);

  DLOG(INFO) << argv[0] << " Executing Test";
  guide.TxRPCs();
  guide.RxRPCs();
  DLOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

DEFINE_string(db_path, "route_guide_db.json", 
              "path to route_guide_db.json");


