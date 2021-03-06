#ifndef __XMLIO_GroupParser__
#define __XMLIO_GroupParser__

/// boost headers ///
#include <boost/cast.hpp>

namespace xmlioserver
{
   /// ////////////////////// Définitions ////////////////////// ///
   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::parse(xml::CXMLNode & node, bool withAttr)
   {

      StdString name = node.getElementName();
      xml::THashAttributes attributes = node.getAttributes();
      if (withAttr)
      {
         CGroupTemplate<U, V, W>::SuperClass::parse(node);
         if (attributes.end() != attributes.find("src"))
         {
            StdIFStream ifs ( attributes["src"].c_str() , StdIFStream::in );
            if (!ifs.good())
               ERROR("CGroupTemplate<U, V, W>::parse(xml::CXMLNode & node, bool withAttr)",
                     << "[ filename = " << attributes["src"] << " ] Bad xml stream !");
            xml::CXMLParser::ParseInclude(ifs, *this);
         }
      }

      // PARSING POUR GESTION DES ENFANTS
      boost::shared_ptr<V> group_ptr = (this->hasId())
         ? CObjectFactory::GetObject<V>(this->getId())
         : CObjectFactory::GetObject(boost::polymorphic_downcast<V*>(this));

      if (!(node.goToChildElement()))
      {
         if (this->hasId())
         {
            DEBUG(<< "L'objet de type \'" << V::GetName()
                  << "\' nommé \'" << this->getId()
                  << "\' ne contient pas d\'enfant !");
         }
      }
      else
      {
         do { // Parcours pour traitement.

            StdString name = node.getElementName();
            attributes.clear();
            attributes = node.getAttributes();

            if (name.compare(V::GetName()) == 0)
            {
               if (attributes.end() == attributes.find("id"))
                  CGroupFactory::CreateGroup(group_ptr)->parse(node);
               else
                  CGroupFactory::CreateGroup(group_ptr, attributes["id"])->parse(node);
               continue;
            }

            if (name.compare(U::GetName()) == 0)
            {
               if (attributes.end() == attributes.find("id"))
                  CGroupFactory::CreateChild(group_ptr)->parse(node);
               else
                  CGroupFactory::CreateChild(group_ptr, attributes["id"])->parse(node);
               continue;
            }

            DEBUG(<< "Dans le contexte \'" << CObjectFactory::GetCurrentContextId()
                  << "\', un objet de type \'" << V::GetName()
                  << "\' ne peut contenir qu'un objet de type \'" << V::GetName()
                  << "\' ou de type \'" << U::GetName()
                  << "\' (reçu : " << name << ") !");

         } while (node.goToNextElement());
         node.goToParentElement(); // Retour au parent
      }
   }

} // namespace xmlioserver

#endif // __XMLIO_GroupParser__
