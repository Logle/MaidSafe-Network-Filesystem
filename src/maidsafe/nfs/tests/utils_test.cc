/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/nfs/utils.h"

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "maidsafe/common/crypto.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/passport.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/data_types/immutable_data.h"
#include "maidsafe/data_types/mutable_data.h"

#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/message.h"


namespace maidsafe {

namespace nfs {

namespace test {

typedef std::vector<std::future<std::string>> RoutingFutures;

template<typename Data>
std::pair<Identity, NonEmptyString> GetNameAndContent();

template<typename Fob>
std::pair<Identity, NonEmptyString> MakeNameAndContentPair(const Fob& fob) {
  maidsafe::passport::detail::PublicFob<typename Fob::name_type::tag_type> public_fob(fob);
  return std::make_pair(public_fob.name().data, public_fob.Serialise().data);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicAnmid>() {
  passport::Anmid anmid;
  return MakeNameAndContentPair(anmid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicAnsmid>() {
  passport::Ansmid ansmid;
  return MakeNameAndContentPair(ansmid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicAntmid>() {
  passport::Antmid antmid;
  return MakeNameAndContentPair(antmid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicAnmaid>() {
  passport::Anmaid anmaid;
  return MakeNameAndContentPair(anmaid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicMaid>() {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  return MakeNameAndContentPair(maid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicPmid>() {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  return MakeNameAndContentPair(pmid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicAnmpid>() {
  passport::Anmpid anmpid;
  return MakeNameAndContentPair(anmpid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::PublicMpid>() {
  passport::Anmpid anmpid;
  passport::Mpid mpid(NonEmptyString("Test"), anmpid);
  return MakeNameAndContentPair(mpid);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::Mid>() {
  const UserPassword kKeyword(RandomAlphaNumericString(20));
  const UserPassword kPassword(RandomAlphaNumericString(20));
  const uint32_t kPin(RandomUint32() % 9999 + 1);
  const NonEmptyString kMasterData(RandomString(34567));
  auto encrypted_session(passport::EncryptSession(kKeyword, kPin, kPassword, kMasterData));
  passport::Antmid antmid;
  passport::Tmid tmid(encrypted_session, antmid);

  passport::Anmid anmid;
  passport::Mid mid(passport::MidName(kKeyword, kPin),
                    passport::EncryptTmidName(kKeyword, kPin, tmid.name()),
                    anmid);
  return std::make_pair(mid.name().data, mid.Serialise().data);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::Smid>() {
  const UserPassword kKeyword(RandomAlphaNumericString(20));
  const UserPassword kPassword(RandomAlphaNumericString(20));
  const uint32_t kPin(RandomUint32() % 9999 + 1);
  const NonEmptyString kMasterData(RandomString(34567));
  auto encrypted_session(passport::EncryptSession(kKeyword, kPin, kPassword, kMasterData));
  passport::Antmid antmid;
  passport::Tmid tmid(encrypted_session, antmid);

  passport::Ansmid ansmid;
  passport::Smid smid(passport::SmidName(kKeyword, kPin),
                      passport::EncryptTmidName(kKeyword, kPin, tmid.name()),
                      ansmid);
  return std::make_pair(smid.name().data, smid.Serialise().data);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<passport::Tmid>() {
  const UserPassword kKeyword(RandomAlphaNumericString(20));
  const UserPassword kPassword(RandomAlphaNumericString(20));
  const uint32_t kPin(RandomUint32() % 9999 + 1);
  const NonEmptyString kMasterData(RandomString(34567));
  auto encrypted_session(passport::EncryptSession(kKeyword, kPin, kPassword, kMasterData));
  passport::Antmid antmid;
  passport::Tmid tmid(encrypted_session, antmid);
  return std::make_pair(tmid.name().data, tmid.Serialise().data);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<ImmutableData>() {
  NonEmptyString value(RandomString(RandomUint32() % 10000 + 10));
  Identity name(crypto::Hash<crypto::SHA512>(value));
  ImmutableData immutable(ImmutableData::name_type(name), value);
  return std::make_pair(immutable.name().data, immutable.Serialise().data);
}

template<>
std::pair<Identity, NonEmptyString> GetNameAndContent<MutableData>() {
  NonEmptyString value(RandomString(RandomUint32() % 10000 + 10));
  Identity name(crypto::Hash<crypto::SHA512>(value));
  passport::Anmid anmid;
  auto signature(asymm::Sign(value, anmid.private_key()));
  MutableData mutable_data(MutableData::name_type(name), value, signature, 99);
  return std::make_pair(mutable_data.name().data, mutable_data.Serialise().data);
}

template<typename T>
class UtilsTest : public testing::Test {
 protected:
  std::string MakeSerialisedMessage(const std::pair<Identity, NonEmptyString>& name_and_content) {
    Persona destination_persona(Persona::kMetadataManager);
    MessageSource source(Persona::kClientMaid, NodeId(NodeId::kRandomId));
    DataMessage::Data data(T::name_type::tag_type::kEnumValue, name_and_content.first,
                           name_and_content.second);
    DataMessage data_message(DataMessage::Action::kGet, destination_persona, source, data);
    Message message(DataMessage::message_type_identifier, data_message.Serialise().data);
    return message.Serialise()->string();
  }

  RoutingFutures SendReturnsAllFailed() const {
    RoutingFutures futures;
    for (int i(0); i != 4; ++i) {
      auto promise(std::make_shared<std::promise<std::string>>());
      futures.push_back(promise->get_future());
      std::thread worker([promise, i] {
          std::this_thread::sleep_for(std::chrono::milliseconds(i * 100));
          promise->set_exception(std::make_exception_ptr(MakeError(RoutingErrors::timed_out)));
      });
      worker.detach();
    }
    return std::move(futures);
  }

  RoutingFutures SendReturnsOneSuccess(const std::string& serialised_message) const {
    RoutingFutures futures;
    int succeed_index(RandomUint32() % 4);
    for (int i(0); i != 4; ++i) {
      auto promise(std::make_shared<std::promise<std::string>>());
      futures.push_back(promise->get_future());
      std::thread worker([promise, i, succeed_index, serialised_message] {
          std::this_thread::sleep_for(std::chrono::milliseconds(i * 100));
          if (i == succeed_index) {
//            LOG(kSuccess) << "Setting value for i == " << i;
            promise->set_value(serialised_message);
          } else {
//            LOG(kWarning) << "Setting exception for i == " << i;
            promise->set_exception(std::make_exception_ptr(MakeError(RoutingErrors::timed_out)));
          }
      });
      worker.detach();
    }
    return std::move(futures);
  }

  RoutingFutures SendReturnsAllSuccesses(const std::string& serialised_message) const {
    RoutingFutures futures;
    for (int i(0); i != 4; ++i) {
      auto promise(std::make_shared<std::promise<std::string>>());
      futures.push_back(promise->get_future());
      std::thread worker([promise, i, serialised_message] {
          std::this_thread::sleep_for(std::chrono::milliseconds(i * 100));
          promise->set_value(serialised_message);
      });
      worker.detach();
    }
    return std::move(futures);
  }
};

TYPED_TEST_CASE_P(UtilsTest);

TYPED_TEST_P(UtilsTest, BEH_HandleGetFuturesAllFail) {
  auto promise(std::make_shared<std::promise<TypeParam>>());
  std::future<TypeParam> future(promise->get_future());
  auto routing_futures(std::make_shared<RoutingFutures>(this->SendReturnsAllFailed()));

  HandleGetFutures<TypeParam>(promise, routing_futures);
  EXPECT_THROW(future.get(), nfs_error);
}

TYPED_TEST_P(UtilsTest, BEH_HandleGetFuturesOneSucceeds) {
  auto promise(std::make_shared<std::promise<TypeParam>>());
  std::future<TypeParam> future(promise->get_future());

  std::pair<Identity, NonEmptyString> name_and_content(GetNameAndContent<TypeParam>());
  auto serialised_message(this->MakeSerialisedMessage(name_and_content));
  auto routing_futures(std::make_shared<RoutingFutures>(
                           this->SendReturnsOneSuccess(serialised_message)));

  HandleGetFutures<TypeParam>(promise, routing_futures);
  auto data(future.get());
  EXPECT_EQ(data.name().data, name_and_content.first);
}

TYPED_TEST_P(UtilsTest, BEH_HandleGetFuturesAllSucceed) {
  auto promise(std::make_shared<std::promise<TypeParam>>());
  std::future<TypeParam> future(promise->get_future());

  std::pair<Identity, NonEmptyString> name_and_content(GetNameAndContent<TypeParam>());
  auto serialised_message(this->MakeSerialisedMessage(name_and_content));
  auto routing_futures(std::make_shared<RoutingFutures>(
                           this->SendReturnsOneSuccess(serialised_message)));

  HandleGetFutures<TypeParam>(promise, routing_futures);
  auto data(future.get());
  EXPECT_EQ(data.name().data, name_and_content.first);
}

REGISTER_TYPED_TEST_CASE_P(UtilsTest,
                           BEH_HandleGetFuturesAllFail,
                           BEH_HandleGetFuturesOneSucceeds,
                           BEH_HandleGetFuturesAllSucceed);

typedef testing::Types<passport::PublicAnmid,
                       passport::PublicAnsmid,
                       passport::PublicAntmid,
                       passport::PublicAnmaid,
                       passport::PublicMaid,
                       passport::PublicPmid,
                       passport::Mid,
                       passport::Smid,
                       passport::Tmid,
                       passport::PublicAnmpid,
                       passport::PublicMpid,
                       ImmutableData,
                       MutableData> DataTypes;
INSTANTIATE_TYPED_TEST_CASE_P(All, UtilsTest, DataTypes);

}  // namespace test

}  // namespace nfs

}  // namespace maidsafe
