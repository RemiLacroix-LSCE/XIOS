/*!
   \file ep_message.cpp
   \since 2 may 2016

   \brief Definitions of MPI endpoint function: Message_Check
 */

#include "ep_lib.hpp"
#include <mpi.h>
#include "ep_declaration.hpp"
#include "ep_mpi.hpp"

using namespace std;

extern std::list< ep_lib::MPI_Request* > * EP_PendingRequests;
#pragma omp threadprivate(EP_PendingRequests)

namespace ep_lib
{

  int Message_Check(MPI_Comm comm)
  {
    if(!comm.is_ep) return 0;

    if(comm.is_intercomm)
    {
      return  Message_Check_intercomm(comm);
    }

    int flag = true;
    ::MPI_Message message;
    ::MPI_Status status;
    int mpi_source;

    while(flag) // loop until the end of global queue
    {
      Debug("Message probing for intracomm\n");
      

      //#ifdef _openmpi
      #pragma omp critical (_mpi_call)
      {
        ::MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, to_mpi_comm(comm.mpi_comm), &flag, &status);
        if(flag)
        {
          Debug("find message in mpi comm \n");
          mpi_source = status.MPI_SOURCE;
          int tag = status.MPI_TAG;
          ::MPI_Mprobe(mpi_source, tag, to_mpi_comm(comm.mpi_comm), &message, &status);

        }
      }
      //#elif _intelmpi
      //#pragma omp critical (_mpi_call)
      //{
      //  ::MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, to_mpi_comm(comm.mpi_comm), &flag, &status);
      //  if(flag)
      //  {
      //    Debug("find message in mpi comm \n");
      //    mpi_source = status.MPI_SOURCE;
      //    int tag = status.MPI_TAG;
      //    ::MPI_Mprobe(mpi_source, tag, to_mpi_comm(comm.mpi_comm), &message, &status);
      //  }
      //}
      //#endif
      
      if(flag)
      {

        MPI_Message *msg_block = new MPI_Message; 
        msg_block->mpi_message = message;  
        msg_block->ep_tag = bitset<15>(status.MPI_TAG >> 16).to_ulong(); 
        int src_loc       = bitset<8> (status.MPI_TAG >> 8) .to_ulong(); 
        int dest_loc      = bitset<8> (status.MPI_TAG)	    .to_ulong();
        int src_mpi       = status.MPI_SOURCE;
             
        msg_block->ep_src  = get_ep_rank(comm, src_loc,  src_mpi);       
        msg_block->mpi_status = new ::MPI_Status(status);

        MPI_Comm* ptr_comm_list = comm.ep_comm_ptr->comm_list;
        MPI_Comm* ptr_comm_target = &ptr_comm_list[dest_loc];


        #pragma omp critical (_query)
        {
          #pragma omp flush
          comm.ep_comm_ptr->comm_list[dest_loc].ep_comm_ptr->message_queue->push_back(*msg_block);      
          #pragma omp flush
        }
        
        delete msg_block;        
      }

    }

    return MPI_SUCCESS;
  }



  int Message_Check_intercomm(MPI_Comm comm)
  {
    if(!comm.ep_comm_ptr->intercomm->mpi_inter_comm) return 0;

    Debug("Message probing for intercomm\n");

    int flag = true;
    ::MPI_Message message;
    ::MPI_Status status;
    int mpi_source;
    int current_ep_rank;
    MPI_Comm_rank(comm, &current_ep_rank);

    while(flag) // loop until the end of global queue "comm.ep_comm_ptr->intercomm->mpi_inter_comm"
    {
      Debug("Message probing for intracomm\n");

      //#ifdef _openmpi
      #pragma omp critical (_mpi_call)
      {
        ::MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, to_mpi_comm(comm.ep_comm_ptr->intercomm->mpi_inter_comm), &flag, &status);
        if(flag)
        {
          Debug("find message in mpi comm \n");
          mpi_source = status.MPI_SOURCE;
          int tag = status.MPI_TAG;
          ::MPI_Mprobe(mpi_source, tag, to_mpi_comm(comm.ep_comm_ptr->intercomm->mpi_inter_comm), &message, &status);

        }
      }
      

      if(flag)
      {

        MPI_Message *msg_block = new MPI_Message;

        msg_block->mpi_message = message;
        msg_block->ep_tag = bitset<15>(status.MPI_TAG >> 16).to_ulong();
        int src_loc       = bitset<8> (status.MPI_TAG >> 8) .to_ulong();
        int dest_loc      = bitset<8> (status.MPI_TAG)	    .to_ulong();
        int src_mpi       = status.MPI_SOURCE;
        int current_inter = comm.ep_comm_ptr->intercomm->local_rank_map->at(current_ep_rank).first;
             
        msg_block->ep_src  = get_ep_rank_intercomm(comm, src_loc,  src_mpi);
        msg_block->mpi_status = new ::MPI_Status(status);


        MPI_Comm* ptr_comm_list = comm.ep_comm_ptr->comm_list;
        MPI_Comm* ptr_comm_target = &ptr_comm_list[dest_loc];


        #pragma omp critical (_query)
        {
          #pragma omp flush
          comm.ep_comm_ptr->comm_list[dest_loc].ep_comm_ptr->message_queue->push_back(*msg_block);
          #pragma omp flush
        }
        
        delete msg_block;
        
      }

    }

    flag = true;
    while(flag) // loop until the end of global queue "comm.mpi_comm"
    {
      Debug("Message probing for intracomm\n");
     
      //#ifdef _openmpi
      #pragma omp critical (_mpi_call)
      {
        ::MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, to_mpi_comm(comm.mpi_comm), &flag, &status);
        if(flag)
        {
          Debug("find message in mpi comm \n");
          mpi_source = status.MPI_SOURCE;
          int tag = status.MPI_TAG;
          ::MPI_Mprobe(mpi_source, tag, to_mpi_comm(comm.mpi_comm), &message, &status);

        }
      }
      

      if(flag)
      {

        MPI_Message *msg_block = new MPI_Message;
       
        msg_block->mpi_message = message;
        msg_block->ep_tag = bitset<15>(status.MPI_TAG >> 16).to_ulong();
        int src_loc       = bitset<8> (status.MPI_TAG >> 8) .to_ulong();
        int dest_loc      = bitset<8> (status.MPI_TAG)	    .to_ulong();
        int src_mpi       = status.MPI_SOURCE;
        
        msg_block->ep_src  = get_ep_rank_intercomm(comm, src_loc, src_mpi);
        msg_block->mpi_status = new ::MPI_Status(status);
        

        MPI_Comm* ptr_comm_list = comm.ep_comm_ptr->comm_list;
        MPI_Comm* ptr_comm_target = &ptr_comm_list[dest_loc];


        #pragma omp critical (_query)
        {
          #pragma omp flush
          comm.ep_comm_ptr->comm_list[dest_loc].ep_comm_ptr->message_queue->push_back(*msg_block);
          #pragma omp flush
        }
        
        delete msg_block;
        
      }

    }

    return MPI_SUCCESS;
  }

  int Request_Check()
  {
    MPI_Status status;
    MPI_Message message;
    int probed = false;
    int recv_count = 0;
    std::list<MPI_Request* >::iterator it;
    
    for(it = EP_PendingRequests->begin(); it!=EP_PendingRequests->end(); it++)
    { 
      Message_Check((*it)->comm);
    }


    for(it = EP_PendingRequests->begin(); it!=EP_PendingRequests->end(); )
    {
      MPI_Improbe((*it)->ep_src, (*it)->ep_tag, (*it)->comm, &probed, &message, &status);
      if(probed)
      {
        MPI_Get_count(&status, (*it)->ep_datatype, &recv_count);
        MPI_Imrecv((*it)->buf, recv_count, (*it)->ep_datatype, &message, *it);
        (*it)->type = 3;
        EP_PendingRequests->erase(it);
        it = EP_PendingRequests->begin();
        continue;
      }
      it++;
    }
  }

}
