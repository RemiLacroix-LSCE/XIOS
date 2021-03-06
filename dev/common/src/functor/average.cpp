#include "average.hpp"

namespace xmlioserver
{
   namespace func
   {
      /// ////////////////////// Définitions ////////////////////// ///

      CAverage::CAverage(DoubleArray doutput)
         : SuperClass(StdString("average"), doutput)
      { /* Ne rien faire de plus */ }

      CAverage::~CAverage(void)
      { /* Ne rien faire de plus */ }

      //---------------------------------------------------------------

      void CAverage::apply(const DoubleArray _dinput,
                                 DoubleArray _doutput)
      {
         const double * it1  = _dinput->data(),
                      * end1 = _dinput->data() + _dinput->num_elements(); 
               double * it   = _doutput->data();
         if (this->nbcall == 1)
              for (; it1 != end1; it1++, it++) *it  = *it1;
         else for (; it1 != end1; it1++, it++) *it += *it1;               
      }
      
      void CAverage::final(void)
      {
          double * it1  = this->getDataOutput()->data(),
                 * end1 = this->getDataOutput()->data() + this->getDataOutput()->num_elements();
          for (; it1 != end1; it1++) *it1 /= this->nbcall;
          this->nbcall = 0;                                                   
      }
   } // namespace func
} // namespace xmlioserver
