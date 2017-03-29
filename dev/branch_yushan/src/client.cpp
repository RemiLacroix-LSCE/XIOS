#include "globalScopeData.hpp"
#include "xios_spl.hpp"
#include "cxios.hpp"
#include "client.hpp"
#include <boost/functional/hash.hpp>
#include "type.hpp"
#include "context.hpp"
#include "context_client.hpp"
#include "oasis_cinterface.hpp"
#include "mpi.hpp"
#include "timer.hpp"
#include "buffer_client.hpp"

namespace xios
{

    MPI_Comm CClient::intraComm ;
    MPI_Comm CClient::interComm ;
    std::list<MPI_Comm> CClient::contextInterComms;
    int CClient::serverLeader ;
    bool CClient::is_MPI_Initialized ;
    int CClient::rank = INVALID_RANK;
    StdOFStream CClient::m_infoStream;
    StdOFStream CClient::m_errorStream;

    void CClient::initialize(const string& codeId, MPI_Comm& localComm, MPI_Comm& returnComm)
    {
      int initialized ;
      MPI_Initialized(&initialized) ;
      if (initialized) is_MPI_Initialized=true ;
      else is_MPI_Initialized=false ;


      
// don't use OASIS
      if (!CXios::usingOasis)
      {
// localComm doesn't given

        if (localComm == MPI_COMM_NULL)
        {
          if (!is_MPI_Initialized)
          {
            //MPI_Init(NULL, NULL);
            int return_level;
            #ifdef _intelmpi
            MPI_Init_thread(NULL, NULL, 3, &return_level);
            assert(return_level == 3);
            #elif _openmpi
            MPI_Init_thread(NULL, NULL, 2, &return_level);
            assert(return_level == 2);
            #endif
          }
          CTimer::get("XIOS").resume() ;
          CTimer::get("XIOS init").resume() ;
          boost::hash<string> hashString ;

          unsigned long hashClient=hashString(codeId) ;
          unsigned long hashServer=hashString(CXios::xiosCodeId) ;
          //hashServer=hashString("xios.x") ;
          unsigned long* hashAll ;
          int size ;
          int myColor ;
          int i,c ;

          MPI_Comm_size(CXios::globalComm,&size);
          MPI_Comm_rank(CXios::globalComm,&rank);

          printf("client init : rank = %d, size = %d\n", rank, size);
          

          hashAll=new unsigned long[size] ;

          MPI_Allgather(&hashClient,1,MPI_LONG,hashAll,1,MPI_LONG,CXios::globalComm) ;

          map<unsigned long, int> colors ;
          map<unsigned long, int> leaders ;

          for(i=0,c=0;i<size;i++)
          {
            if (colors.find(hashAll[i])==colors.end())
            {
              colors[hashAll[i]] =c ;
              leaders[hashAll[i]]=i ;
              c++ ;
            }
          }

          // Verify whether we are on server mode or not
          CXios::setNotUsingServer();
          for (i=0; i < size; ++i)
          {
            if (hashServer == hashAll[i])
            {
              CXios::setUsingServer();
              break;
            }
          }

          myColor=colors[hashClient] ;

          MPI_Comm_split(CXios::globalComm,myColor,rank,&intraComm) ;
        

          if (CXios::usingServer)
          {
            //int clientLeader=leaders[hashClient] ;
            serverLeader=leaders[hashServer] ;

            int intraCommSize, intraCommRank ;
            MPI_Comm_size(intraComm,&intraCommSize) ;
            MPI_Comm_rank(intraComm,&intraCommRank) ;
            #pragma omp critical(_output)
            {
              info(50)<<"intercommCreate::client "<<rank<<" intraCommSize : "<<intraCommSize
                 <<" intraCommRank :"<<intraCommRank<<"  serverLeader "<< serverLeader
                 <<" globalComm : "<< &(CXios::globalComm) << endl ;  
            }
            
            MPI_Intercomm_create(intraComm,0,CXios::globalComm,serverLeader,0,&interComm) ;

            int interCommSize, interCommRank ;
            MPI_Comm_size(interComm,&interCommSize) ;
            MPI_Comm_rank(interComm,&interCommRank) ;

            #pragma omp critical(_output)
            {
              info(50)<<" interCommRank :"<<interCommRank
                 <<" interCommSize : "<< interCommSize << endl ;  
            }


          }
          else
          {
            MPI_Comm_dup(intraComm,&interComm) ;
          }
          delete [] hashAll ;
        }
        // localComm argument is given
        else
        {
          if (CXios::usingServer)
          {
            //ERROR("void CClient::initialize(const string& codeId,MPI_Comm& localComm,MPI_Comm& returnComm)", << " giving a local communictor is not compatible with using server mode") ;
          }
          else
          {
            MPI_Comm_dup(localComm,&intraComm) ;
            MPI_Comm_dup(intraComm,&interComm) ;
          }
        }
      }
      // using OASIS
      else
      {
        // localComm doesn't given
        if (localComm == MPI_COMM_NULL)
        {
          if (!is_MPI_Initialized) oasis_init(codeId) ;
          oasis_get_localcomm(localComm) ;
        }
        MPI_Comm_dup(localComm,&intraComm) ;

        CTimer::get("XIOS").resume() ;
        CTimer::get("XIOS init").resume() ;

        if (CXios::usingServer)
        {
          MPI_Status status ;
          MPI_Comm_rank(intraComm,&rank) ;

          oasis_get_intercomm(interComm,CXios::xiosCodeId) ;
          if (rank==0) MPI_Recv(&serverLeader,1, MPI_INT, 0, 0, interComm, &status) ;
          MPI_Bcast(&serverLeader,1,MPI_INT,0,intraComm) ;

        }
        else MPI_Comm_dup(intraComm,&interComm) ;
      }

      MPI_Comm_dup(intraComm,&returnComm) ;

    }


    void CClient::registerContext(const string& id,MPI_Comm contextComm)
    {
      //info(50) << "Client "<<getRank() << " start registerContext using info output" << endl;
      printf("Client %d start registerContext\n", getRank());
      //printf("Client start registerContext\n");

      CContext::setCurrent(id) ;
      printf("Client %d CContext::setCurrent OK\n", getRank());
      CContext* context = CContext::create(id);
      printf("Client %d context=CContext::create(%s) OK, *context = %p\n", getRank(), id, &(*context));
      
      StdString idServer(id);
      idServer += "_server";

      if (!CXios::isServer)  // server mode
      {      
        int size,rank,globalRank ;
        size_t message_size ;
        int leaderRank ;
        MPI_Comm contextInterComm ;

        
        MPI_Comm_size(contextComm,&size) ;
        MPI_Comm_rank(contextComm,&rank) ;
        MPI_Comm_rank(CXios::globalComm,&globalRank) ;
        if (rank!=0) globalRank=0 ;


        CMessage msg ;
        msg<<idServer<<size<<globalRank ;


        int messageSize=msg.size() ;
        void * buff = new char[messageSize] ;
        CBufferOut buffer(buff,messageSize) ;
        buffer<<msg ;
        
        

        MPI_Send(buff,buffer.count(),MPI_CHAR,serverLeader,1,CXios::globalComm) ;
        delete [] buff ;
              

        MPI_Intercomm_create(contextComm,0,CXios::globalComm,serverLeader,10+globalRank,&contextInterComm) ;
        
        #pragma omp critical (_output)
        info(10)<<"Register new Context : "<<id<<endl ;
                      

        MPI_Comm inter ;
        MPI_Intercomm_merge(contextInterComm,0,&inter) ;
        MPI_Barrier(inter) ;

        #pragma omp critical (_output)
        printf("Client %d : MPI_Intercomm_merge OK \n", getRank()) ;
        
        
        context->initClient(contextComm,contextInterComm) ;
        #pragma omp critical (_output)
        printf("Client %d : context->initClient(contextComm,contextInterComm) OK \n", getRank()) ;
        

        contextInterComms.push_back(contextInterComm);
        MPI_Comm_free(&inter);
      }
      else  // attached mode
      {
        MPI_Comm contextInterComm ;
        MPI_Comm_dup(contextComm,&contextInterComm) ;
        CContext* contextServer = CContext::create(idServer);
        
        // Firstly, initialize context on client side
        context->initClient(contextComm,contextInterComm, contextServer);

        // Secondly, initialize context on server side
        contextServer->initServer(contextComm,contextInterComm, context);

        // Finally, we should return current context to context client
        CContext::setCurrent(id);

        contextInterComms.push_back(contextInterComm);
      }
    }

    void CClient::finalize(void)
    {
      int rank ;
      int msg=0 ;

      MPI_Comm_rank(intraComm,&rank) ;
       
      if (!CXios::isServer)
      {
        MPI_Comm_rank(intraComm,&rank) ;
        if (rank==0)
        {
          MPI_Send(&msg,1,MPI_INT,0,0,interComm) ;
        }
      }

      for (std::list<MPI_Comm>::iterator it = contextInterComms.begin(); it != contextInterComms.end(); ++it)
        MPI_Comm_free(&(*it));
      
      MPI_Comm_free(&interComm);
      MPI_Comm_free(&intraComm);

      CTimer::get("XIOS finalize").suspend() ;
      CTimer::get("XIOS").suspend() ;

      if (!is_MPI_Initialized)
      {
        if (CXios::usingOasis) oasis_finalize();
        else  MPI_Finalize(); 
      }
      
      info(20) << "Client side context is finalized"<<endl ;
      report(0) <<" Performance report : total time spent for XIOS : "<< CTimer::get("XIOS").getCumulatedTime()<<" s"<<endl ;
      report(0)<< " Performance report : time spent for waiting free buffer : "<< CTimer::get("Blocking time").getCumulatedTime()<<" s"<<endl ;
      report(0)<< " Performance report : Ratio : "<< CTimer::get("Blocking time").getCumulatedTime()/CTimer::get("XIOS").getCumulatedTime()*100.<<" %"<<endl ;
      report(0)<< " Performance report : This ratio must be close to zero. Otherwise it may be usefull to increase buffer size or numbers of server"<<endl ;
//      report(0)<< " Memory report : Current buffer_size : "<<CXios::bufferSize<<endl ;
      report(0)<< " Memory report : Minimum buffer size required : " << CClientBuffer::maxRequestSize << " bytes" << endl ;
      report(0)<< " Memory report : increasing it by a factor will increase performance, depending of the volume of data wrote in file at each time step of the file"<<endl ;
   }

   int CClient::getRank()
   {
     return rank;
   }

    /*!
    * Open a file specified by a suffix and an extension and use it for the given file buffer.
    * The file name will be suffix+rank+extension.
    * 
    * \param fileName[in] protype file name
    * \param ext [in] extension of the file
    * \param fb [in/out] the file buffer
    */
    void CClient::openStream(const StdString& fileName, const StdString& ext, std::filebuf* fb)
    {
      StdStringStream fileNameClient;
      int numDigit = 0;
      int size = 0;
      MPI_Comm_size(CXios::globalComm, &size);
      while (size)
      {
        size /= 10;
        ++numDigit;
      }

      fileNameClient << fileName << "_" << std::setfill('0') << std::setw(numDigit) << getRank() << ext;
      printf("getrank() = %d, file name = %s\n", getRank(), fileNameClient.str().c_str());
      
        fb->open(fileNameClient.str().c_str(), std::ios::out);
        if (!fb->is_open())
          ERROR("void CClient::openStream(const StdString& fileName, const StdString& ext, std::filebuf* fb)",
              << std::endl << "Can not open <" << fileNameClient << "> file to write the client log(s).");  
      
      
    }

    /*!
    * \brief Open a file stream to write the info logs
    * Open a file stream with a specific file name suffix+rank
    * to write the info logs.
    * \param fileName [in] protype file name
    */
    void CClient::openInfoStream(const StdString& fileName)
    {
      std::filebuf* fb = m_infoStream.rdbuf();
      openStream(fileName, ".out", fb);

      info.write2File(fb);
      report.write2File(fb);
    }

    //! Write the info logs to standard output
    void CClient::openInfoStream()
    {
      info.write2StdOut();
      report.write2StdOut();
    }

    //! Close the info logs file if it opens
    void CClient::closeInfoStream()
    {
      if (m_infoStream.is_open()) m_infoStream.close();
    }

    /*!
    * \brief Open a file stream to write the error log
    * Open a file stream with a specific file name suffix+rank
    * to write the error log.
    * \param fileName [in] protype file name
    */
    void CClient::openErrorStream(const StdString& fileName)
    {
      std::filebuf* fb = m_errorStream.rdbuf();
      openStream(fileName, ".err", fb);

      error.write2File(fb);
    }

    //! Write the error log to standard error output
    void CClient::openErrorStream()
    {
      error.write2StdErr();
    }

    //! Close the error log file if it opens
    void CClient::closeErrorStream()
    {
      if (m_errorStream.is_open()) m_errorStream.close();
    }
}
