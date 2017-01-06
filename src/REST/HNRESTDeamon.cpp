#include <iostream>
#include <algorithm>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#include "hnode-rest.hpp"

#define PORT 8888
#define POSTBUFFERSIZE 512
#define MAXNAMESIZE 20
#define MAXANSWERSIZE 512
#define GET 0
#define POST 1

#if 0
struct connection_info_struct
{
    int connectiontype;
    char *answerstring;
    struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "<html><body>\
What’s your name, Sir?<br>\
<form action=\"/namepost\" method=\"post\">\
<input name=\"name\" type=\"text\"\
<input type=\"submit\" value=\" Send \"></form>\
</body></html>";

const char *greetingpage =
"<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
"<html><body>This doesn’t seem to be right.</body></html>";
#endif


RESTDaemon::RESTDaemon()
{
    listenPort = REST_DAEMON_DEFAULT_PORT;
}

RESTDaemon::~RESTDaemon()
{

}

void 
RESTDaemon::setListeningPort( unsigned short port )
{
    listenPort = port;
}

int 
RESTDaemon::start()
{
    daemon = MHD_start_daemon( MHD_USE_SELECT_INTERNALLY, listenPort, NULL, NULL, &connection_request, this, MHD_OPTION_NOTIFY_COMPLETED, request_completed, this, MHD_OPTION_END );

    if( NULL == daemon )
        return 1;

    return 0;
}

void 
RESTDaemon::stop()
{
    MHD_stop_daemon( daemon );
}

int
RESTDaemon::sendResponse( RESTRequest *request )
{
    int ret;
    struct MHD_Response *response;
    RESTRepresentation *payload;

    std::string rspContentType;
    unsigned long rspLength;
    unsigned char *rspBufPtr;

    payload = request->getOutboundRepresentation();
    rspBufPtr = payload->getSimpleContentPtr( rspContentType, rspLength );

    // Build and send the response
    ret = MHD_NO;
    
    printf( "Response Content(%ld): %*.*s\n", rspLength, (int)rspLength, (int)rspLength, rspBufPtr );

    response = MHD_create_response_from_buffer( rspLength, rspBufPtr, MHD_RESPMEM_MUST_COPY );

    if( response )
    {
        std::map< std::string, std::string > headerMap;

        // Add any return headers that were specified in the outResponse.
        payload->getHTTPHeaderMap( headerMap );

        for( std::map< std::string, std::string >::iterator it = headerMap.begin(); it != headerMap.end(); ++it )
        {
            MHD_add_response_header( response, it->first.c_str(), it->second.c_str() );
        }

        ret = MHD_queue_response( request->getConnection(), request->getResponseCode(), response );
        MHD_destroy_response( response );
    }

    // Tell the daemon what to do.
    return ret; 
}

int
RESTDaemon::newRequest( RESTRequest *request, const char *upload_data, size_t *upload_data_size )
{
    int                  ret;
    struct MHD_Response *response;

    printf("newRequest -- size: %ld\n", *upload_data_size);

    // Check if this is a new request.
    printf("newRequest2\n");

    // Create a new request
    //payload = new RESTRepresentation();
    //request->setInboundRepresentation( payload );

    // Handle any inbound 
    if( *upload_data_size )
    {
        *upload_data_size = request->processUploadData( upload_data, *upload_data_size );
    }

    printf("newRequest3\n");

    request->requestHeaderData();

    // Scan through the registered resources and see if 
    // a match exists.
    bool request_handled = false;
    for( std::list<RESTResource *>::iterator it=resourceList.begin(); it != resourceList.end(); ++it )
    {
        printf("newRequest4\n");

        // Try to process the request, exit if the request gets handled
        if( (*it)->linkRequest( request ) == true )
        {
            request_handled = true;
            break;
        }
    }

    printf("newRequest5\n");

    // Build and send the response
    ret = MHD_YES;
/*
    if( request_handled == true )
    {
        printf("Response Content(%ld): %*.*s\n", payload->getLength(), (int)payload->getLength(), (int)payload->getLength(), payload->getBuffer());

        response = MHD_create_response_from_buffer( payload->getLength(), payload->getBuffer(), MHD_RESPMEM_MUST_COPY );

        if( response )
        {
            ret = MHD_queue_response( request->getConnection(), request->getResponseCode(), response );
            MHD_destroy_response( response );
        }
    }
    else
*/
    if( request_handled == false )
    {
        response = MHD_create_response_from_buffer( 0, 0, MHD_RESPMEM_MUST_COPY );

        printf("WARN: No handler found for url!");

        if( response )
        {
            ret = MHD_queue_response( request->getConnection(), MHD_HTTP_BAD_REQUEST, response );
            MHD_destroy_response( response );
        }

        // Cleanup
        //delete request;

        // Request is being rejected
        return MHD_NO;
    }


    // Continue the request processing
    return MHD_YES;

        // No match for the resource so return a bad request indication

#if 0
        if( 0 == strcmp( method, "POST" ) )
        {
            con_info->postprocessor = MHD_create_post_processor( connection, POSTBUFFERSIZE, iterate_post, (void *) con_info );
            if( NULL == con_info->postprocessor )
            {
                free( con_info );
                return MHD_NO;
            }
            con_info->connectiontype = POST;
        }
        else
            con_info->connectiontype = GET;
            *con_cls = (void *) con_info;
            return MHD_YES;
    }
#endif
   // } 

    // This is a continuing request for a POST/PUT upload.
    //request = (RESTRequest *) *con_cls;

    // Process the continueing operation
    

#if 0
    if( 0 == strcmp( method, "GET" ) )
    {
        return send_page (connection, askpage);
    }

    if( 0 == strcmp( method, "POST" ) )
    {
        struct connection_info_struct *con_info = *con_cls;
        if( *upload_data_size != 0 )
        {
            MHD_post_process( con_info->postprocessor, upload_data, *upload_data_size );
            *upload_data_size = 0;
            return MHD_YES;
        }
        else if( NULL != con_info->answerstring )
            return send_page( connection, con_info->answerstring );
    }
    return send_page( connection, errorpage );
#endif
}

int
RESTDaemon::continueRequest( RESTRequest *request, const char *upload_data, size_t *upload_data_size )
{
    RESTRepresentation  *payload;
    int                  ret;
    struct MHD_Response *response;

    printf("continueRequest -- size: %ld\n", *upload_data_size);

    // Handle any inbound 
    *upload_data_size = request->processUploadData( upload_data, *upload_data_size );

    // Continue
    return MHD_YES;
}

int 
RESTDaemon::requestReady( RESTRequest *request )
{
    printf("requestReady\n");

    request->execute();

    return MHD_YES;
}

void
RESTDaemon::requestComplete( struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe )
{
    printf("requestComplete\n");

    RESTRequest *request = (RESTRequest *) *con_cls;
    delete request;
}

bool resourceSortFunc( RESTResource *a, RESTResource *b ) 
{ 
    if( a->getMatchCount() > b->getMatchCount() )
    {
        // A before B
        return true;
    }
    else if( a->getMatchCount() == b->getMatchCount() )
    {
        // Wildcard matches should be sorted to the end
        if( a->hasWildcardMatch() && !b->hasWildcardMatch() )
        {
            // B before A
            return false;
        }
        else if( !a->hasWildcardMatch() && b->hasWildcardMatch() )
        {
            // A before B
            return true;
        }
    }

    // B before A
    return false; 
}

bool 
RESTDaemon::registerResource( RESTResource *resource )
{
    // Add it to the end
    resourceList.push_back( resource );

    // Sort the list so longest prefix matches will occur first.
    resourceList.sort( resourceSortFunc );
}

// Static callback to go from flat C to the daemon object
int
RESTDaemon::connection_request( void *cls, struct MHD_Connection *connection,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls )
{
    RESTDaemon *daemon = (RESTDaemon *)cls;
    RESTRequest *request = NULL;

    printf("connection_request -- method: %s, size: %ld, url: %s\n", method, *upload_data_size, url);

    // Check if this is a new request or a continuing one
    if( *con_cls == NULL ) 
    {
        request = new RESTRequest( connection, daemon );
        *con_cls = request;

        request->setURL( url );
        request->decodeRequestMethod( method );
        request->setVersion( version );
       
        return daemon->newRequest( request, upload_data, upload_data_size );
    }
    else if( *upload_data_size )
    {
        RESTRequest *request = (RESTRequest *) *con_cls;
        return daemon->continueRequest( request, upload_data, upload_data_size );
    }
    else
    {
        RESTRequest *request = (RESTRequest *) *con_cls;
        return daemon->requestReady( request );
    }
}

void
RESTDaemon::request_completed( void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe )
{
    RESTDaemon *daemon = (RESTDaemon *)cls;

    printf("request_completed\n");

    daemon->requestComplete( connection, con_cls, toe );
}



