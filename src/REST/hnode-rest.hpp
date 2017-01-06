#ifndef __REST_H__
#define __REST_H__

#include <string>
#include <list>
#include <vector>
#include <map>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/tuple/tuple.hpp>

#define REST_DAEMON_DEFAULT_PORT 8200

typedef enum RestRepDataItemType
{
   REST_RDI_TYPE_NOTSET,
   REST_RDI_TYPE_URI_PARAMETER,
   REST_RDI_TYPE_HTTP_HEADER,
   REST_RDI_TYPE_QUERY_PARAMETER,
   REST_RDI_TYPE_ENCODED_PARAMETER,
   REST_RDI_TYPE_ENCODED_FILE,
   REST_RDI_TYPE_COOKIE,
   REST_RDI_TYPE_SIMPLE_CONTENT,
}REST_RDI_TYPE_T;

class RESTRepDataItem
{
    private:
        REST_RDI_TYPE_T type;

        std::string     key;
        std::string     contentType;

        std::string     filename;
        std::string     tmpfilepath;

        unsigned long   bufferLength;
        unsigned long   dataLength;
        unsigned char  *bufferPtr;
 
        size_t appendDataToBuffer( const char *buffer, size_t length );
        size_t appendDataToFile( const char *buffer, size_t length );

    public:
        RESTRepDataItem();
       ~RESTRepDataItem();
 
        void setType( REST_RDI_TYPE_T typeValue );
        REST_RDI_TYPE_T getType();

        void setKey( std::string keyValue );
        std::string getKey();

        void setContentType( std::string contentTypeValue );
        std::string getContentType();

        void setFilename( std::string filename );
        std::string getFilename();

        void generateTmpFilePath();
        std::string getTempFilePath();

        unsigned long  getLength();
        unsigned char *getBuffer();

        void resetData();
        size_t addData( unsigned long offset, const char *buffer, size_t length );

        std::string getDataAsStr();
};

class RESTRepresentation
{
    private:
        std::map<std::string, RESTRepDataItem *> uriParam;
        std::map<std::string, RESTRepDataItem *> queryParam;
        std::map<std::string, RESTRepDataItem *> httpHeader;
        std::map<std::string, RESTRepDataItem *> encodedParam;
        std::map<std::string, RESTRepDataItem *> cookieMap;
 
        RESTRepDataItem *simpleContent;

    public:
        RESTRepresentation();
       ~RESTRepresentation();

        // Copy from other representation
        bool setFromRepresentation( RESTRepresentation *srep );

        // REST_RDI_TYPE_URI_PARAMETER
        bool hasURIParameters();
        void clearURIParameters();
        void addURIParameter( std::string name, std::string value );
        bool getURIParameter( std::string name, std::string &value );
        void getURIParameterMap( std::map< std::string, std::string > &paramMap );

        // REST_RDI_TYPE_HTTP_HEADER
        bool hasHTTPHeaders();
        void clearHTTPHeaders();
        void addHTTPHeader( std::string name, std::string value );
        bool getHTTPHeader( std::string name, std::string &value );
        void getHTTPHeaderMap( std::map< std::string, std::string > &headerMap );

        //bool getHTTPHeaderNameList( std::vector< std::string > &nameList );

        // REST_RDI_TYPE_QUERY_PARAMETER
        bool hasQueryParameters();
        void clearQueryParameters();
        void addQueryParameter( std::string name, std::string value );
        bool getQueryParameter( std::string name, std::string &value );
        void getQueryParameterMap( std::map< std::string, std::string > &paramMap );

        // REST_RDI_TYPE_ENCODED_PARAMETER
        // REST_RDI_TYPE_ENCODED_FILE
        bool hasEncodedParameters();
        void clearEncodedParameters();

        void addEncodedParameter( std::string key, std::string contentType, const char *data, unsigned long offset, unsigned long size );
        void addEncodedFile( std::string key, std::string filename, std::string contentType, const char *data, unsigned long offset, unsigned long size );
        void updateEncodedData( std::string key, const char* data, unsigned long offset, unsigned long size );
 
        bool getEncodedParamInfo( std::string name, bool &isFile, std::string &contentType, unsigned long &contentLength );
        bool getEncodedFileInfo( std::string name, std::string &filename, std::string &localpath );

        bool getEncodedDataAsStr( std::string name, std::string &value );
        unsigned char *getEncodedDataAsPtr( std::string name, unsigned long offset, unsigned long windowLength );

        // REST_RDI_TYPE_COOKIE
        bool hasCookies();
        void clearCookies();
        void addCookie( std::string name, std::string cookie );
        bool getCookie( std::string name, std::string &cookie );

        // REST_RDI_TYPE_SIMPLE_CONTENT,
        bool hasSimpleContent();
        void clearSimpleContent();

        void setSimpleContent( std::string contentType );

        void setSimpleContent( unsigned char *contentPtr, unsigned long contentLength );
        void setSimpleContent( std::string contentType, unsigned char *contentPtr, unsigned long contentLength );

        void appendSimpleContent( unsigned char *contentPtr, unsigned long contentLength );

        unsigned long getSimpleContentLength();
        unsigned char* getSimpleContentPtr( std::string &contentType, unsigned long &contentLength );
};

class RESTContentFieldDef
{
    private:
        std::string name;
        bool        required;
        
    public:
        RESTContentFieldDef();
       ~RESTContentFieldDef();

        void setName( std::string nameStr );
        std::string getName();
};

class RESTContentField
{
    private:
        std::string name;
        std::string value;
        
    public:
        RESTContentField();
       ~RESTContentField();

        void setName( std::string nameStr );
        std::string getName();

        void setValue( std::string valueStr );
        std::string getValue();
};

class RESTContentReferenceDef
{
    private:
        std::string name;
        bool        required;
        
    public:
        RESTContentReferenceDef();
       ~RESTContentReferenceDef();

        void setName( std::string nameStr );
        std::string getName();
};

#if 0
class RESTContentReference
{
    private:
        std::string name;
        std::string value;
        
    public:
        RESTContentReference();
       ~RESTContentReference();

        void setName( std::string nameStr );
        std::string getName();

        void setValue( std::string valueStr );
        std::string getValue();
};
#endif

typedef enum RESTContentNodeType
{
    RCNT_NOTSET,
    RCNT_ARRAY,
    RCNT_ID,
    RCNT_OBJECT,
}RCN_TYPE;

class RESTContentBase
{
    private:
        std::string name;

        RESTContentBase *parent;        
        std::vector< RESTContentBase * > children;

        RCN_TYPE type;

    public:
        RESTContentBase();
       ~RESTContentBase();

        void setAsArray( std::string name );
        void setAsID( std::string idValue );
        void setAsObject( std::string name );

        RCN_TYPE getType();

        bool isArray();
        bool isID();
        bool isObj();

        void setName( std::string nameValue );
        bool hasName();
        std::string getName();

        void setParent( RESTContentBase *parentPtr );
        RESTContentBase *getParent();

        void addChild( RESTContentBase *childPtr );
        unsigned long getChildCount();
        RESTContentBase *getChildByIndex( unsigned long index );
        RESTContentBase *getChildByName( std::string name );
};

// Forward Declaration
class RESTContentTemplate : public RESTContentBase
{
    private:

        // Used to indicate a static, singalton object
        bool staticFlag;

        std::string factoryType;
        unsigned int templateID;

        std::map< std::string, RESTContentFieldDef >     fieldDefs;
        std::map< std::string, RESTContentReferenceDef > referenceDefs;

    public:
        RESTContentTemplate();
       ~RESTContentTemplate();

        void setFactoryType( std::string type );
        std::string getFactoryType();

        void setTemplateID( unsigned int idVal );
        unsigned int getTemplateID();

        bool isStatic();
        void setStatic();
        void clearStatic();

        void defineField( std::string name, bool required );
        void defineRef( std::string name, bool required );

        bool getFieldInfo( std::string name, bool &required );

        std::vector< RESTContentFieldDef * > getFieldList();

        bool checkForFieldMatch( std::string name );

        bool checkForRefMatch( std::string name );

        bool checkForTagMatch( std::string name );

        bool checkForChildMatch( std::string name, RESTContentTemplate **cnPtr );

};

struct RESTCMVertex 
{ 
    std::string objID; 
};

struct RESTCMEdge
{ 
    std::string relationID; 
};

typedef boost::adjacency_list< boost::listS, boost::listS, boost::undirectedS, RESTCMVertex, RESTCMEdge > refGraph_t;
typedef boost::graph_traits< refGraph_t >::vertex_descriptor refGraphVertex_t;

class RESTContentNode : public RESTContentBase
{
    private:
        std::map< std::string, RESTContentField >     fieldValues;
        //std::map< std::string, RESTContentReference > refValues;

        refGraphVertex_t vertex;

    public:
        RESTContentNode();
       ~RESTContentNode();

        void setID( std::string idValue );
        std::string getID();

        void setField( std::string name, std::string value );
        bool clearField( std::string name );
        bool getField( std::string name, std::string &value );

        std::vector< RESTContentField* > getFieldList();

        //void setRef( std::string name, std::string value );
        //bool clearRef( std::string name );
        //bool getRef( std::string name, std::string &value );

        //std::vector< RESTContentReference* > getRefList();

        void setVertex( refGraphVertex_t newVertex );
        refGraphVertex_t getVertex();

        virtual unsigned int getObjType();
        virtual void setFieldsFromContentNode( RESTContentNode *objCN );
        virtual void setContentNodeFromFields( RESTContentNode *objCN );
};

class RESTContentException : public std::exception
{
    private:
        unsigned long eCode;
        std::string eMsg;

    public:
        RESTContentException( unsigned long errCode, std::string errMsg )
        {
            eCode = errCode;
            eMsg  = errMsg;
        }

       ~RESTContentException() throw() {};

        virtual const char* what() const throw()
        {
            return eMsg.c_str();
        }

        unsigned long getErrorCode() const throw()
        {
            return eCode;
        }

        std::string getErrorMsg() const throw()
        {
            return eMsg;
        }
};

typedef enum RESTContentRefElementTypeEnum
{
    RCRE_TYPE_ROOT,
    RCRE_TYPE_PATH,
    RCRE_TYPE_TERMINAL,
}RCRE_TYPE_T;

class RESTContentRefElement
{
    private:
        RCRE_TYPE_T  type;

        std::string  objID;
        std::string  listName;
        unsigned int relIndex;

    public:
        RESTContentRefElement();
       ~RESTContentRefElement();

        void setAsRoot( std::string listName, unsigned int relIndex );
        void setAsPath( std::string objID, std::string listName, unsigned int relIndex );
        void setAsTerminal( std::string objID );
};

// A class for holding references to other RESTContentNode objects
//class RESTContentRef
//{
//    private:
//        std::string refID;

//    public:
//        RESTContentRef();
//       ~RESTContentRef();

//        void setID( std::string newID );
//        std::string getID();
//};

class RCMException : public std::exception
{
    private:
        unsigned long eCode;
        std::string eMsg;

    public:
        RCMException( unsigned long errCode, std::string errMsg )
        {
            eCode = errCode;
            eMsg  = errMsg;
        }

       ~RCMException() throw() {};

        virtual const char* what() const throw()
        {
            return eMsg.c_str();
        }

        unsigned long getErrorCode() const throw()
        {
            return eCode;
        }

        std::string getErrorMsg() const throw()
        {
            return eMsg;
        }
};

class RESTContentEdge
{
    private:
        std::string srcID;
        std::string dstID;
        std::string edgeType;

    public:
        RESTContentEdge();
       ~RESTContentEdge();

        void setEdgeData( std::string srcNodeID, std::string dstNodeID, std::string type );

        std::string getSrcID();
        std::string getDstID();
        std::string getEdgeType();
};

// A base class for manager objects
class RESTContentManager
{
    private:

        // Store the object pointers themselves
        std::map< std::string, RESTContentNode* > objMap; 

        // Store the references between objects
        refGraph_t refGraph;

        // Keep track of next available ID
        unsigned int nextID;

        // Setup the base configuration
        void init();

        // Generate new objects
        std::string getUniqueObjID( std::string prefix );
        virtual RESTContentNode* newObject( unsigned int type ) = 0;
        virtual void freeObject( RESTContentNode *objPtr ) = 0;

    public:
        RESTContentManager();
       ~RESTContentManager();

        virtual unsigned int getTypeFromObjectElementName( std::string name ) = 0;

        virtual RESTContentTemplate *getContentTemplateForType( unsigned int type ) = 0;

        virtual void notifyCfgChange();
        virtual void notifyClear();

        virtual void populateContentNodeFromStatusProvider( unsigned int id, RESTContentNode *outNode, std::map< std::string, std::string > paramMap ) = 0;

        void clear();

        void createObj( unsigned int type, std::string idPrefix, std::string &objID );
        void addObj( unsigned int type, std::string objID );

        void updateObj( std::string objID, RESTContentNode *inputNode );

        void removeObjByID( std::string objID );
        void deleteObjByID( std::string objID );

        void removeAllObjects();
        void deleteAllObjects();

        void addRelationship( std::string relID, std::string parentID, std::string childID );
        void removeRelationship( std::string relID, std::string parentID, std::string childID );

        bool hasRelationship( std::string rootID, std::string childID, std::string listID );

        RESTContentNode* getObjectByID( std::string idStr );
        //RESTContentNode* getObjectFromRef( RESTContentRef &ref );

        void getIDListForRelationship( std::string parentID, std::string relID, std::vector< std::string > &idList );  

        unsigned int getObjectCountByType( unsigned int type );

        void getIDVectorByType( unsigned int type, std::vector< std::string > &rtnVector );
        void getObjectVectorByType( unsigned int type, std::vector< RESTContentNode* > &rtnVector );

        void getObjectList( std::list< RESTContentNode* > &objList );

        void getObjectRepresentationList( std::vector< RESTContentNode > &objList );
        void getEdgeRepresentationList( std::vector< RESTContentEdge > &edgeList );

        //void addReference( std::string rootID, std::string listName, std::string tgtID );

        void lookupTerminal( std::vector< RESTContentRefElement > path );

};

class RESTContentHelper
{
    private:
        RESTContentNode *rootNode;

    public:
        RESTContentHelper();
       ~RESTContentHelper();

        void clearRootNode();
        RESTContentNode *getRootNode();
        RESTContentNode *detachRootNode();
        static void freeDetachedRootNode( RESTContentNode *objPtr );

        bool hasRootObject( std::string objectName, RESTContentNode **objPtr );

        RESTContentNode *getObject( std::string objectName );

        virtual bool parseRawData( RESTRepresentation *repPtr ) = 0;
        virtual bool parseWithTemplate( RESTContentTemplate *templateCN, RESTRepresentation *repPtr ) = 0;

        virtual bool generateContentRepresentation( RESTRepresentation *repPtr ) = 0;
};

class RESTContentHelperXML : public RESTContentHelper
{
    private:

        bool getAttribute( void *docPtr, void *nodePtr, std::string attrName, std::string &result );
        bool getChildContent( void *docPtr, void *nodePtr, std::string childName, std::string &result );
        void addFieldValues( void *docPtr, void *nodePtr, RESTContentNode *cnPtr );
        void parseTree( void *docPtr, void *nodePtr, RESTContentNode *cnPtr );

        bool findFieldValue( std::string fieldName, RESTContentNode *cnPtr, void *docPtr, void *nodePtr );

        bool generateChildContent(  RESTContentNode *childCN, RESTRepresentation *repPtr );
        bool generateArrayContent( RESTContentNode *arrCN, RESTRepresentation *repPtr );
        bool generateObjectContent( RESTContentNode *objCN, RESTRepresentation *repPtr );
        bool generateIDContent( RESTContentNode *idCN, RESTRepresentation *repPtr );

    public:
        RESTContentHelperXML();
       ~RESTContentHelperXML();
 
        static bool supportsContentDecode( std::string contentType );
        static bool supportsContentCreate( std::string contentType );

        virtual bool parseRawData( RESTRepresentation *repPtr );
        virtual bool parseWithTemplate( RESTContentTemplate *templateCN, RESTRepresentation *repPtr );

        virtual bool generateContentRepresentation( RESTRepresentation *repPtr );

};

class RESTContentHelperJSON : public RESTContentHelper
{
    private:

    public:
        RESTContentHelperJSON();
       ~RESTContentHelperJSON();
 
};

class RESTContentHelperFactory
{
    private:

    public:
        //static RESTContentHelper *decodeSimpleContent( RESTRepresentation *repPtr );
        //static RESTContentHelper *decodeSimpleContent( RESTContentNode *templateRoot, RESTRepresentation *repPtr );

        static RESTContentNode *newContentNode();
        static void freeContentNode( RESTContentNode *nodePtr );

        static RESTContentTemplate *newContentTemplate();
        static void freeContentTemplate( RESTContentTemplate *nodePtr );

        static RESTContentHelper *getRequestSimpleContentHelper( RESTRepresentation *repPtr );
        static RESTContentHelper *getResponseSimpleContentHelper( RESTRepresentation *repPtr );

        static void freeContentHelper( RESTContentHelper *chPtr );
};

typedef enum RESTResourceMethodFlags
{
    REST_RMETHOD_NONE   = 0x00,
    REST_RMETHOD_GET    = 0x01,
    REST_RMETHOD_PUT    = 0x02,
    REST_RMETHOD_POST   = 0x04,
    REST_RMETHOD_DELETE = 0x08,
}REST_RMETHOD_T;

typedef enum RESTResponseCode
{
    REST_HTTP_RCODE_NONE             = 0,
    REST_HTTP_RCODE_OK               = 200,
    REST_HTTP_RCODE_CREATED          = 201,
    REST_HTTP_RCODE_ACCEPTED         = 202,
    REST_HTTP_RCODE_PARTIALINFO      = 203,
    REST_HTTP_RCODE_NO_RESPONSE      = 204,
    REST_HTTP_RCODE_MOVED            = 301,
    REST_HTTP_RCODE_FOUND            = 302,
    REST_HTTP_RCODE_METHOD           = 303,
    REST_HTTP_RCODE_NOT_MODIFIED     = 304,
    REST_HTTP_RCODE_BAD_REQUEST      = 400,
    REST_HTTP_RCODE_UNAUTHORIZED     = 401,
    REST_HTTP_RCODE_PAYMENT_REQUIRED = 402,
    REST_HTTP_RCODE_FORBIDDEN        = 403,
    REST_HTTP_RCODE_NOT_FOUND        = 404,
    REST_HTTP_RCODE_INTERNAL_ERROR   = 500,
    REST_HTTP_RCODE_NOT_IMPLEMENTED  = 501,
    REST_HTTP_RCODE_OVERLOADED       = 502,
    REST_HTTP_RCODE_GATEWAY_TIMEOUT  = 503,
}REST_HTTP_RCODE_T;

class RESTRequest
{
    private:
        int connectiontype;
        char *answerstring;
        //struct MHD_PostProcessor *postprocessor;
        
        struct MHD_Connection    *connection;
        struct MHD_PostProcessor *postProcessor;

        RESTRepresentation  inRepresentation;
        RESTRepresentation  outRepresentation;

        std::string urlStr;

        REST_RMETHOD_T reqMethod;

//        std::map<std::string, std::string> reqParams;

        std::vector<std::string> wildcardElems;

        REST_HTTP_RCODE_T rspCode;

        void *resourcePtr;
        void *daemonPtr;

        static int iterate_post( void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                                 const char *filename, const char *content_type,
                                 const char *transfer_encoding, const char *data, uint64_t off,
                                 size_t size );

        static int iterate_headers( void *cls, MHD_ValueKind kind, const char *key, const char *value );
        static int iterate_arguments( void *cls, MHD_ValueKind kind, const char *key, const char *value );


    public:
        RESTRequest( struct MHD_Connection *connection, void *daemonPtr );
       ~RESTRequest();

        struct MHD_Connection *getConnection();

        RESTRepresentation *getInboundRepresentation();
        RESTRepresentation *getOutboundRepresentation();

        void setURL( std::string url );
        std::string getURL();

        void decodeRequestMethod( std::string method );
        void setVersion( std::string version );

        REST_RMETHOD_T getMethod();

        void setResponseCode( REST_HTTP_RCODE_T code );
        REST_HTTP_RCODE_T getResponseCode();

        void clearURIParameters();
        void setURIParameter( std::string name, std::string value );
        bool getURIParameter( std::string name, std::string &value );

#if 0
        void clearParameters();
        void setParameter( std::string name, std::string value );
        bool getParameter( std::string name, std::string &value );
        unsigned int getParameterCount();
        bool getParameterByIndex( unsigned int index, std::string &name, std::string &value );
        std::string getParametersAsQueryStr();
#endif

        void clearWildcardElements();
        bool appendWildcardElement( std::string elemStr );
        std::string getWildcardURL();

        int processUploadData( const char *upload_data, size_t upload_data_size );

        void requestRXData();

        int processHeader( enum MHD_ValueKind kind, const char *key, const char *value );
        int processUrlArg( enum MHD_ValueKind kind, const char *key, const char *value );
        int processDataIteration( enum MHD_ValueKind kind, const char *key,
                           const char *filename, const char *content_type,
                           const char *transfer_encoding, const char *data, uint64_t off,
                           size_t size );

        void requestHeaderData();

        void setLink( void *resource );

        void execute();

        void sendResourceCreatedResponse( std::string newResource );

        void sendErrorResponse( REST_HTTP_RCODE_T httpCode, unsigned long errCode, std::string errStr );

        void sendResponse();
};

typedef enum RESTResourcePatternElementMatchType
{
    REST_RPE_MATCH_NOTSET   = 0,
    REST_RPE_MATCH_STRING   = 1,
    REST_RPE_MATCH_PARAM    = 2,
    REST_RPE_MATCH_WILDCARD = 3,
}REST_RPE_MATCH_T;

class RESTResourcePE
{
    private:
        REST_RPE_MATCH_T  matchType;
        std::string       patternStr;

    public:
        
        RESTResourcePE();
       ~RESTResourcePE();

        void setMatch( REST_RPE_MATCH_T matchType, std::string elemStr );

        REST_RPE_MATCH_T getMatchType();

        std::string getMatchElement();

        bool isMatch( std::string testElem );
};

class RESTResourceRelationship
{
    private:
        std::string list;
        std::string paramID;

    public:
        RESTResourceRelationship();
       ~RESTResourceRelationship();

        void setListID( std::string listID );
        void setChildParamID( std::string urlID );

        std::string getListID();
        std::string getChildParamID();
};

class RESTResource
{
    private:
        std::string    urlPattern;
        REST_RMETHOD_T methods;
        bool           wildcard;
       
        std::vector<RESTResourcePE> patternElements;

        void parse(const std::string& url_s);

    public:
        RESTResource();
       ~RESTResource();

        void setURLPattern( std::string pattern, REST_RMETHOD_T methodFlags );

        unsigned int getMatchCount();

        bool hasWildcardMatch();

        bool linkRequest( RESTRequest *request );

        bool executeRequest( RESTRequest *request );

        virtual void restGet( RESTRequest *request );

        virtual void restPut( RESTRequest *request );

        virtual void restPost( RESTRequest *request );

        virtual void restDelete( RESTRequest *request );

};

class RESTContentManagerResource : public RESTResource
{
    private:

        std::vector<RESTResourceRelationship> relationshipStack;

    public:

        RESTContentManager &contentMgr;

        RESTContentManagerResource( RESTContentManager &mgr );
       ~RESTContentManagerResource();

        void clearRelationships();
        void appendRelationship( std::string listName, std::string childParamID );

        bool checkObjectRelationships( RESTRequest *request, std::string &terminalID );

        virtual void restGet( RESTRequest *request );

        virtual void restPut( RESTRequest *request );

        virtual void restPost( RESTRequest *request );

        virtual void restDelete( RESTRequest *request );

};

class RESTResourceRESTContentList : public RESTContentManagerResource
{
    private:
        unsigned int objType;

        std::string listElement;
        std::string objPrefix;
        //std::string relName;
        //std::string relRoot;

    public:
        RESTResourceRESTContentList( std::string pattern, RESTContentManager &mgr, unsigned int type, std::string list, std::string prefix );
       ~RESTResourceRESTContentList();

        virtual void restGet( RESTRequest *request );

        virtual void restPost( RESTRequest *request );
};

class RESTResourceRESTContentObject : public RESTContentManagerResource
{
    private:
        unsigned int objType;

    public:
        RESTResourceRESTContentObject(  std::string pattern, RESTContentManager &mgr, unsigned int type );
       ~RESTResourceRESTContentObject();

        virtual void restGet( RESTRequest *request );

        virtual void restPut( RESTRequest *request );

        virtual void restDelete( RESTRequest *request );
};

// A generic interface for providers of status information, with CRUD symatics
class RESTResourceRESTStatusProvider : public RESTContentManagerResource
{
    private:
        unsigned int id;

    public:
        RESTResourceRESTStatusProvider( std::string pattern, RESTContentManager &mgr, unsigned int dataID );
       ~RESTResourceRESTStatusProvider();

        virtual void restGet( RESTRequest *request );
};

class RESTDaemon
{
    private:
        unsigned short listenPort;

        struct MHD_Daemon *daemon;

        std::list<RESTResource *> resourceList;

        static int connection_request( void *cls, struct MHD_Connection *connection,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls);

        static void request_completed( void *cls, struct MHD_Connection *connection, 
                                       void **con_cls, enum MHD_RequestTerminationCode toe );

        int newRequest( RESTRequest *request, const char *upload_data, size_t *upload_data_size );
        int continueRequest( RESTRequest *request, const char *upload_data, size_t *upload_data_size );
        int requestReady( RESTRequest *request );

        void requestComplete( struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe );

    public:
        RESTDaemon();
       ~RESTDaemon();

        void setListeningPort( unsigned short port );

        int start();
        void stop();

        bool registerResource( RESTResource *resource );

        int sendResponse( RESTRequest *request );

};

typedef enum RESTHttpClientRequestType
{
    RHC_REQTYPE_GET,
    RHC_REQTYPE_POST,
    RHC_REQTYPE_PUT,
    RHC_REQTYPE_DELETE
}RHC_REQTYPE_T;

class RESTHttpClient
{
    private:
        RHC_REQTYPE_T  rType;
        std::string    host;
        std::string    bURL;

        bool debug;
        unsigned long timeout;
        unsigned long sentLength;

        RESTRepresentation outData;
        RESTRepresentation inData;

        static size_t processResponseHeader( void *buffer, size_t size, size_t nmemb, void *userp );
        static size_t processInboundData( void *buffer, size_t size, size_t nmemb, void *userp );
        static size_t processOutboundData( void *buffer, size_t size, size_t nmemb, void *userp );

    public:
        RESTHttpClient();
       ~RESTHttpClient();

        void setHost( std::string hostAddr );
        std::string getHost();

        void setRequest( RHC_REQTYPE_T reqType, std::string baseURL );

        void clearRepresentation();

        RESTRepresentation &getInboundRepresentation();
        RESTRepresentation &getOutboundRepresentation();

        void makeRequest();

        unsigned int reqResult();

        bool getLocationHeaderURL( std::string &url );
        bool getLocationHeaderTerminal( std::string &termStr );
};

#endif // __REST_H__

