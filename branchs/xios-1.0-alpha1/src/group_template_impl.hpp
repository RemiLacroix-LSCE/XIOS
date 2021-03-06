#ifndef __XMLIO_CGroupTemplate_impl__
#define __XMLIO_CGroupTemplate_impl__

namespace xmlioserver
{
   using namespace tree;

   /// ////////////////////// Définitions ////////////////////// ///

   template <class U, class V, class W>
      CGroupTemplate<U, V, W>::CGroupTemplate(void)
         : CObjectTemplate<V>() //, V()
         , childMap(), childList()
         , groupMap(), groupList()
   { /* Ne rien faire de plus */ }

   template <class U, class V, class W>
      CGroupTemplate<U, V, W>::CGroupTemplate(const StdString & id)
         : CObjectTemplate<V>(id) //, V()
         , childMap(), childList()
         , groupMap(), groupList()
   { /* Ne rien faire de plus */ }

   template <class U, class V, class W>
      CGroupTemplate<U, V, W>::~CGroupTemplate(void)
   { /* Ne rien faire de plus */ }
   
   ///--------------------------------------------------------------
   
   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::toBinary(StdOStream & os) const
   {
      SuperClass::toBinary(os);
      
      const StdSize grpnb = this->groupList.size();
      const StdSize chdnb = this->childList.size();
      tree::ENodeType cenum = U::GetType();
      tree::ENodeType genum = V::GetType();
      
      os.write (reinterpret_cast<const char*>(&grpnb) , sizeof(StdSize));
      os.write (reinterpret_cast<const char*>(&chdnb) , sizeof(StdSize));      
      
      typename std::vector<boost::shared_ptr<V> >::const_iterator 
         itg = this->groupList.begin(), endg = this->groupList.end();
      typename std::vector<boost::shared_ptr<U> >::const_iterator 
         itc = this->childList.begin(), endc = this->childList.end();
            
      for (; itg != endg; itg++)
      { 
         boost::shared_ptr<V> group = *itg;
         bool hid = group->hasId();
         
         os.write (reinterpret_cast<const char*>(&genum), sizeof(tree::ENodeType));      
         os.write (reinterpret_cast<const char*>(&hid), sizeof(bool));
         
         if (hid)
         {
            const StdString & id = group->getId();
            const StdSize size   = id.size();
               
            os.write (reinterpret_cast<const char*>(&size), sizeof(StdSize));
            os.write (id.data(), size * sizeof(char));         
         }              
         group->toBinary(os);
      }
            
      for (; itc != endc; itc++)
      { 
         boost::shared_ptr<U> child = *itc;
         bool hid = child->hasId();
         
         os.write (reinterpret_cast<const char*>(&cenum), sizeof(tree::ENodeType));
         os.write (reinterpret_cast<const char*>(&hid), sizeof(bool));
         
         if (hid)
         {
            const StdString & id = child->getId();
            const StdSize size   = id.size();
               
            os.write (reinterpret_cast<const char*>(&size), sizeof(StdSize));
            os.write (id.data(), size * sizeof(char));         
         }         
         child->toBinary(os);
      }
      
   }
   
   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::fromBinary(StdIStream & is)
   {
      SuperClass::fromBinary(is);
      
      boost::shared_ptr<V> group_ptr = (this->hasId())
         ? CObjectFactory::GetObject<V>(this->getId())
         : CObjectFactory::GetObject(boost::polymorphic_downcast<V*>(this));
      
      StdSize grpnb = 0;
      StdSize chdnb = 0;
      tree::ENodeType renum = Unknown;
      
      is.read (reinterpret_cast<char*>(&grpnb), sizeof(StdSize));
      is.read (reinterpret_cast<char*>(&chdnb), sizeof(StdSize));
      
      for (StdSize i = 0; i < grpnb; i++)
      {
         bool hid = false;
         is.read (reinterpret_cast<char*>(&renum), sizeof(tree::ENodeType));
         is.read (reinterpret_cast<char*>(&hid), sizeof(bool));
         
         if (renum != V::GetType())
            ERROR("CGroupTemplate<U, V, W>::fromBinary(StdIStream & is)",
                  << "[ renum = " << renum << "] Bad type !");
                        
         if (hid)
         {
            StdSize size  = 0;
            is.read (reinterpret_cast<char*>(&size), sizeof(StdSize));
            StdString id(size, ' ');
            is.read (const_cast<char *>(id.data()), size * sizeof(char));
            CGroupFactory::CreateGroup(group_ptr, id)->fromBinary(is);
         }
         else
         {
            CGroupFactory::CreateGroup(group_ptr)->fromBinary(is);
         }
      }
      
      for (StdSize j = 0; j < chdnb; j++)
      {
         bool hid = false;
         is.read (reinterpret_cast<char*>(&renum), sizeof(tree::ENodeType));
         is.read (reinterpret_cast<char*>(&hid), sizeof(bool));
         
         if (renum != U::GetType())
            ERROR("CGroupTemplate<U, V, W>::fromBinary(StdIStream & is)",
                  << "[ renum = " << renum << "] Bad type !");
                  
         if (hid)
         {
            StdSize size  = 0;
            is.read (reinterpret_cast<char*>(&size), sizeof(StdSize));
            StdString id(size, ' ');
            is.read (const_cast<char *>(id.data()), size * sizeof(char));
            CGroupFactory::CreateChild(group_ptr, id)->fromBinary(is);            
         }
         else
         {
            CGroupFactory::CreateChild(group_ptr)->fromBinary(is);
         }   
      }
   }

   //--------------------------------------------------------------

   template <class U, class V, class W>
      StdString CGroupTemplate<U, V, W>::toString(void) const
   {
      StdOStringStream oss;
      StdString name = (this->getId().compare(V::GetDefName()) != 0)
                     ? V::GetName() : V::GetDefName();

      oss << "<" << name << " ";
      if (this->hasId() && (this->getId().compare(V::GetDefName()) != 0))
         oss << " id=\"" << this->getId() << "\" ";
         
      if (this->hasChild())
      {
         oss << SuperClassAttribute::toString() << ">" << std::endl;
         
         typename std::vector<boost::shared_ptr<V> >::const_iterator 
            itg = this->groupList.begin(), endg = this->groupList.end();
         typename std::vector<boost::shared_ptr<U> >::const_iterator 
            itc = this->childList.begin(), endc = this->childList.end();
            
         for (; itg != endg; itg++)
         { 
            boost::shared_ptr<V> group = *itg;
            oss << *group << std::endl;
         }
            
         for (; itc != endc; itc++)
         { 
            boost::shared_ptr<U> child = *itc;
            oss << *child << std::endl;
         }
            
         oss << "</" << name << " >";
      }
      else
      {
         oss << SuperClassAttribute::toString() << "/>";
      }
      return (oss.str());
   }

   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::fromString(const StdString & str)
   { 
      ERROR("CGroupTemplate<U, V, W>::toString(void)",
            << "[ str = " << str << "] Not implemented yet !");
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      StdString CGroupTemplate<U, V, W>::GetName(void)
   { 
      return (U::GetName().append("_group")); 
   }

   template <class U, class V, class W>
      StdString CGroupTemplate<U, V, W>::GetDefName(void)
   { 
      return (U::GetName().append("_definition")); 
   }
   
   //---------------------------------------------------------------   

   template <class U, class V, class W>
      const std::vector<boost::shared_ptr<U> >&
         CGroupTemplate<U, V, W>::getChildList(void) const
   { 
      return (this->childList); 
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      const xios_map<StdString, boost::shared_ptr<V> >&
         CGroupTemplate<U, V, W>::getGroupMap(void) const
   { 
      return (this->groupList);
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      bool CGroupTemplate<U, V, W>::hasChild(void) const
   { 
      return ((groupList.size() + childList.size()) > 0); 
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::parse(xml::CXMLNode & node)
   { 
      this->parse(node, true); 
   }
   
   //---------------------------------------------------------------
   
   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::solveDescInheritance(const CAttributeMap * const parent)
   {
      if (parent != NULL)
         SuperClassAttribute::setAttributes(parent);
         
      typename std::vector<boost::shared_ptr<U> >::const_iterator 
         itc = this->childList.begin(), endc = this->childList.end();
      typename std::vector<boost::shared_ptr<V> >::const_iterator 
         itg = this->groupList.begin(), endg = this->groupList.end();
             
      for (; itc != endc; itc++)
      { 
         boost::shared_ptr<U> child = *itc;
         child->solveDescInheritance(this);
      }
            
      for (; itg != endg; itg++)
      { 
         boost::shared_ptr<V> group = *itg;
         group->solveRefInheritance();
         group->solveDescInheritance(this);
      }
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::getAllChildren(std::vector<boost::shared_ptr<U> > & allc) const
   {
      allc.insert (allc.end(), childList.begin(), childList.end());
      typename std::vector<boost::shared_ptr<V> >::const_iterator 
         itg = this->groupList.begin(), endg = this->groupList.end();
         
      for (; itg != endg; itg++)
      { 
         boost::shared_ptr<V> group = *itg;
         group->getAllChildren(allc);
      }
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      std::vector<boost::shared_ptr<U> > CGroupTemplate<U, V, W>::getAllChildren(void) const
   { 
      std::vector<boost::shared_ptr<U> > allc;
      this->getAllChildren(allc);
      return (allc);
   }

   //---------------------------------------------------------------

   template <class U, class V, class W>
      void CGroupTemplate<U, V, W>::solveRefInheritance(void)
   { /* Ne rien faire de plus */ }

   ///--------------------------------------------------------------

} // namespace xmlioserver


#endif // __XMLIO_CGroupTemplate_impl__
