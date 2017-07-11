/*!
   \file client_client_dht_template_impl.hpp
   \author Ha NGUYEN
   \since 05 Oct 2015
   \date 23 Mars 2016

   \brief Distributed hashed table implementation.
 */
#include "client_client_dht_template.hpp"
#include "utils.hpp"
#include "mpi_tag.hpp"
#ifdef _usingEP
#include "ep_declaration.hpp"
#include "ep_lib.hpp"
#endif


namespace xios
{
template<typename T, typename H>
CClientClientDHTTemplate<T,H>::CClientClientDHTTemplate(const ep_lib::MPI_Comm& clientIntraComm)
  : H(clientIntraComm), index2InfoMapping_(), indexToInfoMappingLevel_(), nbClient_(0)
{
  MPI_Comm_size(clientIntraComm, &nbClient_);
  this->computeMPICommLevel();
  int nbLvl = this->getNbLevel();
  sendRank_.resize(nbLvl);
  recvRank_.resize(nbLvl);
}

/*!
  Constructor with initial distribution information and the corresponding index
  Each client (process) holds a piece of information as well as the attached index, the index
will be redistributed (projected) into size_t space as long as the associated information.
  \param [in] indexInfoMap initial index and information mapping
  \param [in] clientIntraComm communicator of clients
  \param [in] hierarLvl level of hierarchy
*/
template<typename T, typename H>
CClientClientDHTTemplate<T,H>::CClientClientDHTTemplate(const Index2InfoTypeMap& indexInfoMap,
                                                        const ep_lib::MPI_Comm& clientIntraComm)
  : H(clientIntraComm), index2InfoMapping_(), indexToInfoMappingLevel_(), nbClient_(0)
{
  MPI_Comm_size(clientIntraComm, &nbClient_);
  this->computeMPICommLevel();
  int nbLvl = this->getNbLevel();
  sendRank_.resize(nbLvl);
  recvRank_.resize(nbLvl);
  Index2VectorInfoTypeMap indexToVecInfoMap;
  indexToVecInfoMap.rehash(std::ceil(indexInfoMap.size()/indexToVecInfoMap.max_load_factor()));
  typename Index2InfoTypeMap::const_iterator it = indexInfoMap.begin(), ite = indexInfoMap.end();
  for (; it != ite; ++it) indexToVecInfoMap[it->first].push_back(it->second);
  computeDistributedIndex(indexToVecInfoMap, clientIntraComm, nbLvl-1);
}

/*!
  Constructor with initial distribution information and the corresponding index
  Each client (process) holds a piece of information as well as the attached index, the index
will be redistributed (projected) into size_t space as long as the associated information.
  \param [in] indexInfoMap initial index and information mapping
  \param [in] clientIntraComm communicator of clients
  \param [in] hierarLvl level of hierarchy
*/
template<typename T, typename H>
CClientClientDHTTemplate<T,H>::CClientClientDHTTemplate(const Index2VectorInfoTypeMap& indexInfoMap,
                                                        const ep_lib::MPI_Comm& clientIntraComm)
  : H(clientIntraComm), index2InfoMapping_(), indexToInfoMappingLevel_(), nbClient_(0)
{
  MPI_Comm_size(clientIntraComm, &nbClient_);
  this->computeMPICommLevel();
  int nbLvl = this->getNbLevel();
  sendRank_.resize(nbLvl);
  recvRank_.resize(nbLvl);
  computeDistributedIndex(indexInfoMap, clientIntraComm, nbLvl-1);
}

template<typename T, typename H>
CClientClientDHTTemplate<T,H>::~CClientClientDHTTemplate()
{
}

/*!
  Compute mapping between indices and information corresponding to these indices
  \param [in] indices indices a proc has
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::computeIndexInfoMapping(const CArray<size_t,1>& indices)
{
  int nbLvl = this->getNbLevel();
  computeIndexInfoMappingLevel(indices, this->internalComm_, nbLvl-1);
}

/*!
    Compute mapping between indices and information corresponding to these indices
for each level of hierarchical DHT. Recursive function
   \param [in] indices indices a proc has
   \param [in] commLevel communicator of current level
   \param [in] level current level
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::computeIndexInfoMappingLevel(const CArray<size_t,1>& indices,
                                                                 const ep_lib::MPI_Comm& commLevel,
                                                                 int level)
{
  int clientRank;
  MPI_Comm_rank(commLevel,&clientRank);
  ep_lib::MPI_Barrier(commLevel);
  int groupRankBegin = this->getGroupBegin()[level];
  int nbClient = this->getNbInGroup()[level];
  std::vector<size_t> hashedIndex;
  computeHashIndex(hashedIndex, nbClient);

  size_t ssize = indices.numElements(), hashedVal;

  std::vector<size_t>::const_iterator itbClientHash = hashedIndex.begin(), itClientHash,
                                      iteClientHash = hashedIndex.end();
  std::vector<int> sendBuff(nbClient,0);
  std::vector<int> sendNbIndexBuff(nbClient,0);

  // Number of global index whose mapping server are on other clients
  int nbIndexToSend = 0;
  size_t index;
  HashXIOS<size_t> hashGlobalIndex;
  boost::unordered_map<size_t,int> nbIndices;
  nbIndices.rehash(std::ceil(ssize/nbIndices.max_load_factor()));
  for (int i = 0; i < ssize; ++i)
  {
    index = indices(i);
    if (0 == nbIndices.count(index))
    {
      hashedVal  = hashGlobalIndex(index);
      itClientHash = std::upper_bound(itbClientHash, iteClientHash, hashedVal);
      int indexClient = std::distance(itbClientHash, itClientHash)-1;
      ++sendNbIndexBuff[indexClient];
      nbIndices[index] = 1;
    }
  }

  boost::unordered_map<int, size_t* > client2ClientIndex;
  for (int idx = 0; idx < nbClient; ++idx)
  {
    if (0 != sendNbIndexBuff[idx])
    {
      client2ClientIndex[idx+groupRankBegin] = new unsigned long [sendNbIndexBuff[idx]];
      nbIndexToSend += sendNbIndexBuff[idx];
      sendBuff[idx] = 1;
      sendNbIndexBuff[idx] = 0;
    }
  }

  for (int i = 0; i < ssize; ++i)
  {
    index = indices(i);
    if (1 == nbIndices[index])
    {
      hashedVal  = hashGlobalIndex(index);
      itClientHash = std::upper_bound(itbClientHash, iteClientHash, hashedVal);
      int indexClient = std::distance(itbClientHash, itClientHash)-1;
      client2ClientIndex[indexClient+groupRankBegin][sendNbIndexBuff[indexClient]] = index;
      ++sendNbIndexBuff[indexClient];
      ++nbIndices[index];
    }
  }

  std::vector<int> recvRankClient, recvNbIndexClientCount;
  sendRecvRank(level, sendBuff, sendNbIndexBuff,
               recvRankClient, recvNbIndexClientCount);

  int recvNbIndexCount = 0;
  for (int idx = 0; idx < recvNbIndexClientCount.size(); ++idx)
    recvNbIndexCount += recvNbIndexClientCount[idx];

  unsigned long* recvIndexBuff;
  if (0 != recvNbIndexCount)
    recvIndexBuff = new unsigned long[recvNbIndexCount];

  int request_size = 0;

  int currentIndex = 0;
  int nbRecvClient = recvRankClient.size();

  int position = 0;

  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != recvNbIndexClientCount[idx])
    {
      request_size++;
    }
  }

  request_size += client2ClientIndex.size();


  std::vector<ep_lib::MPI_Request> request(request_size);

  std::vector<int>::iterator itbRecvIndex = recvRankClient.begin(), itRecvIndex,
                             iteRecvIndex = recvRankClient.end(),
                           itbRecvNbIndex = recvNbIndexClientCount.begin(),
                           itRecvNbIndex;
  
  
  boost::unordered_map<int, size_t* >::iterator itbIndex = client2ClientIndex.begin(), itIndex,
                                                iteIndex = client2ClientIndex.end();
  for (itIndex = itbIndex; itIndex != iteIndex; ++itIndex)
  {
    MPI_Isend(itIndex->second, sendNbIndexBuff[itIndex->first-groupRankBegin], MPI_UNSIGNED_LONG, itIndex->first, MPI_DHT_INDEX, commLevel, &request[position]);
    position++;
    //sendIndexToClients(itIndex->first, (itIndex->second), sendNbIndexBuff[itIndex->first-groupRankBegin], commLevel, request);
  }
    
  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != recvNbIndexClientCount[idx])
    {
      MPI_Irecv(recvIndexBuff+currentIndex, recvNbIndexClientCount[idx], MPI_UNSIGNED_LONG,
            recvRankClient[idx], MPI_DHT_INDEX, commLevel, &request[position]);
      position++;
      //recvIndexFromClients(recvRankClient[idx], recvIndexBuff+currentIndex, recvNbIndexClientCount[idx], commLevel, request);
    }
    currentIndex += recvNbIndexClientCount[idx];
  }

  
  std::vector<ep_lib::MPI_Status> status(request_size);
  MPI_Waitall(request.size(), &request[0], &status[0]);


  CArray<size_t,1>* tmpGlobalIndex;
  if (0 != recvNbIndexCount)
    tmpGlobalIndex = new CArray<size_t,1>(recvIndexBuff, shape(recvNbIndexCount), neverDeleteData);
  else
    tmpGlobalIndex = new CArray<size_t,1>();

  // OK, we go to the next level and do something recursive
  if (0 < level)
  {
    --level;
    computeIndexInfoMappingLevel(*tmpGlobalIndex, this->internalComm_, level);
      
  }
  else // Now, we are in the last level where necessary mappings are.
    indexToInfoMappingLevel_= (index2InfoMapping_);

  typename Index2VectorInfoTypeMap::const_iterator iteIndexToInfoMap = indexToInfoMappingLevel_.end(), itIndexToInfoMap;
  std::vector<int> sendNbIndexOnReturn(nbRecvClient,0);
  currentIndex = 0;
  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    for (int i = 0; i < recvNbIndexClientCount[idx]; ++i)
    {
      itIndexToInfoMap = indexToInfoMappingLevel_.find(*(recvIndexBuff+currentIndex+i));
      if (iteIndexToInfoMap != itIndexToInfoMap)
        sendNbIndexOnReturn[idx] += itIndexToInfoMap->second.size();
    }
    currentIndex += recvNbIndexClientCount[idx];
  }

  std::vector<int> recvRankOnReturn(client2ClientIndex.size());
  std::vector<int> recvNbIndexOnReturn(client2ClientIndex.size(),0);
  int indexIndex = 0;
  for (itIndex = itbIndex; itIndex != iteIndex; ++itIndex, ++indexIndex)
  {
    recvRankOnReturn[indexIndex] = itIndex->first;
  }
  sendRecvOnReturn(recvRankClient, sendNbIndexOnReturn,
                   recvRankOnReturn, recvNbIndexOnReturn);

  int recvNbIndexCountOnReturn = 0;
  for (int idx = 0; idx < recvRankOnReturn.size(); ++idx)
    recvNbIndexCountOnReturn += recvNbIndexOnReturn[idx];

  unsigned long* recvIndexBuffOnReturn;
  unsigned char* recvInfoBuffOnReturn;
  if (0 != recvNbIndexCountOnReturn)
  {
    recvIndexBuffOnReturn = new unsigned long[recvNbIndexCountOnReturn];
    recvInfoBuffOnReturn = new unsigned char[recvNbIndexCountOnReturn*ProcessDHTElement<InfoType>::typeSize()];
  }

  request_size = 0;
  for (int idx = 0; idx < recvRankOnReturn.size(); ++idx)
  {
    if (0 != recvNbIndexOnReturn[idx])
    {
      request_size += 2;
    }
  }

  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != sendNbIndexOnReturn[idx])
    {
      request_size += 2;
    }
  }

  std::vector<ep_lib::MPI_Request> requestOnReturn(request_size);
  currentIndex = 0;
  position = 0;
  for (int idx = 0; idx < recvRankOnReturn.size(); ++idx)
  {
    if (0 != recvNbIndexOnReturn[idx])
    {
      //recvIndexFromClients(recvRankOnReturn[idx], recvIndexBuffOnReturn+currentIndex, recvNbIndexOnReturn[idx], commLevel, requestOnReturn);
      MPI_Irecv(recvIndexBuffOnReturn+currentIndex, recvNbIndexOnReturn[idx], MPI_UNSIGNED_LONG,
            recvRankOnReturn[idx], MPI_DHT_INDEX, commLevel, &requestOnReturn[position]);
      position++;
      //recvInfoFromClients(recvRankOnReturn[idx],
      //                    recvInfoBuffOnReturn+currentIndex*ProcessDHTElement<InfoType>::typeSize(),
      //                    recvNbIndexOnReturn[idx]*ProcessDHTElement<InfoType>::typeSize(),
      //                    commLevel, requestOnReturn);
      MPI_Irecv(recvInfoBuffOnReturn+currentIndex*ProcessDHTElement<InfoType>::typeSize(), 
                recvNbIndexOnReturn[idx]*ProcessDHTElement<InfoType>::typeSize(), MPI_CHAR,
                recvRankOnReturn[idx], MPI_DHT_INFO, commLevel, &requestOnReturn[position]);
      position++;
    }
    currentIndex += recvNbIndexOnReturn[idx];
  }

  boost::unordered_map<int,unsigned char*> client2ClientInfoOnReturn;
  boost::unordered_map<int,size_t*> client2ClientIndexOnReturn;
  currentIndex = 0;
  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != sendNbIndexOnReturn[idx])
    {
      int rank = recvRankClient[idx];
      client2ClientIndexOnReturn[rank] = new unsigned long [sendNbIndexOnReturn[idx]];
      client2ClientInfoOnReturn[rank] = new unsigned char [sendNbIndexOnReturn[idx]*ProcessDHTElement<InfoType>::typeSize()];
      unsigned char* tmpInfoPtr = client2ClientInfoOnReturn[rank];
      int infoIndex = 0;
      int nb = 0;
      for (int i = 0; i < recvNbIndexClientCount[idx]; ++i)
      {
        itIndexToInfoMap = indexToInfoMappingLevel_.find(*(recvIndexBuff+currentIndex+i));
        if (iteIndexToInfoMap != itIndexToInfoMap)
        {
          const std::vector<InfoType>& infoTmp = itIndexToInfoMap->second;
          for (int k = 0; k < infoTmp.size(); ++k)
          {
            client2ClientIndexOnReturn[rank][nb] = itIndexToInfoMap->first;
            ProcessDHTElement<InfoType>::packElement(infoTmp[k], tmpInfoPtr, infoIndex);
            ++nb;
          }
        }
      }

      //sendIndexToClients(rank, client2ClientIndexOnReturn[rank],
      //                   sendNbIndexOnReturn[idx], commLevel, requestOnReturn);
      MPI_Isend(client2ClientIndexOnReturn[rank], sendNbIndexOnReturn[idx], MPI_UNSIGNED_LONG,
            rank, MPI_DHT_INDEX, commLevel, &requestOnReturn[position]);
      position++;
      //sendInfoToClients(rank, client2ClientInfoOnReturn[rank],
      //                  sendNbIndexOnReturn[idx]*ProcessDHTElement<InfoType>::typeSize(), commLevel, requestOnReturn);
      MPI_Isend(client2ClientInfoOnReturn[rank], sendNbIndexOnReturn[idx]*ProcessDHTElement<InfoType>::typeSize(), MPI_CHAR,
            rank, MPI_DHT_INFO, commLevel, &requestOnReturn[position]);
      position++;
    }
    currentIndex += recvNbIndexClientCount[idx];
  }

  std::vector<ep_lib::MPI_Status> statusOnReturn(requestOnReturn.size());
  MPI_Waitall(requestOnReturn.size(), &requestOnReturn[0], &statusOnReturn[0]);

  Index2VectorInfoTypeMap indexToInfoMapping;
  indexToInfoMapping.rehash(std::ceil(recvNbIndexCountOnReturn/indexToInfoMapping.max_load_factor()));
  int infoIndex = 0;
  InfoType unpackedInfo;
  for (int idx = 0; idx < recvNbIndexCountOnReturn; ++idx)
  {
    ProcessDHTElement<InfoType>::unpackElement(unpackedInfo, recvInfoBuffOnReturn, infoIndex);
    indexToInfoMapping[recvIndexBuffOnReturn[idx]].push_back(unpackedInfo);
  }

  indexToInfoMappingLevel_.swap(indexToInfoMapping);
  if (0 != recvNbIndexCount) delete [] recvIndexBuff;
  for (boost::unordered_map<int,size_t*>::const_iterator it = client2ClientIndex.begin();
                                                        it != client2ClientIndex.end(); ++it)
      delete [] it->second;
  delete tmpGlobalIndex;

  if (0 != recvNbIndexCountOnReturn)
  {
    delete [] recvIndexBuffOnReturn;
    delete [] recvInfoBuffOnReturn;
  }

  for (boost::unordered_map<int,unsigned char*>::const_iterator it = client2ClientInfoOnReturn.begin();
                                                               it != client2ClientInfoOnReturn.end(); ++it)
      delete [] it->second;

  for (boost::unordered_map<int,size_t*>::const_iterator it = client2ClientIndexOnReturn.begin();
                                            it != client2ClientIndexOnReturn.end(); ++it)
      delete [] it->second;
}

/*!
  Compute the hash index distribution of whole size_t space then each client will have a range of this distribution
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::computeHashIndex(std::vector<size_t>& hashedIndex, int nbClient)
{
  // Compute range of hash index for each client
  hashedIndex.resize(nbClient+1);
  size_t nbHashIndexMax = std::numeric_limits<size_t>::max();
  size_t nbHashIndex;
  hashedIndex[0] = 0;
  for (int i = 1; i < nbClient; ++i)
  {
    nbHashIndex = nbHashIndexMax / nbClient;
    if (i < (nbHashIndexMax%nbClient)) ++nbHashIndex;
    hashedIndex[i] = hashedIndex[i-1] + nbHashIndex;
  }
  hashedIndex[nbClient] = nbHashIndexMax;
}

/*!
  Compute distribution of global index for servers
  Each client already holds a piece of information and its associated index.
This information will be redistributed among processes by projecting indices into size_t space,
the corresponding information will be also distributed on size_t space.
After the redistribution, each client holds rearranged index and its corresponding information.
  \param [in] indexInfoMap index and its corresponding info (usually server index)
  \param [in] commLevel communicator of current level
  \param [in] level current level
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::computeDistributedIndex(const Index2VectorInfoTypeMap& indexInfoMap,
                                                            const ep_lib::MPI_Comm& commLevel,
                                                            int level)
{
  int clientRank;
  MPI_Comm_rank(commLevel,&clientRank);
  computeSendRecvRank(level, clientRank);
  ep_lib::MPI_Barrier(commLevel);

  int groupRankBegin = this->getGroupBegin()[level];
  int nbClient = this->getNbInGroup()[level];
  std::vector<size_t> hashedIndex;
  computeHashIndex(hashedIndex, nbClient);

  std::vector<int> sendBuff(nbClient,0);
  std::vector<int> sendNbIndexBuff(nbClient,0);
  std::vector<size_t>::const_iterator itbClientHash = hashedIndex.begin(), itClientHash,
                                      iteClientHash = hashedIndex.end();
  typename Index2VectorInfoTypeMap::const_iterator itb = indexInfoMap.begin(),it,
                                                   ite = indexInfoMap.end();
  HashXIOS<size_t> hashGlobalIndex;

  // Compute size of sending and receving buffer
  for (it = itb; it != ite; ++it)
  {
    size_t hashIndex = hashGlobalIndex(it->first);
    itClientHash = std::upper_bound(itbClientHash, iteClientHash, hashIndex);
    int indexClient = std::distance(itbClientHash, itClientHash)-1;
    sendNbIndexBuff[indexClient] += it->second.size();
  }

  boost::unordered_map<int, size_t*> client2ClientIndex;
  boost::unordered_map<int, unsigned char*> client2ClientInfo;
  for (int idx = 0; idx < nbClient; ++idx)
  {
    if (0 != sendNbIndexBuff[idx])
    {
      client2ClientIndex[idx+groupRankBegin] = new unsigned long [sendNbIndexBuff[idx]];
      client2ClientInfo[idx+groupRankBegin] = new unsigned char [sendNbIndexBuff[idx]*ProcessDHTElement<InfoType>::typeSize()];
      sendNbIndexBuff[idx] = 0;
      sendBuff[idx] = 1;
    }
  }

  std::vector<int> sendNbInfo(nbClient,0);
  for (it = itb; it != ite; ++it)
  {
    const std::vector<InfoType>& infoTmp = it->second;
    size_t hashIndex = hashGlobalIndex(it->first);
    itClientHash = std::upper_bound(itbClientHash, iteClientHash, hashIndex);
    int indexClient = std::distance(itbClientHash, itClientHash)-1;
    for (int idx = 0; idx < infoTmp.size(); ++idx)
    {
      client2ClientIndex[indexClient + groupRankBegin][sendNbIndexBuff[indexClient]] = it->first;;
      ProcessDHTElement<InfoType>::packElement(infoTmp[idx], client2ClientInfo[indexClient + groupRankBegin], sendNbInfo[indexClient]);
      ++sendNbIndexBuff[indexClient];
    }
  }

  // Calculate from how many clients each client receive message.
  // Calculate size of buffer for receiving message
  std::vector<int> recvRankClient, recvNbIndexClientCount;
  sendRecvRank(level, sendBuff, sendNbIndexBuff,
               recvRankClient, recvNbIndexClientCount);

  int recvNbIndexCount = 0;
  for (int idx = 0; idx < recvNbIndexClientCount.size(); ++idx)
  { 
    recvNbIndexCount += recvNbIndexClientCount[idx];
  }

  unsigned long* recvIndexBuff;
  unsigned char* recvInfoBuff;
  if (0 != recvNbIndexCount)
  {
    recvIndexBuff = new unsigned long[recvNbIndexCount];
    recvInfoBuff = new unsigned char[recvNbIndexCount*ProcessDHTElement<InfoType>::typeSize()];
  }

  // If a client holds information about index and the corresponding which don't belong to it,
  // it will send a message to the correct clients.
  // Contents of the message are index and its corresponding informatioin
  int request_size = 0;  
  int currentIndex = 0;
  int nbRecvClient = recvRankClient.size();
  int current_pos = 0; 

  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != recvNbIndexClientCount[idx])
    {
      request_size += 2;
    }
    //currentIndex += recvNbIndexClientCount[idx];
  }

  request_size += client2ClientIndex.size();
  request_size += client2ClientInfo.size();



  std::vector<ep_lib::MPI_Request> request(request_size);
  
  //unsigned long* tmp_send_buf_long[client2ClientIndex.size()];
  //unsigned char* tmp_send_buf_char[client2ClientInfo.size()];
  
  int info_position = 0;
  int index_position = 0;


  boost::unordered_map<int, size_t* >::iterator itbIndex = client2ClientIndex.begin(), itIndex,
                                                iteIndex = client2ClientIndex.end();
  for (itIndex = itbIndex; itIndex != iteIndex; ++itIndex)
  {
    //sendIndexToClients(itIndex->first, itIndex->second, sendNbIndexBuff[itIndex->first-groupRankBegin], commLevel, request);

    //tmp_send_buf_long[index_position] = new unsigned long[sendNbIndexBuff[itIndex->first-groupRankBegin]];
    //for(int i=0; i<sendNbIndexBuff[itIndex->first-groupRankBegin]; i++)
    //{
    //  tmp_send_buf_long[index_position][i] = (static_cast<unsigned long * >(itIndex->second))[i];
    //}
    //MPI_Isend(tmp_send_buf_long[current_pos], sendNbIndexBuff[itIndex->first-groupRankBegin], MPI_UNSIGNED_LONG,
    MPI_Isend(itIndex->second, sendNbIndexBuff[itIndex->first-groupRankBegin], MPI_UNSIGNED_LONG,
              itIndex->first, MPI_DHT_INDEX, commLevel, &request[current_pos]);
    current_pos++; 
    index_position++;

  }

  boost::unordered_map<int, unsigned char*>::iterator itbInfo = client2ClientInfo.begin(), itInfo,
                                                      iteInfo = client2ClientInfo.end();
  for (itInfo = itbInfo; itInfo != iteInfo; ++itInfo)
  {
    //sendInfoToClients(itInfo->first, itInfo->second, sendNbInfo[itInfo->first-groupRankBegin], commLevel, request);

    //tmp_send_buf_char[info_position] = new unsigned char[sendNbInfo[itInfo->first-groupRankBegin]];
    //for(int i=0; i<sendNbInfo[itInfo->first-groupRankBegin]; i++)
    //{
    //  tmp_send_buf_char[info_position][i] = (static_cast<unsigned char * >(itInfo->second))[i];
    //}

    MPI_Isend(itInfo->second, sendNbInfo[itInfo->first-groupRankBegin], MPI_CHAR,
              itInfo->first, MPI_DHT_INFO, commLevel, &request[current_pos]);

    current_pos++;
    info_position++;
  }
  
  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    if (0 != recvNbIndexClientCount[idx])
    {
      //recvIndexFromClients(recvRankClient[idx], recvIndexBuff+currentIndex, recvNbIndexClientCount[idx], commLevel, request);
      MPI_Irecv(recvIndexBuff+currentIndex, recvNbIndexClientCount[idx], MPI_UNSIGNED_LONG,
                recvRankClient[idx], MPI_DHT_INDEX, commLevel, &request[current_pos]);
      current_pos++;
      
      
      MPI_Irecv(recvInfoBuff+currentIndex*ProcessDHTElement<InfoType>::typeSize(), 
                recvNbIndexClientCount[idx]*ProcessDHTElement<InfoType>::typeSize(), 
                MPI_CHAR, recvRankClient[idx], MPI_DHT_INFO, commLevel, &request[current_pos]);
      
      current_pos++;
      


      // recvInfoFromClients(recvRankClient[idx],
      //                     recvInfoBuff+currentIndex*ProcessDHTElement<InfoType>::typeSize(),
      //                     recvNbIndexClientCount[idx]*ProcessDHTElement<InfoType>::typeSize(),
      //                     commLevel, request);
    }
    currentIndex += recvNbIndexClientCount[idx];
  }

  std::vector<ep_lib::MPI_Status> status(request.size());
  
  MPI_Waitall(request.size(), &request[0], &status[0]);
  
 
  //for(int i=0; i<client2ClientInfo.size(); i++)
  //  delete[] tmp_send_buf_char[i];

  

  //for(int i=0; i<client2ClientIndex.size(); i++)
  //  delete[] tmp_send_buf_long[i];


  Index2VectorInfoTypeMap indexToInfoMapping;
  indexToInfoMapping.rehash(std::ceil(currentIndex/indexToInfoMapping.max_load_factor()));
  currentIndex = 0;
  InfoType infoValue;
  int infoIndex = 0;
  unsigned char* infoBuff = recvInfoBuff;
  for (int idx = 0; idx < nbRecvClient; ++idx)
  {
    size_t index;
    int count = recvNbIndexClientCount[idx];
    for (int i = 0; i < count; ++i)
    {
      ProcessDHTElement<InfoType>::unpackElement(infoValue, infoBuff, infoIndex);
      indexToInfoMapping[*(recvIndexBuff+currentIndex+i)].push_back(infoValue);
    }
    currentIndex += count;
  }

  if (0 != recvNbIndexCount)
  {
    delete [] recvIndexBuff;
    delete [] recvInfoBuff;
  }
  for (boost::unordered_map<int,unsigned char*>::const_iterator it = client2ClientInfo.begin();
                                                               it != client2ClientInfo.end(); ++it)
      delete [] it->second;

  for (boost::unordered_map<int,size_t*>::const_iterator it = client2ClientIndex.begin();
                                                        it != client2ClientIndex.end(); ++it)
      delete [] it->second;

  // Ok, now do something recursive
  if (0 < level)
  {
    --level;
    computeDistributedIndex(indexToInfoMapping, this->internalComm_, level);
  }
  else
    index2InfoMapping_.swap(indexToInfoMapping);
  
}

/*!
  Send message containing index to clients
  \param [in] clientDestRank rank of destination client
  \param [in] indices index to send
  \param [in] indiceSize size of index array to send
  \param [in] clientIntraComm communication group of client
  \param [in] requestSendIndex list of sending request
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::sendIndexToClients(int clientDestRank, size_t* indices, size_t indiceSize,
                                                       const ep_lib::MPI_Comm& clientIntraComm,
                                                       std::vector<ep_lib::MPI_Request>& requestSendIndex)
{
  printf("should not call this function sendIndexToClients");  
  ep_lib::MPI_Request request;
  requestSendIndex.push_back(request);
  MPI_Isend(indices, indiceSize, MPI_UNSIGNED_LONG,
            clientDestRank, MPI_DHT_INDEX, clientIntraComm, &(requestSendIndex.back()));
}

/*!
  Receive message containing index to clients
  \param [in] clientDestRank rank of destination client
  \param [in] indices index to send
  \param [in] clientIntraComm communication group of client
  \param [in] requestRecvIndex list of receiving request
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::recvIndexFromClients(int clientSrcRank, size_t* indices, size_t indiceSize,
                                                         const ep_lib::MPI_Comm& clientIntraComm,
                                                         std::vector<ep_lib::MPI_Request>& requestRecvIndex)
{
  printf("should not call this function recvIndexFromClients");
  ep_lib::MPI_Request request;
  requestRecvIndex.push_back(request);
  MPI_Irecv(indices, indiceSize, MPI_UNSIGNED_LONG,
            clientSrcRank, MPI_DHT_INDEX, clientIntraComm, &(requestRecvIndex.back()));
}

/*!
  Send message containing information to clients
  \param [in] clientDestRank rank of destination client
  \param [in] info info array to send
  \param [in] infoSize info array size to send
  \param [in] clientIntraComm communication group of client
  \param [in] requestSendInfo list of sending request
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::sendInfoToClients(int clientDestRank, unsigned char* info, int infoSize,
                                                      const ep_lib::MPI_Comm& clientIntraComm,
                                                      std::vector<ep_lib::MPI_Request>& requestSendInfo)
{
  printf("should not call this function sendInfoToClients");
  ep_lib::MPI_Request request;
  requestSendInfo.push_back(request);
  MPI_Isend(info, infoSize, MPI_CHAR,
            clientDestRank, MPI_DHT_INFO, clientIntraComm, &(requestSendInfo.back()));
}

/*!
  Receive message containing information from other clients
  \param [in] clientDestRank rank of destination client
  \param [in] info info array to receive
  \param [in] infoSize info array size to receive
  \param [in] clientIntraComm communication group of client
  \param [in] requestRecvInfo list of sending request
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::recvInfoFromClients(int clientSrcRank, unsigned char* info, int infoSize,
                                                        const ep_lib::MPI_Comm& clientIntraComm,
                                                        std::vector<ep_lib::MPI_Request>& requestRecvInfo)
{
  printf("should not call this function recvInfoFromClients\n");
  ep_lib::MPI_Request request;
  requestRecvInfo.push_back(request);

  MPI_Irecv(info, infoSize, MPI_CHAR,
            clientSrcRank, MPI_DHT_INFO, clientIntraComm, &(requestRecvInfo.back()));
}

/*!
  Compute how many processes one process needs to send to and from how many processes it will receive.
  This computation is only based on hierachical structure of distributed hash table (e.x: number of processes)
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::computeSendRecvRank(int level, int rank)
{
  int groupBegin = this->getGroupBegin()[level];
  int nbInGroup  = this->getNbInGroup()[level];
  const std::vector<int>& groupParentBegin = this->getGroupParentsBegin()[level];
  const std::vector<int>& nbInGroupParents = this->getNbInGroupParents()[level];

  std::vector<size_t> hashedIndexGroup;
  computeHashIndex(hashedIndexGroup, nbInGroup);
  size_t a = hashedIndexGroup[rank-groupBegin];
  size_t b = hashedIndexGroup[rank-groupBegin+1]-1;

  int currentGroup, offset;
  size_t e,f;

  // Do a simple math [a,b) intersect [c,d)
  for (int idx = 0; idx < groupParentBegin.size(); ++idx)
  {
    std::vector<size_t> hashedIndexGroupParent;
    int nbInGroupParent = nbInGroupParents[idx];
    if (0 != nbInGroupParent)
      computeHashIndex(hashedIndexGroupParent, nbInGroupParent);
    for (int i = 0; i < nbInGroupParent; ++i)
    {
      size_t c = hashedIndexGroupParent[i];
      size_t d = hashedIndexGroupParent[i+1]-1;

    if (!((d < a) || (b <c)))
        recvRank_[level].push_back(groupParentBegin[idx]+i);
    }

    offset = rank - groupParentBegin[idx];
    if ((offset<nbInGroupParents[idx]) && (0 <= offset))
    {
      e = hashedIndexGroupParent[offset];
      f = hashedIndexGroupParent[offset+1]-1;
    }
  }

  std::vector<size_t>::const_iterator itbHashGroup = hashedIndexGroup.begin(), itHashGroup,
                                      iteHashGroup = hashedIndexGroup.end();
  itHashGroup = std::lower_bound(itbHashGroup, iteHashGroup, e+1);
  int begin = std::distance(itbHashGroup, itHashGroup)-1;
  itHashGroup = std::upper_bound(itbHashGroup, iteHashGroup, f);
  int end = std::distance(itbHashGroup, itHashGroup) -1;
  sendRank_[level].resize(end-begin+1);
  for (int idx = 0; idx < sendRank_[level].size(); ++idx) sendRank_[level][idx] = idx + groupBegin + begin;
}

/*!
  Compute number of clients as well as corresponding number of elements each client will receive on returning searching result
  \param [in] sendNbRank Rank of clients to send to
  \param [in] sendNbElements Number of elements each client to send to
  \param [in] receiveNbRank Rank of clients to receive from
  \param [out] recvNbElements Number of elements each client to send to
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::sendRecvOnReturn(const std::vector<int>& sendNbRank, std::vector<int>& sendNbElements,
                                                     const std::vector<int>& recvNbRank, std::vector<int>& recvNbElements)
{
  recvNbElements.resize(recvNbRank.size());
  std::vector<ep_lib::MPI_Request> request(sendNbRank.size()+recvNbRank.size());
  std::vector<ep_lib::MPI_Status> requestStatus(sendNbRank.size()+recvNbRank.size());

  int nRequest = 0;
  

  for (int idx = 0; idx < sendNbRank.size(); ++idx)
  {
    MPI_Isend(&sendNbElements[0]+idx, 1, MPI_INT,
              sendNbRank[idx], MPI_DHT_INDEX_1, this->internalComm_, &request[nRequest]);
    ++nRequest;
  }
  
  for (int idx = 0; idx < recvNbRank.size(); ++idx)
  {
    MPI_Irecv(&recvNbElements[0]+idx, 1, MPI_INT,
              recvNbRank[idx], MPI_DHT_INDEX_1, this->internalComm_, &request[nRequest]);
    ++nRequest;
  }

  MPI_Waitall(sendNbRank.size()+recvNbRank.size(), &request[0], &requestStatus[0]);
}

/*!
  Send and receive number of process each process need to listen to as well as number
  of index it will receive during the initalization phase
  \param [in] level current level
  \param [in] sendNbRank Rank of clients to send to
  \param [in] sendNbElements Number of elements each client to send to
  \param [out] receiveNbRank Rank of clients to receive from
  \param [out] recvNbElements Number of elements each client to send to
*/
template<typename T, typename H>
void CClientClientDHTTemplate<T,H>::sendRecvRank(int level,
                                                 const std::vector<int>& sendNbRank, const std::vector<int>& sendNbElements,
                                                 std::vector<int>& recvNbRank, std::vector<int>& recvNbElements)
{
  int groupBegin = this->getGroupBegin()[level];

  int offSet = 0;
  std::vector<int>& sendRank = sendRank_[level];
  std::vector<int>& recvRank = recvRank_[level];
  int sendBuffSize = sendRank.size();
  std::vector<int> sendBuff(sendBuffSize*2);
  int recvBuffSize = recvRank.size();
  std::vector<int> recvBuff(recvBuffSize*2,0);

  std::vector<ep_lib::MPI_Request> request(sendBuffSize+recvBuffSize);
  std::vector<ep_lib::MPI_Status> requestStatus(sendBuffSize+recvBuffSize);
  //ep_lib::MPI_Request request[sendBuffSize+recvBuffSize];
  //ep_lib::MPI_Status requestStatus[sendBuffSize+recvBuffSize];
  
  int my_rank;
  MPI_Comm_rank(this->internalComm_, &my_rank);
  
  int nRequest = 0;
  for (int idx = 0; idx < recvBuffSize; ++idx)
  {
    MPI_Irecv(&recvBuff[2*idx], 2, MPI_INT,
              recvRank[idx], MPI_DHT_INDEX_0, this->internalComm_, &request[nRequest]);
    ++nRequest;
  }
  

  for (int idx = 0; idx < sendBuffSize; ++idx)
  {
    offSet = sendRank[idx]-groupBegin;
    sendBuff[idx*2] = sendNbRank[offSet];
    sendBuff[idx*2+1] = sendNbElements[offSet];
  }
  
  

  for (int idx = 0; idx < sendBuffSize; ++idx)
  {
    MPI_Isend(&sendBuff[idx*2], 2, MPI_INT,
              sendRank[idx], MPI_DHT_INDEX_0, this->internalComm_, &request[nRequest]);
    ++nRequest;
  }
  
  

  //MPI_Barrier(this->internalComm_);

  MPI_Waitall(sendBuffSize+recvBuffSize, &request[0], &requestStatus[0]);
  //MPI_Waitall(sendBuffSize+recvBuffSize, request, requestStatus);

  
  int nbRecvRank = 0, nbRecvElements = 0;
  recvNbRank.clear();
  recvNbElements.clear();
  for (int idx = 0; idx < recvBuffSize; ++idx)
  {
    if (0 != recvBuff[2*idx])
    {
      recvNbRank.push_back(recvRank[idx]);
      recvNbElements.push_back(recvBuff[2*idx+1]);
    }
  }


  
  
}

}
