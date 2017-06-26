/*!
   \file ep_recv.cpp
   \since 2 may 2016

   \brief Definitions of MPI receive functions: MPI_Recv, MPI_Mrecv, MPI_Irecv, MPI_Imrecv
 */


#include "ep_lib.hpp"
#include <mpi.h>
#include "ep_declaration.hpp"

using namespace std;


namespace ep_lib 
{
  

  int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status)
  {
    int dest_rank;
    MPI_Comm_rank(comm, &dest_rank);

    if(!comm.is_ep)
    {
      ::MPI_Comm mpi_comm = static_cast< ::MPI_Comm >(comm.mpi_comm);
      ::MPI_Status mpi_status;
      ::MPI_Recv(buf, count, static_cast< ::MPI_Datatype >(datatype), src<0? MPI_ANY_SOURCE : src, tag<0? MPI_ANY_TAG: tag, mpi_comm, &mpi_status);

      status->ep_src = src;
      status->ep_tag = tag;
      status->ep_datatype = datatype;

      return 0;  
    }

    Message_Check(comm);

    MPI_Request request;
    MPI_Irecv(buf, count, datatype, src, tag, comm, &request);
    MPI_Wait(&request, status);

    return 0;
  }




  int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request *request)
  {

    Debug("MPI_Irecv with EP");
    int dest_rank;
    MPI_Comm_rank(comm, &dest_rank);

    if(!comm.is_ep)
    {
      ::MPI_Request mpi_request;
      ::MPI_Comm mpi_comm = static_cast< ::MPI_Comm > (comm.mpi_comm);
      ::MPI_Irecv(buf, count, static_cast< ::MPI_Datatype> (datatype), src<0? MPI_ANY_SOURCE : src, tag<0? MPI_ANY_TAG: tag, mpi_comm, &mpi_request);

      request->mpi_request = mpi_request;
      request->ep_src = src;
      request->ep_datatype = datatype;
      request->ep_tag = tag;
    }

    request->mpi_request = MPI_REQUEST_NULL_STD;
    request->buf = buf;
    request->comm = comm;
    request->type = 2;

    request->ep_src = src;
    request->ep_tag = tag;
    request->ep_datatype = datatype;

    /* With Improbe*/
    Message_Check(comm);

    EP_PendingRequests->push_back(request);
    printf("proc %d : EP_PendingRequests insert one request, add = %p(%p), buf = %p(%p)\n", dest_rank, EP_PendingRequests->back(), request, buf, request->buf);
    
    // check all EP_PendingRequests
    
    //if(EP_PendingRequests == 0 ) EP_PendingRequests = new std::list< MPI_Request* >; 
    if(!EP_PendingRequests->empty())
    {
      printf("proc %d have %d pending irecv request\n", dest_rank, EP_PendingRequests->size());
      
      for(std::list<MPI_Request* >::iterator it = EP_PendingRequests->begin(); it!=EP_PendingRequests->end(); )
      {
        if((*it)->type != 2) 
        {
          printf("proc %d : pending request type = %d, src= %d, tag = %d, add = %p skip\n", dest_rank, (*it)->type, (*it)->ep_src, (*it)->ep_tag, *it);
          EP_PendingRequests->erase(it);
          it = EP_PendingRequests->begin();
          printf("proc %d : pending request processed, size = %d, it = %p\n", dest_rank, EP_PendingRequests->size(), *it);
          continue;
        }
        
        printf("proc %d : pending irecv request src = %d, tag = %d, type = %d, add = %p\n", dest_rank, (*it)->ep_src, (*it)->ep_tag, (*it)->type, *it);
        int probed = false;
        MPI_Message pending_message;
        MPI_Status pending_status;
    
        MPI_Improbe((*it)->ep_src, (*it)->ep_tag, (*it)->comm, &probed, &pending_message, &pending_status);
        printf("proc %d : pending irecv request probed to be %d, add = %p\n",dest_rank, probed, *it);
    
        if(probed) 
        { 
          int count;
          MPI_Get_count(&pending_status, (*it)->ep_datatype, &count);
          MPI_Imrecv((*it)->buf, count, (*it)->ep_datatype, &pending_message, *it);
          printf("proc %d : pending request is imrecving src = %d, tag = %d, add = %p, buf = %p, count = %d\n", dest_rank, (*it)->ep_src, (*it)->ep_tag, *it, (*it)->buf, count);
          EP_PendingRequests->erase(it);
          it = EP_PendingRequests->begin();
          printf("proc %d : pending request processed, size = %d\n", dest_rank, EP_PendingRequests->size());
          continue;
        }
        else it++;
      }
    }
/*
    int flag = false;
    MPI_Message message;
    MPI_Status status;
    
    MPI_Improbe(src, tag, comm, &flag, &message, &status);
            
    if(flag)
    {
      MPI_Imrecv(buf, count, datatype, &message, request);
      printf("proc %d : found message in local queue, src = %d, tag = %d\n", dest_rank, src, tag);
    }
  */  

    return 0;
  }

  int MPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request)
  {
    Debug("MPI_Imrecv");

    request->type = 3;

    ::MPI_Request mpi_request;
    ::MPI_Message mpi_message = static_cast< ::MPI_Message >(message->mpi_message);
		
    ::MPI_Imrecv(buf, count, static_cast< ::MPI_Datatype>(datatype), &mpi_message, &mpi_request);

    request->mpi_request = mpi_request;
    request->ep_datatype = datatype;
    request->ep_tag = message->ep_tag;
    request->ep_src = message->ep_src;
    //request->buf = buf;

    return 0;
  }


  int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status)
  {
    Debug("EP Mrecv called\n");

    ::MPI_Status mpi_status;
    ::MPI_Message mpi_message = static_cast< ::MPI_Message >(message->mpi_message);
    
    ::MPI_Mrecv(buf, count, static_cast< ::MPI_Datatype>(datatype), &mpi_message, &mpi_status);

    status->mpi_status = new ::MPI_Status(mpi_status);
    status->ep_src = message->ep_src;
    status->ep_datatype = datatype;
    status->ep_tag = message->ep_tag;

    //check_sum_recv(buf, count, datatype, message->ep_src, message->ep_tag);

    return 0;
  }

}


