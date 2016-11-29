#ifndef __XIOS_CFileWriterFilter__
#define __XIOS_CFileWriterFilter__

#include "input_pin.hpp"

namespace xios
{
  class CField;

  /*!
   * A terminal filter which transmits the packets it receives to a field for writting in a file.
   */
  class CFileWriterFilter : public CInputPin
  {
    public:
      /*!
       * Constructs the filter (with one input slot) associated to the specified field
       * and a garbage collector.
       *
       * \param gc the associated garbage collector
       * \param field the associated field
       */
      CFileWriterFilter(CGarbageCollector& gc, CField* field);

    protected:
      /*!
       * Callbacks a field to write a packet to a file.
       *
       * \param data a vector of packets corresponding to each slot
       */
      void virtual onInputReady(std::vector<CDataPacketPtr> data);

    private:
      CField* field; //<! The associated field
      std::map<Time, CDataPacketPtr> packets; //<! The stored packets
  }; // class CFileWriterFilter
} // namespace xios

#endif //__XIOS_CFileWriterFilter__