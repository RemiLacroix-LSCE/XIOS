#include "variable.hpp"

#include "attribute_template_impl.hpp"
#include "object_template_impl.hpp"
#include "group_template_impl.hpp"

#include "object_factory.hpp"
#include "object_factory_impl.hpp"

namespace xmlioserver {
namespace tree {

   /// ////////////////////// Définitions ////////////////////// ///

   CVariable::CVariable(void)
      : CObjectTemplate<CVariable>()
      , CVariableAttributes()
      , content()
   { /* Ne rien faire de plus */ }

   CVariable::CVariable(const StdString & id)
      : CObjectTemplate<CVariable>(id)
      , CVariableAttributes()
      , content()
   { /* Ne rien faire de plus */ }

   CVariable::~CVariable(void)
   { /* Ne rien faire de plus */ }

   StdString CVariable::GetName(void)   { return (StdString("variable")); }
   StdString CVariable::GetDefName(void){ return (CVariable::GetName()); }
   ENodeType CVariable::GetType(void)   { return (eVariable); }

   void CVariable::parse(xml::CXMLNode & node)
   {
      SuperClass::parse(node);
      StdString id = (this->hasId()) ? this->getId() : StdString("undefined");
      if (!node.getContent(this->content))
      {
         ERROR("CVariable::parse(xml::CXMLNode & node)",
               << "[ variable id = " << id
               << " ] variable is not defined !");
      }
   }

   const StdString & CVariable::getContent (void) const
   {
      return (this->content);
   }

   StdString CVariable::toString(void) const
   {
      StdOStringStream oss;

      oss << "<" << CVariable::GetName() << " ";
      if (this->hasId())
         oss << " id=\"" << this->getId() << "\" ";
      oss << SuperClassAttribute::toString() << ">" << std::endl
          << this->content /*<< std::endl*/;
      oss << "</" << CVariable::GetName() << " >";
      return (oss.str());
   }

   void CVariable::toBinary(StdOStream & os) const
   {
     const StdString & content = this->content;
     const StdSize size        =  content.size();
     SuperClass::toBinary(os);

     os.write (reinterpret_cast<const char*>(&size), sizeof(StdSize));
     os.write (content.data(), size * sizeof(char));
   }

   void CVariable::fromBinary(StdIStream & is)
   {
      SuperClass::fromBinary(is);
      StdSize size  = 0;
      is.read (reinterpret_cast<char*>(&size), sizeof(StdSize));
      this->content.assign(size, ' ');
      is.read (const_cast<char *>(this->content.data()), size * sizeof(char));
   }

   void CVariableGroup::parse(xml::CXMLNode & node, bool withAttr)
   {
      boost::shared_ptr<CVariableGroup> group_ptr = (this->hasId())
         ? CObjectFactory::GetObject<CVariableGroup>(this->getId())
         : CObjectFactory::GetObject(this);

      StdString content;
      if (this->getId().compare(CVariableGroup::GetDefName()) != 0 && node.getContent(content))
      {
        StdSize beginid = 0, endid = 0, begindata = 0, enddata = 0;
        StdString subdata, subid;

        while ((beginid = content.find_first_not_of ( " \r\n\t;", enddata)) != StdString::npos)
        {
           endid   = content.find_first_of ( " \r\n\t=", beginid );
           subid   = content.substr ( beginid, endid-beginid);

           begindata = content.find_first_of ( "=", endid ) + 1;
           enddata   = content.find_first_of ( ";", begindata );
           subdata   = content.substr ( begindata, enddata-begindata);

           //std::cout << "\"" << subid << "\":\"" << subdata << "\"" << std::endl;
           CGroupFactory::CreateChild(group_ptr, subid)->content = subdata;
        }
      }
      else
      {
         SuperClass::parse(node, withAttr);
      }
      //SuperClass::parse(node, withAttr);

   }

} // namespace tree
} // namespace xmlioserver
