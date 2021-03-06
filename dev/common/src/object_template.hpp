#ifndef __XMLIO_CObjectTemplate__
#define __XMLIO_CObjectTemplate__

/// xmlioserver headers ///
#include "xmlioserver_spl.hpp"
#include "attribute_map.hpp"
#include "node_enum.hpp"
#include "buffer_in.hpp"
#include "event_server.hpp"
#include "attribute.hpp"

namespace xmlioserver
{
   /// ////////////////////// Déclarations ////////////////////// ///
   template <class T>
      class CObjectTemplate
         : public CObject
         , public virtual tree::CAttributeMap
   {

         /// Friend ///
         friend class CObjectFactory;

         /// Typedef ///
         typedef tree::CAttributeMap SuperClassMap;
         typedef CObject SuperClass;
         typedef T DerivedType;
         
         enum EEventId
         {
           EVENT_ID_SEND_ATTRIBUTE=100
         } ;

      public :

         /// Autres ///
         virtual StdString toString(void) const;
         virtual void fromString(const StdString & str);

         virtual void toBinary  (StdOStream & os) const;
         virtual void fromBinary(StdIStream & is);

         virtual void parse(xml::CXMLNode & node);
         
         /// Accesseurs ///
         tree::ENodeType getType(void) const;

         /// Test ///
         virtual bool hasChild(void) const;

         /// Traitements ///
         virtual void solveDescInheritance(const CAttributeMap * const parent = 0);

         /// Traitement statique ///
         static void ClearAllAttributes(void);
         void sendAttributToServer(const string& id);
         void sendAttributToServer(tree::CAttribute& attr) ;
         static void recvAttributFromClient(CEventServer& event) ;
         static bool dispatchEvent(CEventServer& event) ;

         /// Accesseur statique ///
         static std::vector<boost::shared_ptr<DerivedType> > &
            GetAllVectobject(const StdString & contextId);

         /// Destructeur ///
         virtual ~CObjectTemplate(void);
         
         static bool has(const string& id) ;
         static boost::shared_ptr<T> get(const string& id) ;
         boost::shared_ptr<T> get(void) ;
         static boost::shared_ptr<T> create(const string& id=string("")) ;
         
      protected :

         /// Constructeurs ///
         CObjectTemplate(void);
         explicit CObjectTemplate(const StdString & id);
         CObjectTemplate(const CObjectTemplate<T> & object,
                         bool withAttrList = true, bool withId = true);
         CObjectTemplate(const CObjectTemplate<T> * const object); // Not implemented.

      private :

         /// Propriétés statiques ///
         static xios_map<StdString,
                xios_map<StdString,
                boost::shared_ptr<DerivedType> > > AllMapObj; 
         static xios_map<StdString,
                std::vector<boost::shared_ptr<DerivedType> > > AllVectObj;
                
         static xios_map< StdString, long int > GenId ;

   }; // class CObjectTemplate
} // namespace xmlioserver

//#include "object_template_impl.hpp"

#endif // __XMLIO_CObjectTemplate__
