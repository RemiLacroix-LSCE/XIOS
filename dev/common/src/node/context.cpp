#include "context.hpp"
#include "tree_manager.hpp"

#include "attribute_template_impl.hpp"
#include "object_template_impl.hpp"
#include "group_template_impl.hpp"

#include "calendar_type.hpp"
#include "duration.hpp"

#include "data_treatment.hpp"
#include "context_client.hpp"
#include "context_server.hpp"
#include "nc4_data_output.hpp"

namespace xmlioserver {
namespace tree {
   
   /// ////////////////////// Définitions ////////////////////// ///

   CContext::CContext(void)
      : CObjectTemplate<CContext>(), CContextAttributes()
      , calendar(),hasClient(false),hasServer(false)
   { /* Ne rien faire de plus */ }

   CContext::CContext(const StdString & id)
      : CObjectTemplate<CContext>(id), CContextAttributes()
      , calendar(),hasClient(false),hasServer(false)
   { /* Ne rien faire de plus */ }

   CContext::~CContext(void)
   { 
     if (hasClient) delete client ;
     if (hasServer) delete server ;
   }

   //----------------------------------------------------------------

   StdString CContext::GetName(void)   { return (StdString("context")); }
   StdString CContext::GetDefName(void){ return (CContext::GetName()); }
   ENodeType CContext::GetType(void)   { return (eContext); }

   //----------------------------------------------------------------

   boost::shared_ptr<CContextGroup> CContext::GetContextGroup(void)
   {  
      static boost::shared_ptr<CContextGroup> group_context
                          (new CContextGroup(xml::CXMLNode::GetRootName()));
      return (group_context); 
   }
   
   //----------------------------------------------------------------
   void CContext::setDataTreatment(boost::shared_ptr<data::CDataTreatment> ndatat)
   {
      this->datat = ndatat;
   }

   //----------------------------------------------------------------
   boost::shared_ptr<data::CDataTreatment> CContext::getDataTreatment(void) const
   {
      return (this->datat);
   }

   //----------------------------------------------------------------

   boost::shared_ptr<date::CCalendar> CContext::getCalendar(void) const
   {
      return (this->calendar);
   }
   
   //----------------------------------------------------------------
   
   void CContext::setCalendar(boost::shared_ptr<date::CCalendar> newCalendar)
   {
      this->calendar = newCalendar;
      calendar_type.setValue(this->calendar->getId());
      start_date.setValue(this->calendar->getInitDate().toString());
   }
   
   //----------------------------------------------------------------

   void CContext::solveCalendar(void)
   {
      if (this->calendar.get() != NULL) return;
      if (calendar_type.isEmpty() || start_date.isEmpty())
         ERROR(" CContext::solveCalendar(void)",
               << "[ context id = " << this->getId() << " ] "
               << "Impossible de définir un calendrier (un attribut est manquant).");

#define DECLARE_CALENDAR(MType  , mtype)                            \
   if (calendar_type.getValue().compare(#mtype) == 0)               \
   {                                                                \
      this->calendar =  boost::shared_ptr<date::CCalendar>          \
         (new date::C##MType##Calendar(start_date.getValue()));     \
      if (!this->timestep.isEmpty())                                \
       this->calendar->setTimeStep                                  \
          (date::CDuration::FromString(this->timestep.getValue())); \
      return;                                                       \
   }
#include "calendar_type.conf"

      ERROR("CContext::solveCalendar(void)",
            << "[ calendar_type = " << calendar_type.getValue() << " ] "
            << "Le calendrier n'est pas définie dans le code !");
   }
   
   //----------------------------------------------------------------

   void CContext::parse(xml::CXMLNode & node)
   {
      CContext::SuperClass::parse(node);

      // PARSING POUR GESTION DES ENFANTS
      xml::THashAttributes attributes = node.getAttributes();

      if (attributes.end() != attributes.find("src"))
      {
         StdIFStream ifs ( attributes["src"].c_str() , StdIFStream::in );
         if (!ifs.good())
            ERROR("CContext::parse(xml::CXMLNode & node)",
                  << "[ filename = " << attributes["src"] << " ] Bad xml stream !");
         xml::CXMLParser::ParseInclude(ifs, *this);
      }

      if (node.getElementName().compare(CContext::GetName()))
         DEBUG("Le noeud est mal nommé mais sera traité comme un contexte !");

      if (!(node.goToChildElement()))
      {
         DEBUG("Le context ne contient pas d'enfant !");
      }
      else
      {
         do { // Parcours des contextes pour traitement.

            StdString name = node.getElementName();
            attributes.clear();
            attributes = node.getAttributes();

            if (attributes.end() != attributes.find("id"))
            { DEBUG(<< "Le noeud de définition possède un identifiant,"
                    << " ce dernier ne sera pas pris en compte lors du traitement !"); }

#define DECLARE_NODE(Name_, name_)    \
   if (name.compare(C##Name_##Definition::GetDefName()) == 0) \
   { CObjectFactory::CreateObject<C##Name_##Definition>(C##Name_##Definition::GetDefName()) -> parse(node); \
   continue; }
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"

            DEBUG(<< "L'élément nommé \'"     << name
                  << "\' dans le contexte \'" << CObjectFactory::GetCurrentContextId()
                  << "\' ne représente pas une définition !");

         } while (node.goToNextElement());

         node.goToParentElement(); // Retour au parent
      }
   }

   //----------------------------------------------------------------

   void CContext::ShowTree(StdOStream & out)
   {
      StdString currentContextId =
         CObjectFactory::GetCurrentContextId();
      std::vector<boost::shared_ptr<CContext> > def_vector =
         CContext::GetContextGroup()->getChildList();
      std::vector<boost::shared_ptr<CContext> >::iterator
         it = def_vector.begin(), end = def_vector.end();

      out << "<? xml version=\"1.0\" ?>" << std::endl;
      out << "<"  << xml::CXMLNode::GetRootName() << " >" << std::endl;
      
      for (; it != end; it++)
      {
         boost::shared_ptr<CContext> context = *it;         
         CTreeManager::SetCurrentContextId(context->getId());         
         out << *context << std::endl;
      }
      
      out << "</" << xml::CXMLNode::GetRootName() << " >" << std::endl;
      CTreeManager::SetCurrentContextId(currentContextId);  
   }
   
   //----------------------------------------------------------------
   
   void CContext::toBinary(StdOStream & os) const
   {
      SuperClass::toBinary(os);
       
#define DECLARE_NODE(Name_, name_)                                         \
   {                                                                       \
      ENodeType renum = C##Name_##Definition::GetType();                   \
      bool val = CObjectFactory::HasObject<C##Name_##Definition>           \
                     (C##Name_##Definition::GetDefName());                 \
      os.write (reinterpret_cast<const char*>(&renum), sizeof(ENodeType)); \
      os.write (reinterpret_cast<const char*>(&val), sizeof(bool));        \
      if (val) CObjectFactory::GetObject<C##Name_##Definition>             \
                     (C##Name_##Definition::GetDefName())->toBinary(os);   \
   }   
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"
   }
   
   //----------------------------------------------------------------
   
   void CContext::fromBinary(StdIStream & is)
   {
      SuperClass::fromBinary(is);
#define DECLARE_NODE(Name_, name_)                                         \
   {                                                                       \
      bool val = false;                                                    \
      ENodeType renum = Unknown;                                           \
      is.read (reinterpret_cast<char*>(&renum), sizeof(ENodeType));        \
      is.read (reinterpret_cast<char*>(&val), sizeof(bool));               \
      if (renum != C##Name_##Definition::GetType())                        \
         ERROR("CContext::fromBinary(StdIStream & is)",                    \
               << "[ renum = " << renum << "] Bad type !");                \
      if (val) CObjectFactory::CreateObject<C##Name_##Definition>          \
                   (C##Name_##Definition::GetDefName()) -> fromBinary(is); \
   }   
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"
      
   }
   
   
   //----------------------------------------------------------------

   StdString CContext::toString(void) const
   {
      StdOStringStream oss;
      oss << "<" << CContext::GetName()
          << " id=\"" << this->getId() << "\" "
          << SuperClassAttribute::toString() << ">" << std::endl;
      if (!this->hasChild())
      {
         //oss << "<!-- No definition -->" << std::endl; // fait planter l'incrémentation
      }
      else
      {

#define DECLARE_NODE(Name_, name_)    \
   if (CObjectFactory::HasObject<C##Name_##Definition>(C##Name_##Definition::GetDefName())) \
   oss << *CObjectFactory::GetObject<C##Name_##Definition>(C##Name_##Definition::GetDefName()) << std::endl;
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"

      }

      oss << "</" << CContext::GetName() << " >";

      return (oss.str());
   }

   //----------------------------------------------------------------

   void CContext::solveDescInheritance(const CAttributeMap * const UNUSED(parent))
   {
#define DECLARE_NODE(Name_, name_)    \
   if (CObjectFactory::HasObject<C##Name_##Definition>(C##Name_##Definition::GetDefName())) \
   CObjectFactory::GetObject<C##Name_##Definition>(C##Name_##Definition::GetDefName())->solveDescInheritance();
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"
   }

   //----------------------------------------------------------------

   bool CContext::hasChild(void) const
   {
      return (
#define DECLARE_NODE(Name_, name_)    \
   CObjectFactory::HasObject<C##Name_##Definition>  (C##Name_##Definition::GetDefName())   ||
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"
      false);
}

   //----------------------------------------------------------------

   void CContext::solveFieldRefInheritance(void)
   {
      if (!this->hasId()) return;
      std::vector<boost::shared_ptr<CField> > allField
               = CObjectTemplate<CField>::GetAllVectobject(this->getId());
      std::vector<boost::shared_ptr<CField> >::iterator 
         it = allField.begin(), end = allField.end();
            
      for (; it != end; it++)
      {
         boost::shared_ptr<CField> field = *it;
         field->solveRefInheritance();
      }
   }

   //----------------------------------------------------------------

   void CContext::CleanTree(void)
   {
#define DECLARE_NODE(Name_, name_) C##Name_##Group::ClearAllAttributes();
#define DECLARE_NODE_PAR(Name_, name_)
#include "node_type.conf"
   }
   ///---------------------------------------------------------------
   
   void CContext::initClient(MPI_Comm intraComm, MPI_Comm interComm)
   {
     hasClient=true ;
     client = new CContextClient(this,intraComm, interComm) ;
   } 

   void CContext::initServer(MPI_Comm intraComm,MPI_Comm interComm)
   {
     hasServer=true ;
     server = new CContextServer(this,intraComm,interComm) ;
   } 

   bool CContext::eventLoop(void)
   {
     return server->eventLoop() ;
   } 
   
   void CContext::finalize(void)
   {
      if (hasClient && !hasServer)
      {
         client->finalize() ;
      }
      if (hasServer)
      {
        closeAllFile() ;
      }
   }
       
       
 
   
   void CContext::closeDefinition(void)
   {
      if (hasClient && !hasServer) sendCloseDefinition() ;
      
      solveCalendar();         
         
      // Résolution des héritages pour le context actuel.
      this->solveAllInheritance();

      //Initialisation du vecteur 'enabledFiles' contenant la liste des fichiers à sortir.
      this->findEnabledFiles();

      //Recherche des champs à sortir (enable à true + niveau de sortie correct)
      // pour chaque fichier précédemment listé.
      this->findAllEnabledFields();

      // Résolution des références de grilles pour chacun des champs.
      this->solveAllGridRef();

      // Traitement des opérations.
      this->solveAllOperation();

      // Nettoyage de l'arborescence
      CleanTree();
      if (hasClient) sendCreateFileHeader() ;
   }
   
   void CContext::findAllEnabledFields(void)
   {
     for (unsigned int i = 0; i < this->enabledFiles.size(); i++)
     (void)this->enabledFiles[i]->getEnabledFields();
   }

   void CContext::solveAllGridRef(void)
   {
     for (unsigned int i = 0; i < this->enabledFiles.size(); i++)
     this->enabledFiles[i]->solveEFGridRef();
   }

   void CContext::solveAllOperation(void)
   {
      for (unsigned int i = 0; i < this->enabledFiles.size(); i++)
      this->enabledFiles[i]->solveEFOperation();
   }

   void CContext::solveAllInheritance(void)
   {
     // Résolution des héritages descendants (càd des héritages de groupes)
     // pour chacun des contextes.
      solveDescInheritance();

     // Résolution des héritages par référence au niveau des fichiers.
      const std::vector<boost::shared_ptr<CFile> > & allFiles
             = CObjectFactory::GetObjectVector<CFile>();

      for (unsigned int i = 0; i < allFiles.size(); i++)
         allFiles[i]->solveFieldRefInheritance();
   }

   void CContext::findEnabledFiles(void)
   {
      const std::vector<boost::shared_ptr<CFile> > & allFiles
          = CObjectFactory::GetObjectVector<CFile>();

      for (unsigned int i = 0; i < allFiles.size(); i++)
         if (!allFiles[i]->enabled.isEmpty()) // Si l'attribut 'enabled' est défini.
            if (allFiles[i]->enabled.getValue()) // Si l'attribut 'enabled' est fixé à vrai.
               enabledFiles.push_back(allFiles[i]);

      if (enabledFiles.size() == 0)
         DEBUG(<<"Aucun fichier ne va être sorti dans le contexte nommé \""
               << getId() << "\" !");
   }

   void CContext::closeAllFile(void)
   {
     std::vector<boost::shared_ptr<CFile> >::const_iterator
            it = this->enabledFiles.begin(), end = this->enabledFiles.end();
         
     for (; it != end; it++)
     {
       info(30)<<"Closing File : "<<(*it)->getId()<<endl;
       (*it)->close();
     }
   }
   
   bool CContext::dispatchEvent(CEventServer& event)
   {
      
      if (SuperClass::dispatchEvent(event)) return true ;
      else
      {
        switch(event.type)
        {
           case EVENT_ID_CLOSE_DEFINITION :
             recvCloseDefinition(event) ;
             return true ;
             break ;
           case EVENT_ID_UPDATE_CALENDAR :
             recvUpdateCalendar(event) ;
             return true ;
             break ;
           case EVENT_ID_CREATE_FILE_HEADER :
             recvCreateFileHeader(event) ;
             return true ;
             break ;
           default :
             ERROR("bool CContext::dispatchEvent(CEventServer& event)",
                    <<"Unknown Event") ;
           return false ;
         }
      }
   }
   
   void CContext::sendCloseDefinition(void)
   {

     CEventClient event(getType(),EVENT_ID_CLOSE_DEFINITION) ;   
     if (client->isServerLeader())
     {
       CMessage msg ;
       msg<<this->getId() ;
       event.push(client->getServerLeader(),1,msg) ;
       client->sendEvent(event) ;
     }
     else client->sendEvent(event) ;
   }
   
   void CContext::recvCloseDefinition(CEventServer& event)
   {
      
      CBufferIn* buffer=event.subEvents.begin()->buffer;
      string id;
      *buffer>>id ;
      get(id)->closeDefinition() ;
   }
   
   void CContext::sendUpdateCalendar(int step)
   {
     if (!hasServer)
     {
       CEventClient event(getType(),EVENT_ID_UPDATE_CALENDAR) ;   
       if (client->isServerLeader())
       {
         CMessage msg ;
         msg<<this->getId()<<step ;
         event.push(client->getServerLeader(),1,msg) ;
         client->sendEvent(event) ;
       }
       else client->sendEvent(event) ;
     }
   }
   
   void CContext::recvUpdateCalendar(CEventServer& event)
   {
      
      CBufferIn* buffer=event.subEvents.begin()->buffer;
      string id;
      *buffer>>id ;
      get(id)->recvUpdateCalendar(*buffer) ;
   }
   
   void CContext::recvUpdateCalendar(CBufferIn& buffer)
   {
      int step ;
      buffer>>step ;
      updateCalendar(step) ;
   }
   
   void CContext::sendCreateFileHeader(void)
   {

     CEventClient event(getType(),EVENT_ID_CREATE_FILE_HEADER) ;   
     if (client->isServerLeader())
     {
       CMessage msg ;
       msg<<this->getId() ;
       event.push(client->getServerLeader(),1,msg) ;
       client->sendEvent(event) ;
     }
     else client->sendEvent(event) ;
   }
   
   void CContext::recvCreateFileHeader(CEventServer& event)
   {
      
      CBufferIn* buffer=event.subEvents.begin()->buffer;
      string id;
      *buffer>>id ;
      get(id)->recvCreateFileHeader(*buffer) ;
   }
   
   void CContext::recvCreateFileHeader(CBufferIn& buffer)
   {
      createFileHeader() ;
   }
   
   void CContext::updateCalendar(int step)
   {
      calendar->update(step) ;
   }
 
   void CContext::createFileHeader(void )
   {
      vector<shared_ptr<CFile> >::const_iterator it ;
         
      for (it=enabledFiles.begin(); it != enabledFiles.end(); it++)
      {
/*         shared_ptr<CFile> file = *it;
         StdString filename = (!file->name.isEmpty()) ?   file->name.getValue() : file->getId();
         StdOStringStream oss;
         if (! output_dir.isEmpty()) oss << output_dir.getValue();
         oss << filename;
         if (!file->name_suffix.isEmpty()) oss << file->name_suffix.getValue();

         bool multifile=true ;
         if (!file->type.isEmpty())
         {
           StdString str=file->type.getValue() ; 
           if (str.compare("one_file")==0) multifile=false ;
           else if (str.compare("multi_file")==0) multifile=true ;
           else ERROR("void Context::createDataOutput(void)",
                      "incorrect file <type> attribut : must be <multi_file> or <one_file>, "
                      <<"having : <"<<str<<">") ;
         } 
         if (multifile) 
         {
            if (server->intraCommSize > 1) oss << "_" << server->intraCommRank;
         }
         oss << ".nc";

         shared_ptr<io::CDataOutput> dout(new T(oss.str(), false,server->intraComm,multifile));
*/
         (*it)->createHeader();
      }
   } 
   
   shared_ptr<CContext> CContext::current(void)
   {
     return CObjectFactory::GetObject<CContext>(CObjectFactory::GetCurrentContextId()) ;
   }
   
} // namespace tree
} // namespace xmlioserver
