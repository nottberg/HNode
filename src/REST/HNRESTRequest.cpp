#include <string.h>
#include <stdio.h>

#include "hnode-rest.hpp"

#define POSTBUFFERSIZE 1024

RESTRequest::RESTRequest(struct MHD_Connection *newConnection, void *daemon )
{
    //int connectiontype;
    //char *answerstring;
    //struct MHD_PostProcessor *postprocessor;
    connection = newConnection;

    reqMethod = REST_RMETHOD_NONE;

    rspCode = REST_HTTP_RCODE_NONE;

    resourcePtr = NULL;
    daemonPtr = daemon;

    postProcessor = NULL;
}

RESTRequest::~RESTRequest()
{
    if( postProcessor )
        MHD_destroy_post_processor( postProcessor );
}

void 
RESTRequest::setLink( void *resource )
{
    resourcePtr = resource;
}

void 
RESTRequest::setURL( std::string url )
{
    urlStr = url;
}

std::string 
RESTRequest::getURL()
{
    return urlStr;
}

struct MHD_Connection *
RESTRequest::getConnection()
{
    return connection;
}

RESTRepresentation *
RESTRequest::getInboundRepresentation()
{
    return &inRepresentation;
}

RESTRepresentation *
RESTRequest::getOutboundRepresentation()
{
    return &outRepresentation;
}

void 
RESTRequest::decodeRequestMethod( std::string method )
{
    reqMethod = REST_RMETHOD_NONE;

    if( "GET" == method )
        reqMethod = REST_RMETHOD_GET;
    else if( "PUT" == method )
        reqMethod = REST_RMETHOD_PUT;
    else if( "POST" == method )
        reqMethod = REST_RMETHOD_POST;
    else if( "DELETE" == method )
        reqMethod = REST_RMETHOD_DELETE;

}

void 
RESTRequest::setVersion( std::string version )
{

}

void 
RESTRequest::setResponseCode( REST_HTTP_RCODE_T code )
{
    rspCode = code;
}

REST_HTTP_RCODE_T
RESTRequest::getResponseCode()
{
    return rspCode;
}

REST_RMETHOD_T 
RESTRequest::getMethod()
{
    return reqMethod;
}

void 
RESTRequest::clearURIParameters()
{
    inRepresentation.clearURIParameters();
}

void 
RESTRequest::setURIParameter( std::string name, std::string value )
{
    inRepresentation.addURIParameter( name, value );
}

bool
RESTRequest::getURIParameter( std::string name, std::string &value )
{
    return inRepresentation.getURIParameter( name, value );
}

#if 0
void 
RESTRequest::clearParameters()
{
    reqParams.clear();
}

void 
RESTRequest::setParameter( std::string name, std::string value )
{
    reqParams.insert( std::pair<std::string, std::string>(name, value) );
}

bool
RESTRequest::getParameter( std::string name, std::string &value )
{
    std::map<std::string, std::string>::iterator it;

    value.clear();

    it=reqParams.find( name );

    if( it == reqParams.end() )
    {
        return true;
    }   

    value = it->second;
    
    return false;
}

unsigned int
RESTRequest::getParameterCount()
{
    return reqParams.size();
}

bool
RESTRequest::getParameterByIndex( unsigned int index, std::string &name, std::string &value )
{
    std::map<std::string, std::string>::iterator it;

    name.clear();
    value.clear();

    if( index >= reqParams.size() )
    {
        return false;
    }

    it = reqParams.begin();
    do
    {
       if( index == 0 )
       {
           name =  it->first;
           value = it->second;
           return true;
       }

       index -= 1;
       it++;
    }
    while( index );

    return false;
}

std::string
RESTRequest::getParametersAsQueryStr()
{
    std::map<std::string, std::string>::iterator it;
    std::string queryStr;

    std::string sepChar = "";
    for( it = reqParams.begin(); it != reqParams.end(); it++ )
    {
        queryStr = queryStr + sepChar + it->first + "=" + it->second;        
        sepChar="&";
    }

    return queryStr;
}
#endif

void
RESTRequest::clearWildcardElements()
{
    wildcardElems.clear();
}

bool 
RESTRequest::appendWildcardElement( std::string elemStr )
{
    wildcardElems.push_back( elemStr );
}

std::string
RESTRequest::getWildcardURL()
{
    std::string result;

    for( std::vector<std::string>::iterator it = wildcardElems.begin() ; it != wildcardElems.end(); ++it)
    {
        result += "/";
        result += *it;
    }
     
    return result;
}

int
RESTRequest::processUploadData( const char *upload_data, size_t upload_data_size )
{
    if( postProcessor == NULL )
    {
        postProcessor = MHD_create_post_processor( connection, POSTBUFFERSIZE, RESTRequest::iterate_post, (void *) this );
    }

    if( NULL == postProcessor )
    {
        if( upload_data_size )
        {
            // The data was not in url-encoded form so just store it raw.
            if( inRepresentation.hasSimpleContent() == false )
                inRepresentation.setSimpleContent( (unsigned char *)upload_data, upload_data_size );
            else
                inRepresentation.appendSimpleContent( (unsigned char *)upload_data, upload_data_size );
        }

        return 0;
    }

    printf( "processUploadData - length: %d \n", (int) upload_data_size );

    int result = MHD_post_process( postProcessor, upload_data, upload_data_size );

    printf( "processUploadData: %d\n", result );

    // Attempt to do form data processing.
    if( result == MHD_NO )
    {
        // The data was not in url-encoded form so just store it raw.
        if( inRepresentation.hasSimpleContent() == false )
            inRepresentation.setSimpleContent( (unsigned char *)upload_data, upload_data_size );
        else
            inRepresentation.appendSimpleContent( (unsigned char *)upload_data, upload_data_size );
    }

    return 0;
}

void 
RESTRequest::requestHeaderData()
{
    MHD_get_connection_values( connection, MHD_HEADER_KIND, RESTRequest::iterate_headers, this);	

    MHD_get_connection_values( connection, MHD_GET_ARGUMENT_KIND, RESTRequest::iterate_arguments, this);	

}

int
RESTRequest::processDataIteration( enum MHD_ValueKind kind, const char *keyValue,
                                   const char *filenameValue, const char *content_type,
                                   const char *transfer_encoding, const char *data, uint64_t off,
                                   size_t size )
{
    printf("processDataIteration -- kind: %d\n", kind);
    std::string key;
    std::string contentType;
    std::string filename;

    if( keyValue )
        printf("processDataIteration -- key: %s\n", keyValue);

    if( filenameValue )
        printf("processDataIteration -- filename: %s\n", filenameValue);

    if( content_type )
        printf("processDataIteration -- content_type: %s\n", content_type);

    if( transfer_encoding )
        printf("processDataIteration -- transfer_encoding: %s\n", transfer_encoding);

    printf("processDataIteration -- offset: %ld\n", off);
    printf("processDataIteration -- size: %ld\n", size);

    if( keyValue )
    {
        key = keyValue;
      
        if( content_type )
            contentType = content_type;

        if( filenameValue )
        {
            filename = filenameValue;

            if( off != 0 )
                inRepresentation.updateEncodedData( key, data, off, size );
            else
                inRepresentation.addEncodedFile( key, filename, contentType, data, off, size );
        }
        else
        {
            if( off != 0 )
                inRepresentation.updateEncodedData( key, data, off, size );
            else
                inRepresentation.addEncodedParameter( key, contentType, data, off, size );
        }
    }

    return MHD_YES;
}

int
RESTRequest::processHeader( enum MHD_ValueKind kind, const char *key, const char *value )
{
    printf( "processHeaderValue -- kind: %d, key: %s, value: %s\n", kind, key, value );

    inRepresentation.addHTTPHeader( key, value );

    return MHD_YES;
}

int
RESTRequest::processUrlArg( enum MHD_ValueKind kind, const char *key, const char *value )
{
    printf( "processUrlArg -- kind: %d, key: %s, value: %s\n", kind, key, value );

    inRepresentation.addQueryParameter( key, value );

    return MHD_YES;
}

int
RESTRequest::iterate_post( void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                                  const char *filename, const char *content_type,
                                  const char *transfer_encoding, const char *data, uint64_t off,
                                  size_t size )
{
    RESTRequest *request = (RESTRequest *) coninfo_cls;

    printf( "iterate_post -- \n" );

    return request->processDataIteration( kind, key, filename, content_type, transfer_encoding, data, off, size );
}


int
RESTRequest::iterate_headers( void *cls, MHD_ValueKind kind, const char *key, const char *value )
{
    RESTRequest *request = (RESTRequest *) cls;

    return request->processHeader( kind, key, value );
}

int
RESTRequest::iterate_arguments( void *cls, MHD_ValueKind kind, const char *key, const char *value )
{
    RESTRequest *request = (RESTRequest *) cls;

    return request->processUrlArg( kind, key, value );
}

void 
RESTRequest::execute()
{
    printf( "execute\n" );
    if( resourcePtr )
    {
        ((RESTResource *)resourcePtr)->executeRequest( this );
    }
}

void
RESTRequest::sendResourceCreatedResponse( std::string newResource )
{
    std::string locURL = getURL() + "/" + newResource;

    // Send the location header to tell where the new resource was created.
    outRepresentation.addHTTPHeader( "Location", locURL );

    // Send back the response
    setResponseCode( REST_HTTP_RCODE_CREATED );
    sendResponse();
}

void 
RESTRequest::sendErrorResponse( REST_HTTP_RCODE_T httpCode, unsigned long errCode, std::string errStr )
{
    std::string rspData = "<error>" + errStr + "</error>";

    std::cout << "ErrorResponse: " << rspData << std::endl;

    getOutboundRepresentation()->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 

    setResponseCode( httpCode );

    sendResponse();
}

void 
RESTRequest::sendResponse()
{
    printf( "sendResponse\n" );
    if( daemonPtr )
    {
        ((RESTDaemon *)daemonPtr)->sendResponse( this );
    }
}


