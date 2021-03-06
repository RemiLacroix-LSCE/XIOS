#ifndef __XMLIO_CD360Calendar__
#define __XMLIO_CD360Calendar__

/// xmlioserver headers ///
#include "xmlioserver_spl.hpp"
#include "calendar.hpp"

namespace xmlioserver
{
   namespace date
   {
      /// ////////////////////// Déclarations ////////////////////// ///
      class CD360Calendar : public CCalendar
      {
            /// Typedef ///
            typedef CCalendar SuperClass;

         public :

            /// Constructeur ///
            CD360Calendar(void);                                   // Not implemented yet.
            CD360Calendar(const StdString & dateStr);
            CD360Calendar(int yr = 0, int mth = 1, int d   = 1,
                          int hr = 0, int min = 0, int sec = 0);
            CD360Calendar(const CD360Calendar & calendar);       // Not implemented yet.
            CD360Calendar(const CD360Calendar * calendar);       // Not implemented yet.

            /// Accesseurs ///
            virtual int getYearTotalLength(const CDate & date) const;
            virtual int getMonthLength(const CDate & date) const;
            virtual StdString getType(void) const;

            /// Destructeur ///
            virtual ~CD360Calendar(void);

      }; // class CD360Calendar

   } // namespace date
} // namespace xmlioserver

#endif // __XMLIO_CD360Calendar__

