#include "ep_lib.hpp"
#include <mpi.h>
#include "ep_declaration.hpp"
#include "ep_mpi.hpp"

using namespace std;

namespace ep_lib
{
  int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
  {
    assert(local_comm.is_ep);

    int ep_rank, ep_rank_loc, mpi_rank;
    int ep_size, num_ep, mpi_size;

    ep_rank = local_comm.ep_comm_ptr->size_rank_info[0].first;
    ep_rank_loc = local_comm.ep_comm_ptr->size_rank_info[1].first;
    mpi_rank = local_comm.ep_comm_ptr->size_rank_info[2].first;
    ep_size = local_comm.ep_comm_ptr->size_rank_info[0].second;
    num_ep = local_comm.ep_comm_ptr->size_rank_info[1].second;
    mpi_size = local_comm.ep_comm_ptr->size_rank_info[2].second;


    MPI_Barrier(local_comm);



    int leader_ranks[6]; //! 0: rank in world, 1: mpi_size, 2: rank_in_peer.
                         //! 3, 4, 5 : remote

    bool is_decider = false;


    if(ep_rank == local_leader)
    {
      MPI_Comm_rank(MPI_COMM_WORLD, &leader_ranks[0]);

      leader_ranks[1] = mpi_size;
      MPI_Comm_rank(peer_comm, &leader_ranks[2]);

      MPI_Request request[2];
      MPI_Status status[2];

      MPI_Isend(&leader_ranks[0], 3, MPI_INT, remote_leader, tag, peer_comm, &request[0]);
      MPI_Irecv(&leader_ranks[3], 3, MPI_INT, remote_leader, tag, peer_comm, &request[1]);

      MPI_Waitall(2, request, status);

    }


    MPI_Bcast(leader_ranks, 6, MPI_INT, local_leader, local_comm);

    
    MPI_Barrier(local_comm);
    

    if(leader_ranks[0] == leader_ranks[3])
    {
      if( leader_ranks[1] * leader_ranks[4] == 1)
      {
        if(ep_rank == local_leader) Debug("calling MPI_Intercomm_create_unique_leader\n");
        local_comm.ep_comm_ptr->comm_label = -99;

        return MPI_Intercomm_create_unique_leader(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
      }
      else // leader_ranks[1] * leader_ranks[4] != 1
      {
        // change leader
        int new_local_leader;

        if(leader_ranks[2] < leader_ranks[5])
        {
          if(leader_ranks[1] > 1) //! change leader
          {
            // change leader
            is_decider = true;
            int target = local_comm.rank_map->at(local_leader).second;
            {
              for(int i=0; i<ep_size; i++)
              {
                if(local_comm.rank_map->at(i).second != target && local_comm.rank_map->at(i).first == 0)
                {
                  new_local_leader = i;
                  break;
                }
              }
            }
          }
          else
          {
            new_local_leader = local_leader;
          }
        }
        else
        {
          if(leader_ranks[4] == 1)
          {
            // change leader
            is_decider = true;
            int target = local_comm.rank_map->at(local_leader).second;
            {
              for(int i=0; i<ep_size; i++)
              {
                if(local_comm.rank_map->at(i).second != target && local_comm.rank_map->at(i).first == 0)
                {
                  new_local_leader = i;
                  break;
                }
              }
            }
          }
          else
          {
            new_local_leader = local_leader;
          }
        }


        int new_tag_in_world;

        int leader_in_world[2];


        if(is_decider)
        {
          if(ep_rank == new_local_leader)
          {
            new_tag_in_world = TAG++;
          }
          MPI_Bcast(&new_tag_in_world, 1, MPI_INT, new_local_leader, local_comm);
          if(ep_rank == local_leader) MPI_Send(&new_tag_in_world, 1, MPI_INT, remote_leader, tag, peer_comm);
        }
        else
        {
          if(ep_rank == local_leader)
          {
            MPI_Status status;
            MPI_Recv(&new_tag_in_world, 1, MPI_INT, remote_leader, tag, peer_comm, &status);
          }
          MPI_Bcast(&new_tag_in_world, 1, MPI_INT, new_local_leader, local_comm);
        }


        if(ep_rank == new_local_leader)
        {
          ::MPI_Comm_rank(to_mpi_comm(MPI_COMM_WORLD.mpi_comm), &leader_in_world[0]);
        }

        MPI_Bcast(&leader_in_world[0], 1, MPI_INT, new_local_leader, local_comm);


        if(ep_rank == local_leader)
        {
          MPI_Request request[2];
          MPI_Status status[2];

          MPI_Isend(&leader_in_world[0], 1, MPI_INT, remote_leader, tag, peer_comm, &request[0]);
          MPI_Irecv(&leader_in_world[1], 1, MPI_INT, remote_leader, tag, peer_comm, &request[1]);

          MPI_Waitall(2, request, status);
        }

        MPI_Bcast(&leader_in_world[1], 1, MPI_INT, local_leader, local_comm);

        local_comm.ep_comm_ptr->comm_label = tag;

        if(ep_rank == local_leader) Debug("calling MPI_Intercomm_create_from_world\n");

        return MPI_Intercomm_create_from_world(local_comm, new_local_leader, MPI_COMM_WORLD.mpi_comm, leader_in_world[1], new_tag_in_world, newintercomm);
        
      }
    }

    if(ep_rank == local_leader) Debug("calling MPI_Intercomm_create_kernel\n");

    return MPI_Intercomm_create_kernel(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);

  }

  int MPI_Comm_test_inter(MPI_Comm comm, int *flag)
  {
    *flag = false;
    if(comm.is_ep)
    {
      *flag = comm.is_intercomm;
      return 0;
    } 
    else if(comm.mpi_comm != static_cast< ::MPI_Comm*>(MPI_COMM_NULL.mpi_comm))
    {
      ::MPI_Comm mpi_comm = to_mpi_comm(comm.mpi_comm);
      
      ::MPI_Comm_test_inter(mpi_comm, flag);
      return 0;  
    }
    return 0;
  }


}
