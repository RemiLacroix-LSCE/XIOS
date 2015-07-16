#include "domain.hpp"

#include "attribute_template.hpp"
#include "object_template.hpp"
#include "group_template.hpp"

#include "xios_spl.hpp"
#include "event_client.hpp"
#include "event_server.hpp"
#include "buffer_in.hpp"
#include "message.hpp"
#include "type.hpp"
#include "context.hpp"
#include "context_client.hpp"
#include "array_new.hpp"
#include "server_distribution_description.hpp"
#include "client_server_mapping_distributed.hpp"
#include "zoom_domain.hpp"

namespace xios {

   /// ////////////////////// Définitions ////////////////////// ///

   CDomain::CDomain(void)
      : CObjectTemplate<CDomain>(), CDomainAttributes()
      , isChecked(false), relFiles(), isClientChecked(false), nbConnectedClients_(), indSrv_(), connectedServerRank_()
      , hasBounds(false), hasArea(false), isDistributed_(false)
      , global_zoom_ibegin(0), global_zoom_ni(0), global_zoom_jbegin(0), global_zoom_nj(0)
   { /* Ne rien faire de plus */ }

   CDomain::CDomain(const StdString & id)
      : CObjectTemplate<CDomain>(id), CDomainAttributes()
      , isChecked(false), relFiles(), isClientChecked(false), nbConnectedClients_(), indSrv_(), connectedServerRank_()
      , hasBounds(false), hasArea(false), isDistributed_(false)
      , global_zoom_ibegin(0), global_zoom_ni(0), global_zoom_jbegin(0), global_zoom_nj(0)
   { /* Ne rien faire de plus */ }

   CDomain::~CDomain(void)
   {
   }

   ///---------------------------------------------------------------

   CDomain* CDomain::createDomain()
   {
     CDomain* domain = CDomainGroup::get("domain_definition")->createChild();
     return domain;
   }

   void CDomain::duplicateAttributes(CDomain* domain)
   {
     domain->setAttributes(this);
   }

   const std::set<StdString> & CDomain::getRelFiles(void) const
   {
      return (this->relFiles);
   }

   //----------------------------------------------------------------

   bool CDomain::hasZoom(void) const
   {
      return ((this->zoom_ni.getValue() != this->ni_glo.getValue()) &&
              (this->zoom_nj.getValue() != this->nj_glo.getValue()));
   }

   //----------------------------------------------------------------

   bool CDomain::isEmpty(void) const
   {
      return ((this->zoom_ni_srv == 0) ||
              (this->zoom_nj_srv == 0));
   }

   //----------------------------------------------------------------

   bool CDomain::IsWritten(const StdString & filename) const
   {
      return (this->relFiles.find(filename) != this->relFiles.end());
   }

   //----------------------------------------------------------------

   bool CDomain::isDistributed(void) const
   {
      return isDistributed_;
   }

   //----------------------------------------------------------------

   void CDomain::addRelFile(const StdString & filename)
   {
      this->relFiles.insert(filename);
   }


   StdString CDomain::GetName(void)   { return (StdString("domain")); }
   StdString CDomain::GetDefName(void){ return (CDomain::GetName()); }
   ENodeType CDomain::GetType(void)   { return (eDomain); }

   //----------------------------------------------------------------

   void CDomain::checkDomain(void)
   {
      if (!type.isEmpty() && type==type_attr::unstructured)
      {
         if (ni_glo.isEmpty() || ni_glo <= 0 )
         {
            ERROR("CDomain::checkAttributes(void)",
               << "[ Id = " << this->getId() << " ] "
               << "The global domain is badly defined,"
               << " check the \'ni_glo\'  value !")
         }
         nj_glo=ni_glo ;
         ni_glo=1 ;
         if (!ni.isEmpty()) nj=ni ;
         if (!ibegin.isEmpty()) jbegin=ibegin ;
         if (!iend.isEmpty()) jend=iend ;
         if (!i_index.isEmpty())
         {
          j_index.resize(1,nj) ;
          for(int i=0;i<ni;i++) j_index(0,i)=i_index(i,0) ;
          i_index.resize(1,nj) ;
          for(int j=0;j<nj;j++) i_index(0,j)=0 ;
         }

         if (!mask.isEmpty())
         {
          CArray<int,2> mask_tmp(nj,1) ;
          mask_tmp = mask ;
          mask.resize(1,nj) ;
          for(int j=0;j<nj;j++) mask(0,j)=mask_tmp(j,0) ;
         }

         if (!area.isEmpty())
           area.transposeSelf(1, 0);

         ni=1 ;
         ibegin=0 ;
         iend=0 ;

      }
      else if ((ni_glo.isEmpty() || ni_glo.getValue() <= 0 ) ||
          (nj_glo.isEmpty() || nj_glo.getValue() <= 0 ))
      {
         ERROR("CDomain::checkAttributes(void)",
               << "[ Id = " << this->getId() << " ] "
               << "The global domain is badly defined,"
               << " check the \'ni_glo\' et \'nj_glo\' values !")
      }

      isDistributed_ = !ibegin.isEmpty() || !iend.isEmpty() || !ni.isEmpty() || !jbegin.isEmpty() || !jend.isEmpty() || !nj.isEmpty();

      checkLocalIDomain();
      checkLocalJDomain();

      ibegin_client = ibegin; iend_client = iend; ni_client = ni;
      jbegin_client = jbegin; jend_client = jend; nj_client = nj;

      if (i_index.isEmpty())
      {
        i_index.resize(ni,nj);
        for (int j = 0; j < nj; j++)
          for (int i = 0; i < ni; i++) i_index(i,j) = i;
      }

      if (j_index.isEmpty())
      {
        j_index.resize(ni,nj);
        for (int j = 0; j < nj; j++)
          for (int i = 0; i < ni; i++) j_index(i,j) = j;
      }
   }

   //----------------------------------------------------------------

   void CDomain::checkLocalIDomain(void)
   {
      if (!ni.isEmpty() && !ibegin.isEmpty() && iend.isEmpty())
        iend.setValue(ibegin.getValue() + ni.getValue() - 1);
      else if (!ni.isEmpty() && !iend.isEmpty() && ibegin.isEmpty())
        ibegin.setValue(iend.getValue() - ni.getValue()  + 1);
      else if (!ibegin.isEmpty() && !iend.isEmpty() && ni.isEmpty())
        ni.setValue(iend.getValue() - ibegin.getValue() + 1);
      else if (!ibegin.isEmpty() && !iend.isEmpty() && !ni.isEmpty())
      {
        if (iend.getValue() != ibegin.getValue() + ni.getValue() - 1)
          ERROR("CDomain::checkLocalIDomain(void)",
                << "[ Id = " << this->getId() << " ] "
                << "The local domain is wrongly defined,"
                << " iend is different from (ibegin + ni - 1)");
      }
      else if (ibegin.isEmpty() && iend.isEmpty() && ni.isEmpty())
      {
        ibegin = 0;
        iend = ni_glo - 1;
        ni = ni_glo;
      }
      else
      {
        ERROR("CDomain::checkLocalIDomain(void)",
              << "[ Id = " << this->getId() << " ] "
              << "The local domain is wrongly defined,"
              << " defining just one attribute among 'ibegin', 'iend' or 'ni' is invalid");
      }

      if (ni.getValue() < 0 || ibegin.getValue() > iend.getValue() ||
          ibegin.getValue() < 0 || iend.getValue() > (ni_glo.getValue() - 1))
      {
        ERROR("CDomain::checkLocalIDomain(void)",
              << "[ Id = " << this->getId() << " ] "
              << "The local domain is wrongly defined,"
              << " check the attributes 'ni_glo', 'ni', 'ibegin' and 'iend'");
      }
   }

   //----------------------------------------------------------------

   void CDomain::checkLocalJDomain(void)
   {
      if (!nj.isEmpty() && !jbegin.isEmpty() && jend.isEmpty())
        jend.setValue(jbegin.getValue() + nj.getValue() - 1);
      else if (!nj.isEmpty() && !jend.isEmpty() && jbegin.isEmpty())
        jbegin.setValue(jend.getValue() - nj.getValue() + 1);
      else if (!jbegin.isEmpty() && !jend.isEmpty() && nj.isEmpty())
        nj.setValue(jend.getValue() - jbegin.getValue() + 1);
      else if (!jbegin.isEmpty() && !jend.isEmpty() && !nj.isEmpty())
      {
        if (jend.getValue() != jbegin.getValue() + nj.getValue() - 1)
          ERROR("CDomain::checkLocalJDomain(void)",
                << "[ Id = " << this->getId() << " ] "
                << "The local domain is wrongly defined,"
                << " jend is different from (jbegin + nj - 1)");
      }
      else if (jbegin.isEmpty() && jend.isEmpty() && nj.isEmpty())
      {
        jbegin = 0;
        jend = nj_glo - 1;
        nj = nj_glo;
      }
      else
      {
        ERROR("CDomain::checkLocalJDomain(void)",
              << "[ Id = " << this->getId() << " ] "
              << "The local domain is wrongly defined,"
              << " defining just one attribute among 'jbegin', 'jend' or 'nj' is invalid");
      }

      if (nj.getValue() < 0 || jbegin.getValue() > jend.getValue() ||
          jbegin.getValue() < 0 || jend.getValue() > (nj_glo.getValue() - 1))
      {
        ERROR("CDomain::checkLocalJDomain(void)",
              << "[ Id = " << this->getId() << " ] "
              << "The local domain is wrongly defined,"
              << " check the attributes 'nj_glo', 'nj', 'jbegin' and 'jend'");
      }
   }

   //----------------------------------------------------------------

   void CDomain::checkMask(void)
   {
      using namespace std;

      int ibegin_mask = 0,
          jbegin_mask = 0,
          iend_mask = iend.getValue() - ibegin.getValue(),
          jend_mask = jend.getValue() - jbegin.getValue();

      if (!zoom_ibegin.isEmpty())
      {
         int zoom_iend = zoom_ibegin.getValue() + zoom_ni.getValue() - 1;
         int zoom_jend = zoom_jbegin.getValue() + zoom_nj.getValue() - 1;

         ibegin_mask = max (ibegin.getValue(), zoom_ibegin.getValue());
         jbegin_mask = max (jbegin.getValue(), zoom_jbegin.getValue());
         iend_mask   = min (iend.getValue(), zoom_iend);
         jend_mask   = min (jend.getValue(), zoom_jend);

         ibegin_mask -= ibegin.getValue();
         jbegin_mask -= jbegin.getValue();
         iend_mask   -= ibegin.getValue();
         jend_mask   -= jbegin.getValue();
      }


      if (!mask.isEmpty())
      {
         if ((mask.extent(0) != ni) ||
             (mask.extent(1) != nj))
            ERROR("CDomain::checkAttributes(void)",
                  <<"the mask has not the same size than the local domain"<<endl
                   <<"Local size is "<<ni<<"x"<<nj<<endl
                  <<"Mask size is "<<mask.extent(0)<<"x"<<mask.extent(1));
         for (int i = 0; i < ni; i++)
         {
            for (int j = 0; j < nj; j++)
            {
               if (i < ibegin_mask && i > iend_mask &&
                   j < jbegin_mask && j > jend_mask )
                     mask(i,j) = false;
            }
         }
      }
      else // (!mask.hasValue())
      { // Si aucun masque n'est défini,
        // on en crée un nouveau qui valide l'intégralité du domaine.
         mask.resize(ni,nj) ;
         for (int i = 0; i < ni.getValue(); i++)
         {
            for (int j = 0; j < nj.getValue(); j++)
            {
               if (i >= ibegin_mask && i <= iend_mask &&
                   j >= jbegin_mask && j <= jend_mask )
                     mask(i,j) = true;
               else  mask(i,j) = false;
            }
         }
      }
   }


   //----------------------------------------------------------------

   void CDomain::checkDomainData(void)
   {
      if (!data_dim.isEmpty() &&
         !(data_dim.getValue() == 1 || data_dim.getValue() == 2))
      {
         ERROR("CDomain::checkAttributes(void)",
               << "Data dimension incompatible (must be 1 or 2) !") ;
      }
      else if (data_dim.isEmpty())
      {
         ERROR("CDomain::checkAttributes(void)",
               << "Data dimension undefined !") ;
      }

      if (data_ibegin.isEmpty())
         data_ibegin.setValue(0) ;
      if (data_jbegin.isEmpty() && (data_dim.getValue() == 2))
         data_jbegin.setValue(0) ;

      if (!data_ni.isEmpty() && (data_ni.getValue() <= 0))
      {
         ERROR("CDomain::checkAttributes(void)",
               << "Data dimension is negative (data_ni).") ;
      }
      else if (data_ni.isEmpty())
      {
         data_ni.setValue((data_dim.getValue() == 1)
                           ? (ni.getValue() * nj.getValue())
                           : ni.getValue());
      }

      if (data_dim.getValue() == 2)
      {
         if (!data_nj.isEmpty() && (data_nj.getValue() <= 0) )
         {
            ERROR("CDomain::checkAttributes(void)",
                  << "Data dimension is negative (data_nj).") ;
         }
         else if (data_nj.isEmpty())
            data_nj.setValue(nj.getValue()) ;
      }

   }

   //----------------------------------------------------------------

   void CDomain::checkCompression(void)
   {
      if (!data_i_index.isEmpty())
      {
         int ssize = data_i_index.numElements();
         if (!data_n_index.isEmpty() &&
            (data_n_index.getValue() != ssize))
         {
            ERROR("CDomain::checkAttributes(void)",
                  <<"Dimension data_i_index incompatible with data_n_index.") ;
         }
         else if (data_n_index.isEmpty())
            data_n_index.setValue(ssize) ;

         if (data_dim.getValue() == 2)
         {
            if (!data_j_index.isEmpty() &&
               (data_j_index.numElements() != data_i_index.numElements()))
            {
               ERROR("CDomain::checkAttributes(void)",
                     <<"Dimension data_j_index incompatible with data_i_index.") ;
            }
            else if (data_j_index.isEmpty())
            {
               ERROR("CDomain::checkAttributes(void)",
                     <<"data_j_index must be defined !") ;
            }
         }
      }
      else
      {
         if (!data_n_index.isEmpty() ||
            ((data_dim.getValue() == 2) && (!data_j_index.isEmpty())))
            ERROR("CDomain::checkAttributes(void)", << "data_i_index undefined") ;
      }

      if (data_n_index.isEmpty())
      { // -> bloc re-vérifié OK
         if (data_dim.getValue() == 1)
         {
            const int dni = data_ni.getValue();
            data_i_index.resize(dni) ;
            data_n_index.setValue(dni);
            for (int i = 0; i < dni; i++) data_i_index(i) = i+1 ;
         }
         else   // (data_dim == 2)
         {
            const int dni = data_ni.getValue() * data_nj.getValue();
            data_i_index.resize(dni) ;
            data_j_index.resize(dni) ;

            data_n_index.setValue(dni);

            for(int count = 0, j = 0; j  < data_nj.getValue(); j++)
            {
               for(int i = 0; i < data_ni.getValue(); i++, count++)
               {
                  data_i_index(count) = i+1 ;
                  data_j_index(count) = j+1 ;
               }
            }
         }
      }
   }

   //----------------------------------------------------------------

   void CDomain::completeLonLatClient(void)
   {
      int i,j,k ;
      CArray<double,1> lonvalue_temp(ni*nj) ;
      CArray<double,1> latvalue_temp(ni*nj) ;
      CArray<double,2> bounds_lon_temp(nvertex,ni*nj) ;
      CArray<double,2> bounds_lat_temp(nvertex,ni*nj) ;

      if (type.isEmpty())
      {
        if ( lonvalue.numElements() == ni*nj && latvalue.numElements() == ni*nj )
        {
          type.setValue(type_attr::curvilinear) ;
          isCurvilinear=true ;
        }
        else if ( lonvalue.numElements() == ni && latvalue.numElements() == nj )
        {
          type.setValue(type_attr::regular) ;
          isCurvilinear=false ;
        }
        else ERROR("void CDomain::completeLonLatClient(void)",<<"the grid is nor curvilinear, nor cartesian, because the size of longitude and latitude array is not coherent with the domain size"<<endl
                                                              <<"lonvalue size = " << lonvalue.numElements() << "different of ni or ni*nj"<<endl
                                                              <<"latvalue size = " << latvalue.numElements() << "different of nj or ni*nj" ) ;
      }
      if (type==type_attr::curvilinear || type==type_attr::unstructured)
      {
        lonvalue_temp=lonvalue ;
        latvalue_temp=latvalue ;
        if (hasBounds) bounds_lon_temp=bounds_lon ;
        if (hasBounds) bounds_lat_temp=bounds_lat ;
      }
      else
      {
        for(j=0;j<nj;j++)
          for(i=0;i<ni;i++)
          {
            k=j*ni+i ;
            lonvalue_temp(k)=lonvalue(i) ;
            latvalue_temp(k)=latvalue(j) ;
            if (hasBounds)
            {
              for(int n=0;n<nvertex;n++)
              {
                bounds_lon_temp(n,k)=bounds_lon(n,i) ;
                bounds_lat_temp(n,k)=bounds_lat(n,j) ;
              }
            }
          }
      }

      StdSize dm = zoom_ni_client * zoom_nj_client;

      // Make sure that this attribute is non-empty for every client.
      if (0 != dm)
      {
        lonvalue.resize(dm);
        latvalue.resize(dm);
      }


      for (int i = 0; i < zoom_ni_client; i++)
      {
        for (int j = 0; j < zoom_nj_client; j++)
        {
          lonvalue(i + j * zoom_ni_client) = lonvalue_temp( (i + zoom_ibegin_client-ibegin) + (j + zoom_jbegin_client -jbegin)*ni );
          latvalue(i + j * zoom_ni_client) = latvalue_temp( (i + zoom_ibegin_client - ibegin)+(j + zoom_jbegin_client - jbegin)*ni );
          if (hasBounds)
          {
            for(int n=0;n<nvertex;n++)
            {
              bounds_lon(n,i + j * zoom_ni_client) = bounds_lon_temp( n, (i + zoom_ibegin_client - ibegin) + (j + zoom_jbegin_client -jbegin)*ni );
              bounds_lat(n,i + j * zoom_ni_client) = bounds_lat_temp( n, (i + zoom_ibegin_client - ibegin)+(j + zoom_jbegin_client -jbegin)*ni );
            }
          }
        }
      }
    }


   //----------------------------------------------------------------

   void CDomain::checkZoom(void)
   {
      // Résolution et vérification des données globales de zoom.
      if (!this->zoom_ni.isEmpty() || !this->zoom_nj.isEmpty() ||
          !this->zoom_ibegin.isEmpty() || !this->zoom_jbegin.isEmpty())
      {
         if (this->zoom_ni.isEmpty()     || this->zoom_nj.isEmpty() ||
             this->zoom_ibegin.isEmpty() || this->zoom_jbegin.isEmpty())
         {
            ERROR("CDomain::checkZoom(void)",
                  <<"if one of zoom attributes is defined then all zoom attributes must be defined") ;
         }
         else
         {
            int zoom_iend = zoom_ibegin + zoom_ni - 1;
            int zoom_jend = zoom_jbegin + zoom_nj - 1;

            if (zoom_ibegin < 0  || zoom_jbegin < 0 || zoom_iend > (ni_glo-1) || zoom_jend > (nj_glo-1))
               ERROR("CDomain::checkZoom(void)",
                     << "Zoom is wrongly defined,"
                     << " Check the values : zoom_ni, zoom_nj, zoom_ibegin, zoom_jbegin") ;
         }
      }
      else
      {
         zoom_ni = ni_glo;
         zoom_nj = nj_glo;
         zoom_ibegin = 0;
         zoom_jbegin = 0;
      }

      // compute client zoom indices
      // compute client zoom indices
      if (0 == global_zoom_ni) global_zoom_ni = ni_glo;
      if (0 == global_zoom_nj) global_zoom_nj = nj_glo;

      int global_zoom_iend=global_zoom_ibegin+global_zoom_ni-1 ;
      zoom_ibegin_client = ibegin_client > global_zoom_ibegin ? ibegin_client : global_zoom_ibegin ;
      zoom_iend_client = iend_client < global_zoom_iend ? iend_client : global_zoom_iend ;
      zoom_ni_client=zoom_iend_client-zoom_ibegin_client+1 ;
      if (zoom_ni_client<0) zoom_ni_client=0 ;


      int global_zoom_jend=global_zoom_jbegin+global_zoom_nj-1 ;
      zoom_jbegin_client = jbegin_client > global_zoom_jbegin ? jbegin_client : global_zoom_jbegin ;
      zoom_jend_client = jend_client < global_zoom_jend ? jend_client : global_zoom_jend ;
      zoom_nj_client=zoom_jend_client-zoom_jbegin_client+1 ;
      if (zoom_nj_client<0) zoom_nj_client=0;
   }

   void CDomain::checkBounds(void)
   {
     if (!nvertex.isEmpty() && !bounds_lon.isEmpty() && !bounds_lat.isEmpty())
     {
       hasBounds=true ;
     }
     else
     {
       hasBounds=false;
       nvertex=0 ;
     }
   }

   void CDomain::checkArea(void)
   {
     hasArea = !area.isEmpty();
     if (hasArea)
     {
       if (area.extent(0) != ni || area.extent(1) != nj)
       {
         ERROR("void CDomain::checkArea(void)",
               "The area attribute must be of size ni x nj.");
       }
     }
   }

   //----------------------------------------------------------------
   // Divide function checkAttributes into 2 seperate ones
   // This function only checks all attributes of current domain
   void CDomain::checkAttributesOnClient()
   {
     if (this->isClientChecked) return;
     CContext* context=CContext::getCurrent();

      this->checkDomain();
      this->checkBounds();
      this->checkArea();

      if (context->hasClient)
      { // Côté client uniquement
         this->checkMask();
         this->checkDomainData();
         this->checkCompression();
      }
      else
      { // Côté serveur uniquement
//         if (!this->isEmpty())
// ne sert plus //   this->completeLonLatServer();
      }

      this->isClientChecked = true;
   }

   // Send all checked attributes to server
   void CDomain::sendCheckedAttributes()
   {
     if (!this->isClientChecked) checkAttributesOnClient();
     CContext* context=CContext::getCurrent() ;

     this->checkZoom();
     if (this->isChecked) return;
     if (context->hasClient)
     {
       this->computeConnectedServer();
       this->completeLonLatClient();

       sendServerAttribut() ;
       sendLonLatArea() ;
     }

     this->isChecked = true;
   }

   void CDomain::checkAttributes(void)
   {
      if (this->isChecked) return;
      CContext* context=CContext::getCurrent() ;

      this->checkDomain();
      this->checkZoom();
      this->checkBounds();
      this->checkArea();

      if (context->hasClient)
      { // Côté client uniquement
         this->checkMask();
         this->checkDomainData();
         this->checkCompression();
         this->completeLonLatClient();
      }
      else
      { // Côté serveur uniquement
//         if (!this->isEmpty())
// ne sert plus //   this->completeLonLatServer();
      }

      if (context->hasClient)
      {
        computeConnectedServer() ;
        sendServerAttribut() ;
        sendLonLatArea() ;
      }

      this->isChecked = true;
   }

  void CDomain::sendServerAttribut(void)
  {
    std::vector<int> nGlobDomain(2);
    nGlobDomain[0] = ni_glo.getValue();
    nGlobDomain[1] = nj_glo.getValue();
    CServerDistributionDescription serverDescription(nGlobDomain);

    CContext* context = CContext::getCurrent();
    CContextClient* client = context->client;
    int nbServer = client->serverSize;

    serverDescription.computeServerDistribution(nbServer);
    std::vector<std::vector<int> > serverIndexBegin = serverDescription.getServerIndexBegin();
    std::vector<std::vector<int> > serverDimensionSizes = serverDescription.getServerDimensionSizes();

    CEventClient event(getType(),EVENT_ID_SERVER_ATTRIBUT);
    if (client->isServerLeader())
    {
      std::list<CMessage> msgs;

      const std::list<int>& ranks = client->getRanksServerLeader();
      for (std::list<int>::const_iterator itRank = ranks.begin(), itRankEnd = ranks.end(); itRank != itRankEnd; ++itRank)
      {
        // Use const int to ensure CMessage holds a copy of the value instead of just a reference
        const int ibegin_srv = serverIndexBegin[*itRank][0];
        const int jbegin_srv = serverIndexBegin[*itRank][1];
        const int ni_srv = serverDimensionSizes[*itRank][0];
        const int nj_srv = serverDimensionSizes[*itRank][1];
        const int iend_srv = ibegin_srv + ni_srv - 1;
        const int jend_srv = jbegin_srv + nj_srv - 1;

        msgs.push_back(CMessage());
        CMessage& msg = msgs.back();
        msg << this->getId() ;
        msg << ni_srv << ibegin_srv << iend_srv << nj_srv << jbegin_srv << jend_srv;
        msg << global_zoom_ni << global_zoom_ibegin << global_zoom_nj << global_zoom_jbegin;

        event.push(*itRank,1,msg);
      }
      client->sendEvent(event);
    }
    else client->sendEvent(event);
  }

  void CDomain::computeConnectedServer(void)
  {
    ibegin_client=ibegin ; iend_client=iend ; ni_client=ni ;
    jbegin_client=jbegin ; jend_client=jend ; nj_client=nj ;

    CContext* context=CContext::getCurrent() ;
    CContextClient* client=context->client ;
    int nbServer=client->serverSize;
    bool doComputeGlobalIndexServer = true;

    int i,j,i_ind,j_ind ;
    int zoom_iend=global_zoom_ibegin+global_zoom_ni-1 ;
    int zoom_jend=global_zoom_jbegin+global_zoom_nj-1 ;

    // Precompute number of index
    int globalIndexCountZoom = 0;
    for(j=0;j<nj;j++)
      for(i=0;i<ni;i++)
      {
        i_ind=ibegin+i_index(i,j) ;
        j_ind=jbegin+j_index(i,j) ;

        if (i_ind >= global_zoom_ibegin && i_ind <= zoom_iend && j_ind >= global_zoom_jbegin && j_ind <= zoom_jend)
        {
          ++globalIndexCountZoom;
        }
      }

    // Fill in index
    CArray<size_t,1> globalIndexDomainZoom(globalIndexCountZoom);
    CArray<size_t,1> globalIndexDomain(ni*nj);
    size_t globalIndex;
    int globalIndexCount = 0;
    globalIndexCountZoom = 0;

    for(j=0;j<nj;j++)
      for(i=0;i<ni;i++)
      {
        i_ind=ibegin+i_index(i,j) ;
        j_ind=jbegin+j_index(i,j) ;

        globalIndex = i_ind + j_ind * ni_glo;
        globalIndexDomain(globalIndexCount) = globalIndex;
        ++globalIndexCount;
        if (i_ind >= global_zoom_ibegin && i_ind <= zoom_iend && j_ind >= global_zoom_jbegin && j_ind <= zoom_jend)
        {
          globalIndexDomainZoom(globalIndexCountZoom) = globalIndex;
          ++globalIndexCountZoom;
        }
      }

     std::vector<int> nGlobDomain(2);
     nGlobDomain[0] = ni_glo.getValue();
     nGlobDomain[1] = nj_glo.getValue();
     size_t globalSizeIndex = 1, indexBegin, indexEnd;
     int range, clientSize = client->clientSize;
     for (int i = 0; i < nGlobDomain.size(); ++i) globalSizeIndex *= nGlobDomain[i];
     indexBegin = 0;
     for (int i = 0; i < clientSize; ++i)
     {
       range = globalSizeIndex / clientSize;
       if (i < (globalSizeIndex%clientSize)) ++range;
       if (i == client->clientRank) break;
       indexBegin += range;
     }
     indexEnd = indexBegin + range - 1;

    CServerDistributionDescription serverDescription(nGlobDomain);
    serverDescription.computeServerGlobalIndexInRange(nbServer, std::make_pair<size_t,size_t>(indexBegin, indexEnd));
    CClientServerMapping* clientServerMap = new CClientServerMappingDistributed(serverDescription.getGlobalIndexRange(),
                                                                                client->intraComm);
    clientServerMap->computeServerIndexMapping(globalIndexDomain);
    const std::map<int, std::vector<size_t> >& globalIndexDomainOnServer = clientServerMap->getGlobalIndexOnServer();

    std::map<int, std::vector<size_t> >::const_iterator it = globalIndexDomainOnServer.begin(),
                                                       ite = globalIndexDomainOnServer.end();
    indSrv_.clear();
    for (; it != ite; ++it)
    {
      int rank = it->first;
      std::vector<size_t>::const_iterator itbVec  = (it->second).begin(),
                                           iteVec = (it->second).end();
      int nb = globalIndexDomainZoom.numElements();
      for (int i = 0; i < nb; ++i)
      {
        if (iteVec != std::find(itbVec, iteVec, globalIndexDomainZoom(i)))
        {
          indSrv_[rank].push_back(globalIndexDomainZoom(i));
        }
      }
    }

    connectedServerRank_.clear();
    for (it = globalIndexDomainOnServer.begin(); it != ite; ++it) {
      connectedServerRank_.push_back(it->first);
    }

    if (!indSrv_.empty())
    {
      connectedServerRank_.clear();
      for (it = indSrv_.begin(); it != indSrv_.end(); ++it)
        connectedServerRank_.push_back(it->first);
    }
    nbConnectedClients_ = clientServerMap->computeConnectedClients(client->serverSize, client->clientSize, client->intraComm, connectedServerRank_);

    delete clientServerMap;
  }

  void CDomain::sendLonLatArea(void)
  {
    int ns, n, i, j, ind, nv, idx;
    CContext* context = CContext::getCurrent();
    CContextClient* client=context->client;

    // send lon lat for each connected server
    CEventClient eventIndex(getType(), EVENT_ID_INDEX);
    CEventClient eventLon(getType(), EVENT_ID_LON);
    CEventClient eventLat(getType(), EVENT_ID_LAT);
    CEventClient eventArea(getType(), EVENT_ID_AREA);

    list<CMessage> list_msgsIndex, list_msgsLon, list_msgsLat, list_msgsArea;
    list<CArray<int,1> > list_indi, list_indj;
    list<CArray<double,1> > list_lon, list_lat;
    list<CArray<double,2> > list_boundslon, list_boundslat;
    list<CArray<double,1> > list_area;

    std::map<int, std::vector<size_t> >::const_iterator it, iteMap;
    iteMap = indSrv_.end();
    for (int k = 0; k < connectedServerRank_.size(); ++k)
    {
      int nbData = 0;
      int rank = connectedServerRank_[k];
      it = indSrv_.find(rank);
      if (iteMap != it)
        nbData = it->second.size();

      list_indi.push_back(CArray<int,1>(nbData));
      list_indj.push_back(CArray<int,1>(nbData));
      list_lon.push_back(CArray<double,1>(nbData));
      list_lat.push_back(CArray<double,1>(nbData));

      if (hasBounds)
      {
        list_boundslon.push_back(CArray<double,2>(nvertex, nbData));
        list_boundslat.push_back(CArray<double,2>(nvertex, nbData));
      }
      if (hasArea)
        list_area.push_back(CArray<double,1>(nbData));

      CArray<int,1>& indi = list_indi.back();
      CArray<int,1>& indj = list_indj.back();
      CArray<double,1>& lon = list_lon.back();
      CArray<double,1>& lat = list_lat.back();

      for (n = 0; n < nbData; ++n)
      {
        idx = static_cast<int>(it->second[n]);
        i = idx % ni_glo;
        j = idx / ni_glo;
        ind = (i - zoom_ibegin_client) + (j - zoom_jbegin_client) * zoom_ni_client;

        lon(n) = lonvalue(ind);
        lat(n) = latvalue(ind);

        if (hasBounds)
        {
          CArray<double,2>& boundslon = list_boundslon.back();
          CArray<double,2>& boundslat = list_boundslat.back();

          for (nv = 0; nv < nvertex; nv++)
          {
            boundslon(nv, n) = bounds_lon(nv, ind);
            boundslat(nv, n) = bounds_lat(nv, ind);
          }
        }

        indi(n) = ibegin + i_index(i - ibegin, j - jbegin);
        indj(n) = jbegin + j_index(i - ibegin, j - jbegin);

        if (hasArea)
          list_area.back()(n) = area(i - ibegin, j - jbegin);
      }

      list_msgsIndex.push_back(CMessage());

      list_msgsIndex.back() << this->getId() << (int)type; // enum ne fonctionne pour les message => ToFix
      list_msgsIndex.back() << isCurvilinear;
      list_msgsIndex.back() << list_indi.back() << list_indj.back();

      list_msgsLon.push_back(CMessage());
      list_msgsLat.push_back(CMessage());

      list_msgsLon.back() << this->getId() << list_lon.back();
      list_msgsLat.back() << this->getId() << list_lat.back();

      if (hasBounds)
      {
        list_msgsLon.back() << list_boundslon.back();
        list_msgsLat.back() << list_boundslat.back();
      }

      eventIndex.push(rank, nbConnectedClients_[rank], list_msgsIndex.back());
      eventLon.push(rank, nbConnectedClients_[rank], list_msgsLon.back());
      eventLat.push(rank, nbConnectedClients_[rank], list_msgsLat.back());

      if (hasArea)
      {
        list_msgsArea.push_back(CMessage());
        list_msgsArea.back() << this->getId() << list_area.back();
        eventArea.push(rank, nbConnectedClients_[rank], list_msgsArea.back());
      }
    }

    client->sendEvent(eventIndex);
    client->sendEvent(eventLon);
    client->sendEvent(eventLat);
    if (hasArea)
      client->sendEvent(eventArea);
  }

  bool CDomain::dispatchEvent(CEventServer& event)
  {
    if (SuperClass::dispatchEvent(event)) return true;
    else
    {
      switch(event.type)
      {
        case EVENT_ID_SERVER_ATTRIBUT:
          recvServerAttribut(event);
          return true;
          break;
        case EVENT_ID_INDEX:
          recvIndex(event);
          return true;
          break;
        case EVENT_ID_LON:
          recvLon(event);
          return true;
          break;
        case EVENT_ID_LAT:
          recvLat(event);
          return true;
          break;
        case EVENT_ID_AREA:
          recvArea(event);
          return true;
          break;
        default:
          ERROR("bool CContext::dispatchEvent(CEventServer& event)",
                << "Unknown Event");
          return false;
       }
    }
  }

  void CDomain::recvServerAttribut(CEventServer& event)
  {
    CBufferIn* buffer=event.subEvents.begin()->buffer;
    string domainId ;
    *buffer>>domainId ;
    get(domainId)->recvServerAttribut(*buffer) ;
  }

  void CDomain::recvServerAttribut(CBufferIn& buffer)
  {
    buffer >> ni_srv >> ibegin_srv >> iend_srv >> nj_srv >> jbegin_srv >> jend_srv
           >> global_zoom_ni >> global_zoom_ibegin >> global_zoom_nj >> global_zoom_jbegin;

    int zoom_iend = global_zoom_ibegin + global_zoom_ni - 1;
    int zoom_jend = global_zoom_jbegin + global_zoom_nj - 1;

    zoom_ibegin_srv = global_zoom_ibegin > ibegin_srv ? global_zoom_ibegin : ibegin_srv ;
    zoom_iend_srv = zoom_iend < iend_srv ? zoom_iend : iend_srv ;
    zoom_ni_srv=zoom_iend_srv-zoom_ibegin_srv+1 ;

    zoom_jbegin_srv = global_zoom_jbegin > jbegin_srv ? global_zoom_jbegin : jbegin_srv ;
    zoom_jend_srv = zoom_jend < jend_srv ? zoom_jend : jend_srv ;
    zoom_nj_srv=zoom_jend_srv-zoom_jbegin_srv+1 ;

    if (zoom_ni_srv<=0 || zoom_nj_srv<=0)
    {
      zoom_ibegin_srv=0 ; zoom_iend_srv=0 ; zoom_ni_srv=0 ;
      zoom_jbegin_srv=0 ; zoom_jend_srv=0 ; zoom_nj_srv=0 ;
    }
    lonvalue_srv.resize(zoom_ni_srv*zoom_nj_srv) ;
    lonvalue_srv = 0. ;
    latvalue_srv.resize(zoom_ni_srv*zoom_nj_srv) ;
    latvalue_srv = 0. ;
    if (hasBounds)
    {
      bounds_lon_srv.resize(nvertex,zoom_ni_srv*zoom_nj_srv) ;
      bounds_lon_srv = 0. ;
      bounds_lat_srv.resize(nvertex,zoom_ni_srv*zoom_nj_srv) ;
      bounds_lat_srv = 0. ;
    }

    if (hasArea)
      area_srv.resize(zoom_ni_srv * zoom_nj_srv);
  }

  void CDomain::recvIndex(CEventServer& event)
  {
    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      CBufferIn* buffer = it->buffer;
      string domainId;
      *buffer >> domainId;
      get(domainId)->recvIndex(it->rank, *buffer);
    }
  }

  void CDomain::recvIndex(int rank, CBufferIn& buffer)
  {
    int type_int;
    buffer >> type_int >> isCurvilinear >> indiSrv[rank] >> indjSrv[rank];
    type.setValue((type_attr::t_enum)type_int); // probleme des type enum avec les buffers : ToFix
  }

  void CDomain::recvLon(CEventServer& event)
  {
    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      CBufferIn* buffer = it->buffer;
      string domainId;
      *buffer >> domainId;
      get(domainId)->recvLon(it->rank, *buffer);
    }
  }

  void CDomain::recvLon(int rank, CBufferIn& buffer)
  {
    CArray<int,1> &indi = indiSrv[rank], &indj = indjSrv[rank];
    CArray<double,1> lon;
    CArray<double,2> boundslon;

    buffer >> lon;
    if (hasBounds) buffer >> boundslon;

    int i, j, ind_srv;
    for (int ind = 0; ind < indi.numElements(); ind++)
    {
      i = indi(ind); j = indj(ind);
      ind_srv = (i - zoom_ibegin_srv) + (j - zoom_jbegin_srv) * zoom_ni_srv;
      lonvalue_srv(ind_srv) = lon(ind);
      if (hasBounds)
      {
        for (int nv = 0; nv < nvertex; nv++)
          bounds_lon_srv(nv, ind_srv) = boundslon(nv, ind);
      }
    }
  }

  void CDomain::recvLat(CEventServer& event)
  {
    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      CBufferIn* buffer = it->buffer;
      string domainId;
      *buffer >> domainId;
      get(domainId)->recvLat(it->rank, *buffer);
    }
  }

  void CDomain::recvLat(int rank, CBufferIn& buffer)
  {
    CArray<int,1> &indi = indiSrv[rank], &indj = indjSrv[rank];
    CArray<double,1> lat;
    CArray<double,2> boundslat;

    buffer >> lat;
    if (hasBounds) buffer >> boundslat;

    int i, j, ind_srv;
    for (int ind = 0; ind < indi.numElements(); ind++)
    {
      i = indi(ind); j = indj(ind);
      ind_srv = (i - zoom_ibegin_srv) + (j - zoom_jbegin_srv) * zoom_ni_srv;
      latvalue_srv(ind_srv) = lat(ind);
      if (hasBounds)
      {
        for (int nv = 0; nv < nvertex; nv++)
          bounds_lat_srv(nv, ind_srv) = boundslat(nv, ind);
      }
    }
  }

  void CDomain::recvArea(CEventServer& event)
  {
    list<CEventServer::SSubEvent>::iterator it;
    for (it = event.subEvents.begin(); it != event.subEvents.end(); ++it)
    {
      CBufferIn* buffer = it->buffer;
      string domainId;
      *buffer >> domainId;
      get(domainId)->recvArea(it->rank, *buffer);
    }
  }

  void CDomain::recvArea(int rank, CBufferIn& buffer)
  {
    CArray<int,1> &indi = indiSrv[rank], &indj = indjSrv[rank];
    CArray<double,1> clientArea;

    buffer >> clientArea;

    int i, j, ind_srv;
    for (int ind = 0; ind < indi.numElements(); ind++)
    {
      i = indi(ind); j = indj(ind);
      ind_srv = (i - zoom_ibegin_srv) + (j - zoom_jbegin_srv) * zoom_ni_srv;
      area_srv(ind_srv) = clientArea(ind);
    }
  }

  bool CDomain::hasTransformation()
  {
    return (!transformationMap_.empty());
  }

  void CDomain::setTransformations(const TransMapTypes& domTrans)
  {
    transformationMap_ = domTrans;
  }

  CDomain::TransMapTypes CDomain::getAllTransformations(void)
  {
    return transformationMap_;
  }

  /*!
    Check the validity of all transformations applied on domain
  This functions is called AFTER all inherited attributes are solved
  */
  void CDomain::checkTransformations()
  {
    TransMapTypes::const_iterator itb = transformationMap_.begin(), it,
                                  ite = transformationMap_.end();
    for (it = itb; it != ite; ++it)
    {
      (it->second)->checkValid(this);
    }
  }

  void CDomain::solveInheritanceTransformation()
  {
    if (this->hasTransformation()) return;

    std::vector<CDomain*> refDomain;
    CDomain* refer_sptr;
    CDomain* refer_ptr = this;
    while (refer_ptr->hasDirectDomainReference())
    {
      refDomain.push_back(refer_ptr);
      refer_sptr = refer_ptr->getDirectDomainReference();
      refer_ptr  = refer_sptr;
      if (refer_ptr->hasTransformation()) break;
    }

    if (refer_ptr->hasTransformation())
      for (int idx = 0; idx < refDomain.size(); ++idx)
        refDomain[idx]->setTransformations(refer_ptr->getAllTransformations());
  }

  void CDomain::parse(xml::CXMLNode & node)
  {
    SuperClass::parse(node);

    if (node.goToChildElement())
    {
      StdString zoomDomainDefRoot("zoom_domain_definition");
      StdString zoom("zoom_domain");
      do
      {
        if (node.getElementName() == zoom) {
          CZoomDomain* tmp = (CZoomDomainGroup::get(zoomDomainDefRoot))->createChild();
          tmp->parse(node);
          transformationMap_.push_back(std::make_pair(TRANS_ZOOM_DOMAIN,tmp));
        }
      } while (node.goToNextElement()) ;
      node.goToParentElement();
    }
  }
   //----------------------------------------------------------------

   DEFINE_REF_FUNC(Domain,domain)

   ///---------------------------------------------------------------

} // namespace xios
