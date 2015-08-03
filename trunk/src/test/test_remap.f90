PROGRAM test_remap

  USE xios
  USE mod_wait
  USE netcdf

  IMPLICIT NONE
  INCLUDE "mpif.h"
  INTEGER :: rank
  INTEGER :: size
  INTEGER :: ierr

  CHARACTER(len=*),PARAMETER :: id="client"
  INTEGER :: comm
  TYPE(xios_duration) :: dtime
  TYPE(xios_context) :: ctx_hdl

  DOUBLE PRECISION,ALLOCATABLE :: src_lon(:), dst_lon(:)
  DOUBLE PRECISION,ALLOCATABLE :: src_lat(:), dst_lat(:)
  DOUBLE PRECISION,ALLOCATABLE :: src_boundslon(:,:), dst_boundslon(:,:)
  DOUBLE PRECISION,ALLOCATABLE :: src_boundslat(:,:), dst_boundslat(:,:)
  DOUBLE PRECISION,ALLOCATABLE :: src_field(:)
  INTEGER :: src_ni_glo, dst_ni_glo;
  INTEGER :: src_nvertex, dst_nvertex;
  INTEGER :: src_ibegin, dst_ibegin;
  INTEGER :: src_ni, dst_ni;
  CHARACTER(LEN=*),PARAMETER :: src_file="h14.nc"
  CHARACTER(LEN=*),PARAMETER :: dst_file="r180x90.nc"
  INTEGER :: ncid
  INTEGER :: dimids(2)
  INTEGER :: div,remain
  INTEGER :: varid
  INTEGER :: ts
  INTEGER :: i

  CALL MPI_INIT(ierr)
  CALL init_wait

!!! XIOS Initialization (get the local communicator)

  CALL xios_initialize(id,return_comm=comm)
  CALL MPI_COMM_RANK(comm,rank,ierr)
  CALL MPI_COMM_SIZE(comm,size,ierr)

  ierr=NF90_OPEN(src_file, NF90_NOWRITE, ncid)
  ierr=NF90_INQ_VARID(ncid,"bounds_lon",varid)
  ierr=NF90_INQUIRE_VARIABLE(ncid, varid,dimids=dimids)
  ierr=NF90_INQUIRE_DIMENSION(ncid, dimids(1), len=src_nvertex)
  ierr=NF90_INQUIRE_DIMENSION(ncid, dimids(2), len=src_ni_glo)

  div    = src_ni_glo/size
  remain = MOD( src_ni_glo, size )
  IF (rank < remain) THEN
    src_ni=div+1 ;
    src_ibegin=rank*(div+1) ;
  ELSE
    src_ni=div ;
    src_ibegin= remain * (div+1) + (rank-remain) * div ;
  ENDIF

  ALLOCATE(src_lon(src_ni))
  ALLOCATE(src_lat(src_ni))
  ALLOCATE(src_boundslon(src_nvertex,src_ni))
  ALLOCATE(src_boundslat(src_nvertex,src_ni))
  ALLOCATE(src_field(src_ni))

  ierr=NF90_INQ_VARID(ncid,"lon",varid)
  ierr=NF90_GET_VAR(ncid,varid, src_lon, start=(/src_ibegin+1/),count=(/src_ni/))
  ierr=NF90_INQ_VARID(ncid,"lat",varid)
  ierr=NF90_GET_VAR(ncid,varid, src_lat, start=(/src_ibegin+1/),count=(/src_ni/))
  ierr=NF90_INQ_VARID(ncid,"bounds_lon",varid)
  ierr=NF90_GET_VAR(ncid,varid,src_boundslon, start=(/1,src_ibegin+1/),count=(/src_nvertex,src_ni/))
  ierr=NF90_INQ_VARID(ncid,"bounds_lat",varid)
  ierr=NF90_GET_VAR(ncid,varid, src_boundslat, start=(/1,src_ibegin+1/),count=(/src_nvertex,src_ni/))
  ierr=NF90_INQ_VARID(ncid,"val",varid)
  ierr=NF90_GET_VAR(ncid,varid, src_field, start=(/src_ibegin+1/),count=(/src_ni/))


  ierr=NF90_OPEN(dst_file, NF90_NOWRITE, ncid)
  ierr=NF90_INQ_VARID(ncid,"bounds_lon",varid)
  ierr=NF90_INQUIRE_VARIABLE(ncid, varid,dimids=dimids)
  ierr=NF90_INQUIRE_DIMENSION(ncid, dimids(1), len=dst_nvertex)
  ierr=NF90_INQUIRE_DIMENSION(ncid, dimids(2), len=dst_ni_glo)

  div    = dst_ni_glo/size
  remain = MOD( dst_ni_glo, size )
  IF (rank < remain) THEN
    dst_ni=div+1 ;
    dst_ibegin=rank*(div+1) ;
  ELSE
    dst_ni=div ;
    dst_ibegin= remain * (div+1) + (rank-remain) * div ;
  ENDIF

  ALLOCATE(dst_lon(dst_ni))
  ALLOCATE(dst_lat(dst_ni))
  ALLOCATE(dst_boundslon(dst_nvertex,dst_ni))
  ALLOCATE(dst_boundslat(dst_nvertex,dst_ni))

  ierr=NF90_INQ_VARID(ncid,"lon",varid)
  ierr=NF90_GET_VAR(ncid,varid, dst_lon, start=(/dst_ibegin+1/),count=(/dst_ni/))
  ierr=NF90_INQ_VARID(ncid,"lat",varid)
  ierr=NF90_GET_VAR(ncid,varid, dst_lat, start=(/dst_ibegin+1/),count=(/dst_ni/))
  ierr=NF90_INQ_VARID(ncid,"bounds_lon",varid)
  ierr=NF90_GET_VAR(ncid,varid,dst_boundslon, start=(/1,dst_ibegin+1/),count=(/dst_nvertex,dst_ni/))
  ierr=NF90_INQ_VARID(ncid,"bounds_lat",varid)
  ierr=NF90_GET_VAR(ncid,varid, dst_boundslat, start=(/1,dst_ibegin+1/),count=(/dst_nvertex,dst_ni/))


  CALL xios_context_initialize("test",comm)
  CALL xios_get_handle("test",ctx_hdl)
  CALL xios_set_current_context(ctx_hdl)

  CALL xios_set_domain_attr("src_domain", ni_glo=src_ni_glo, ibegin=src_ibegin, ni=src_ni, type="unstructured")
  CALL xios_set_domain_attr("src_domain", lonvalue=src_lon, latvalue=src_lat, &
                            bounds_lon=src_boundslon, bounds_lat=src_boundslat, nvertex=src_nvertex)
  CALL xios_set_domain_attr("dst_domain", ni_glo=dst_ni_glo, ibegin=dst_ibegin, ni=dst_ni, type="unstructured")
  CALL xios_set_domain_attr("dst_domain", lonvalue=dst_lon, latvalue=dst_lat, &
                            bounds_lon=dst_boundslon, bounds_lat=dst_boundslat, nvertex=dst_nvertex)
 
  dtime%second = 3600
  CALL xios_set_timestep(dtime)

  CALL xios_close_context_definition()

  DO ts=1,1
    CALL xios_update_calendar(ts)
    CALL xios_send_field("src_field",src_field)
    CALL wait_us(5000) ;
  ENDDO

  CALL xios_context_finalize()

  DEALLOCATE(src_lon, src_lat, src_boundslon,src_boundslat, src_field)
  DEALLOCATE(dst_lon, dst_lat, dst_boundslon,dst_boundslat)

  CALL MPI_COMM_FREE(comm, ierr)

  CALL xios_finalize()

  CALL MPI_FINALIZE(ierr)

END PROGRAM test_remap





