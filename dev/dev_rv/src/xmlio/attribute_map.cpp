#include "attribute_map.hpp"

namespace xmlioserver
{
   namespace tree
   {
      /// ////////////////////// Définitions ////////////////////// ///
      CAttributeMap * CAttributeMap::Current = NULL;

      CAttributeMap::CAttributeMap(void)
         : xios_map<StdString, CAttribute*>()
      { CAttributeMap::Current = this; }

      CAttributeMap::~CAttributeMap(void)
      { /* Ne rien faire de plus */ }
      
      ///--------------------------------------------------------------

      void CAttributeMap::clearAllAttributes(void)
      {
         typedef std::pair<StdString, CAttribute*> StdStrAttPair;
         SuperClassMap::const_iterator it = SuperClassMap::begin(), end = SuperClassMap::end();
         for (; it != end; it++)
         {
            const StdStrAttPair & att = *it;
            if (!att.second->isEmpty())
               att.second->clear();
         }
      }

      //---------------------------------------------------------------

      bool CAttributeMap::hasAttribute(const StdString & key) const
      { 
         return (this->find(key) != this->end()); 
      }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::setAttribute(const StdString & key, CAttribute * const attr)
      {
         if (!this->hasAttribute(key))
            ERROR("CAttributeMap::setAttribute(key, attr)",
                   << "[ key = " << key << "] key not found !");
         if (attr == NULL)
            ERROR("CAttributeMap::setAttribute(key, attr)",
                   << "[ key = " << key << "] attr is null !");
         this->find(key)->second->setAnyValue(attr->getAnyValue());
      }
      
      //---------------------------------------------------------------
      
      CAttribute * CAttributeMap::operator[](const StdString & key)
      {
         if (!this->hasAttribute(key))
            ERROR("CAttributeMap::operator[](const StdString & key)",
                  << "[ key = " << key << "] key not found !");
         return (SuperClassMap::operator[](key));
      }
      
      //---------------------------------------------------------------
      
      StdString CAttributeMap::toString(void) const
      {
         typedef std::pair<StdString, CAttribute*> StdStrAttPair;
         StdOStringStream oss;
         
         SuperClassMap::const_iterator it = SuperClassMap::begin(), end = SuperClassMap::end();
         for (; it != end; it++)
         {
            const StdStrAttPair & att = *it;
            if (!att.second->isEmpty())
               oss << *att.second << " ";
         }
         return (oss.str());
      }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::fromString(const StdString & str)
      { 
         ERROR("CAttributeMap::fromString(const StdString & str)",
               << "[ str = " << str << "] Not implemented yet !"); 
      }
      
      //---------------------------------------------------------------

      //StdOStream & operator << (StdOStream & os, const CAttributeMap & attributmap)
      //{ os << attributmap.toString(); return (os); }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::setAttributes(const xml::THashAttributes & attributes)
      {
         for (xml::THashAttributes::const_iterator it  = attributes.begin();
                                                   it != attributes.end();
                                                   it ++)
         {
            if ((*it).first.compare(StdString("id")) != 0 &&
                (*it).first.compare(StdString("src"))!= 0)
            {
               //if (CAttributeMap::operator[]((*it).first)->isEmpty())
               CAttributeMap::operator[]((*it).first)->fromString((*it).second);
            }
         }
      }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::setAttributes(const CAttributeMap * const _parent)
      {
         typedef std::pair<StdString, CAttribute*> StdStrAttPair;
         
         SuperClassMap::const_iterator it = _parent->begin(), end = _parent->end();
         for (; it != end; it++)
         {
            const StdStrAttPair & el = *it;
            if (this->hasAttribute(el.first))
            {
               CAttribute * currAtt = CAttributeMap::operator[](el.first);
               if (currAtt->isEmpty() && !el.second->isEmpty())
               {
                  this->setAttribute(el.first, el.second);
               }
            }
         }
      }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::toBinary(StdOStream & os) const
      {
         typedef std::pair<StdString, CAttribute*> StdStrAttPair;
         SuperClassMap::const_iterator it = this->begin(), end = this->end();
         
         const StdSize nbatt = SuperClassMap::size();
         os.write (reinterpret_cast<const char*>(&nbatt) , sizeof(StdSize));
         
         for (; it != end; it++)
         {
            const StdString & key   = it->first;
            const CAttribute* value = it->second;            
            const StdSize size = key.size();
            
            os.write (reinterpret_cast<const char*>(&size) , sizeof(StdSize));
            os.write (key.data(), size * sizeof(char));
            
            if (!value->isEmpty())
            {
               bool b = true;
               os.write (reinterpret_cast<const char*>(&b) , sizeof(bool));
               value->toBinary(os);
            }
            else 
            {
               bool b = false;
               os.write (reinterpret_cast<const char*>(&b) , sizeof(bool));
            }
         }
      }
      
      //---------------------------------------------------------------
      
      void CAttributeMap::fromBinary(StdIStream & is)
      {
         StdSize nbatt = 0;
         is.read (reinterpret_cast<char*>(&nbatt), sizeof(StdSize));
         
         for (StdSize i = 0; i < nbatt; i++)
         {
            bool hasValue = false;
            StdSize size  = 0;
            is.read (reinterpret_cast<char*>(&size), sizeof(StdSize));
            StdString key(size, ' ');
            is.read (const_cast<char *>(key.data()), size * sizeof(char));
            
            if (!this->hasAttribute(key))
               ERROR("CAttributeMap::fromBinary(StdIStream & is)",
                     << "[ key = " << key << "] key not found !");
                                        
            is.read (reinterpret_cast<char*>(&hasValue), sizeof(bool));
            
            if (hasValue)          
               this->operator[](key)->fromBinary(is);
         }
      }
      
      ///--------------------------------------------------------------

   } // namespace tree
} // namespace xmlioser
