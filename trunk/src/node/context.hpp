#ifndef __XMLIO_CContext__
#define __XMLIO_CContext__

/// xios headers ///
#include "xmlioserver_spl.hpp"
//#include "node_type.hpp"
#include "calendar.hpp"

#include "declare_group.hpp"
//#include "context_client.hpp"
//#include "context_server.hpp"
#include "data_output.hpp"

#include "mpi.hpp"


namespace xios {
   class CContextClient ;
   class CContextServer ;


   /// ////////////////////// Déclarations ////////////////////// ///
   class CContextGroup;
   class CContextAttributes;
   class CContext;
   class CFile;
   ///--------------------------------------------------------------

   // Declare/Define CFileAttribute
   BEGIN_DECLARE_ATTRIBUTE_MAP(CContext)
#  include "context_attribute.conf"
   END_DECLARE_ATTRIBUTE_MAP(CContext)

   ///--------------------------------------------------------------
  /*!
  \class CContext
   This class corresponds to the concrete presentation of context in xml file and in play an essential role in XIOS
   Each object of this class contains all root definition of elements: files, fiels, domains, axis, etc, ... from which
   we can have access to each element.
   In fact, every thing must a be inside a particuliar context. After the xml file (iodef.xml) is parsed,
   object of the class is created and its contains all information of other elements in the xml file.
  */
   class CContext
      : public CObjectTemplate<CContext>
      , public CContextAttributes
   {
         public :
         enum EEventId
         {
           EVENT_ID_CLOSE_DEFINITION,EVENT_ID_UPDATE_CALENDAR,
           EVENT_ID_CREATE_FILE_HEADER,EVENT_ID_CONTEXT_FINALIZE,
           EVENT_ID_POST_PROCESS
         } ;

         /// typedef ///
         typedef CObjectTemplate<CContext>   SuperClass;
         typedef CContextAttributes SuperClassAttribute;

      public :

         typedef CContextAttributes RelAttributes;
         typedef CContext           RelGroup;

         //---------------------------------------------------------

      public :

         /// Constructeurs ///
         CContext(void);
         explicit CContext(const StdString & id);
         CContext(const CContext & context);       // Not implemented yet.
         CContext(const CContext * const context); // Not implemented yet.

         /// Destructeur ///
         virtual ~CContext(void);

         //---------------------------------------------------------

      public :

         /// Mutateurs ///
         void setCalendar(boost::shared_ptr<CCalendar> newCalendar);

         /// Accesseurs ///
         boost::shared_ptr<CCalendar>      getCalendar(void) const;

      public :
         // Initialize server or client
         void initServer(MPI_Comm intraComm, MPI_Comm interComm) ;
         void initClient(MPI_Comm intraComm, MPI_Comm interComm) ;
         bool isInitialized(void) ;

         // Put sever or client into loop state
         bool eventLoop(void) ;
         bool serverLoop(void) ;
         void clientLoop(void) ;

         // Process all information of calendar
         void solveCalendar(void);

         // Finalize a context
         void finalize(void) ;
         void closeDefinition(void) ;

         // Some functions to process context
         void findAllEnabledFields(void);
         void processEnabledFiles(void) ;
         void solveAllInheritance(bool apply=true) ;
         void findEnabledFiles(void);
         void closeAllFile(void) ;
         void updateCalendar(int step) ;
         void createFileHeader(void ) ;
         void solveAllRefOfEnabledFields(bool sendToServer);
         void buildAllExpressionOfEnabledFields();
         void postProcessing();

         std::map<int, StdSize>& getDataSize();
         void setClientServerBuffer();

         // Send context close definition
         void sendCloseDefinition(void) ;
         // There are something to send on closing context defintion
         void sendUpdateCalendar(int step) ;
         void sendCreateFileHeader(void) ;
         void sendEnabledFiles();
         void sendEnabledFields();
         void sendRefDomainsAxis();
         void sendRefGrid();
         void sendPostProcessing();

         // Client side: Receive and process messages
         static void recvUpdateCalendar(CEventServer& event) ;
         void recvUpdateCalendar(CBufferIn& buffer) ;
         static void recvCloseDefinition(CEventServer& event) ;
         static void recvCreateFileHeader(CEventServer& event) ;
         void recvCreateFileHeader(CBufferIn& buffer) ;
         static void recvSolveInheritanceContext(CEventServer& event);
         void recvSolveInheritanceContext(CBufferIn& buffer);
         static void recvPostProcessing(CEventServer& event);
         void recvPostProcessing(CBufferIn& buffer);

         // dispatch event
         static bool dispatchEvent(CEventServer& event) ;

      public:
        // Get current context
        static CContext* getCurrent(void);

        // Get context root
        static CContextGroup* getRoot(void);

        // Set current context
        static void setCurrent(const string& id);

        // Create new context
        static CContext* create(const string& id = "");

        /// Accesseurs statiques ///
        static StdString GetName(void);
        static StdString GetDefName(void);
        static ENodeType GetType(void);

        static CContextGroup* GetContextGroup(void);

        // Some functions to visualize structure of current context
        static void ShowTree(StdOStream & out = std::clog);
        static void CleanTree(void);

      public :
         // Parse xml node and write all info into context
         virtual void parse(xml::CXMLNode & node);

         // Visualize a context
         virtual StdString toString(void) const;


         // Solve all inheritance relation in current context
         virtual void solveDescInheritance(bool apply, const CAttributeMap * const parent = 0);

         // Verify if all root definition in a context have children
         virtual bool hasChild(void) const;

      public :
         // Calendar of context
         boost::shared_ptr<CCalendar>   calendar;

         // List of all enabled files (files on which fields are written)
         std::vector<CFile*> enabledFiles;

         // Context root
         static shared_ptr<CContextGroup> root ;

         // Determine context on client or not
         bool hasClient ;

         // Determine context on server or not
         bool hasServer ;

         // Concrete context server
         CContextServer* server ;

         // Concrete contex client
         CContextClient* client ;

      private:
         bool isPostProcessed;
         std::map<int, StdSize> dataSize_;


      public: // Some function maybe removed in the near future
        // virtual void toBinary  (StdOStream & os) const;
        // virtual void fromBinary(StdIStream & is);
        // void solveAllGridRef(void);
        // void solveAllOperation(void);
        // void solveAllExpression(void);
        // void solveFieldRefInheritance(bool apply);

   }; // class CContext

   ///--------------------------------------------------------------

   // Declare/Define CContextGroup and CContextDefinition
   DECLARE_GROUP(CContext);

   ///--------------------------------------------------------------

} // namespace xios

#endif // __XMLIO_CContext__
