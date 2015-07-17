/*!
   \file distribution_client.cpp
   \author Ha NGUYEN
   \since 13 Jan 2015
   \date 09 Mars 2015

   \brief Index distribution on client side.
 */
#include "distribution_client.hpp"

namespace xios {

CDistributionClient::CDistributionClient(int rank, int dims, CArray<size_t,1>* globalIndex)
   : CDistribution(rank, dims, globalIndex),
   globalDataSendToServer_(0), localDataIndex_(0), localDataIndexSendToServer_(0), localMaskIndex_(0),
   axisDomainOrder_(),
   nLocal_(), nGlob_(), nBeginLocal_(), nBeginGlobal_(),nZoomBegin_(), nZoomEnd_(),
   dataNIndex_(), dataDims_(), dataBegin_(), dataIndex_(), domainMasks_(), axisMasks_(),
   gridMask_(), localDomainIndex_(), localAxisIndex_(), indexMap_(), indexDomainData_(), indexAxisData_(),
   isDataDistributed_(true), axisNum_(0), domainNum_(0), nIndexDomain_(), nIndexAxis_()
{
}

CDistributionClient::CDistributionClient(int rank, CGrid* grid)
   : CDistribution(rank, 0, 0),
   globalDataSendToServer_(0), localDataIndex_(0), localDataIndexSendToServer_(0), localMaskIndex_(0),
   axisDomainOrder_(),
   nLocal_(), nGlob_(), nBeginLocal_(), nBeginGlobal_(),nZoomBegin_(), nZoomEnd_(),
   dataNIndex_(), dataDims_(), dataBegin_(), dataIndex_(), domainMasks_(), axisMasks_(),
   gridMask_(), localDomainIndex_(), localAxisIndex_(), indexMap_(), indexDomainData_(), indexAxisData_(),
   isDataDistributed_(true), axisNum_(0), domainNum_(0), nIndexDomain_(), nIndexAxis_()
{
  readDistributionInfo(grid);
  createGlobalIndex();
  createGlobalIndexSendToServer();
}

CDistributionClient::~CDistributionClient()
{
  if (0 != globalDataSendToServer_) delete globalDataSendToServer_;
  if (0 != localDataIndex_) delete localDataIndex_;
  if (0 != localDataIndexSendToServer_) delete localDataIndexSendToServer_;
  if (0 != localMaskIndex_) delete localMaskIndex_;
}

/*!
  Read information of a grid to generate distribution.
  Every grid is composed of several axis or/and domain(s). Their information are processed
stored and used to calculate index distribution between client and server
  \param [in] grid Grid to read
*/
void CDistributionClient::readDistributionInfo(CGrid* grid)
{
  std::vector<CDomain*> domList = grid->getDomains();
  std::vector<CAxis*> axisList = grid->getAxis();
  CArray<bool,1> axisDomainOrder = grid->axis_domain_order;

  std::vector<CDomain*>::iterator itbDom, iteDom, itDom;
  std::vector<CAxis*>::iterator itbAxis, iteAxis, itAxis;

  itbDom  = itDom  = domList.begin();  iteDom  = domList.end();
  itbAxis = itAxis = axisList.begin(); iteAxis = axisList.end();

  readDistributionInfo(domList, axisList, axisDomainOrder);

  // Then check mask of grid
  int gridDim = domList.size()*2 + axisList.size();
  grid->checkMask();
  switch (gridDim) {
    case 1:
      readGridMaskInfo(grid->mask1);
      break;
    case 2:
      readGridMaskInfo(grid->mask2);
      break;
    case 3:
      readGridMaskInfo(grid->mask3);
      break;
    default:
      break;
  }
}

void CDistributionClient::readDomainIndex(const std::vector<CDomain*>& domList)
{
  int domainSize = domList.size();
  nIndexDomain_.resize(domainSize);

  for (int k = 0; k < domainSize; ++k)
  {
    nIndexDomain_[k].resize(2);
    int ni = domList[k]->ni;
    int nj = domList[k]->nj;
    nIndexDomain_[k][0].resize(ni,nj);
    nIndexDomain_[k][1].resize(ni,nj);
    nIndexDomain_[k][0] = domList[k]->i_index;
    nIndexDomain_[k][1] = domList[k]->j_index;
  }
}

void CDistributionClient::readAxisIndex(const std::vector<CAxis*>& axisList)
{
  int axisSize = axisList.size();
  nIndexAxis_.resize(axisSize);

  for (int k = 0; k < axisSize; ++k)
  {
    int n = axisList[k]->ni;
    nIndexAxis_[k].resize(n);
    for (int i = 0; i < n; ++i)
      nIndexAxis_[k](i) = i;
  }
}

/*!
  Read information from domain(s) and axis to generate distribution.
  All information related to domain, e.g ibegin, jbegin, ni, nj, ni_glo, nj_glo
as well as related to axis, e.g dataNIndex, dataIndex will be stored to compute
the distribution between clients and servers. Till now, every data structure of domain has been kept
like before, e.g: data_n_index to make sure a compability, however, it should be changed?
  \param [in] domList List of domains of grid
  \param [in] axisList List of axis of grid
  \param [in] axisDomainOrder order of axis and domain inside a grid. True if domain, false if axis
//  \param [in] gridMask Mask of grid, for now, keep it 3 dimension, but it needs changing
*/
void CDistributionClient::readDistributionInfo(const std::vector<CDomain*>& domList,
                                               const std::vector<CAxis*>& axisList,
                                               const CArray<bool,1>& axisDomainOrder)
{
  domainNum_ = domList.size();
  axisNum_   = axisList.size();
  numElement_ = axisDomainOrder.numElements(); // Number of element, e.x: Axis, Domain

  axisDomainOrder_.resize(numElement_);
  axisDomainOrder_ = axisDomainOrder;

  // Each domain or axis has its mask, of course
  domainMasks_.resize(domainNum_);
  for (int i = 0; i < domainNum_;++i)
  {
    domainMasks_[i].resize(domList[i]->mask.extent(0), domList[i]->mask.extent(1));
    domainMasks_[i] = domList[i]->mask;
  }

  axisMasks_.resize(axisNum_);
  for (int i = 0; i < axisNum_; ++i)
  {
    axisMasks_[i].resize(axisList[i]->mask.numElements());
    axisMasks_[i] = axisList[i]->mask;
  }

  // Because domain and axis can be in any order (axis1, domain1, axis2, axis3, )
  // their position should be specified. In axisDomainOrder, domain == true, axis == false
  int idx = 0;
  indexMap_.resize(numElement_);
  this->dims_ = numElement_;
  for (int i = 0; i < numElement_; ++i)
  {
    indexMap_[i] = idx;
    if (true == axisDomainOrder(i))
    {
      ++(this->dims_);
      idx += 2;
    }
    else ++idx;
  }

  // Size of each dimension (local and global)
  nLocal_.resize(this->dims_);
  nGlob_.resize(this->dims_);
  nBeginLocal_.resize(this->dims_,0);
  nBeginGlobal_.resize(this->dims_,0);
  nZoomBegin_.resize(this->dims_);
  nZoomEnd_.resize(this->dims_);

  // Data_n_index of domain or axis (For now, axis uses its size as data_n_index
  dataNIndex_.resize(numElement_);
  dataDims_.resize(numElement_);
  dataBegin_.resize(this->dims_);

  // Data_*_index of each dimension
  dataIndex_.resize(this->dims_);

  // A trick to determine position of each domain in domainList
  int domIndex = 0, axisIndex = 0;
  idx = 0;

  isDataDistributed_ = false;
  // Update all the vectors above
  while (idx < numElement_)
  {
    bool isDomain = axisDomainOrder(idx);

    // If this is a domain
    if (isDomain)
    {
      // On the j axis
      nLocal_.at(indexMap_[idx]+1) = domList[domIndex]->nj.getValue();
      nGlob_.at(indexMap_[idx]+1)  = domList[domIndex]->nj_glo.getValue();
      nBeginLocal_.at(indexMap_[idx]+1) = 0;
      nBeginGlobal_.at(indexMap_[idx]+1) = domList[domIndex]->jbegin;
      nZoomBegin_.at((indexMap_[idx]+1)) = domList[domIndex]->global_zoom_jbegin;
      nZoomEnd_.at((indexMap_[idx]+1))   = domList[domIndex]->global_zoom_jbegin + domList[domIndex]->global_zoom_nj-1;

      dataBegin_.at(indexMap_[idx]+1) = (2 == domList[domIndex]->data_dim) ? domList[domIndex]->data_jbegin.getValue() : -1;
      dataIndex_.at(indexMap_[idx]+1).resize(domList[domIndex]->data_j_index.numElements());
      dataIndex_.at(indexMap_[idx]+1) = domList[domIndex]->data_j_index;

      // On the i axis
      nLocal_.at(indexMap_[idx]) = domList[domIndex]->ni.getValue();
      nGlob_.at(indexMap_[idx]) = domList[domIndex]->ni_glo.getValue();
      nBeginLocal_.at(indexMap_[idx]) = 0;
      nBeginGlobal_.at(indexMap_[idx]) = domList[domIndex]->ibegin;
      nZoomBegin_.at((indexMap_[idx])) = domList[domIndex]->global_zoom_ibegin;
      nZoomEnd_.at((indexMap_[idx]))   = domList[domIndex]->global_zoom_ibegin + domList[domIndex]->global_zoom_ni-1;

      dataBegin_.at(indexMap_[idx]) = domList[domIndex]->data_ibegin.getValue();
      dataIndex_.at(indexMap_[idx]).resize(domList[domIndex]->data_i_index.numElements());
      dataIndex_.at(indexMap_[idx]) = domList[domIndex]->data_i_index;

      dataNIndex_.at(idx) = domList[domIndex]->data_n_index.getValue();
      dataDims_.at(idx) = domList[domIndex]->data_dim.getValue();

      isDataDistributed_ |= domList[domIndex]->isDistributed();

      ++domIndex;
    }
    else // So it's an axis
    {
      nLocal_.at(indexMap_[idx]) = axisList[axisIndex]->ni.getValue();
      nGlob_.at(indexMap_[idx]) = axisList[axisIndex]->size.getValue();
      nBeginLocal_.at(indexMap_[idx]) = 0;
      nBeginGlobal_.at(indexMap_[idx]) = axisList[axisIndex]->ibegin.getValue();
      nZoomBegin_.at((indexMap_[idx])) = axisList[axisIndex]->global_zoom_begin;
      nZoomEnd_.at((indexMap_[idx])) = axisList[axisIndex]->global_zoom_begin + axisList[axisIndex]->global_zoom_size-1;

      dataBegin_.at(indexMap_[idx]) = axisList[axisIndex]->data_begin.getValue();
      dataIndex_.at(indexMap_[idx]).resize(axisList[axisIndex]->data_index.numElements());
      dataIndex_.at(indexMap_[idx]) = axisList[axisIndex]->data_index;
      dataNIndex_.at(idx) = axisList[axisIndex]->data_index.numElements();
      dataDims_.at(idx) = 1;

      isDataDistributed_ |= axisList[axisIndex]->isDistributed();

      ++axisIndex;
    }
    ++idx;
  }
  readDomainIndex(domList);
  readAxisIndex(axisList);
}

/*!
  Create local index of domain(s).
  A domain can have data index which even contains the "ghost" points. Very often, these
data surround the true data. In order to send correct data to server,
a client need to know index of the true data.
*/
void CDistributionClient::createLocalDomainDataIndex()
{
  int numDomain = 0;
  for (int i = 0; i < axisDomainOrder_.numElements(); ++i)
    if (axisDomainOrder_(i)) ++numDomain;

  localDomainIndex_.resize(numDomain*2);
  indexDomainData_.resize(numDomain);

  int idxDomain = 0;
  for (int i = 0; i < axisDomainOrder_.numElements(); ++i)
  {
    if (axisDomainOrder_(i))
    {
      int iIdx, jIdx = 0, count = 0;
      indexDomainData_[idxDomain].resize(dataNIndex_[i], false);
      for (int j = 0; j < dataNIndex_[i]; ++j)
      {
        iIdx = getDomainIndex(dataIndex_[indexMap_[i]](j), dataIndex_[indexMap_[i]+1](j),
                              dataBegin_[indexMap_[i]], dataBegin_[indexMap_[i]+1],
                              dataDims_[i], nLocal_[indexMap_[i]], jIdx);

        if ((iIdx >= nBeginLocal_[indexMap_[i]]) && (iIdx < nLocal_[indexMap_[i]]) &&
           (jIdx >= nBeginLocal_[indexMap_[i]+1]) && (jIdx < nLocal_[indexMap_[i]+1]) &&
           (domainMasks_[idxDomain](iIdx, jIdx)))
        {
          (localDomainIndex_[idxDomain]).push_back(iIdx);
          (localDomainIndex_[idxDomain*2+1]).push_back(jIdx);
          indexDomainData_[idxDomain][j] = true;
        }
      }
      ++idxDomain;
    }
  }
}

/*!
  Create local index of axis.
*/
void CDistributionClient::createLocalAxisDataIndex()
{
  int numAxis = 0;
  for (int i = 0; i < axisDomainOrder_.numElements(); ++i)
    if (!axisDomainOrder_(i)) ++numAxis;

  localAxisIndex_.resize(numAxis);
  indexAxisData_.resize(numAxis);

  int idxAxis = 0;
  for (int i = 0; i < axisDomainOrder_.numElements(); ++i)
  {
    if (!axisDomainOrder_(i))
    {
      int iIdx = 0;
      indexAxisData_[idxAxis].resize(dataNIndex_[i], false);
      for (int j = 0; j < dataNIndex_[i]; ++j)
      {
        iIdx = getAxisIndex(dataIndex_[indexMap_[i]](j), dataBegin_[indexMap_[i]], nLocal_[indexMap_[i]]);
        if ((iIdx >= nBeginLocal_[indexMap_[i]]) &&
           (iIdx < nLocal_[indexMap_[i]]) && (axisMasks_[idxAxis](iIdx)))
        {
          localAxisIndex_[idxAxis].push_back(iIdx);
          indexAxisData_[idxAxis][j] = true;
        }
      }
      ++idxAxis;
    }
  }
}

void CDistributionClient::createGlobalIndex()
{
  size_t ssize = 1, idx = 0;
  for (int i = 0; i < this->dims_; ++i)
    ssize *= nLocal_[i];

  this->globalIndex_ = new CArray<size_t,1>(ssize);
  std::vector<int> idxLoop(this->dims_,0);
  int innnerLoopSize = nLocal_[0];
  while (idx < ssize)
  {
    for (int i = 0; i < this->dims_; ++i)
    {
      if (idxLoop[i] == nLocal_[i])
      {
        idxLoop[i] = 0;
        ++idxLoop[i+1];
      }
    }

    for (int i = 0; i < innnerLoopSize; ++i)
    {
      size_t globalIndex = idxLoop[0] + nBeginGlobal_[0];
      size_t mulDim = 1;
      for (int k = 1; k < this->dims_; ++k)
      {
        mulDim *= nGlob_[k-1];
        globalIndex += (idxLoop[k] + nBeginGlobal_[k])*mulDim;
      }
      (*this->globalIndex_)(idx) = globalIndex;
      ++idxLoop[0];
      ++idx;
    }
  }
}


/*!
   Create global index on client
   In order to do the mapping between client-server, each client creates its own
global index of sending data. This global index is then used to calculate to which server
the client needs to send it data as well as which part of data belongs to the server.
So as to make clients and server coherent in order of index, global index is calculated by
take into account of C-convention, the rightmost dimension varies faster.
*/
void CDistributionClient::createGlobalIndexSendToServer()
{
  createLocalDomainDataIndex();
  createLocalAxisDataIndex();

  int idxDomain = 0, idxAxis = 0;
  std::vector<int> eachElementSize(numElement_);

  // Precompute size of the loop
  for (int i = 0; i < numElement_; ++i)
  {
    if(axisDomainOrder_(i))
    {
      eachElementSize[i] = localDomainIndex_[idxDomain].size();
      idxDomain += 2;
    }
    else
    {
      eachElementSize[i] = localAxisIndex_[idxAxis].size();
      ++idxAxis;
    }
  }

  //   Compute size of the global index on client
  std::vector<StdSize> idxLoop(numElement_,0);
  std::vector<StdSize> currentIndex(this->dims_,0);
  int innerLoopSize = eachElementSize[0];
  size_t idx = 0, indexLocalDataOnClientCount = 0, indexSend2ServerCount = 0;
  size_t ssize = 1;
  for (int i = 0; i < numElement_; ++i) ssize *= eachElementSize[i];
  while (idx < ssize)
  {
    for (int i = 0; i < numElement_-1; ++i)
    {
      if (idxLoop[i] == eachElementSize[i])
      {
        idxLoop[i] = 0;
        ++idxLoop[i+1];
      }
    }

    // Find out outer index
    // Depending the inner-most element is axis or domain,
    // The outer loop index begins correspondingly at one (1) or zero (0)
    idxDomain = idxAxis = 0;
    if (axisDomainOrder_(0)) ++idxDomain;
    else ++idxAxis;
    for (int i = 1; i < numElement_; ++i)
    {
      if (axisDomainOrder_(i))
      {
        currentIndex[indexMap_[i]]   = localDomainIndex_[idxDomain][idxLoop[i]];
        currentIndex[indexMap_[i]+1] = localDomainIndex_[idxDomain+1][idxLoop[i]];
        idxDomain += 2;
      }
      else
      {
        currentIndex[indexMap_[i]]   = localAxisIndex_[idxAxis][idxLoop[i]];
        ++idxAxis;
      }
    }

    int maskIndex = currentIndex[0];
    for (int j = 0; j < this->dims_; ++j)

    // Inner most index
    idxDomain = idxAxis = 0;
    for (int i = 0; i < innerLoopSize; ++i)
    {
      if (axisDomainOrder_(0))
      {
        currentIndex[0] = localDomainIndex_[idxDomain][i];
        currentIndex[1] = localDomainIndex_[idxDomain+1][i];
      }
      else currentIndex[0]   = localAxisIndex_[idxAxis][i];

      StdSize gridMaskIndex = currentIndex[0];
      int mulDimMask = 1;
      for (int k = 1; k < this->dims_; ++k)
      {
        mulDimMask *= nLocal_[k-1];
        gridMaskIndex += (currentIndex[k])*mulDimMask;
      }

      if (gridMask_(gridMaskIndex)) //(gridMask_(currentIndex[0], currentIndex[1], currentIndex[2]))
      {
        ++indexLocalDataOnClientCount;
        bool isIndexOnServer = true;
        for (int j = 0; j < this->dims_; ++j)
          isIndexOnServer = isIndexOnServer && ((currentIndex[j]+nBeginGlobal_[j]) <= nZoomEnd_[j])
                                            && (nZoomBegin_[j] <= (currentIndex[j]+nBeginGlobal_[j]));
        if (isIndexOnServer) ++indexSend2ServerCount;
      }

    }
    idxLoop[0] += innerLoopSize;
    idx += innerLoopSize;
  }


  // Now allocate these arrays
  this->globalDataSendToServer_ = new CArray<size_t,1>(indexSend2ServerCount);
  localDataIndex_ = new CArray<int,1>(indexLocalDataOnClientCount);
  localDataIndexSendToServer_ = new CArray<int,1>(indexSend2ServerCount);
  localMaskIndex_ = new CArray<int,1>(indexSend2ServerCount);

  // We need to loop with data index
  idxLoop.assign(numElement_,0);
  idx = indexLocalDataOnClientCount = indexSend2ServerCount = 0;
  ssize = 1; for (int i = 0; i < numElement_; ++i) ssize *= dataNIndex_[i];
  innerLoopSize = dataNIndex_[0];
  int countLocalData = 0;
  std::vector<int> correctOuterIndex(numElement_,0);
  bool isOuterIndexCorrect = true;
  while (idx < ssize)
  {
    for (int i = 0; i < numElement_-1; ++i)
    {
      if (idxLoop[i] == dataNIndex_[i])
      {
        idxLoop[i] = 0;
        correctOuterIndex[i] = 0;
        ++idxLoop[i+1];
        if (isOuterIndexCorrect) ++correctOuterIndex[i+1];
      }
    }

    // Depending the inner-most element axis or domain,
    // The outer loop index begins correspondingly at one (1) or zero (0)
    idxDomain = idxAxis = 0;
    if (axisDomainOrder_(0)) ++idxDomain;
    else ++idxAxis;
    bool isIndexDomainDataCorrect = true;
    bool isIndexAxisDataCorrect = true;

    for (int i = 1; i < numElement_; ++i)
    {
      if (axisDomainOrder_(i))
      {
        if (indexDomainData_[idxDomain][idxLoop[i]])
        {
          currentIndex[indexMap_[i]]   = localDomainIndex_[idxDomain][correctOuterIndex[i]];
          currentIndex[indexMap_[i]+1] = localDomainIndex_[idxDomain*2+1][correctOuterIndex[i]];
          isIndexDomainDataCorrect &= true;
        }
        else isIndexDomainDataCorrect = false;
        ++idxDomain;
      }
      else
      {
        if (indexAxisData_[idxAxis][idxLoop[i]])
        {
          currentIndex[indexMap_[i]]   = localAxisIndex_[idxAxis][correctOuterIndex[i]];
          isIndexAxisDataCorrect &= true;
        }
        else isIndexAxisDataCorrect = false;
        ++idxAxis;
      }
    }

    isOuterIndexCorrect = (isIndexAxisDataCorrect) && (isIndexDomainDataCorrect);

    // Inner most index
    idxDomain = idxAxis = 0;
    int correctIndexDomain = 0, correctIndexAxis = 0;
    for (int i = 0; i < innerLoopSize; ++i)
    {
      bool isCurrentIndexDomainDataCorrect = isIndexDomainDataCorrect;
      bool isCurrentIndexAxisDataCorrect = isIndexAxisDataCorrect;

      if (axisDomainOrder_(0))
      {
        if (indexDomainData_[idxDomain][i])
        {
          currentIndex[0] = localDomainIndex_[idxDomain][correctIndexDomain];
          currentIndex[1] = localDomainIndex_[idxDomain+1][correctIndexDomain];
          isCurrentIndexDomainDataCorrect &= true;
          ++correctIndexDomain;
        }
        else isCurrentIndexDomainDataCorrect = false;
      }
      else
      {
        if (indexAxisData_[idxAxis][i])
        {
          currentIndex[0] = localAxisIndex_[idxAxis][correctIndexAxis];
          isCurrentIndexAxisDataCorrect &= true;
          ++correctIndexAxis;
        }
        else isCurrentIndexAxisDataCorrect = false;
      }

      int gridMaskIndex = currentIndex[0];
      int mulDimMask = 1;
      for (int k = 1; k < this->dims_; ++k)
      {
        mulDimMask *= nLocal_[k-1];
        gridMaskIndex += (currentIndex[k])*mulDimMask;
      }

      if (isCurrentIndexDomainDataCorrect &&
          isCurrentIndexAxisDataCorrect &&
          gridMask_(gridMaskIndex))
      {
        (*localDataIndex_)(indexLocalDataOnClientCount) = countLocalData;

        bool isIndexOnServer = true;
        for (int j = 0; j < this->dims_; ++j)
          isIndexOnServer = isIndexOnServer &&
                            ((currentIndex[j]+nBeginGlobal_[j]) <= nZoomEnd_[j]) &&
                            (nZoomBegin_[j] <= (currentIndex[j]+nBeginGlobal_[j]));
        if (isIndexOnServer)
        {
          size_t globalIndex = currentIndex[0] + nBeginGlobal_[0];
          size_t mulDim = 1;
          for (int k = 1; k < this->dims_; ++k)
          {
            mulDim *= nGlob_[k-1];
            globalIndex += (currentIndex[k] + nBeginGlobal_[k])*mulDim;
          }
          (*this->globalDataSendToServer_)(indexSend2ServerCount) = globalIndex;
          (*localDataIndexSendToServer_)(indexSend2ServerCount) = indexLocalDataOnClientCount;
          (*localMaskIndex_)(indexSend2ServerCount) = gridMaskIndex;
          ++indexSend2ServerCount;
        }
        ++indexLocalDataOnClientCount;
      }
      ++countLocalData;
    }
    idxLoop[0] += innerLoopSize;
    idx += innerLoopSize;
  }
}

/*!
  Retrieve index i and index j of a domain from its data index
  Data contains not only true data, which are sent to servers, but also ghost data, which
very often play a role of border of each local data, so does data index. Because data of a domain
can be one dimension, or two dimensions, there is a need to convert data index to domain index
  \param [in] dataIIndex index of i data
  \param [in] dataJIndex index of j data
  \param [in] dataIBegin index begin of i data
  \param [in] dataJBegin index begin of j data
  \param [in] dataDim dimension of data (1 or 2)
  \param [in] ni local size ni of domain
  \param [out] j j index of domain
  \return i index of domain
*/
int CDistributionClient::getDomainIndex(const int& dataIIndex, const int& dataJIndex,
                                        const int& dataIBegin, const int& dataJBegin,
                                        const int& dataDim, const int& ni, int& j)
{
  int tempI = dataIIndex + dataIBegin,
      tempJ = (1 == dataDim) ? -1
                             : (dataJIndex + dataJBegin);
  int i = (dataDim == 1) ? (tempI - 1) % ni
                     : (tempI - 1) ;
  j = (dataDim == 1) ? (tempI - 1) / ni
                     : (tempJ - 1) ;

  return i;
}

/*!
  Retrieve index of an axis from its data index
  \param [in] dataIndex index of data
  \param [in] dataBegin index begin of data
  \param [in] ni local size of axis
  \return index of domain
*/
int CDistributionClient::getAxisIndex(const int& dataIndex, const int& dataBegin, const int& ni)
{
   int tempI = dataIndex + dataBegin;
   return ((tempI-1)%ni);
}

const CArray<size_t,1>& CDistributionClient::getGlobalDataIndexSendToServer() const
{
  return (*globalDataSendToServer_);
}

/*!
  Return local data index of client
*/
const CArray<int,1>& CDistributionClient::getLocalDataIndexOnClient() const
{
  return (*localDataIndex_);
}

/*!
  Return local mask index of client
*/
const CArray<int,1>& CDistributionClient::getLocalMaskIndexOnClient() const
{
  return (*localMaskIndex_);
}

/*!
  Return local data index on client which are sent to servers
*/
const CArray<int,1>& CDistributionClient::getLocalDataIndexSendToServer() const
{
  return (*localDataIndexSendToServer_);
}

} // namespace xios