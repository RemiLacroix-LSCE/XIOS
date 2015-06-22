
#include "grid.hpp"

#include "attribute_template.hpp"
#include "object_template.hpp"
#include "group_template.hpp"
#include "message.hpp"
#include <iostream>
#include "xios_spl.hpp"
#include "type.hpp"
#include "context.hpp"
#include "context_client.hpp"
#include "context_server.hpp"
#include "array_new.hpp"
#include "client_server_mapping_distributed.hpp"

namespace xios {

   /// ////////////////////// Définitions ////////////////////// ///

   CGrid::CGrid(void)
      : CObjectTemplate<CGrid>(), CGridAttributes()
      , isChecked(false), isDomainAxisChecked(false), storeIndex(1)
      , vDomainGroup_(), vAxisGroup_(), axisList_(), isAxisListSet(false), isDomListSet(false), clientDistribution_(0), isIndexSent(false)
      , serverDistribution_(0), serverDistributionDescription_(0), clientServerMap_(0), writtenDataSize_(0), globalDim_()
      , connectedDataSize_(), connectedServerRank_(), isDataDistributed_(true), transformations_(0), isTransformed_(false)
      , axisPositionInGrid_()
   {
     setVirtualDomainGroup();
     setVirtualAxisGroup();
   }

   CGrid::CGrid(const StdString & id)
      : CObjectTemplate<CGrid>(id), CGridAttributes()
      , isChecked(false), isDomainAxisChecked(false), storeIndex(1)
      , vDomainGroup_(), vAxisGroup_(), axisList_(), isAxisListSet(false), isDomListSet(false), clientDistribution_(0), isIndexSent(false)
      , serverDistribution_(0), serverDistributionDescription_(0), clientServerMap_(0), writtenDataSize_(0), globalDim_()
      , connectedDataSize_(), connectedServerRank_(), isDataDistributed_(true), transformations_(0), isTransformed_(false)
      , axisPositionInGrid_()
   {
     setVirtualDomainGroup();
     setVirtualAxisGroup();
   }

   CGrid::~CGrid(void)
   {
    deque< CArray<int, 1>* >::iterator it ;

    for(deque< CArray<int,1>* >::iterator it=storeIndex.begin(); it!=storeIndex.end();it++)  delete *it ;
    for(map<int,CArray<size_t,1>* >::iterator it=outIndexFromClient.begin();it!=outIndexFromClient.end();++it) delete (it->second);

    if (0 != clientDistribution_) delete clientDistribution_;
    if (0 != serverDistribution_) delete serverDistribution_;
    if (0 != serverDistributionDescription_) delete serverDistributionDescription_;
    if (0 != transformations_) delete transformations_;
   }

   ///---------------------------------------------------------------

   StdString CGrid::GetName(void)    { return (StdString("grid")); }
   StdString CGrid::GetDefName(void) { return (CGrid::GetName()); }
   ENodeType CGrid::GetType(void)    { return (eGrid); }

   //----------------------------------------------------------------

   const std::deque< CArray<int,1>* > & CGrid::getStoreIndex(void) const
   {
      return (this->storeIndex );
   }

   //---------------------------------------------------------------
//
//   const std::deque< CArray<int,1>* > & CGrid::getOutIIndex(void)  const
//   {
//      return (this->out_i_index );
//   }
//
//   //---------------------------------------------------------------
//
//   const std::deque< CArray<int,1>* > & CGrid::getOutJIndex(void)  const
//   {
//      return (this->out_j_index );
//   }
//
//   //---------------------------------------------------------------
//
//   const std::deque< CArray<int,1>* > & CGrid::getOutLIndex(void)  const
//   {
//      return (this->out_l_index );
//   }
//
//   //---------------------------------------------------------------
//
//   const CAxis*   CGrid::getRelAxis  (void) const
//   {
//      return (this->axis );
//   }

//   //---------------------------------------------------------------
//
//   const CDomain* CGrid::getRelDomain(void) const
//   {
//      return (this->domain );
//   }

   //---------------------------------------------------------------

//   bool CGrid::hasAxis(void) const
//   {
//      return (this->withAxis);
//   }

   //---------------------------------------------------------------

   StdSize CGrid::getDimension(void) const
   {
      return (globalDim_.size());
   }

   //---------------------------------------------------------------

/*
   std::vector<StdSize> CGrid::getLocalShape(void) const
   {
      std::vector<StdSize> retvalue;
      retvalue.push_back(domain->zoom_ni_loc.getValue());
      retvalue.push_back(domain->zoom_nj_loc.getValue());
      if (this->withAxis)
         retvalue.push_back(this->axis->zoom_size.getValue());
      return (retvalue);
   }
*/
   //---------------------------------------------------------------

/*
   StdSize CGrid::getLocalSize(void) const
   {
      StdSize retvalue = 1;
      std::vector<StdSize> shape_ = this->getLocalShape();
      for (StdSize s = 0; s < shape_.size(); s++)
         retvalue *= shape_[s];
      return (retvalue);
   }
*/
   //---------------------------------------------------------------
/*
   std::vector<StdSize> CGrid::getGlobalShape(void) const
   {
      std::vector<StdSize> retvalue;
      retvalue.push_back(domain->ni.getValue());
      retvalue.push_back(domain->nj.getValue());
      if (this->withAxis)
         retvalue.push_back(this->axis->size.getValue());
      return (retvalue);
   }
*/
   //---------------------------------------------------------------

/*
   StdSize CGrid::getGlobalSize(void) const
   {
      StdSize retvalue = 1;
      std::vector<StdSize> shape_ = this->getGlobalShape();
      for (StdSize s = 0; s < shape_.size(); s++)
         retvalue *= shape_[s];
      return (retvalue);
   }
*/
   StdSize CGrid::getDataSize(void) const
   {
     StdSize retvalue = 1;
     if (!isScalarGrid())
     {
       std::vector<int> dataNindex = clientDistribution_->getDataNIndex();
       for (int i = 0; i < dataNindex.size(); ++i) retvalue *= dataNindex[i];
     }
     return retvalue;
   }

   std::map<int, StdSize> CGrid::getConnectedServerDataSize()
   {
     double secureFactor = 2.5 * sizeof(double) * CXios::bufferServerFactorSize;
     StdSize retVal = 1;
     std::map<int, StdSize> ret;
     std::map<int, size_t >::const_iterator itb = connectedDataSize_.begin(), it, itE = connectedDataSize_.end();

     if (isScalarGrid())
     {
       for (it = itb; it != itE; ++it)
       {
         retVal *= secureFactor;
         ret.insert(std::make_pair(it->first, retVal));
       }
       return ret;
     }

     for (it = itb; it != itE; ++it)
     {
        retVal = it->second;
        retVal *= secureFactor;
        ret.insert(std::make_pair<int,StdSize>(it->first, retVal));
     }

     if (connectedDataSize_.empty())
     {
       for (int i = 0; i < connectedServerRank_.size(); ++i)
       {
         retVal = 1;
         retVal *= secureFactor;
         ret.insert(std::make_pair<int,StdSize>(connectedServerRank_[i], retVal));
       }
     }

     return ret;
   }


   void CGrid::solveDomainAxisRef(bool areAttributesChecked)
   {
     if (this->isDomainAxisChecked) return;

     this->solveAxisRef(areAttributesChecked);
     this->solveDomainRef(areAttributesChecked);

     this->isDomainAxisChecked = areAttributesChecked;
   }

   void CGrid::checkMaskIndex(bool doSendingIndex)
   {
     CContext* context = CContext::getCurrent() ;
     CContextClient* client=context->client ;

     if (isScalarGrid())
     {
       if (context->hasClient)
          if (this->isChecked && doSendingIndex && !isIndexSent) { sendIndexScalarGrid(); this->isIndexSent = true; }

       if (this->isChecked) return;
       if (context->hasClient)
       {
          this->computeIndexScalarGrid();
       }

       this->isChecked = true;
       return;
     }

     if (context->hasClient)
      if (this->isChecked && doSendingIndex && !isIndexSent) { sendIndex(); this->isIndexSent = true; }

     if (this->isChecked) return;

     if (context->hasClient)
     {
        checkMask() ;
        this->computeIndex() ;
        this->storeIndex.push_front(new CArray<int,1>() );
     }
     this->isChecked = true;
   }

   void CGrid::checkMask(void)
   {
      using namespace std;
      std::vector<CDomain*> domainP = this->getDomains();
      std::vector<CAxis*> axisP = this->getAxis();
      int dim = domainP.size() * 2 + axisP.size();

      std::vector<CArray<bool,2>* > domainMasks(domainP.size());
      for (int i = 0; i < domainMasks.size(); ++i) domainMasks[i] = &(domainP[i]->mask);
      std::vector<CArray<bool,1>* > axisMasks(axisP.size());
      for (int i = 0; i < axisMasks.size(); ++i) axisMasks[i] = &(axisP[i]->mask);

      switch (dim) {
        case 1:
          checkGridMask(mask1, domainMasks, axisMasks, axis_domain_order);
          break;
        case 2:
          checkGridMask(mask2, domainMasks, axisMasks, axis_domain_order);
          break;
        case 3:
          checkGridMask(mask3, domainMasks, axisMasks, axis_domain_order);
          break;
//        case 4:
//          checkGridMask(mask4, domainMasks, axisMasks, axis_domain_order);
//          break;
//        case 5:
//          checkGridMask(mask5, domainMasks, axisMasks, axis_domain_order);
//          break;
//        case 6:
//          checkGridMask(mask6, domainMasks, axisMasks, axis_domain_order);
//          break;
//        case 7:
//          checkGridMask(mask7, domainMasks, axisMasks, axis_domain_order);
//          break;
        default:
          break;
      }
   }
   //---------------------------------------------------------------

   void CGrid::solveDomainRef(bool sendAtt)
   {
      setDomainList();
      std::vector<CDomain*> domListP = this->getDomains();
      if (!domListP.empty())
      {
        computeGridGlobalDimension(getDomains(), getAxis(), axis_domain_order);
        for (int i = 0; i < domListP.size(); ++i)
        {
          if (sendAtt) domListP[i]->sendCheckedAttributes();
          else domListP[i]->checkAttributesOnClient();
        }
      }
   }

   //---------------------------------------------------------------

   void CGrid::solveAxisRef(bool sendAtt)
   {
      setAxisList();
      std::vector<CAxis*> axisListP = this->getAxis();
      if (!axisListP.empty())
      {
        int idx = 0;
        axisPositionInGrid_.resize(0);
        for (int i = 0; i < axis_domain_order.numElements(); ++i)
        {
          if (false == axis_domain_order(i))
          {
            axisPositionInGrid_.push_back(idx);
            ++idx;
          }
          else idx += 2;
        }

        computeGridGlobalDimension(getDomains(), getAxis(), axis_domain_order);
        for (int i = 0; i < axisListP.size(); ++i)
        {
          if (sendAtt)
            axisListP[i]->sendCheckedAttributes(globalDim_,axisPositionInGrid_[i]);
          else
            axisListP[i]->checkAttributesOnClient(globalDim_,axisPositionInGrid_[i]);
          ++idx;
        }

      }
   }

   std::vector<int> CGrid::getAxisPositionInGrid() const
   {
     return axisPositionInGrid_;
   }


   //---------------------------------------------------------------

   void CGrid::computeIndex(void)
   {
     CContext* context = CContext::getCurrent() ;
     CContextClient* client=context->client ;

     // First of all, compute distribution on client side
     clientDistribution_ = new CDistributionClient(client->clientRank, this);
     // Get local data index on client
     storeIndex_client.resize(clientDistribution_->getLocalDataIndexOnClient().numElements());
     storeIndex_client = (clientDistribution_->getLocalDataIndexOnClient());
     isDataDistributed_= clientDistribution_->isDataDistributed();

     if (!doGridHaveDataDistributed())
     {
        if (0 == client->clientRank)
        {
          size_t ssize = clientDistribution_->getLocalDataIndexOnClient().numElements();
          for (int rank = 0; rank < client->serverSize; ++rank)
            connectedDataSize_[rank] = ssize;
        }
        return;
     }

     // Compute mapping between client and server
     size_t globalSizeIndex = 1, indexBegin, indexEnd;
     int range, clientSize = client->clientSize;
     for (int i = 0; i < globalDim_.size(); ++i) globalSizeIndex *= globalDim_[i];
     indexBegin = 0;
     for (int i = 0; i < clientSize; ++i)
     {
       range = globalSizeIndex / clientSize;
       if (i < (globalSizeIndex%clientSize)) ++range;
       if (i == client->clientRank) break;
       indexBegin += range;
     }
     indexEnd = indexBegin + range - 1;

     // Then compute distribution on server side
     serverDistributionDescription_ = new CServerDistributionDescription(clientDistribution_->getNGlob());
     serverDistributionDescription_->computeServerDistribution(client->serverSize, true);
     serverDistributionDescription_->computeServerGlobalIndexInRange(client->serverSize,
                                                                     std::make_pair<size_t,size_t>(indexBegin, indexEnd));

     // Finally, compute index mapping between client(s) and server(s)
     clientServerMap_ = new CClientServerMappingDistributed(serverDistributionDescription_->getGlobalIndexRange(),
                                                            client->intraComm,
                                                            clientDistribution_->isDataDistributed());

     clientServerMap_->computeServerIndexMapping(clientDistribution_->getGlobalIndex());
     const std::map<int, std::vector<size_t> >& globalIndexOnServer = clientServerMap_->getGlobalIndexOnServer();
     const CArray<size_t,1>& globalIndexSendToServer = clientDistribution_->getGlobalDataIndexSendToServer();

     std::map<int, std::vector<size_t> >::const_iterator iteGlobalMap, itbGlobalMap, itGlobalMap;
     itbGlobalMap = itGlobalMap = globalIndexOnServer.begin();
     iteGlobalMap = globalIndexOnServer.end();

     int nbGlobalIndex = globalIndexSendToServer.numElements();
    for (; itGlobalMap != iteGlobalMap; ++itGlobalMap)
    {
      int serverRank = itGlobalMap->first;
      std::vector<size_t>::const_iterator itbVecGlobal = (itGlobalMap->second).begin(), itVecGlobal,
                                          iteVecGlobal = (itGlobalMap->second).end();
      for (int i = 0; i < nbGlobalIndex; ++i)
      {
        if (iteVecGlobal != std::find(itbVecGlobal, iteVecGlobal, globalIndexSendToServer(i)))
        {
          if (connectedDataSize_.end() == connectedDataSize_.find(serverRank))
            connectedDataSize_[serverRank] = 1;
          else
            ++connectedDataSize_[serverRank];
        }
      }
    }

   connectedServerRank_.clear();
   for (std::map<int, std::vector<size_t> >::const_iterator it = globalIndexOnServer.begin(); it != globalIndexOnServer.end(); ++it) {
     connectedServerRank_.push_back(it->first);
   }
   if (!connectedDataSize_.empty())
   {
     connectedServerRank_.clear();
     for (std::map<int,size_t>::const_iterator it = connectedDataSize_.begin(); it != connectedDataSize_.end(); ++it)
       connectedServerRank_.push_back(it->first);
   }

    nbSenders = clientServerMap_->computeConnectedClients(client->serverSize, client->clientSize, client->intraComm, connectedServerRank_);
   }

   //----------------------------------------------------------------

   CGrid* CGrid::createGrid(CDomain* domain)
   {
      std::vector<CDomain*> vecDom(1,domain);
      std::vector<CAxis*> vecAxis;

      CGrid* grid = createGrid(vecDom, vecAxis);

      return (grid);
   }

   CGrid* CGrid::createGrid(CDomain* domain, CAxis* axis)
   {
      std::vector<CDomain*> vecDom(1,domain);
      std::vector<CAxis*> vecAxis(1,axis);
      CGrid* grid = createGrid(vecDom, vecAxis);

      return (grid);
   }

   CGrid* CGrid::createGrid(std::vector<CDomain*> domains, std::vector<CAxis*> axis)
   {
      StdString new_id = StdString("__");
      if (!domains.empty()) for (int i = 0; i < domains.size(); ++i) new_id += domains[i]->getId() + StdString("_");
      if (!axis.empty()) for (int i = 0; i < axis.size(); ++i) new_id += axis[i]->getId() + StdString("_") ;
      if (domains.empty() && axis.empty()) new_id += StdString("scalar_grid");
      new_id += StdString("_");

      CGrid* grid = CGridGroup::get("grid_definition")->createChild(new_id) ;
      grid->setDomainList(domains);
      grid->setAxisList(axis);

      //By default, domains are always the first ones of a grid
      if (grid->axis_domain_order.isEmpty())
      {
        int size = domains.size()+axis.size();
        grid->axis_domain_order.resize(size);
        for (int i = 0; i < size; ++i)
        {
          if (i < domains.size()) grid->axis_domain_order(i) = true;
          else grid->axis_domain_order(i) = false;
        }
      }

      grid->computeGridGlobalDimension(domains, axis, grid->axis_domain_order);

      return (grid);
   }

   CDomainGroup* CGrid::getVirtualDomainGroup() const
   {
     return (this->vDomainGroup_);
   }

   CAxisGroup* CGrid::getVirtualAxisGroup() const
   {
     return (this->vAxisGroup_);
   }

   void CGrid::outputField(int rank, const CArray<double, 1>& stored, double* field)
   {
     CArray<size_t,1>& out_i=*outIndexFromClient[rank];
     StdSize numElements = stored.numElements();
     for (StdSize n = 0; n < numElements; ++n)
     {
       *(field+out_i(n)) = stored(n);
     }
   }

   void CGrid::inputField(int rank, const double* const field, CArray<double,1>& stored)
   {
     CArray<size_t,1>& out_i = *outIndexFromClient[rank];
     StdSize numElements = stored.numElements();
     for (StdSize n = 0; n < numElements; ++n)
     {
       stored(n) = *(field+out_i(n));
     }
   }

   //----------------------------------------------------------------


   void CGrid::storeField_arr
      (const double * const data, CArray<double, 1>& stored) const
   {
      const StdSize size = storeIndex_client.numElements() ;

      stored.resize(size) ;
      for(StdSize i = 0; i < size; i++) stored(i) = data[storeIndex_client(i)] ;
   }

   void CGrid::restoreField_arr
      (const CArray<double, 1>& stored, double * const data) const
   {
      const StdSize size = storeIndex_client.numElements() ;

      for(StdSize i = 0; i < size; i++) data[storeIndex_client(i)] = stored(i) ;
   }

  void CGrid::computeIndexScalarGrid()
  {
    CContext* context = CContext::getCurrent();
    CContextClient* client=context->client;

    storeIndex_client.resize(1);
    storeIndex_client[0] = 0;
    if (0 == client->clientRank)
    {
      for (int rank = 0; rank < client->serverSize; ++rank)
        connectedDataSize_[rank] = 1;
    }
    isDataDistributed_ = false;
  }

  void CGrid::sendIndexScalarGrid()
  {
    CContext* context = CContext::getCurrent() ;
    CContextClient* client=context->client ;

    CEventClient event(getType(),EVENT_ID_INDEX);
    list<shared_ptr<CMessage> > list_msg ;
    list< CArray<size_t,1>* > listOutIndex;
    if (0 == client->clientRank)
    {
      for (int rank = 0; rank < client->serverSize; ++rank)
      {
        int nb = 1;
        CArray<size_t, 1> outGlobalIndexOnServer(nb);
        CArray<int, 1> outLocalIndexToServer(nb);
        for (int k = 0; k < nb; ++k)
        {
          outGlobalIndexOnServer(k) = 0;
          outLocalIndexToServer(k)  = 0;
        }

        storeIndex_toSrv.insert( pair<int,CArray<int,1>* >(rank,new CArray<int,1>(outLocalIndexToServer) ));
        listOutIndex.push_back(new CArray<size_t,1>(outGlobalIndexOnServer));

        list_msg.push_back(shared_ptr<CMessage>(new CMessage));
        *list_msg.back()<<getId()<<isDataDistributed_<<*listOutIndex.back();

        event.push(rank, 1, *list_msg.back());
      }
      client->sendEvent(event);
    }
    else
      client->sendEvent(event);

    for(list<CArray<size_t,1>* >::iterator it=listOutIndex.begin();it!=listOutIndex.end();++it) delete *it ;
  }

  void CGrid::sendIndex(void)
  {
    CContext* context = CContext::getCurrent() ;
    CContextClient* client=context->client ;

    CEventClient event(getType(),EVENT_ID_INDEX) ;
    int rank ;
    list<shared_ptr<CMessage> > list_msg ;
    list< CArray<size_t,1>* > listOutIndex;
    const std::map<int, std::vector<size_t> >& globalIndexOnServer = clientServerMap_->getGlobalIndexOnServer();
    const CArray<int,1>& localIndexSendToServer = clientDistribution_->getLocalDataIndexSendToServer();
    const CArray<size_t,1>& globalIndexSendToServer = clientDistribution_->getGlobalDataIndexSendToServer();

    if (!doGridHaveDataDistributed())
    {
      if (0 == client->clientRank)
      {
        CArray<size_t, 1> outGlobalIndexOnServer = globalIndexSendToServer;
        CArray<int,1> outLocalIndexToServer = localIndexSendToServer;
        for (rank = 0; rank < client->serverSize; ++rank)
        {
          storeIndex_toSrv.insert( pair<int,CArray<int,1>* >(rank,new CArray<int,1>(outLocalIndexToServer)));
          listOutIndex.push_back(new CArray<size_t,1>(outGlobalIndexOnServer));

          list_msg.push_back(shared_ptr<CMessage>(new CMessage));
          *list_msg.back()<<getId()<<isDataDistributed_<<*listOutIndex.back();

          event.push(rank, 1, *list_msg.back());
        }
        client->sendEvent(event);
      }
      else
        client->sendEvent(event);
    }
    else
    {
      std::map<int, std::vector<size_t> >::const_iterator iteGlobalMap, itbGlobalMap, itGlobalMap;
      itbGlobalMap = itGlobalMap = globalIndexOnServer.begin();
      iteGlobalMap = globalIndexOnServer.end();

      int nbGlobalIndex = globalIndexSendToServer.numElements();
      std::map<int,std::vector<int> >localIndexTmp;
      std::map<int,std::vector<size_t> > globalIndexTmp;
      for (; itGlobalMap != iteGlobalMap; ++itGlobalMap)
      {
        int serverRank = itGlobalMap->first;
        std::vector<size_t>::const_iterator itbVecGlobal = (itGlobalMap->second).begin(),
                                            iteVecGlobal = (itGlobalMap->second).end();
        for (int i = 0; i < nbGlobalIndex; ++i)
        {
          if (iteVecGlobal != std::find(itbVecGlobal, iteVecGlobal, globalIndexSendToServer(i)))
          {
            globalIndexTmp[serverRank].push_back(globalIndexSendToServer(i));
            localIndexTmp[serverRank].push_back(localIndexSendToServer(i));
          }
        }
      }

      for (int ns = 0; ns < connectedServerRank_.size(); ++ns)
      {
        rank = connectedServerRank_[ns];
        int nb = 0;
        if (globalIndexTmp.end() != globalIndexTmp.find(rank))
          nb = globalIndexTmp[rank].size();

        CArray<size_t, 1> outGlobalIndexOnServer(nb);
        CArray<int, 1> outLocalIndexToServer(nb);
        for (int k = 0; k < nb; ++k)
        {
          outGlobalIndexOnServer(k) = globalIndexTmp[rank].at(k);
          outLocalIndexToServer(k)  = localIndexTmp[rank].at(k);
        }

        storeIndex_toSrv.insert( pair<int,CArray<int,1>* >(rank,new CArray<int,1>(outLocalIndexToServer) ));
        listOutIndex.push_back(new CArray<size_t,1>(outGlobalIndexOnServer));

        list_msg.push_back(shared_ptr<CMessage>(new CMessage));
        *list_msg.back()<<getId()<<isDataDistributed_<<*listOutIndex.back();

        event.push(rank, nbSenders[rank], *list_msg.back());
      }

      client->sendEvent(event);
    }

    for(list<CArray<size_t,1>* >::iterator it=listOutIndex.begin();it!=listOutIndex.end();++it) delete *it ;
  }

  void CGrid::recvIndex(CEventServer& event)
  {
    string gridId;
    vector<int> ranks;
    vector<CBufferIn*> buffers;

    list<CEventServer::SSubEvent>::iterator it;
    for (it=event.subEvents.begin();it!=event.subEvents.end();++it)
    {
      ranks.push_back(it->rank);
      CBufferIn* buffer = it->buffer;
      *buffer >> gridId;
      buffers.push_back(buffer);
    }
    get(gridId)->recvIndex(ranks, buffers) ;
  }

  void CGrid::computeGridGlobalDimension(const std::vector<CDomain*>& domains,
                                         const std::vector<CAxis*>& axis,
                                         const CArray<bool,1>& axisDomainOrder)
  {
    globalDim_.resize(domains.size()*2+axis.size());
    int idx = 0, idxDomain = 0, idxAxis = 0;
    for (int i = 0; i < axisDomainOrder.numElements(); ++i)
    {
      if (axisDomainOrder(i))
      {
        globalDim_[idx]   = domains[idxDomain]->ni_glo.getValue();
        globalDim_[idx+1] = domains[idxDomain]->nj_glo.getValue();
        ++idxDomain;
        idx += 2;
      }
      else
      {
        globalDim_[idx] = axis[idxAxis]->size.getValue();
        ++idxAxis;
        ++idx;
      }
    }
  }

  std::vector<int> CGrid::getGlobalDimension()
  {
    return globalDim_;
  }

  bool CGrid::isScalarGrid() const
  {
    return (axisList_.empty() && domList_.empty());
  }

  /*!
    Verify whether one server need to write data
    There are some cases on which one server has nodata to write. For example, when we
  just only want to zoom on a domain.
  */
  bool CGrid::doGridHaveDataToWrite()
  {
    size_t ssize = 0;
    for (map<int, CArray<size_t, 1>* >::const_iterator it = outIndexFromClient.begin();
                                                       it != outIndexFromClient.end(); ++it)
    {
      ssize += (it->second)->numElements();
    }
    return (0 != ssize);
  }

  /*!
    Return size of data which is written on each server
    Whatever dimension of a grid, data which are written on server must be presented as
  an one dimension array.
  \return size of data written on server
  */
  size_t CGrid::getWrittenDataSize() const
  {
    return writtenDataSize_;
  }

  const CDistributionServer* CGrid::getDistributionServer() const
  {
    return serverDistribution_;
  }

  const CDistributionClient* CGrid::getDistributionClient() const
  {
    return clientDistribution_;
  }

  bool CGrid::doGridHaveDataDistributed()
  {
    if (isScalarGrid()) return false;
    else
      return isDataDistributed_;
  }

  void CGrid::recvIndex(vector<int> ranks, vector<CBufferIn*> buffers)
  {
    CContext* context = CContext::getCurrent();
    CContextServer* server = context->server;

    for (int n = 0; n < ranks.size(); n++)
    {
      int rank = ranks[n];
      CBufferIn& buffer = *buffers[n];

      buffer >> isDataDistributed_;
      size_t dataSize = 0;

      if (isScalarGrid())
      {
        writtenDataSize_ = 1;
        CArray<size_t,1> outIndex;
        buffer >> outIndex;
        outIndexFromClient.insert(std::pair<int, CArray<size_t,1>* >(rank, new CArray<size_t,1>(outIndex)));
        std::vector<int> nZoomBegin(1,0), nZoomSize(1,1), nGlob(1,1), nZoomBeginGlobal(1,0);
        serverDistribution_ = new CDistributionServer(server->intraCommRank, nZoomBegin, nZoomSize,
                                                      nZoomBeginGlobal, nGlob);
        return;
      }

      if (0 == serverDistribution_)
      {
        int idx = 0, numElement = axis_domain_order.numElements();
        int ssize = numElement;
        std::vector<int> indexMap(numElement);
        for (int i = 0; i < numElement; ++i)
        {
          indexMap[i] = idx;
          if (true == axis_domain_order(i))
          {
            ++ssize;
            idx += 2;
          }
          else
            ++idx;
        }

        int axisId = 0, domainId = 0;
        std::vector<CDomain*> domainList = getDomains();
        std::vector<CAxis*> axisList = getAxis();
        std::vector<int> nZoomBegin(ssize), nZoomSize(ssize), nGlob(ssize), nZoomBeginGlobal(ssize);
        for (int i = 0; i < numElement; ++i)
        {
          if (axis_domain_order(i))
          {
            nZoomBegin[indexMap[i]] = domainList[domainId]->zoom_ibegin_srv;
            nZoomSize[indexMap[i]]  = domainList[domainId]->zoom_ni_srv;
            nZoomBeginGlobal[indexMap[i]] = domainList[domainId]->zoom_ibegin;
            nGlob[indexMap[i]] = domainList[domainId]->ni_glo;

            nZoomBegin[indexMap[i] + 1] = domainList[domainId]->zoom_jbegin_srv;
            nZoomSize[indexMap[i] + 1] = domainList[domainId]->zoom_nj_srv;
            nZoomBeginGlobal[indexMap[i] + 1] = domainList[domainId]->zoom_jbegin;
            nGlob[indexMap[i] + 1] = domainList[domainId]->nj_glo;
            ++domainId;
          }
          else
          {
            nZoomBegin[indexMap[i]] = axisList[axisId]->zoom_begin_srv;
            nZoomSize[indexMap[i]]  = axisList[axisId]->zoom_size_srv;
            nZoomBeginGlobal[indexMap[i]] = axisList[axisId]->zoom_begin;
            nGlob[indexMap[i]] = axisList[axisId]->size;
            ++axisId;
          }
        }
        dataSize = 1;
        for (int i = 0; i < nZoomSize.size(); ++i)
          dataSize *= nZoomSize[i];

        serverDistribution_ = new CDistributionServer(server->intraCommRank, nZoomBegin, nZoomSize,
                                                      nZoomBeginGlobal, nGlob);
      }

      CArray<size_t,1> outIndex;
      buffer >> outIndex;
      if (isDataDistributed_)
        serverDistribution_->computeLocalIndex(outIndex);
      else
      {
        dataSize = outIndex.numElements();
        for (int i = 0; i < outIndex.numElements(); ++i) outIndex(i) = i;
      }
      writtenDataSize_ += dataSize;

      outIndexFromClient.insert(std::pair<int, CArray<size_t,1>* >(rank, new CArray<size_t,1>(outIndex)));
      connectedDataSize_[rank] = outIndex.numElements();
    }

    nbSenders = CClientServerMappingDistributed::computeConnectedClients(context->client->serverSize, context->client->clientSize, context->client->intraComm, ranks);
  }

   /*!
   \brief Dispatch event received from client
      Whenever a message is received in buffer of server, it will be processed depending on
   its event type. A new event type should be added in the switch list to make sure
   it processed on server side.
   \param [in] event: Received message
   */
  bool CGrid::dispatchEvent(CEventServer& event)
  {

    if (SuperClass::dispatchEvent(event)) return true ;
    else
    {
      switch(event.type)
      {
        case EVENT_ID_INDEX :
          recvIndex(event) ;
          return true ;
          break ;

         case EVENT_ID_ADD_DOMAIN :
           recvAddDomain(event) ;
           return true ;
           break ;

         case EVENT_ID_ADD_AXIS :
           recvAddAxis(event) ;
           return true ;
           break ;
        default :
          ERROR("bool CDomain::dispatchEvent(CEventServer& event)",
                <<"Unknown Event") ;
          return false ;
      }
    }
  }

   void CGrid::inputFieldServer(const std::deque< CArray<double, 1>* > storedClient, CArray<double, 1>&  storedServer) const
   {
      if ((this->storeIndex.size()-1 ) != storedClient.size())
         ERROR("void CGrid::inputFieldServer(const std::deque< CArray<double, 1>* > storedClient, CArray<double, 1>&  storedServer) const",
                << "[ Expected received field = " << (this->storeIndex.size()-1) << ", "
                << "[ received fiedl = "    << storedClient.size() << "] "
                << "Data from clients are missing!") ;
      storedServer.resize(storeIndex[0]->numElements());

      for (StdSize i = 0, n = 0; i < storedClient.size(); i++)
         for (StdSize j = 0; j < storedClient[i]->numElements(); j++)
            storedServer(n++) = (*storedClient[i])(j);
   }

   void CGrid::outputFieldToServer(CArray<double,1>& fieldIn, int rank, CArray<double,1>& fieldOut)
   {
     CArray<int,1>& index = *storeIndex_toSrv[rank] ;
     int nb=index.numElements() ;
     fieldOut.resize(nb) ;

     for(int k=0;k<nb;k++) fieldOut(k)=fieldIn(index(k)) ;
    }
   ///---------------------------------------------------------------

   CDomain* CGrid::addDomain(const std::string& id)
   {
     return vDomainGroup_->createChild(id) ;
   }

   CAxis* CGrid::addAxis(const std::string& id)
   {
     return vAxisGroup_->createChild(id) ;
   }

   //! Change virtual field group to a new one
   void CGrid::setVirtualDomainGroup(CDomainGroup* newVDomainGroup)
   {
      this->vDomainGroup_ = newVDomainGroup;
   }

   //! Change virtual variable group to new one
   void CGrid::setVirtualAxisGroup(CAxisGroup* newVAxisGroup)
   {
      this->vAxisGroup_ = newVAxisGroup;
   }

   //----------------------------------------------------------------
   //! Create virtual field group, which is done normally on initializing file
   void CGrid::setVirtualDomainGroup(void)
   {
      this->setVirtualDomainGroup(CDomainGroup::create());
   }

   //! Create virtual variable group, which is done normally on initializing file
   void CGrid::setVirtualAxisGroup(void)
   {
      this->setVirtualAxisGroup(CAxisGroup::create());
   }

   /*!
   \brief Send a message to create a domain on server side
   \param[in] id String identity of domain that will be created on server
   */
   void CGrid::sendAddDomain(const string& id)
   {
    CContext* context=CContext::getCurrent() ;

    if (! context->hasServer )
    {
       CContextClient* client=context->client ;

       CEventClient event(this->getType(),EVENT_ID_ADD_DOMAIN) ;
       if (client->isServerLeader())
       {
         CMessage msg ;
         msg<<this->getId() ;
         msg<<id ;
         const std::list<int>& ranks = client->getRanksServerLeader();
         for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
           event.push(*itRank,1,msg);
         client->sendEvent(event) ;
       }
       else client->sendEvent(event) ;
    }
   }

   /*!
   \brief Send a message to create an axis on server side
   \param[in] id String identity of axis that will be created on server
   */
   void CGrid::sendAddAxis(const string& id)
   {
    CContext* context=CContext::getCurrent() ;

    if (! context->hasServer )
    {
       CContextClient* client=context->client ;

       CEventClient event(this->getType(),EVENT_ID_ADD_AXIS) ;
       if (client->isServerLeader())
       {
         CMessage msg ;
         msg<<this->getId() ;
         msg<<id ;
         const std::list<int>& ranks = client->getRanksServerLeader();
         for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
           event.push(*itRank,1,msg);
         client->sendEvent(event) ;
       }
       else client->sendEvent(event) ;
    }
   }

   /*!
   \brief Receive a message annoucing the creation of a domain on server side
   \param[in] event Received event
   */
   void CGrid::recvAddDomain(CEventServer& event)
   {

      CBufferIn* buffer=event.subEvents.begin()->buffer;
      string id;
      *buffer>>id ;
      get(id)->recvAddDomain(*buffer) ;
   }

   /*!
   \brief Receive a message annoucing the creation of a domain on server side
   \param[in] buffer Buffer containing message
   */
   void CGrid::recvAddDomain(CBufferIn& buffer)
   {
      string id ;
      buffer>>id ;
      addDomain(id) ;
   }

   /*!
   \brief Receive a message annoucing the creation of an axis on server side
   \param[in] event Received event
   */
   void CGrid::recvAddAxis(CEventServer& event)
   {

      CBufferIn* buffer=event.subEvents.begin()->buffer;
      string id;
      *buffer>>id ;
      get(id)->recvAddAxis(*buffer) ;
   }

   /*!
   \brief Receive a message annoucing the creation of an axis on server side
   \param[in] buffer Buffer containing message
   */
   void CGrid::recvAddAxis(CBufferIn& buffer)
   {
      string id ;
      buffer>>id ;
      addAxis(id) ;
   }

  /*!
  \brief Solve domain and axis references
  As field, domain and axis can refer to other domains or axis. In order to inherit correctly
  all attributes from their parents, they should be processed with this function
  \param[in] apply inherit all attributes of parents (true)
  */
  void CGrid::solveDomainAxisRefInheritance(bool apply)
  {
    CContext* context = CContext::getCurrent();
    unsigned int vecSize, i;
    std::vector<StdString>::iterator it, itE;
    setDomainList();
    it = domList_.begin(); itE = domList_.end();
    for (; it != itE; ++it)
    {
      CDomain* pDom = CDomain::get(*it);
      if (context->hasClient)
      {
        pDom->solveRefInheritance(apply);
        pDom->solveBaseReference();
        if ((!pDom->domain_ref.isEmpty()) && (pDom->name.isEmpty()))
          pDom->name.setValue(pDom->getBaseDomainReference()->getId());
      }
    }

    setAxisList();
    it = axisList_.begin(); itE = axisList_.end();
    for (; it != itE; ++it)
    {
      CAxis* pAxis = CAxis::get(*it);
      if (context->hasClient)
      {
        pAxis->solveRefInheritance(apply);
        pAxis->solveBaseReference();
        pAxis->solveInheritanceTransformation();
        if ((!pAxis->axis_ref.isEmpty()) && (pAxis->name.isEmpty()))
          pAxis->name.setValue(pAxis->getBaseAxisReference()->getId());
      }
    }
  }

  bool CGrid::isTransformed()
  {
    return isTransformed_;
  }

  void CGrid::setTransformed()
  {
    isTransformed_ = true;
  }

  CGridTransformation* CGrid::getTransformations()
  {
    return transformations_;
  }

  void CGrid::transformGrid(CGrid* transformedGrid)
  {
    if (transformedGrid->isTransformed()) return;
    transformedGrid->setTransformed();
    if (axis_domain_order.numElements() != transformedGrid->axis_domain_order.numElements())
    {
      ERROR("CGrid::transformGrid(CGrid* transformedGrid)",
           << "Two grids have different dimension size"
           << "Dimension of grid source " <<this->getId() << " is " << axis_domain_order.numElements() << std::endl
           << "Dimension of grid destination " <<transformedGrid->getId() << " is " << transformedGrid->axis_domain_order.numElements());
    }
    else
    {
      int ssize = axis_domain_order.numElements();
      for (int i = 0; i < ssize; ++i)
        if (axis_domain_order(i) != (transformedGrid->axis_domain_order)(i))
          ERROR("CGrid::transformGrid(CGrid* transformedGrid)",
                << "Grids " <<this->getId() <<" and " << transformedGrid->getId()
                << " don't have elements in the same order");
    }

    transformations_ = new CGridTransformation(transformedGrid, this);
    transformations_->computeTransformationMapping();
    std::cout << "send index " << *(transformations_->getLocalIndexToSendFromGridSource()[0]) << std::endl;
    std::cout << "receive index " << *(transformations_->getLocalIndexToReceiveOnGridDest()[0][0]) << std::endl;
  }

  /*!
  \brief Get the list of domain pointers
  \return list of domain pointers
  */
  std::vector<CDomain*> CGrid::getDomains()
  {
    std::vector<CDomain*> domList;
    if (!domList_.empty())
    {
      for (int i = 0; i < domList_.size(); ++i) domList.push_back(CDomain::get(domList_[i]));
    }
    return domList;
  }

  /*!
  \brief Get the list of  axis pointers
  \return list of axis pointers
  */
  std::vector<CAxis*> CGrid::getAxis()
  {
    std::vector<CAxis*> aList;
    if (!axisList_.empty())
      for (int i =0; i < axisList_.size(); ++i) aList.push_back(CAxis::get(axisList_[i]));

    return aList;
  }

  /*!
  \brief Set domain(s) of a grid from a list
  \param[in] domains list of domains
  */
  void CGrid::setDomainList(const std::vector<CDomain*> domains)
  {
    if (isDomListSet) return;
    std::vector<CDomain*> domList = this->getVirtualDomainGroup()->getAllChildren();
    if (!domains.empty() && domList.empty()) domList = domains;
    if (!domList.empty())
    {
      int sizeDom = domList.size();
      domList_.resize(sizeDom);
      for (int i = 0 ; i < sizeDom; ++i)
      {
        domList_[i] = domList[i]->getId();
      }
      isDomListSet = true;
    }

  }

  /*!
  \brief Set axis(s) of a grid from a list
  \param[in] axis list of axis
  */
  void CGrid::setAxisList(const std::vector<CAxis*> axis)
  {
    if (isAxisListSet) return;
    std::vector<CAxis*> aList = this->getVirtualAxisGroup()->getAllChildren();
    if (!axis.empty() && aList.empty()) aList = axis;
    if (!aList.empty())
    {
      int sizeAxis = aList.size();
      axisList_.resize(sizeAxis);
      for (int i = 0; i < sizeAxis; ++i)
      {
        axisList_[i] = aList[i]->getId();
      }
      isAxisListSet = true;
    }
  }

  /*!
  \brief Get list of id of domains
  \return id list of domains
  */
  std::vector<StdString> CGrid::getDomainList()
  {
    setDomainList();
    return domList_;
  }

  /*!
  \brief Get list of id of axis
  \return id list of axis
  */
  std::vector<StdString> CGrid::getAxisList()
  {
    setAxisList();
    return axisList_;
  }

  void CGrid::sendAllDomains()
  {
    std::vector<CDomain*> domList = this->getVirtualDomainGroup()->getAllChildren();
    int dSize = domList.size();
    for (int i = 0; i < dSize; ++i)
    {
      sendAddDomain(domList[i]->getId());
      domList[i]->sendAllAttributesToServer();
    }
  }

  void CGrid::sendAllAxis()
  {
    std::vector<CAxis*> aList = this->getVirtualAxisGroup()->getAllChildren();
    int aSize = aList.size();

    for (int i = 0; i < aSize; ++i)
    {
      sendAddAxis(aList[i]->getId());
      aList[i]->sendAllAttributesToServer();
    }
  }

  void CGrid::parse(xml::CXMLNode & node)
  {
    SuperClass::parse(node);

    // List order of axis and domain in a grid, if there is a domain, it will take value 1 (true), axis 0 (false)
    std::vector<bool> order;

    if (node.goToChildElement())
    {
      StdString domainName("domain");
      StdString axisName("axis");
      do
      {
        if (node.getElementName() == domainName) {
          order.push_back(true);
          this->getVirtualDomainGroup()->parseChild(node);
        }
        if (node.getElementName() == axisName) {
          order.push_back(false);
          this->getVirtualAxisGroup()->parseChild(node);
        }
      } while (node.goToNextElement()) ;
      node.goToParentElement();
    }

    if (!order.empty())
    {
      int sizeOrd = order.size();
      axis_domain_order.resize(sizeOrd);
      for (int i = 0; i < sizeOrd; ++i)
      {
        axis_domain_order(i) = order[i];
      }
    }

    setDomainList();
    setAxisList();
   }

} // namespace xios
