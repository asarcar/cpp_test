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

// Standard C++ Headers
#include <array>       // array
#include <exception>   // throw
#include <iostream>
#include <fstream>     // fstream
#include <string>      // string
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "experiment_test/pbuf_test/address_book.pb.h"
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using namespace std;
using namespace asarcar;
using namespace asarcar::experiment_test::pbuf_test;

class ABookPBufTester {
 private:
  static constexpr size_t NUM_PEOPLE = 2;
  static constexpr size_t NUM_PHONE_PER_PERSON  = 2;
  static constexpr size_t NUM_PHONE = NUM_PEOPLE*NUM_PHONE_PER_PERSON;
 public:
  ABookPBufTester() = default;
  ~ABookPBufTester() = default;
  void Run(void) {
    InitFile();
    OneTest();
    AppendFile();
    TwoTest();
    return;
  }
 private:
  string                    file_ {"abook_puf_test.db"};
  array<string,NUM_PEOPLE>  name_ {
    {"Rajesh Kumar", "Anthony Gonzales"}
  };
  array<int,NUM_PEOPLE>     id_   {{1, 2}};
  array<string,NUM_PEOPLE>  email_{
    {"rajesh@test.one.com", "anthony@test.two.com"}
  };
  array<string,NUM_PHONE> phone_{
    {"111-222-3333", "111-222-4444", "222-333-4444", "222-333-5555"}
  };
  array<Person_PhoneType,NUM_PHONE> phone_type_ {
    { Person_PhoneType_HOME, Person_PhoneType_WORK, 
      Person_PhoneType_MOBILE, Person_PhoneType_HOME }
  };
  
 private:
  void InitFile(void) {
    AddressBook abook;
    Person*     person_p = abook.add_person();
    person_p->set_name(name_.at(0));
    person_p->set_id(id_.at(0));
    person_p->set_email(email_.at(0));
    Person_PhoneNumber *phone_p = person_p->add_phone();
    phone_p->set_number(phone_.at(0));
    phone_p = person_p->add_phone();
    phone_p->set_number(phone_.at(1));
    phone_p->set_type(phone_type_.at(1));
    
    fstream ofs(file_, ios::out | ios::trunc | ios::binary);
    bool serialize_ok = abook.SerializeToOstream(&ofs);
    FASSERT(serialize_ok);
    return;
  }
  void OneTest(void) {
    AddressBook abook;
    fstream ifs(file_, ios::in | ios::binary);
    bool parse_ok = abook.ParseFromIstream(&ifs);
    FASSERT(parse_ok);
    FASSERT(abook.person_size() == 1);
    const Person& person = abook.person(0);
    CHECK(person.has_name());
    CHECK_EQ(person.name(), name_.at(0));
    CHECK(person.has_id());
    CHECK_EQ(person.id(), id_.at(0));
    CHECK(person.has_email());
    CHECK_EQ(person.email(), email_.at(0));
    FASSERT(person.phone_size() == 2);
    const Person_PhoneNumber& phone = person.phone(0);
    CHECK(phone.has_number());
    CHECK_EQ(phone.number(), phone_.at(0));
    CHECK(!phone.has_type());
    CHECK_EQ(phone.type(), phone_type_.at(0));
    const Person_PhoneNumber& phone2 = person.phone(1);
    CHECK(phone2.has_number());
    CHECK_EQ(phone2.number(), phone_.at(1));
    CHECK_EQ(phone2.type(), phone_type_.at(1));

    LOG(INFO) << "AddressBook Size=" << abook.person_size()
              << ": ABookStruct=" << sizeof(abook)
              << ": Person1Name=" << person.name()
              << ": PersonStruct=" << sizeof(person)
              << ": PhoneNumber Size=" << person.phone_size()
              << ": Number1=" << phone.number()
              << ": PhoneSize=" << sizeof(phone);

    return;
  }
  void AppendFile(void) {
    AddressBook abook;
    fstream ifs(file_, ios::in | ios::binary);
    bool parse_ok = abook.ParseFromIstream(&ifs);
    FASSERT(parse_ok);
    FASSERT(abook.person_size() == 1);
    Person* person_p = abook.add_person();
    person_p->set_name(name_.at(1));
    person_p->set_id(id_.at(1));
    person_p->set_email(email_.at(1));
    Person_PhoneNumber *phone_p = person_p->add_phone();
    phone_p->set_number(phone_.at(2));
    phone_p->set_type(phone_type_.at(2));
    phone_p = person_p->add_phone();
    phone_p->set_number(phone_.at(3));

    fstream ofs(file_, ios::out | ios::trunc | ios::binary);
    bool serialize_ok = abook.SerializeToOstream(&ofs);
    FASSERT(serialize_ok);
    return;
  }
  void TwoTest(void) {
    AddressBook abook;
    fstream ifs(file_, ios::in | ios::binary);
    bool parse_ok = abook.ParseFromIstream(&ifs);
    FASSERT(parse_ok);
    FASSERT(abook.person_size() == 2);
    const Person& person = abook.person(1);
    CHECK(person.has_name());
    CHECK_EQ(person.name(), name_.at(1));
    CHECK(person.has_id());
    CHECK_EQ(person.id(), id_.at(1));
    CHECK(person.has_email());
    CHECK_EQ(person.email(), email_.at(1));
    FASSERT(person.phone_size() == 2);
    const Person_PhoneNumber& phone = person.phone(0);
    CHECK(phone.has_number());
    CHECK_EQ(phone.number(), phone_.at(2));
    CHECK(phone.has_type());
    CHECK_EQ(phone.type(), phone_type_.at(2));
    const Person_PhoneNumber& phone2 = person.phone(1);
    CHECK(phone2.has_number());
    CHECK_EQ(phone2.number(), phone_.at(3));
    CHECK(!phone2.has_type());

    LOG(INFO) << "AddressBook Size=" << abook.person_size()
              << ": ABookStruct=" << sizeof(abook)
              << ": Person2Name=" << person.name()
              << ": PersonStruct=" << sizeof(person)
              << ": PhoneNumber Size=" << person.phone_size()
              << ": Number2=" << phone2.number()
              << ": Phone2Size=" << sizeof(phone2);

    return;
  }
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char** argv) {
  Init::InitEnv(&argc, &argv);

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  LOG(INFO) << argv[0] << " Executing Test";
  ABookPBufTester abt;
  abt.Run();
  // Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

