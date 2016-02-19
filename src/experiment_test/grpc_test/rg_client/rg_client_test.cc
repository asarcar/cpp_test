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
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
// Local Headers
#include "experiment_test/grpc_test/db/db_read_json.h"
#include "experiment_test/grpc_test/protos/route_guide.grpc.pb.h"
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
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

class RouteGuideClient {
 public:
  RouteGuideClient(std::shared_ptr<Channel> channel, const string& db_path)
      : stub_(RouteGuide::NewStub(channel)) {
    DbReadJSON dbrj{DbReadJSON::ReadJSONFile(db_path)};
    dbrj.Parse(&feature_list_);
    DLOG(INFO) << "Features Read: " << feature_list_.size();
  }

  void GetFeature() {
    Point point;
    Feature feature;
    point = MakePoint(409146138, -746188906);
    GetOneFeature(point, &feature);
    point = MakePoint(0, 0);
    GetOneFeature(point, &feature);
  }

  void ListFeatures() {
    routeguide::Rectangle rect;
    Feature feature;
    ClientContext context;

    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    LOG(INFO) << "Looking for features between 40, -75 and 42, -73";

    std::unique_ptr<ClientReader<Feature> > reader(
        stub_->ListFeatures(&context, rect));
    while (reader->Read(&feature)) {
      if (feature.name().empty()) {
        LOG(INFO) << "Found no feature at "
                  << feature.location().latitude()/kCoordFactor_ << ", "
                  << feature.location().longitude()/kCoordFactor_;
        continue;
      }
      LOG(INFO) << "Found feature called "
                << feature.name() << " at "
                << feature.location().latitude()/kCoordFactor_ << ", "
                << feature.location().longitude()/kCoordFactor_;
    }
    Status status = reader->Finish();
    if (status.ok()) {
      LOG(INFO) << "ListFeatures rpc succeeded.";
    } else {
      LOG(ERROR) << "ListFeatures rpc failed.";
    }
  }

  void RecordRoute() {
    Point point;
    RouteSummary stats;
    ClientContext context;
    const int kPoints = 10;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> feature_distribution(
        0, feature_list_.size() - 1);
    std::uniform_int_distribution<int> delay_distribution(
        500, 1500);

    std::unique_ptr<ClientWriter<Point> > writer(
        stub_->RecordRoute(&context, &stats));
    for (int i = 0; i < kPoints; i++) {
      const Feature& f = feature_list_.at(feature_distribution(generator));
      LOG(INFO) << "Visiting point "
                << f.location().latitude()/kCoordFactor_ << ", "
                << f.location().longitude()/kCoordFactor_;
      if (!writer->Write(f.location())) {
        // Broken stream.
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(
          delay_distribution(generator)));
    }
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
      LOG(INFO) << "Finished trip with " << stats.point_count() << " points\n"
                << "Passed " << stats.feature_count() << " features\n"
                << "Travelled " << stats.distance() << " meters\n"
                << "It took " << stats.elapsed_time() << " seconds";
    } else {
      LOG(ERROR) << "RecordRoute rpc failed.";
    }
  }

  void RouteChat() {
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<RouteNote, RouteNote> > stream(
        stub_->RouteChat(&context));

    std::thread writer([stream]() {
      std::vector<RouteNote> notes{
        MakeRouteNote("First message", 0, 0),
        MakeRouteNote("Second message", 0, 1),
        MakeRouteNote("Third message", 1, 0),
        MakeRouteNote("Fourth message", 0, 0)};
      for (const RouteNote& note : notes) {
        LOG(INFO) << "Sending message " << note.message()
                  << " at " << note.location().latitude() << ", "
                  << note.location().longitude();
        stream->Write(note);
      }
      stream->WritesDone();
    });

    RouteNote server_note;
    while (stream->Read(&server_note)) {
      LOG(INFO) << "Got message " << server_note.message()
                << " at " << server_note.location().latitude() << ", "
                << server_note.location().longitude();
    }
    writer.join();
    Status status = stream->Finish();
    if (!status.ok()) {
      LOG(ERROR) << "RouteChat rpc failed"
                 << ": code=" << status.error_code() 
                 << ": message=" << status.error_message();
    }
  }

 private:

  bool GetOneFeature(const Point& point, Feature* feature) {
    ClientContext context;
    Status status = stub_->GetFeature(&context, point, feature);
    if (!status.ok()) {
      LOG(ERROR) << "GetFeature rpc failed"
                 << ": code=" << status.error_code()
                 << ": message=" << status.error_message();
      return false;
    }
    if (!feature->has_location()) {
      LOG(ERROR) << "Server returns incomplete feature.";
      return false;
    }
    if (feature->name().empty()) {
      LOG(INFO) << "Found no feature at "
                << feature->location().latitude()/kCoordFactor_ << ", "
                << feature->location().longitude()/kCoordFactor_;
    } else {
      LOG(INFO) << "Found feature called " << feature->name()  << " at "
                << feature->location().latitude()/kCoordFactor_ << ", "
                << feature->location().longitude()/kCoordFactor_;
    }
    return true;
  }

  const float kCoordFactor_ = 10000000.0;
  std::unique_ptr<RouteGuide::Stub> stub_;
  std::vector<Feature> feature_list_;
};

// Flag Declarations
// --auto_test=[false]
DECLARE_bool(auto_test);
// --db_path=path/to/route_guide_db.json.
DECLARE_string(db_path);

int main(int argc, char** argv) {
  Init::InitEnv(&argc, &argv);

  RouteGuideClient guide(
      grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials()),
      FLAGS_db_path);

  LOG(INFO) << argv[0] << " Executing Test";

  LOG(INFO) << "-------------- GetFeature --------------";
  guide.GetFeature();
  LOG(INFO) << "-------------- ListFeatures --------------";
  guide.ListFeatures();
  LOG(INFO) << "-------------- RecordRoute --------------";
  guide.RecordRoute();
  LOG(INFO) << "-------------- RouteChat --------------";
  guide.RouteChat();

  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

DEFINE_string(db_path, "../testdata/route_guide_db.json", 
              "path to route_guide_db.json");


