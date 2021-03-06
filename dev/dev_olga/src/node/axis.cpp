#include "axis.hpp"

#include "attribute_template.hpp"
#include "object_template.hpp"
#include "group_template.hpp"
#include "message.hpp"
#include "type.hpp"
#include "context.hpp"
#include "context_client.hpp"
#include "context_server.hpp"
#include "xios_spl.hpp"
#include "inverse_axis.hpp"
#include "zoom_axis.hpp"
#include "interpolate_axis.hpp"
#include "server_distribution_description.hpp"
#include "client_server_mapping_distributed.hpp"
#include "distribution_client.hpp"

namespace xios {

   /// ////////////////////// Définitions ////////////////////// ///

   CAxis::CAxis(void)
      : CObjectTemplate<CAxis>()
      , CAxisAttributes(), isChecked(false), relFiles(), areClientAttributesChecked_(false)
      , isClientAfterTransformationChecked(false)
      , hasBounds_(false), isCompressible_(false)
      , numberWrittenIndexes_(0), totalNumberWrittenIndexes_(0), offsetWrittenIndexes_(0)
      , transformationMap_(), hasValue(false), hasLabel(false)
      , computedWrittenIndex_(false)
   {
   }

   CAxis::CAxis(const StdString & id)
      : CObjectTemplate<CAxis>(id)
      , CAxisAttributes(), isChecked(false), relFiles(), areClientAttributesChecked_(false)
      , isClientAfterTransformationChecked(false)
      , hasBounds_(false), isCompressible_(false)
      , numberWrittenIndexes_(0), totalNumberWrittenIndexes_(0), offsetWrittenIndexes_(0)
      , transformationMap_(), hasValue(false), hasLabel(false)
      , computedWrittenIndex_(false)
   {
   }

   CAxis::~CAxis(void)
   { /* Ne rien faire de plus */ }

   std::map<StdString, ETranformationType> CAxis::transformationMapList_ = std::map<StdString, ETranformationType>();
   bool CAxis::dummyTransformationMapList_ = CAxis::initializeTransformationMap(CAxis::transformationMapList_);
   bool CAxis::initializeTransformationMap(std::map<StdString, ETranformationType>& m)
   {
     m["zoom_axis"] = TRANS_ZOOM_AXIS;
     m["interpolate_axis"] = TRANS_INTERPOLATE_AXIS;
     m["inverse_axis"] = TRANS_INVERSE_AXIS;
     m["reduce_domain"] = TRANS_REDUCE_DOMAIN_TO_AXIS;
     m["extract_domain"] = TRANS_EXTRACT_DOMAIN_TO_AXIS;
   }

   ///---------------------------------------------------------------

   const std::set<StdString> & CAxis::getRelFiles(void) const
   {
      return (this->relFiles);
   }

   bool CAxis::IsWritten(const StdString & filename) const
   {
      return (this->relFiles.find(filename) != this->relFiles.end());
   }

   bool CAxis::isWrittenCompressed(const StdString& filename) const
   {
      return (this->relFilesCompressed.find(filename) != this->relFilesCompressed.end());
   }

   bool CAxis::isDistributed(void) const
   {
      bool distributed = (!this->begin.isEmpty() && !this->n.isEmpty() && (this->begin + this->n < this->n_glo)) ||
             (!this->n.isEmpty() && (this->n != this->n_glo));
      // A same stupid condition to make sure that if there is only one client, axis
      // should be considered to be distributed. This should be a temporary solution     
      distributed |= (1 == CContext::getCurrent()->client->clientSize);
      return distributed;
   }

   /*!
    * Test whether the data defined on the axis can be outputted in a compressed way.
    *
    * \return true if and only if a mask was defined for this axis
    */
   bool CAxis::isCompressible(void) const
   {
      return isCompressible_;
   }

   void CAxis::addRelFile(const StdString & filename)
   {
      this->relFiles.insert(filename);
   }

   void CAxis::addRelFileCompressed(const StdString& filename)
   {
      this->relFilesCompressed.insert(filename);
   }

   //----------------------------------------------------------------

   /*!
     Returns the number of indexes written by each server.
     \return the number of indexes written by each server
   */
   int CAxis::getNumberWrittenIndexes() const
   {
     return numberWrittenIndexes_;
   }

   /*!
     Returns the total number of indexes written by the servers.
     \return the total number of indexes written by the servers
   */
   int CAxis::getTotalNumberWrittenIndexes() const
   {
     return totalNumberWrittenIndexes_;
   }

   /*!
     Returns the offset of indexes written by each server.
     \return the offset of indexes written by each server
   */
   int CAxis::getOffsetWrittenIndexes() const
   {
     return offsetWrittenIndexes_;
   }

   //----------------------------------------------------------------

   /*!
    * Compute the minimum buffer size required to send the attributes to the server(s).
    *
    * \return A map associating the server rank with its minimum buffer size.
    */
   std::map<int, StdSize> CAxis::getAttributesBufferSize()
   {
     // For now the assumption is that secondary server pools consist of the same number of procs.
     // CHANGE the line below if the assumption changes.
     CContext* context = CContext::getCurrent();
     CContextClient* client = (0 != context->clientPrimServer.size()) ? context->clientPrimServer[0] : context->client;

     std::map<int, StdSize> attributesSizes = getMinimumBufferSizeForAttributes();

     bool isNonDistributed = (n == n_glo);

     if (client->isServerLeader())
     {
       // size estimation for sendServerAttribut
       size_t size = 6 * sizeof(size_t);
       // size estimation for sendNonDistributedValue
       if (isNonDistributed)
         size = std::max(size, CArray<double,1>::size(n_glo) + (isCompressible_ ? CArray<int,1>::size(n_glo) : 0));
       size += CEventClient::headerSize + getId().size() + sizeof(size_t);

       const std::list<int>& ranks = client->getRanksServerLeader();
       for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
       {
         if (size > attributesSizes[*itRank])
           attributesSizes[*itRank] = size;
       }
     }

     if (!isNonDistributed)
     {
       // size estimation for sendDistributedValue
       boost::unordered_map<int, vector<size_t> >::const_iterator it, ite = indSrv_.end();
       for (it = indSrv_.begin(); it != ite; ++it)
       {
         size_t sizeIndexEvent = CArray<int,1>::size(it->second.size());
         if (isCompressible_)
           sizeIndexEvent += CArray<int,1>::size(indWrittenSrv_[it->first].size());

         size_t sizeValEvent = CArray<double,1>::size(it->second.size());
         if (hasBounds_)
           sizeValEvent += CArray<double,2>::size(2 * it->second.size());
 
         if (hasLabel)
           sizeValEvent += CArray<StdString,1>::size(it->second.size());

         size_t size = CEventClient::headerSize + getId().size() + sizeof(size_t) + std::max(sizeIndexEvent, sizeValEvent);
         if (size > attributesSizes[it->first])
           attributesSizes[it->first] = size;
       }
     }

     return attributesSizes;
   }

   //----------------------------------------------------------------

   StdString CAxis::GetName(void)   { return (StdString("axis")); }
   StdString CAxis::GetDefName(void){ return (CAxis::GetName()); }
   ENodeType CAxis::GetType(void)   { return (eAxis); }

   //----------------------------------------------------------------

   CAxis* CAxis::createAxis()
   {
     CAxis* axis = CAxisGroup::get("axis_definition")->createChild();
     return axis;
   }

   /*!
     Check common attributes of an axis.
     This check should be done in the very beginning of work flow
   */
   void CAxis::checkAttributes(void)
   {
      if (this->n_glo.isEmpty())
        ERROR("CAxis::checkAttributes(void)",
              << "[ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] "
              << "The axis is wrongly defined, attribute 'n_glo' must be specified");
      StdSize size = this->n_glo.getValue();

      if (!this->index.isEmpty())
      {
        if (n.isEmpty()) n = index.numElements();

        // It's not so correct but if begin is not the first value of index 
        // then data on the local axis has user-defined distribution. In this case, begin has no meaning.
        if (begin.isEmpty()) begin = index(0);         
      }
      else 
      {
        if (!this->begin.isEmpty())
        {
          if (begin < 0 || begin > size - 1)
            ERROR("CAxis::checkAttributes(void)",
                  << "[ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] "
                  << "The axis is wrongly defined, attribute 'begin' (" << begin.getValue() << ") must be non-negative and smaller than size-1 (" << size - 1 << ").");
        }
        else this->begin.setValue(0);

        if (!this->n.isEmpty())
        {
          if (n < 0 || n > size)
            ERROR("CAxis::checkAttributes(void)",
                  << "[ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] "
                  << "The axis is wrongly defined, attribute 'n' (" << n.getValue() << ") must be non-negative and smaller than size (" << size << ").");
        }
        else this->n.setValue(size);

        {
          index.resize(n);
          for (int i = 0; i < n; ++i) index(i) = i+begin;
        }
      }

      if (!this->value.isEmpty())
      {
        StdSize true_size = value.numElements();
        if (this->n.getValue() != true_size)
          ERROR("CAxis::checkAttributes(void)",
                << "[ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] "
                << "The axis is wrongly defined, attribute 'value' has a different size (" << true_size << ") than the one defined by the \'size\' attribute (" << n.getValue() << ").");
        this->hasValue = true;
      }

      this->checkData();
      this->checkZoom();
      this->checkMask();
      this->checkBounds();
      this->checkLabel();
   }

   /*!
      Check the validity of data and fill in values if any.
   */
   void CAxis::checkData()
   {
      if (data_begin.isEmpty()) data_begin.setValue(0);

      if (data_n.isEmpty())
      {
        data_n.setValue(n);
      }
      else if (data_n.getValue() < 0)
      {
        ERROR("CAxis::checkData(void)",
              << "[ id = " << this->getId() << " , context = '" << CObjectFactory::GetCurrentContextId() << " ] "
              << "The data size should be strictly positive ('data_n' = " << data_n.getValue() << ").");
      }

      if (data_index.isEmpty())
      {
        data_index.resize(data_n);
        for (int i = 0; i < data_n; ++i) data_index(i) = i;
      }
   }

   /*!
     Check validity of zoom info and fill in values if any.
   */
   void CAxis::checkZoom(void)
   {
     if (global_zoom_begin.isEmpty()) global_zoom_begin.setValue(0);
     if (global_zoom_n.isEmpty()) global_zoom_n.setValue(n_glo.getValue());
     if (zoom_index.isEmpty())
     {
       zoom_index.setValue(index.getValue());
     }
     if (zoom_n.isEmpty()) zoom_n.setValue(n);
     if (zoom_begin.isEmpty()) zoom_begin.setValue(begin);
   }

   /*!
     Check validity of mask info and fill in values if any.
   */
   void CAxis::checkMask()
   {
      if (!mask.isEmpty())
      {
         if (mask.extent(0) != n)
           ERROR("CAxis::checkMask(void)",
                 << "[ id = " << this->getId() << " , context = '" << CObjectFactory::GetCurrentContextId() << " ] "
                 << "The mask does not have the same size as the local domain." << std::endl
                 << "Local size is " << n.getValue() << "." << std::endl
                 << "Mask size is " << mask.extent(0) << ".");
      }
      else // (mask.isEmpty())
      { // If no mask was defined, we create a default one without any masked point.
         mask.resize(n);
         for (int i = 0; i < n; ++i)
         {
           mask(i) = true;
         }
      }
   }

   /*!
     Check validity of bounds info and fill in values if any.
   */
   void CAxis::checkBounds()
   {
     if (!bounds.isEmpty())
     {
       if (bounds.extent(0) != 2 || bounds.extent(1) != n)
         ERROR("CAxis::checkAttributes(void)",
               << "The bounds array of the axis [ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] must be of dimension 2 x axis size." << std::endl
               << "Axis size is " << n.getValue() << "." << std::endl
               << "Bounds size is "<< bounds.extent(0) << " x " << bounds.extent(1) << ".");
       hasBounds_ = true;
     }
     else hasBounds_ = false;
   }

  void CAxis::checkLabel()
  {
    if (!label.isEmpty())
    {
      if (label.extent(0) != n)
        ERROR("CAxis::checkLabel(void)",
              << "The label array of the axis [ id = '" << getId() << "' , context = '" << CObjectFactory::GetCurrentContextId() << "' ] must be of dimension of axis size." << std::endl
              << "Axis size is " << n.getValue() << "." << std::endl
              << "label size is "<< label.extent(0)<<  " .");
      hasLabel = true;
    }
    else hasLabel = false;
  }

  void CAxis::checkEligibilityForCompressedOutput()
  {
    // We don't check if the mask is valid here, just if a mask has been defined at this point.
    isCompressible_ = !mask.isEmpty();
  }

  bool CAxis::zoomByIndex()
  {
    return (!global_zoom_index.isEmpty() && (0 != global_zoom_index.numElements()));
  }

   bool CAxis::dispatchEvent(CEventServer& event)
   {
      if (SuperClass::dispatchEvent(event)) return true;
      else
      {
        switch(event.type)
        {
           case EVENT_ID_DISTRIBUTION_ATTRIBUTE :
             recvDistributionAttribute(event);
             return true;
             break;
          case EVENT_ID_NON_DISTRIBUTED_ATTRIBUTES:
            recvNonDistributedAttributes(event);
            return true;
            break;
          case EVENT_ID_DISTRIBUTED_ATTRIBUTES:
            recvDistributedAttributes(event);
            return true;
            break;
           default :
             ERROR("bool CAxis::dispatchEvent(CEventServer& event)",
                    << "Unknown Event");
           return false;
         }
      }
   }

   /*!
     Check attributes on client side (This name is still adequate???)
   */
   void CAxis::checkAttributesOnClient()
   {
     if (this->areClientAttributesChecked_) return;

     this->checkAttributes();

     this->areClientAttributesChecked_ = true;
   }

   /*
     The (spatial) transformation sometimes can change attributes of an axis. Therefore, we should recheck them.
   */
   void CAxis::checkAttributesOnClientAfterTransformation(const std::vector<int>& globalDim, int orderPositionInGrid,
                                                          CServerDistributionDescription::ServerDistributionType distType)
   {
     CContext* context=CContext::getCurrent() ;

     if (this->isClientAfterTransformationChecked) return;
     if (context->hasClient)
     {
       if (index.numElements() != n_glo.getValue()) computeConnectedServer(globalDim, orderPositionInGrid, distType);
     }

     this->isClientAfterTransformationChecked = true;
   }

   // Send all checked attributes to server
   void CAxis::sendCheckedAttributes(const std::vector<int>& globalDim, int orderPositionInGrid,
                                     CServerDistributionDescription::ServerDistributionType distType)
   {
     if (!this->areClientAttributesChecked_) checkAttributesOnClient();
     if (!this->isClientAfterTransformationChecked) checkAttributesOnClientAfterTransformation(globalDim, orderPositionInGrid, distType);
     CContext* context = CContext::getCurrent();

     if (this->isChecked) return;
     if (context->hasClient) sendAttributes(globalDim, orderPositionInGrid, distType);    

     this->isChecked = true;
   }

  /*!
    Send attributes from one client to other clients
    \param[in] globalDim global dimension of grid which contains this axis
    \param[in] order
  */
  void CAxis::sendAttributes(const std::vector<int>& globalDim, int orderPositionInGrid,
                             CServerDistributionDescription::ServerDistributionType distType)
  {
     if (index.numElements() == n_glo.getValue())
       sendNonDistributedAttributes();
     else
     {
       sendDistributedAttributes();       
     }
     sendDistributionAttribute(globalDim, orderPositionInGrid, distType);
  }

  void CAxis::computeConnectedServer(const std::vector<int>& globalDim, int orderPositionInGrid,
                                     CServerDistributionDescription::ServerDistributionType distType)
  {
    CContext* context = CContext::getCurrent();

    int nbSrvPools = (context->hasServer) ? (context->hasClient ? context->clientPrimServer.size() : 1) : 1;
    for (int p = 0; p < nbSrvPools; ++p)
    {
      CContextClient* client = (0 != context->clientPrimServer.size()) ? context->clientPrimServer[p] : context->client;
      int nbServer = client->serverSize;
      int range, clientSize = client->clientSize;
      int rank = client->clientRank;

      size_t ni = this->n.getValue();
      size_t ibegin = this->begin.getValue();
      size_t global_zoom_end = global_zoom_begin+global_zoom_n-1;
      size_t nZoomCount = 0;
      size_t nbIndex = index.numElements();

      for (size_t idx = 0; idx < nbIndex; ++idx)
      {
        globalLocalIndexMap_[index(idx)] = idx;
      }
      std::set<int> writtenInd;
      if (isCompressible_)
      {
        for (int idx = 0; idx < data_index.numElements(); ++idx)
        {
          int ind = CDistributionClient::getAxisIndex(data_index(idx), data_begin, ni);

          if (ind >= 0 && ind < ni && mask(ind))
          {
            ind += ibegin;
            if (ind >= global_zoom_begin && ind <= global_zoom_end)
              writtenInd.insert(ind);
          }
        }
      }

      CServerDistributionDescription serverDescriptionGlobal(globalDim, nbServer, distType);
      int distributedDimensionOnServer = serverDescriptionGlobal.getDimensionDistributed();
      CClientServerMapping::GlobalIndexMap globalIndexAxisOnServer;
      if (distributedDimensionOnServer == orderPositionInGrid) // So we have distributed axis on client side and also on server side*
      {
        std::vector<int> nGlobAxis(1);
        nGlobAxis[0] = n_glo.getValue();

        size_t globalSizeIndex = 1, indexBegin, indexEnd;
        for (int i = 0; i < nGlobAxis.size(); ++i) globalSizeIndex *= nGlobAxis[i];
        indexBegin = 0;
        if (globalSizeIndex <= clientSize)
        {
          indexBegin = rank%globalSizeIndex;
          indexEnd = indexBegin;
        }
        else
        {
          for (int i = 0; i < clientSize; ++i)
          {
            range = globalSizeIndex / clientSize;
            if (i < (globalSizeIndex%clientSize)) ++range;
            if (i == client->clientRank) break;
            indexBegin += range;
          }
          indexEnd = indexBegin + range - 1;
        }

        CArray<size_t,1> globalIndex(index.numElements());
        for (size_t idx = 0; idx < globalIndex.numElements(); ++idx)
          globalIndex(idx) = index(idx);

        CServerDistributionDescription serverDescription(nGlobAxis, nbServer);
        serverDescription.computeServerGlobalIndexInRange(std::make_pair<size_t,size_t>(indexBegin, indexEnd));
        CClientServerMapping* clientServerMap = new CClientServerMappingDistributed(serverDescription.getGlobalIndexRange(), client->intraComm);
        clientServerMap->computeServerIndexMapping(globalIndex);
        globalIndexAxisOnServer = clientServerMap->getGlobalIndexOnServer();
        delete clientServerMap;
      }
      else
      {
        std::vector<size_t> globalIndexServer(n_glo.getValue());
        for (size_t idx = 0; idx < n_glo.getValue(); ++idx)
        {
          globalIndexServer[idx] = idx;
        }

        for (int idx = 0; idx < nbServer; ++idx)
        {
          globalIndexAxisOnServer[idx] = globalIndexServer;
        }
      }

      indSrv_.swap(globalIndexAxisOnServer);

      CClientServerMapping::GlobalIndexMap::const_iterator it  = indSrv_.begin(),
                                                           ite = indSrv_.end();

      connectedServerRank_.clear();
      for (it = indSrv_.begin(); it != ite; ++it) {
        connectedServerRank_.push_back(it->first);
      }

      nbConnectedClients_ = CClientServerMapping::computeConnectedClients(client->serverSize, client->clientSize, client->intraComm, connectedServerRank_);
    }
  }

   void CAxis::computeWrittenIndex()
   {  
      if (computedWrittenIndex_) return;
      computedWrittenIndex_ = true;

      CContext* context=CContext::getCurrent();      
      CContextServer* server = context->server; 

      std::vector<int> nBegin(1), nSize(1), nBeginGlobal(1), nGlob(1);
      nBegin[0]       = zoom_begin;
      nSize[0]        = zoom_n;   
      nBeginGlobal[0] = 0; 
      nGlob[0]        = n_glo;
      CDistributionServer srvDist(server->intraCommSize, nBegin, nSize, nBeginGlobal, nGlob); 
      const CArray<size_t,1>& writtenGlobalIndex  = srvDist.getGlobalIndex();

      size_t nbWritten = 0, indGlo;      
      boost::unordered_map<size_t,size_t>::const_iterator itb = globalLocalIndexMap_.begin(),
                                                          ite = globalLocalIndexMap_.end(), it;          
      CArray<size_t,1>::const_iterator itSrvb = writtenGlobalIndex.begin(),
                                       itSrve = writtenGlobalIndex.end(), itSrv;  
      if (!zoomByIndex())
      {   
        for (itSrv = itSrvb; itSrv != itSrve; ++itSrv)
        {
          indGlo = *itSrv;
          if (ite != globalLocalIndexMap_.find(indGlo))
          {          
            ++nbWritten;
          }                 
        }

        localIndexToWriteOnServer.resize(nbWritten);

        nbWritten = 0;
        for (itSrv = itSrvb; itSrv != itSrve; ++itSrv)
        {
          indGlo = *itSrv;
          if (ite != globalLocalIndexMap_.find(indGlo))
          {
            localIndexToWriteOnServer(nbWritten) = globalLocalIndexMap_[indGlo];
            ++nbWritten;
          }                 
        }
      }
      else
      {
        nbWritten = 0;
        boost::unordered_map<size_t,size_t>::const_iterator itb = globalLocalIndexMap_.begin(),
                                                            ite = globalLocalIndexMap_.end(), it;
        for (int i = 0; i < zoom_index.numElements(); ++i)
        {
           if (ite != globalLocalIndexMap_.find(zoom_index(i)))
            ++nbWritten;
        }

        localIndexToWriteOnServer.resize(nbWritten);

        nbWritten = 0;
        for (int i = 0; i < zoom_index.numElements(); ++i)
        {
           if (ite != globalLocalIndexMap_.find(zoom_index(i)))
           {
             localIndexToWriteOnServer(nbWritten) = globalLocalIndexMap_[zoom_index(i)];
             ++nbWritten;
           }
        }
      }

      if (isCompressible())
      {
        nbWritten = 0;
        boost::unordered_map<size_t,size_t> localGlobalIndexMap;
        for (itSrv = itSrvb; itSrv != itSrve; ++itSrv)
        {
          indGlo = *itSrv;
          if (ite != globalLocalIndexMap_.find(indGlo))
          {
            localGlobalIndexMap[localIndexToWriteOnServer(nbWritten)] = indGlo;
            ++nbWritten;
          }                 
        }

        nbWritten = 0;
        for (int idx = 0; idx < data_index.numElements(); ++idx)
        {
          if (localGlobalIndexMap.end() != localGlobalIndexMap.find(data_index(idx)))
          {
            ++nbWritten;
          }
        }

        compressedIndexToWriteOnServer.resize(nbWritten);
        nbWritten = 0;
        for (int idx = 0; idx < data_index.numElements(); ++idx)
        {
          if (localGlobalIndexMap.end() != localGlobalIndexMap.find(data_index(idx)))
          {
            compressedIndexToWriteOnServer(nbWritten) = localGlobalIndexMap[data_index(idx)];
            ++nbWritten;
          }
        }

        numberWrittenIndexes_ = nbWritten;
        if (isDistributed())
        {
               
          MPI_Allreduce(&numberWrittenIndexes_, &totalNumberWrittenIndexes_, 1, MPI_INT, MPI_SUM, server->intraComm);
          MPI_Scan(&numberWrittenIndexes_, &offsetWrittenIndexes_, 1, MPI_INT, MPI_SUM, server->intraComm);
          offsetWrittenIndexes_ -= numberWrittenIndexes_;
        }
        else
          totalNumberWrittenIndexes_ = numberWrittenIndexes_;
      }

   }



  void CAxis::sendDistributionAttribute(const std::vector<int>& globalDim, int orderPositionInGrid,
                                        CServerDistributionDescription::ServerDistributionType distType)
  {
    CContext* context = CContext::getCurrent();

    int nbSrvPools = (context->hasServer) ? (context->hasClient ? context->clientPrimServer.size() : 0) : 1;
    for (int i = 0; i < nbSrvPools; ++i)
    {
      CContextClient* contextClientTmp = (context->hasServer) ? context->clientPrimServer[i]
                                                                         : context->client;
      int nbServer = contextClientTmp->serverSize;

      CServerDistributionDescription serverDescription(globalDim, nbServer);
      serverDescription.computeServerDistribution();

      std::vector<std::vector<int> > serverIndexBegin = serverDescription.getServerIndexBegin();
      std::vector<std::vector<int> > serverDimensionSizes = serverDescription.getServerDimensionSizes();

      globalDimGrid.resize(globalDim.size());
      for (int idx = 0; idx < globalDim.size(); ++idx) globalDimGrid(idx) = globalDim[idx];

      CEventClient event(getType(),EVENT_ID_DISTRIBUTION_ATTRIBUTE);
      if (contextClientTmp->isServerLeader())
      {
        std::list<CMessage> msgs;

        const std::list<int>& ranks = contextClientTmp->getRanksServerLeader();
        for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
        {
          // Use const int to ensure CMessage holds a copy of the value instead of just a reference
          const int begin = serverIndexBegin[*itRank][orderPositionInGrid];
          const int ni    = serverDimensionSizes[*itRank][orderPositionInGrid];
          const int end   = begin + ni - 1;          

          msgs.push_back(CMessage());
          CMessage& msg = msgs.back();
          msg << this->getId();
          msg << ni << begin << end;
          // msg << global_zoom_begin.getValue() << global_zoom_n.getValue();
          msg << isCompressible_;
          msg << orderPositionInGrid;
          msg << globalDimGrid;

          event.push(*itRank,1,msg);
        }
        contextClientTmp->sendEvent(event);
      }
      else contextClientTmp->sendEvent(event);
    }
  }

  void CAxis::sendNonDistributedAttributes()
  {
    CContext* context = CContext::getCurrent();

    int nbSrvPools = (context->hasServer) ? (context->hasClient ? context->clientPrimServer.size() : 1) : 1;
    for (int p = 0; p < nbSrvPools; ++p)
    {
      CContextClient* client = (0 != context->clientPrimServer.size()) ? context->clientPrimServer[p] : context->client;

      CEventClient event(getType(), EVENT_ID_NON_DISTRIBUTED_ATTRIBUTES);
      size_t nbIndex = index.numElements();
      size_t nbDataIndex = 0;

      for (int idx = 0; idx < data_index.numElements(); ++idx)
      {
        int ind = data_index(idx);
        if (ind >= 0 && ind < nbIndex) ++nbDataIndex;
      }

      CArray<int,1> dataIndex(nbDataIndex);
      nbDataIndex = 0;
      for (int idx = 0; idx < data_index.numElements(); ++idx)
      {
        int ind = data_index(idx);
        if (ind >= 0 && ind < nbIndex)
        {
          dataIndex(nbDataIndex) = ind;
          ++nbDataIndex;
        }
      }

      if (client->isServerLeader())
      {
        std::list<CMessage> msgs;

        const std::list<int>& ranks = client->getRanksServerLeader();
        for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
        {
          msgs.push_back(CMessage());
          CMessage& msg = msgs.back();
          msg << this->getId();
          msg << index.getValue() << dataIndex << mask.getValue();
          msg << hasValue;
          if (hasValue) msg << value.getValue();
          msg << hasBounds_;
          if (hasBounds_) msg << bounds.getValue();

          event.push(*itRank, 1, msg);
        }
        client->sendEvent(event);
      }
      else client->sendEvent(event);
    }
  }

  void CAxis::recvNonDistributedAttributes(CEventServer& event)
  {
    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      CBufferIn* buffer = it->buffer;
      string axisId;
      *buffer >> axisId;
      get(axisId)->recvNonDistributedAttributes(it->rank, *buffer);
    }
  }

  void CAxis::recvNonDistributedAttributes(int rank, CBufferIn& buffer)
  { 
    CArray<int,1> tmp_index, tmp_data_index, tmp_zoom_index;
    CArray<bool,1> tmp_mask;
    CArray<double,1> tmp_val;
    CArray<double,2> tmp_bnds;

    buffer >> tmp_index;
    index.reference(tmp_index);
    buffer >> tmp_data_index;
    data_index.reference(tmp_data_index);
    buffer >> tmp_mask;
    mask.reference(tmp_mask);

    buffer >> hasValue;
    if (hasValue)
    {
      buffer >> tmp_val;
      value.reference(tmp_val);
    }

    buffer >> hasBounds_;
    if (hasBounds_)
    {
      buffer >> tmp_bnds;
      bounds.reference(tmp_bnds);
    }

    data_begin.setValue(0);
    globalLocalIndexMap_.rehash(std::ceil(index.numElements()/globalLocalIndexMap_.max_load_factor()));
    for (int idx = 0; idx < index.numElements(); ++idx) globalLocalIndexMap_[idx] = index(idx);
  }

  void CAxis::sendDistributedAttributes(void)
  {
    int ns, n, i, j, ind, nv, idx;
    CContext* context = CContext::getCurrent();
    
    int nbSrvPools = (context->hasServer) ? (context->hasClient ? context->clientPrimServer.size() : 1) : 1;
    for (int p = 0; p < nbSrvPools; ++p)
    {
      CContextClient* client = (0 != context->clientPrimServer.size()) ? context->clientPrimServer[p] : context->client;

      CEventClient eventData(getType(), EVENT_ID_DISTRIBUTED_ATTRIBUTES);

      list<CMessage> listData;
      list<CArray<int,1> > list_indi, list_dataInd, list_zoomInd;
      list<CArray<bool,1> > list_mask;
      list<CArray<double,1> > list_val;
      list<CArray<double,2> > list_bounds;

      int nbIndex = index.numElements();
      CArray<int,1> dataIndex(nbIndex);
      dataIndex = -1;
      for (int inx = 0; inx < data_index.numElements(); ++inx)
      {
        if (0 <= data_index(inx) && data_index(inx) < nbIndex)
          dataIndex(inx) = data_index(inx);
      }

      boost::unordered_map<int, std::vector<size_t> >::const_iterator it, iteMap;
      iteMap = indSrv_.end();
      for (int k = 0; k < connectedServerRank_.size(); ++k)
      {
        int nbData = 0;
        int rank = connectedServerRank_[k];
        int nbSendingClient = nbConnectedClients_[rank];
        it = indSrv_.find(rank);
        if (iteMap != it)
          nbData = it->second.size();

        list_indi.push_back(CArray<int,1>(nbData));
        list_dataInd.push_back(CArray<int,1>(nbData));        
        list_mask.push_back(CArray<bool,1>(nbData));

        if (hasValue)
          list_val.push_back(CArray<double,1>(nbData));

        if (hasBounds_)
        {
          list_bounds.push_back(CArray<double,2>(2,nbData));
        }

        CArray<int,1>& indi = list_indi.back();
        CArray<int,1>& dataIndi = list_dataInd.back();        
        CArray<bool,1>& maskIndi = list_mask.back();

        for (n = 0; n < nbData; ++n)
        {
          idx = static_cast<int>(it->second[n]);
          indi(n) = idx;

          ind = globalLocalIndexMap_[idx];
          dataIndi(n) = dataIndex(ind);
          maskIndi(n) = mask(ind);

          if (hasValue)
          {
            CArray<double,1>& val = list_val.back();
            val(n) = value(ind);
          }

          if (hasBounds_)
          {
            CArray<double,2>& boundsVal = list_bounds.back();
            boundsVal(0, n) = bounds(0,n);
            boundsVal(1, n) = bounds(1,n);
          }
        }

        listData.push_back(CMessage());
        listData.back() << this->getId()
                        << list_indi.back() << list_dataInd.back() << list_mask.back();

        listData.back() << hasValue;
        if (hasValue)
          listData.back() << list_val.back();

        listData.back() << hasBounds_;
        if (hasBounds_)
          listData.back() << list_bounds.back();

        eventData.push(rank, nbConnectedClients_[rank], listData.back());
      }

      client->sendEvent(eventData);
    }
  }

  void CAxis::recvDistributedAttributes(CEventServer& event)
  {
    string axisId;
    vector<int> ranks;
    vector<CBufferIn*> buffers;

    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      ranks.push_back(it->rank);
      CBufferIn* buffer = it->buffer;
      *buffer >> axisId;
      buffers.push_back(buffer);
    }
    get(axisId)->recvDistributedAttributes(ranks, buffers);
  }


  void CAxis::recvDistributedAttributes(vector<int>& ranks, vector<CBufferIn*> buffers)
  {
    int nbReceived = ranks.size();
    vector<CArray<int,1> > vec_indi(nbReceived), vec_dataInd(nbReceived), vec_zoomInd(nbReceived);    
    vector<CArray<bool,1> > vec_mask(nbReceived);
    vector<CArray<double,1> > vec_val(nbReceived);
    vector<CArray<double,2> > vec_bounds(nbReceived);
    
    for (int idx = 0; idx < nbReceived; ++idx)
    {      
      CBufferIn& buffer = *buffers[idx];
      buffer >> vec_indi[idx];
      buffer >> vec_dataInd[idx];      
      buffer >> vec_mask[idx];

      buffer >> hasValue;
      if (hasValue)
        buffer >> vec_val[idx];

      buffer >> hasBounds_;
      if (hasBounds_)
        buffer >> vec_bounds[idx];
    }

    int nbData = 0;
    for (int idx = 0; idx < nbReceived; ++idx)
    {
      nbData += vec_indi[idx].numElements();
    }

    index.resize(nbData);
    globalLocalIndexMap_.rehash(std::ceil(index.numElements()/globalLocalIndexMap_.max_load_factor()));
    CArray<int,1> nonCompressedData(nbData);    
    mask.resize(nbData);
    if (hasValue)
      value.resize(nbData);
    if (hasBounds_)
      bounds.resize(2,nbData);

    nbData = 0;
    for (int idx = 0; idx < nbReceived; ++idx)
    {
      CArray<int,1>& indi = vec_indi[idx];
      CArray<int,1>& dataIndi = vec_dataInd[idx];
      CArray<bool,1>& maskIndi = vec_mask[idx];
      int nb = indi.numElements();
      for (int n = 0; n < nb; ++n)
      {
        index(nbData) = indi(n);
        globalLocalIndexMap_[indi(n)] = nbData;
        nonCompressedData(nbData) = (0 <= dataIndi(n)) ? nbData : -1;
        mask(nbData) = maskIndi(n);
        if (hasValue)
          value(nbData) = vec_val[idx](n);
        if (hasBounds_)
        {
          bounds(0,nbData) = vec_bounds[idx](0,n);
          bounds(1,nbData) = vec_bounds[idx](1,n);
        }
        ++nbData;
      }
    }

    int nbIndex = index.numElements();
    int nbCompressedData = 0; 
    for (int idx = 0; idx < nonCompressedData.numElements(); ++idx)
    {
      if (0 <= nonCompressedData(idx) && nonCompressedData(idx) < nbIndex)
        ++nbCompressedData;        
    }

    data_index.resize(nbCompressedData);
    nbCompressedData = 0;
    for (int idx = 0; idx < nonCompressedData.numElements(); ++idx)
    {
      if (0 <= nonCompressedData(idx) && nonCompressedData(idx) < nbIndex)
      {
        data_index(nbCompressedData) = nonCompressedData(idx);
        ++nbCompressedData;        
      }
    }
    data_begin.setValue(0);

    if (hasLabel)
    {
      //label_srv(ind_srv) = labelVal( ind);
    }
  }

  void CAxis::recvDistributionAttribute(CEventServer& event)
  {
    CBufferIn* buffer = event.subEvents.begin()->buffer;
    string axisId;
    *buffer >> axisId;
    get(axisId)->recvDistributionAttribute(*buffer);
  }

  void CAxis::recvDistributionAttribute(CBufferIn& buffer)
  {
    int ni_srv, begin_srv, end_srv;
    int global_zoom_end, zoom_end;
    bool zoomIndex = zoomByIndex();
    
    std::vector<int> zoom_index_tmp;
    std::vector<int>::iterator itZoomBegin, itZoomEnd, itZoom;

    buffer >> ni_srv >> begin_srv >> end_srv;
    // buffer >> global_zoom_begin_tmp >> global_zoom_n_tmp;   
    buffer >> isCompressible_;
    buffer >> orderPosInGrid;
    buffer >> globalDimGrid;    

    // Set up new local size of axis on the receiving clients
    n.setValue(ni_srv);
    begin.setValue(begin_srv);

    // If we have zoom by index then process it
    if (zoomIndex)
    {
      zoom_index_tmp.resize(global_zoom_index.numElements());
      std::copy(global_zoom_index.begin(), global_zoom_index.end(), zoom_index_tmp.begin());
      std::sort(zoom_index_tmp.begin(), zoom_index_tmp.end());
      itZoomBegin = std::lower_bound(zoom_index_tmp.begin(), zoom_index_tmp.end(), begin_srv);
      itZoomEnd   = std::upper_bound(zoom_index_tmp.begin(), zoom_index_tmp.end(), end_srv);      
      int sz = std::distance(itZoomBegin, itZoomEnd);
      zoom_index.resize(sz);
      itZoom = itZoomBegin;
      for (int i = 0; i < sz; ++i, ++itZoom)
      {
        zoom_index(i) = *(itZoom);
      }
    }

    global_zoom_begin = zoomIndex ? 0 : global_zoom_begin ;
    global_zoom_n     = zoomIndex ? zoom_index_tmp.size() : global_zoom_n;
    global_zoom_end   = global_zoom_begin + global_zoom_n - 1;

    zoom_begin = zoomIndex ? std::distance(itZoomBegin, zoom_index_tmp.begin())
                           : global_zoom_begin > begin_srv ? global_zoom_begin : begin_srv ;
    zoom_end   = zoomIndex ? std::distance(zoom_index_tmp.begin(), itZoomEnd) - 1 
                           : global_zoom_end < end_srv ? global_zoom_end : end_srv ;
    zoom_n     = zoom_end - zoom_begin + 1;

    if (zoom_n<=0)
    {
      zoom_begin = 0; zoom_n = 0;
    }

    if (n_glo == n)
    {
      zoom_begin = zoomIndex ? std::distance(itZoomBegin, zoom_index_tmp.begin())
                             : global_zoom_begin;      
      zoom_n     = zoomIndex ? zoom_index_tmp.size() : global_zoom_n;
    }
  }

  /*!
    Compare two axis objects. 
    They are equal if only if they have identical attributes as well as their values.
    Moreover, they must have the same transformations.
  \param [in] axis Compared axis
  \return result of the comparison
  */
  bool CAxis::isEqual(CAxis* obj)
  {
    vector<StdString> excludedAttr;
    excludedAttr.push_back("axis_ref");

    bool objEqual = SuperClass::isEqual(obj, excludedAttr);    
    if (!objEqual) return objEqual;

    TransMapTypes thisTrans = this->getAllTransformations();
    TransMapTypes objTrans  = obj->getAllTransformations();

    TransMapTypes::const_iterator it, itb, ite;
    std::vector<ETranformationType> thisTransType, objTransType;
    for (it = thisTrans.begin(); it != thisTrans.end(); ++it)
      thisTransType.push_back(it->first);
    for (it = objTrans.begin(); it != objTrans.end(); ++it)
      objTransType.push_back(it->first);

    if (thisTransType.size() != objTransType.size()) return false;
    for (int idx = 0; idx < thisTransType.size(); ++idx)
      objEqual &= (thisTransType[idx] == objTransType[idx]);

    return objEqual;
  }

  CTransformation<CAxis>* CAxis::addTransformation(ETranformationType transType, const StdString& id)
  {
    transformationMap_.push_back(std::make_pair(transType, CTransformation<CAxis>::createTransformation(transType,id)));
    return transformationMap_.back().second;
  }

  bool CAxis::hasTransformation()
  {
    return (!transformationMap_.empty());
  }

  void CAxis::setTransformations(const TransMapTypes& axisTrans)
  {
    transformationMap_ = axisTrans;
  }

  CAxis::TransMapTypes CAxis::getAllTransformations(void)
  {
    return transformationMap_;
  }

  void CAxis::duplicateTransformation(CAxis* src)
  {
    if (src->hasTransformation())
    {
      this->setTransformations(src->getAllTransformations());
    }
  }

  /*!
   * Go through the hierarchy to find the axis from which the transformations must be inherited
   */
  void CAxis::solveInheritanceTransformation()
  {
    if (hasTransformation() || !hasDirectAxisReference())
      return;

    CAxis* axis = this;
    std::vector<CAxis*> refAxis;
    while (!axis->hasTransformation() && axis->hasDirectAxisReference())
    {
      refAxis.push_back(axis);
      axis = axis->getDirectAxisReference();
    }

    if (axis->hasTransformation())
      for (size_t i = 0; i < refAxis.size(); ++i)
        refAxis[i]->setTransformations(axis->getAllTransformations());
  }

  void CAxis::parse(xml::CXMLNode & node)
  {
    SuperClass::parse(node);

    if (node.goToChildElement())
    {
      StdString nodeElementName;
      do
      {
        StdString nodeId("");
        if (node.getAttributes().end() != node.getAttributes().find("id"))
        { nodeId = node.getAttributes()["id"]; }

        nodeElementName = node.getElementName();
        std::map<StdString, ETranformationType>::const_iterator ite = transformationMapList_.end(), it;
        it = transformationMapList_.find(nodeElementName);
        if (ite != it)
        {
          transformationMap_.push_back(std::make_pair(it->second, CTransformation<CAxis>::createTransformation(it->second,
                                                                                                               nodeId,
                                                                                                               &node)));
        }
        else
        {
          ERROR("void CAxis::parse(xml::CXMLNode & node)",
                << "The transformation " << nodeElementName << " has not been supported yet.");
        }
      } while (node.goToNextElement()) ;
      node.goToParentElement();
    }
  }

  DEFINE_REF_FUNC(Axis,axis)

   ///---------------------------------------------------------------

} // namespace xios
