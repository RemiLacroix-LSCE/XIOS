/*!
   \file client_server_mapping.hpp
   \author Ha NGUYEN
   \since 27 Feb 2015
   \date 16 Mars 2016

   \brief Mapping between index client and server.
   Clients pre-calculate all information of server distribution.
 */
#include "client_server_mapping_distributed.hpp"
#include <limits>
#include <boost/functional/hash.hpp>
#include "utils.hpp"
#include "mpi_tag.hpp"
#include "context.hpp"
#include "context_client.hpp"

namespace xios
{

CClientServerMappingDistributed::CClientServerMappingDistributed(const boost::unordered_map<size_t,int>& globalIndexOfServer,
                                                                 const MPI_Comm& clientIntraComm, bool isDataDistributed)
  : CClientServerMapping(), ccDHT_(0)
{
  ccDHT_ = new CClientClientDHTInt(globalIndexOfServer,
                                   clientIntraComm);
}

CClientServerMappingDistributed::~CClientServerMappingDistributed()
{
  if (0 != ccDHT_) delete ccDHT_;
}

/*!
   Compute mapping global index of server which client sends to.
   \param [in] globalIndexOnClient global index client has
*/
void CClientServerMappingDistributed::computeServerIndexMapping(const CArray<size_t,1>& globalIndexOnClient)
{
  CContext* context=CContext::getCurrent() ;
//  CContextClient* client=context->client ;
  // For now the assumption is that secondary server pools consist of the same number of procs.
  // CHANGE the line below if the assumption changes.
  CContextClient* client = (0 != context->clientPrimServer.size()) ? context->clientPrimServer[0] : context->client;
  int nbServer=client->serverSize;

  ccDHT_->computeIndexInfoMapping(globalIndexOnClient);
  const CClientClientDHTInt::Index2VectorInfoTypeMap& infoIndexMap = (ccDHT_->getInfoIndexMap());
  CClientClientDHTInt::Index2VectorInfoTypeMap::const_iterator itb = infoIndexMap.begin(), ite = infoIndexMap.end(), it;
  std::vector<size_t> nbInfoIndex(std::max(ccDHT_->getNbClient(),nbServer),0);

  for (it = itb; it != ite; ++it)
  {
    ++nbInfoIndex[it->second[0]];
  }

  for (int idx = 0; idx < nbInfoIndex.size(); ++idx)
  {
    if (0 != nbInfoIndex[idx])
    {
      indexGlobalOnServer_[idx].resize(nbInfoIndex[idx]);
      nbInfoIndex[idx] = 0;
    }
  }

  for (it = itb; it != ite; ++it)
  {
    indexGlobalOnServer_[it->second[0]][nbInfoIndex[it->second[0]]] = (it->first);
    ++nbInfoIndex[it->second[0]];
  }
}

}
