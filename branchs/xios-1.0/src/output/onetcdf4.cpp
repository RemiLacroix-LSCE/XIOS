#include "onetcdf4.hpp"
#include "group_template.hpp"
#include "mpi.hpp"
#include "netcdf.hpp"
#include "netCdfInterface.hpp"
#include "netCdfException.hpp"

namespace xios
{
      /// ////////////////////// Définitions ////////////////////// ///

      CONetCDF4::CONetCDF4
         (const StdString & filename, bool append, const MPI_Comm * comm, bool multifile)
            : path()
            , wmpi(false)
      {
         this->initialize(filename, append, comm,multifile);
      }

      //---------------------------------------------------------------


      CONetCDF4::~CONetCDF4(void)
      {
//         CheckError(nc_close(this->ncidp));
      }

      ///--------------------------------------------------------------

      void CONetCDF4::initialize
         (const StdString & filename, bool append, const MPI_Comm * comm, bool multifile)
      {
         // Don't use parallel mode if there is only one process
         if (comm)
         {
            int commSize = 0;
            MPI_Comm_size(*comm, &commSize);
            if (commSize <= 1)
               comm = NULL;
         }
         wmpi = comm && !multifile;

         // If the file does not exist, we always create it
         if (!append || !std::ifstream(filename.c_str()))
         {
            if (wmpi)
               CNetCdfInterface::createPar(filename, NC_NETCDF4|NC_MPIIO, *comm, MPI_INFO_NULL, this->ncidp);
            else
               CNetCdfInterface::create(filename, NC_NETCDF4, this->ncidp);

            this->appendMode = false;
            this->recordOffset = 0;
         }
         else
         {
            if (wmpi)
               CNetCdfInterface::openPar(filename, NC_NETCDF4|NC_MPIIO|NC_WRITE, *comm, MPI_INFO_NULL, this->ncidp);
            else
               CNetCdfInterface::open(filename, NC_NETCDF4|NC_WRITE, this->ncidp);

            this->appendMode = true;
            // Find out how many temporal records have been written already to the file we are opening
            int ncUnlimitedDimId;
            CNetCdfInterface::inqUnLimDim(this->ncidp, ncUnlimitedDimId);
            if (ncUnlimitedDimId != -1)
               CNetCdfInterface::inqDimLen(this->ncidp, ncUnlimitedDimId, this->recordOffset);
            else
               this->recordOffset = 0;
         }
      }

      void CONetCDF4::close()
      {
        (CNetCdfInterface::close(this->ncidp));
      }

      //---------------------------------------------------------------

      void CONetCDF4::definition_start(void)
      {
         (CNetCdfInterface::reDef(this->ncidp));
      }

      //---------------------------------------------------------------

      void CONetCDF4::definition_end(void)
      {
         (CNetCdfInterface::endDef(this->ncidp));
      }

      //---------------------------------------------------------------

// Don't need it anymore?
//      void CONetCDF4::CheckError(int status)
//      {
//         if (status != NC_NOERR)
//         {
//            StdString errormsg (nc_strerror(status)); // fuite mémoire ici ?
//            ERROR("CONetCDF4::CheckError(int status)",
//                  << "[ status = " << status << " ] " << errormsg);
//         }
//      }

      //---------------------------------------------------------------

      int CONetCDF4::getCurrentGroup(void)
      {
         return (this->getGroup(this->getCurrentPath()));
      }

      //---------------------------------------------------------------

      int CONetCDF4::getGroup(const CONetCDF4Path & path)
      {
         int retvalue = this->ncidp;

         CONetCDF4Path::const_iterator
            it  = path.begin(), end = path.end();

         for (;it != end; it++)
         {
            const StdString & groupid = *it;
            (CNetCdfInterface::inqNcId(retvalue, groupid, retvalue));
         }
         return (retvalue);
      }

      //---------------------------------------------------------------

      int CONetCDF4::getVariable(const StdString & varname)
      {
         int varid = 0;
         int grpid = this->getCurrentGroup();
         (CNetCdfInterface::inqVarId(grpid, varname, varid));
         return (varid);
      }

      //---------------------------------------------------------------

      int CONetCDF4::getDimension(const StdString & dimname)
      {
         int dimid = 0;
         int grpid = this->getCurrentGroup();
         (CNetCdfInterface::inqDimId(grpid, dimname, dimid));
         return (dimid);
      }

      //---------------------------------------------------------------

      int CONetCDF4::getUnlimitedDimension(void)
      {
         int dimid = 0;
         int grpid = this->getCurrentGroup();
         (CNetCdfInterface::inqUnLimDim(grpid, dimid));
         return (dimid);
      }

      StdString CONetCDF4::getUnlimitedDimensionName(void)
      {
         int grpid = this->getGroup(path);
         int dimid = this->getUnlimitedDimension();

         if (dimid == -1) return (std::string());
         StdString dimname;
         (CNetCdfInterface::inqDimName(grpid, dimid, dimname));

         return (dimname);
      }

      //---------------------------------------------------------------

      std::vector<StdSize> CONetCDF4::getDimensions(const StdString & varname)
      {
         StdSize size = 0;
         std::vector<StdSize> retvalue;
         int grpid = this->getCurrentGroup();
         int varid = this->getVariable(varname);
         int nbdim = 0, *dimid = NULL;

         (CNetCdfInterface::inqVarNDims(grpid, varid, nbdim));
         dimid = new int[nbdim]();
         (CNetCdfInterface::inqVarDimId(grpid, varid, dimid));

         for (int i = 0; i < nbdim; i++)
         {
            (CNetCdfInterface::inqDimLen(grpid, dimid[i], size));
            if (size == NC_UNLIMITED)
                size = UNLIMITED_DIM;
            retvalue.push_back(size);
         }
         delete [] dimid;
         return (retvalue);
      }

      std::vector<std::string> CONetCDF4::getDimensionsIdList (const std::string * _varname)
      {
         int nDimNull = 0;
         int nbdim = 0, *dimid = NULL;
         int grpid = this->getCurrentGroup();
         int varid = (_varname != NULL) ? this->getVariable(*_varname) : NC_GLOBAL;
         std::vector<std::string> retvalue;

         if (_varname != NULL)
         {
            (CNetCdfInterface::inqVarNDims(grpid, varid, nbdim));
            dimid = new int[nbdim]();
            (CNetCdfInterface::inqVarDimId(grpid, varid, dimid));
         }
         else
         {
            (CNetCdfInterface::inqDimIds(grpid, nbdim, NULL, 1));
            dimid = new int[nbdim]();
            (CNetCdfInterface::inqDimIds(grpid, nDimNull, dimid, 1));
         }

         for (int i = 0; i < nbdim; i++)
         {
            std::string dimname;
            (CNetCdfInterface::inqDimName(grpid, dimid[i], dimname));
            retvalue.push_back(dimname);
         }
         delete [] dimid;

         return (retvalue);
      }


      //---------------------------------------------------------------

      const CONetCDF4::CONetCDF4Path & CONetCDF4::getCurrentPath(void) const
      { return (this->path); }

      void CONetCDF4::setCurrentPath(const CONetCDF4Path & path)
      { this->path = path; }

      //---------------------------------------------------------------

      int CONetCDF4::addGroup(const StdString & name)
      {
         int retvalue = 0;
         int grpid = this->getCurrentGroup();
         (CNetCdfInterface::defGrp(grpid, name, retvalue));
         return (retvalue);
      }

      //---------------------------------------------------------------

      int CONetCDF4::addDimension(const StdString& name, const StdSize size)
      {
         int retvalue = 0;
         int grpid = this->getCurrentGroup();
         if (size != UNLIMITED_DIM)
            (CNetCdfInterface::defDim(grpid, name, size, retvalue));
         else
            (CNetCdfInterface::defDim(grpid, name, NC_UNLIMITED, retvalue));
         return (retvalue);
      }

      //---------------------------------------------------------------

      int CONetCDF4::addVariable(const StdString & name, nc_type type,
                                  const std::vector<StdString> & dim)
      {
         int varid = 0;
         std::vector<int> dimids;
         std::vector<StdSize> dimsizes ;
         StdSize size ;
         StdSize totalSize ;
         StdSize maxSize=1024*1024*256 ; // == 2GB/8 if output double

         int grpid = this->getCurrentGroup();

         std::vector<StdString>::const_iterator
            it  = dim.begin(), end = dim.end();

         for (;it != end; it++)
         {
            const StdString & dimid = *it;
            dimids.push_back(this->getDimension(dimid));
            (CNetCdfInterface::inqDimLen(grpid, this->getDimension(dimid), size));
            if (size==NC_UNLIMITED) size=1 ;
            dimsizes.push_back(size) ;
         }

         (CNetCdfInterface::defVar(grpid, name, type, dimids.size(), &(dimids[0]), varid));

// set chunksize : size of one record
// but must not be > 2GB (netcdf or HDF5 problem)
         totalSize=1 ;
         for(vector<StdSize>::reverse_iterator it=dimsizes.rbegin(); it!=dimsizes.rend();++it)
         {
           totalSize*= *it ;
           if (totalSize>=maxSize) *it=1 ;
         }

         (CNetCdfInterface::defVarChunking(grpid, varid, NC_CHUNKED, &(dimsizes[0])));
         (CNetCdfInterface::defVarFill(grpid, varid, true, NULL));
         return (varid);
      }

      //---------------------------------------------------------------

      void CONetCDF4::setCompressionLevel(const StdString& varname, int compressionLevel)
      {
         if (compressionLevel < 0 || compressionLevel > 9)
           ERROR("void CONetCDF4::setCompressionLevel(const StdString& varname, int compressionLevel)",
                 "Invalid compression level for variable \"" << varname << "\", the value should range between 0 and 9.");
         if (compressionLevel && wmpi)
           ERROR("void CONetCDF4::setCompressionLevel(const StdString& varname, int compressionLevel)",
                 "Impossible to use compression for variable \"" << varname << "\" when using parallel mode.");

         int grpid = this->getCurrentGroup();
         int varid = this->getVariable(varname);
         CNetCdfInterface::defVarDeflate(grpid, varid, compressionLevel);
      }

      //---------------------------------------------------------------

      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const StdString & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAtt(grpid, varid, name, NC_CHAR, value.size(), value.c_str()));
         //CheckError(nc_put_att_string(grpid, varid, name.c_str(), 1, &str));
      }

      //---------------------------------------------------------------

      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const double & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, 1, &value));
      }

       template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const CArray<double,1>& value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, value.numElements(), value.dataFirst()));
      }
      //---------------------------------------------------------------

      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const float & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, 1, &value));
      }

       template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const CArray<float,1>& value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, value.numElements(), value.dataFirst()));
      }

      //---------------------------------------------------------------

      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const int & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, 1, &value));
      }

       template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const CArray<int,1>& value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, value.numElements(), value.dataFirst()));
      }

      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const short int & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, 1, &value));
      }

       template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const CArray<short int,1>& value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, value.numElements(), value.dataFirst()));
      }



      template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const long int & value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, 1, &value));
      }

       template <>
         void CONetCDF4::addAttribute
            (const StdString & name, const CArray<long int,1>& value, const StdString * varname )
      {
         int grpid = this->getCurrentGroup();
         int varid = (varname == NULL) ? NC_GLOBAL : this->getVariable(*varname);
         (CNetCdfInterface::putAttType(grpid, varid, name, value.numElements(), value.dataFirst()));
      }



                //---------------------------------------------------------------

      void CONetCDF4::getWriteDataInfos(const StdString & name, StdSize record, StdSize & array_size,
                                        std::vector<StdSize> & sstart,
                                        std::vector<StdSize> & scount,
                                        const std::vector<StdSize> * start,
                                        const std::vector<StdSize> * count)
      {
         std::vector<std::size_t> sizes  = this->getDimensions(name);
         std::vector<std::string> iddims = this->getDimensionsIdList (&name);
         std::vector<std::size_t>::const_iterator
            it  = sizes.begin(), end = sizes.end();
         int i = 0;

         if (iddims.begin()->compare(this->getUnlimitedDimensionName()) == 0)
         {
            sstart.push_back(record + recordOffset);
            scount.push_back(1);
            if ((start == NULL) &&
                (count == NULL)) i++;
            it++;
         }

         for (;it != end; it++)
         {
            if ((start != NULL) && (count != NULL))
            {
               sstart.push_back((*start)[i]);
               scount.push_back((*count)[i]);
               array_size *= (*count)[i];
               i++;
            }
            else
            {
               sstart.push_back(0);
               scount.push_back(sizes[i]);
               array_size *= sizes[i];
               i++;
            }
         }

      }



      template <>
         void CONetCDF4::writeData_(int grpid, int varid,
                                    const std::vector<StdSize> & sstart,
                                    const std::vector<StdSize> & scount, const double * data)
      {
         (CNetCdfInterface::putVaraType(grpid, varid, &(sstart[0]), &(scount[0]), data));
//         sync() ;
      }

      //---------------------------------------------------------------

      template <>
         void CONetCDF4::writeData_(int grpid, int varid,
                                    const std::vector<StdSize> & sstart,
                                    const std::vector<StdSize> & scount, const int * data)
      {
          (CNetCdfInterface::putVaraType(grpid, varid, &(sstart[0]), &(scount[0]), data));
//          sync() ;
      }

      //---------------------------------------------------------------

      template <>
         void CONetCDF4::writeData_(int grpid, int varid,
                                    const std::vector<StdSize> & sstart,
                                    const std::vector<StdSize> & scount, const float * data)
      {
          (CNetCdfInterface::putVaraType(grpid, varid, &(sstart[0]), &(scount[0]), data));
//          sync() ;
      }

      //---------------------------------------------------------------

      void CONetCDF4::writeData(const CArray<int, 2>& data, const StdString & name)
      {
         int grpid = this->getCurrentGroup();
         int varid = this->getVariable(name);
         StdSize array_size = 1;
         std::vector<StdSize> sstart, scount;

         this->getWriteDataInfos(name, 0, array_size,  sstart, scount, NULL, NULL);
         this->writeData_(grpid, varid, sstart, scount, data.dataFirst());
      }

      void CONetCDF4::writeTimeAxisData(const CArray<double, 1>& data, const StdString & name,
                                        bool collective, StdSize record, bool isRoot)
      {
         int grpid = this->getCurrentGroup();
         int varid = this->getVariable(name);

         map<int,size_t>::iterator it=timeAxis.find(varid) ;
         if (it==timeAxis.end()) timeAxis[varid]=record ;
         else
         {
           if (it->second >= record) return ;
           else it->second =record ;
         }

         StdSize array_size = 1;
         std::vector<StdSize> sstart, scount;

         if (this->wmpi && collective)
         (CNetCdfInterface::varParAccess(grpid, varid, NC_COLLECTIVE));
         if (this->wmpi && !collective)
         (CNetCdfInterface::varParAccess(grpid, varid, NC_INDEPENDENT));

         this->getWriteDataInfos(name, record, array_size,  sstart, scount, NULL, NULL);
         if (using_netcdf_internal)  if (!isRoot) { sstart[0]=sstart[0]+1 ; scount[0]=0 ;}
         this->writeData_(grpid, varid, sstart, scount, data.dataFirst());
       }

      //---------------------------------------------------------------

      bool CONetCDF4::varExist(const StdString & varname)
      {
         int grpid = this->getCurrentGroup();
         return (CNetCdfInterface::isVarExisted(grpid, varname));
      }

      void CONetCDF4::sync(void)
      {
         (CNetCdfInterface::sync(this->ncidp)) ;
      }
      ///--------------------------------------------------------------
 } // namespace xios
