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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
#include <grpc/grpc.h>
//CLIENT
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
//SERVER
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
// Local Headers
#include "experiment_test/grpc_test/db/db_read_json.h"
#include "experiment_test/grpc_test/protos/route_guide.grpc.pb.h"
#include "experiment_test/grpc_test/protos/route_guide.pb.h"
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

//CLIENT
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
//SERVER
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using std::chrono::system_clock;
//BOTH
using asarcar::routeguide::Point;
using asarcar::routeguide::Feature;
using asarcar::routeguide::Rectangle;
using asarcar::routeguide::RouteSummary;
using asarcar::routeguide::RouteNote;
using asarcar::routeguide::RouteGuide;
using grpc::Status;

using namespace std;
using namespace asarcar;
using namespace asarcar::routeguide;

// CLIENT
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

static std::string
rg_db_json_str = "\
[\
  {\
    \"location\": \
    {\
        \"latitude\": 407838351,\
        \"longitude\": -746143763\
    },\
    \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
  }, \
  {\
    \"location\": \
    {\
        \"latitude\": 409146138,\
        \"longitude\": -746188906\
    },\
    \"name\": \"Berkshire Valley Management Area Trail, Jefferson, NJ, USA\"\
  },  \
  {\
    \"location\": \
    {\
        \"latitude\": 411733222,\
        \"longitude\": -744228360\
    },\
    \"name\": \"\"\
  }, \
  {\
    \"location\": \
    {\
        \"latitude\": 410248224,\
        \"longitude\": -747127767\
    },\
    \"name\": \"3 Hasta Way, Newton, NJ 07860, USA\"\
  }\
]";

class RouteGuideClient {
 public:
  RouteGuideClient(std::shared_ptr<Channel> channel)
      : stub_(RouteGuide::NewStub(channel)) {
    DbReadJSON dbrj{string{rg_db_json_str}};
    dbrj.Parse(&feature_list_);
    CHECK_EQ(feature_list_.size(), 4) 
        << "Features Read: " << feature_list_.size();
  }

  void TestGetFeature() {
    Point point;
    Feature feature;
    point = MakePoint(409146138, -746188906);
    GetOneFeature(point, &feature);
    CHECK_STREQ(feature.name().c_str(), 
                "BerkshireValleyManagementAreaTrail,Jefferson,NJ,USA");
    point = MakePoint(0, 0);
    GetOneFeature(point, &feature);
    CHECK_STREQ(feature.name().c_str(), "");
  }

  void TestListFeatures() {
    routeguide::Rectangle rect;
    Feature feature;
    ClientContext context;
    const int kPoints = feature_list_.size();

    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    DLOG(INFO) << "Looking for features between 40, -75 and 42, -73";

    std::unique_ptr<ClientReader<Feature> > reader(
        stub_->ListFeatures(&context, rect));
    int i=0;
    while (reader->Read(&feature)) {
      ++i;
      DLOG(INFO) << "Found feature called "
                 << feature.name() << " at "
                 << feature.location().latitude()/kCoordFactor_ << ", "
                 << feature.location().longitude()/kCoordFactor_;
    }
    CHECK_EQ(i, kPoints) << "Number of Features Expected mismatch";
    Status status = reader->Finish();
    CHECK(status.ok()) << "ListFeatures rpc failed" 
                       << ": code=" << status.error_code() 
                       << ": message=" << status.error_message();
  }

  void TestRecordRoute() {
    Point point;
    RouteSummary stats;
    ClientContext context;
    const int kPoints = feature_list_.size();
    const int kDistance = 77056;
    std::unique_ptr<ClientWriter<Point> > writer(
        stub_->RecordRoute(&context, &stats));
    for (int i = 0; i < kPoints; i++) {
      const Feature& f = feature_list_.at(i);
      DLOG(INFO) << "Visiting point "
                 << f.location().latitude()/kCoordFactor_ << ", "
                 << f.location().longitude()/kCoordFactor_;
      if (!writer->Write(f.location())) {
        // Broken stream.
        break;
      }
    }
    writer->WritesDone();
    Status status = writer->Finish();
    CHECK(status.ok()) << "RecordRoute rpc failed"
                       << ": code=" << status.error_code() 
                       << ": message=" << status.error_message();
    DLOG(INFO) << "Finished trip with " << stats.point_count() << " points"
               << ": Passed " << stats.feature_count() << " features"
               << ": Travelled " << stats.distance() << " meters"
               << ": Took " << stats.elapsed_time() << " seconds";
    CHECK_EQ(stats.point_count(), kPoints); 
    CHECK_EQ(stats.feature_count(), kPoints-1); 
    CHECK_EQ(stats.distance(), kDistance); 
  }

  void TestRouteChat() {
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
        DLOG(INFO) << "Sending message " << note.message()
                   << " at " << note.location().latitude() << ", "
                   << note.location().longitude();
        stream->Write(note);
      }
      stream->WritesDone();
    });

    RouteNote server_note;
    CHECK(stream->Read(&server_note));
    DLOG(INFO) << "Rcvd message " << server_note.message()
               << " at " << server_note.location().latitude() << ", "
               << server_note.location().longitude();
    CHECK_STREQ(server_note.message().c_str(), "First message");
    CHECK_EQ(server_note.location().latitude(), 0);
    CHECK_EQ(server_note.location().longitude(), 0);
    CHECK(!stream->Read(&server_note));
    writer.join();
    Status status = stream->Finish();
    CHECK(status.ok()) << "RouteChat rpc failed"
                       << ": code=" << status.error_code() 
                       << ": message=" << status.error_message();
  }

 private:
  std::vector<Feature> feature_list_;

  bool GetOneFeature(const Point& point, Feature* feature) {
    ClientContext context;
    Status status = stub_->GetFeature(&context, point, feature);
    CHECK(status.ok()) << "GetFeature rpc failed"
                       << ": code=" << status.error_code()
                       << ": message=" << status.error_message();
    CHECK(feature->has_location())  
        << "Server returns incomplete feature: location not set.";

    DLOG(INFO) << "Found feature called " << feature->name()  << " at "
               << feature->location().latitude()/kCoordFactor_ << ", "
               << feature->location().longitude()/kCoordFactor_;
    return true;
  }

  const float kCoordFactor_ = 10000000.0;
  std::unique_ptr<RouteGuide::Stub> stub_;
};

// SERVER
float ConvertToRadians(float num) {
  return num * 3.1415926 /180;
}

float GetDistance(const Point& start, const Point& end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2-lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2-lon_1);

  float a = pow(sin(delta_lat_rad/2), 2) + cos(lat_rad_1) * cos(lat_rad_2) *
            pow(sin(delta_lon_rad/2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1-a));
  int R = 6371000; // metres

  return R * c;
}

std::string GetFeatureName(const Point& point,
                           const std::vector<Feature>& feature_list) {
  for (const Feature& f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

class RouteGuideImpl final : public RouteGuide::Service {
 public:
  explicit RouteGuideImpl() {
    DbReadJSON dbrj{string{rg_db_json_str}};
    dbrj.Parse(&feature_list_);
    CHECK_EQ(feature_list_.size(), 4) 
        << "Features Read: " << feature_list_.size();
  }

  Status GetFeature(ServerContext* context, const Point* point,
                    Feature* feature) override {
    DLOG(INFO) << "GetFeature gRPC request at (" << point->latitude()
               << "," << point->latitude() << ")";
    feature->set_name(GetFeatureName(*point, feature_list_));
    feature->mutable_location()->CopyFrom(*point);
    return Status::OK;
  }

  Status ListFeatures(ServerContext* context,
                      const routeguide::Rectangle* rectangle,
                      ServerWriter<Feature>* writer) override {
    auto lo = rectangle->lo();
    auto hi = rectangle->hi();
    long left = (std::min)(lo.longitude(), hi.longitude());
    long right = (std::max)(lo.longitude(), hi.longitude());
    long top = (std::max)(lo.latitude(), hi.latitude());
    long bottom = (std::min)(lo.latitude(), hi.latitude());
    for (const Feature& f : feature_list_) {
      if (f.location().longitude() >= left &&
          f.location().longitude() <= right &&
          f.location().latitude() >= bottom &&
          f.location().latitude() <= top) {
        writer->Write(f);
      }
    }
    return Status::OK;
  }

  Status RecordRoute(ServerContext* context, ServerReader<Point>* reader,
                     RouteSummary* summary) override {
    Point point;
    int point_count = 0;
    int feature_count = 0;
    float distance = 0.0;
    Point previous;

    system_clock::time_point start_time = system_clock::now();
    while (reader->Read(&point)) {
      point_count++;
      if (!GetFeatureName(point, feature_list_).empty()) {
        feature_count++;
      }
      if (point_count != 1) {
        distance += GetDistance(previous, point);
      }
      previous = point;
    }
    system_clock::time_point end_time = system_clock::now();
    summary->set_point_count(point_count);
    summary->set_feature_count(feature_count);
    summary->set_distance(static_cast<long>(distance));
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time);
    summary->set_elapsed_time(secs.count());

    return Status::OK;
  }

  Status RouteChat(ServerContext* context,
                   ServerReaderWriter<RouteNote, RouteNote>* stream) override {
    std::vector<RouteNote> received_notes;
    RouteNote note;
    while (stream->Read(&note)) {
      for (const RouteNote& n : received_notes) {
        if (n.location().latitude() == note.location().latitude() &&
            n.location().longitude() == note.location().longitude()) {
          stream->Write(n);
        }
      }
      received_notes.push_back(note);
    }

    return Status::OK;
  }

 private:
  std::vector<Feature> feature_list_;
};

// Forward Declarations
DECLARE_bool(auto_test); // --auto_test=[false]

int main(int argc, char** argv) {
  Init::InitEnv(&argc, &argv);

  LOG(INFO) << argv[0] << " Executing Test";
      
  // Run Server in separate thread
  RouteGuideImpl service{};
  string server_address("0.0.0.0:50051");
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  unique_ptr<Server> server_p(builder.BuildAndStart());
  LOG(INFO) << "Server listening on " << server_address;
  
  // Run Client
  std::thread client([]() {
      RouteGuideClient guide(
          grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
      
      DLOG(INFO) << "-------------- GetFeature --------------";
      guide.TestGetFeature();
      DLOG(INFO) << "-------------- ListFeatures --------------";
      guide.TestListFeatures();
      DLOG(INFO) << "-------------- RecordRoute --------------";
      guide.TestRecordRoute();
      DLOG(INFO) << "-------------- RouteChat --------------";
      guide.TestRouteChat();
    });

  client.join();
  // Terminate Server now the client thread has completed RPCs
  server_p->Shutdown();

  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
