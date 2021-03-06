#ifndef __XMLIO_CGrid__
#define __XMLIO_CGrid__

/// xmlioserver headers ///
#include "xmlioserver_spl.hpp"
#include "group_factory.hpp"

#include "declare_group.hpp"
#include "domain.hpp"
#include "axis.hpp"

namespace xmlioserver {
namespace tree {
   
   /// ////////////////////// Déclarations ////////////////////// ///

   class CGridGroup;
   class CGridAttributes;
   class CGrid;

   ///--------------------------------------------------------------

   // Declare/Define CGridAttribute
   BEGIN_DECLARE_ATTRIBUTE_MAP(CGrid)
#  include "grid_attribute.conf"
   END_DECLARE_ATTRIBUTE_MAP(CGrid)

   ///--------------------------------------------------------------

   class CGrid
      : public CObjectTemplate<CGrid>
      , public CGridAttributes
   {
         /// typedef ///
         typedef CObjectTemplate<CGrid>   SuperClass;
         typedef CGridAttributes SuperClassAttribute;

      public :

         typedef CGridAttributes RelAttributes;
         typedef CGridGroup      RelGroup;

         /// Constructeurs ///
         CGrid(void);
         explicit CGrid(const StdString & id);
         CGrid(const CGrid & grid);       // Not implemented yet.
         CGrid(const CGrid * const grid); // Not implemented yet.

         /// Traitements ///
         void solveReference(void);

         virtual void toBinary  (StdOStream & os) const;
         virtual void fromBinary(StdIStream & is);

         /// Tests ///
         bool hasAxis(void) const;

      public :

         /// Accesseurs ///
         const std::deque<ARRAY(int, 1)> & getStoreIndex(void) const;
         const std::deque<ARRAY(int, 1)> & getOutIIndex(void)  const;
         const std::deque<ARRAY(int, 1)> & getOutJIndex(void)  const;
         const std::deque<ARRAY(int, 1)> & getOutLIndex(void)  const;

         const boost::shared_ptr<CAxis>   getRelAxis  (void) const;
         const boost::shared_ptr<CDomain> getRelDomain(void) const;

         StdSize getDimension(void) const;
         
         StdSize getLocalSize(void) const;
         StdSize getGlobalSize(void) const;
         StdSize  getDataSize(void) const;
         std::vector<StdSize> getLocalShape(void) const;
         std::vector<StdSize> getGlobalShape(void) const;

         /// Entrées-sorties de champs ///
         template <StdSize n>
            void inputField(const ARRAY(double, n) field, ARRAY(double, 1) stored) const;
            
         void inputFieldServer(const std::deque<ARRAY(double, 1)> storedClient,
                                                ARRAY(double, 1)  storedServer) const;

         template <StdSize n>
            void outputField(const ARRAY(double, 1) stored,  ARRAY(double, n) field) const;

         /// Destructeur ///
         virtual ~CGrid(void);

      public :

         /// Accesseurs statiques ///
         static StdString GetName(void);
         static StdString GetDefName(void);
         
         static ENodeType GetType(void);

         /// Instanciateurs Statiques ///
         static boost::shared_ptr<CGrid>
            CreateGrid(boost::shared_ptr<CDomain> domain);
         static boost::shared_ptr<CGrid>
            CreateGrid(boost::shared_ptr<CDomain> domain, boost::shared_ptr<CAxis> axis);

      protected :

         /// Entrées-sorties de champs (interne) ///
         void storeField_arr(const double * const data, ARRAY(double, 1) stored) const;

         /// Traitements protégés ///
         void computeIndexServer(void);
         void computeIndex(void);
         void solveDomainRef(void);
         void solveAxisRef(void);

      public:

         /// Propriétés privées ///
         bool withAxis ;
         bool isChecked;

         boost::shared_ptr<CAxis>   axis ;
         boost::shared_ptr<CDomain> domain ;

         std::deque<ARRAY(int, 1)> storeIndex ;
         std::deque<ARRAY(int, 1)> out_i_index ;
         std::deque<ARRAY(int, 1)> out_j_index ;
         std::deque<ARRAY(int, 1)> out_l_index ;

   }; // class CGrid

   ///--------------------------------------------------------------

   template <StdSize n>
      void CGrid::inputField(const  ARRAY(double, n) field, ARRAY(double, 1) stored) const
   {
      if (this->getDataSize() != field->num_elements())
         ERROR("CGrid::inputField(const  ARRAY(double, n) field, ARRAY(double, 1) stored)",
                << "[ Taille des données attendue = " << this->getDataSize()       << ", "
                << "Taille des données reçue = "      << field->num_elements() << " ] "
                << "Le tableau de données n'a pas la bonne taille !") ;
      this->storeField_arr(field->data(), stored) ;
   }

   ///--------------------------------------------------------------

   // Declare/Define CGridGroup and CGridDefinition
   DECLARE_GROUP(CGrid);

   ///--------------------------------------------------------------

} // namespace tree
} // namespace xmlioserver

#endif // __XMLIO_CGrid__
