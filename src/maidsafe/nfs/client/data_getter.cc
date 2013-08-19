/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"


namespace maidsafe {

namespace nfs_client {

DataGetter::DataGetter(AsioService& asio_service,
                       routing::Routing& routing,
                       const std::vector<passport::PublicPmid>& public_pmids_from_file)
    : get_timer_(asio_service),
      get_versions_timer_(asio_service),
      get_branch_timer_(asio_service),
      dispatcher_(routing),
      service_(routing)
#ifdef TESTING
      , kAllPmids_(public_pmids_from_file)
#endif
{
#ifndef TESTING
  if (!public_pmids_from_file.empty()) {
    LOG(kError) << "Cannot use fake key getter if TESTING is not defined";
    ThrowError(CommonErrors::invalid_parameter);
  }
#endif
}

template<>
void DataGetter::Get<passport::PublicPmid>(const typename passport::PublicPmid::Name& data_name,
                                           const GetFunctor& response_functor,
                                           const std::chrono::steady_clock::duration& timeout) {
#ifdef TESTING
  if (kAllPmids_.empty()) {
#endif
    typedef DataGetterService::GetResponse::Contents ResponseContents;
    auto op_data(std::make_shared<nfs::OpData<ResponseContents>>(1, response_functor));
    auto task_id(get_timer_.AddTask(
        timeout,
        [op_data](ResponseContents get_response) {
            op_data->HandleResponseContents(std::move(get_response));
        },
        // TODO(Fraser#5#): 2013-08-18 - Confirm expected count
        routing::Parameters::node_group_size * 2));
    dispatcher_.SendGetRequest<passport::PublicPmid>(task_id, data_name);
#ifdef TESTING
  } else {
    auto itr(std::find_if(std::begin(kAllPmids_), std::end(kAllPmids_),
        [&data_name](const passport::PublicPmid& pmid) { return pmid.name() == data_name; }));
    if (itr == kAllPmids_.end()) {
      DataNameAndReturnCode data_name_and_return_code;
      data_name_and_return_code.name = nfs_vault::DataName(data_name);
      data_name_and_return_code.return_code = ReturnCode(NfsErrors::failed_to_get_data);
      DataOrDataNameAndReturnCode response(data_name_and_return_code);
      return response_functor(response);
    }
    DataOrDataNameAndReturnCode response(*itr);
    response_functor(response);
  }
#endif
}

}  // namespace nfs_client

}  // namespace maidsafe
