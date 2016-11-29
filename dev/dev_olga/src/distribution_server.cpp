/*!
   \file distribution_server.cpp
   \author Ha NGUYEN
   \since 13 Jan 2015
   \date 10 Sep 2016

   \brief Index distribution on server side.
 */
#include "distribution_server.hpp"
#include "utils.hpp"

namespace xios {

CDistributionServer::CDistributionServer(int rank, int dims, const CArray<size_t,1>& globalIndex)
  : CDistribution(rank, dims, globalIndex), nGlobal_(), nZoomSize_(), nZoomBegin_(), globalLocalIndexMap_()
{
}

CDistributionServer::CDistributionServer(int rank, const std::vector<int>& nZoomBegin,
                                         const std::vector<int>& nZoomSize, const std::vector<int>& nGlobal)
  : CDistribution(rank, nGlobal.size()), nGlobal_(nGlobal), nZoomSize_(nZoomSize), nZoomBegin_(nZoomBegin), globalLocalIndexMap_()
{
  createGlobalIndex();
}

CDistributionServer::CDistributionServer(int rank, const std::vector<int>& nZoomBegin,
                                         const std::vector<int>& nZoomSize,
                                         const std::vector<int>& nZoomBeginGlobal,
                                         const std::vector<int>& nGlobal)
  : CDistribution(rank, nGlobal.size()), nGlobal_(nGlobal), nZoomBeginGlobal_(nZoomBeginGlobal),
    nZoomSize_(nZoomSize), nZoomBegin_(nZoomBegin), globalLocalIndexMap_()
{
  createGlobalIndex();
}

CDistributionServer::~CDistributionServer()
{
}

/*!
  Create global index on server side
  Like the similar function on client side, this function serves on creating global index
for data written by the server. The global index is used to calculating local index of data
written on each server
*/
void CDistributionServer::createGlobalIndex()
{
  size_t idx = 0, ssize = 1;
  for (int i = 0; i < nZoomSize_.size(); ++i) ssize *= nZoomSize_[i];

  this->globalIndex_.resize(ssize);
  std::vector<int> idxLoop(this->getDims(),0);
  std::vector<int> currentIndex(this->getDims());
  int innerLoopSize = nZoomSize_[0];

  globalLocalIndexMap_.rehash(std::ceil(ssize/globalLocalIndexMap_.max_load_factor()));
  while (idx<ssize)
  {
    for (int i = 0; i < this->dims_-1; ++i)
    {
      if (idxLoop[i] == nZoomSize_[i])
      {
        idxLoop[i] = 0;
        ++idxLoop[i+1];
      }
    }

    for (int i = 1; i < this->dims_; ++i)  currentIndex[i] = idxLoop[i] + nZoomBegin_[i];

    size_t mulDim, globalIndex;
    for (int i = 0; i < innerLoopSize; ++i)
    {
      mulDim = 1;
      globalIndex = i + nZoomBegin_[0];

      for (int k = 1; k < this->dims_; ++k)
      {
        mulDim *= nGlobal_[k-1];
        globalIndex += (currentIndex[k])*mulDim;
      }
      globalLocalIndexMap_[globalIndex] = idx;
      this->globalIndex_(idx) = globalIndex;

      ++idx;
    }
    idxLoop[0] += innerLoopSize;
  }
}

/*!
  Compute local index for writing data on server
  \param [in] globalIndex global index received from client
  \return local index of written data
*/
CArray<size_t,1> CDistributionServer::computeLocalIndex(const CArray<size_t,1>& globalIndex)
{
  int ssize = globalIndex.numElements();
  CArray<size_t,1> localIndex(ssize);
  GlobalLocalMap::const_iterator ite = globalLocalIndexMap_.end(), it;
  for (int idx = 0; idx < ssize; ++idx)
  {
    it = globalLocalIndexMap_.find(globalIndex(idx));
    if (ite != it)
      localIndex(idx) = it->second;
  }

  return localIndex;
}

/*!
  Compute local index for writing data on server
  \param [in] globalIndex Global index received from client
*/
void CDistributionServer::computeLocalIndex(CArray<size_t,1>& globalIndex)
{
  int ssize = globalIndex.numElements();
  CArray<size_t,1> localIndex(ssize);
  GlobalLocalMap::const_iterator ite = globalLocalIndexMap_.end(), it;
  for (int idx = 0; idx < ssize; ++idx)
  {
    it = globalLocalIndexMap_.find(globalIndex(idx));
    if (ite != it)
      localIndex(idx) = it->second;
  }

  globalIndex.reference(localIndex);
}

/*!
  Transforms local indexes owned by the server into global indexes
  \param [in/out] indexes on input, local indexes of the server,
                          on output, the corresponding global indexes
*/
void CDistributionServer::computeGlobalIndex(CArray<int,1>& indexes) const
{
  for (int i = 0; i < indexes.numElements(); ++i)
  {
    indexes(i) = globalIndex_(indexes(i));
  }
}


const std::vector<int>& CDistributionServer::getZoomBeginGlobal() const
{
  return nZoomBeginGlobal_;
}

const std::vector<int>& CDistributionServer::getZoomBeginServer() const
{
  return nZoomBegin_;
}

const std::vector<int>& CDistributionServer::getZoomSizeServer() const
{
  return nZoomSize_;
}
} // namespace xios