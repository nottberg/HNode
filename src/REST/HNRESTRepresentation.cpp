#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>

#include "hnode-rest.hpp"

RESTRepDataItem::RESTRepDataItem()
{
    type = REST_RDI_TYPE_NOTSET;

    bufferLength = 0;
    dataLength   = 0;
    bufferPtr    = NULL;
}

RESTRepDataItem::~RESTRepDataItem()
{

}

void 
RESTRepDataItem::setType( REST_RDI_TYPE_T typeValue )
{
    type = typeValue;
}

REST_RDI_TYPE_T RESTRepDataItem::getType()
{
    return type;
}

void 
RESTRepDataItem::setKey( std::string keyValue )
{
    key = keyValue;
}

std::string 
RESTRepDataItem::getKey()
{
    return key;
}

void 
RESTRepDataItem::setContentType( std::string ctValue )
{
    contentType = ctValue;
}

std::string 
RESTRepDataItem::getContentType()
{
    return contentType;
}

void 
RESTRepDataItem::setFilename( std::string fname )
{
    filename = fname;
}

std::string 
RESTRepDataItem::getFilename()
{
    return filename;
}

void 
RESTRepDataItem::generateTmpFilePath()
{
    char templateStr[] = "/tmp/hrestXXXXXX";
    int fd;

    // Create a temporary file with a unique prefix
    fd = mkstemp( templateStr );

    if( fd != -1 )
    {
        // Copy it into the member string
        tmpfilepath = templateStr;

        // Close the file for now
        close( fd );
    }
}

std::string 
RESTRepDataItem::getTempFilePath()
{
    return tmpfilepath;
}

unsigned long  
RESTRepDataItem::getLength()
{
    return dataLength;
}

unsigned char *
RESTRepDataItem::getBuffer()
{
    return bufferPtr;
}

void 
RESTRepDataItem::resetData()
{
    dataLength = 0;
}

size_t 
RESTRepDataItem::appendDataToBuffer( const char *buffer, size_t length )
{
    unsigned char *tmpBuf;
    unsigned long  neededBytes;

    //printf("RESTRepDataItem::appendData -- size: %ld\n", length);
    
    // Calculate the number of bytes needed
    neededBytes = dataLength + length;

    // Check if there is already enough space in the buffer
    if( neededBytes > bufferLength )
    {
        // Buffer needs to be increased in size.  Calculate the size of
        // new buffer.
        neededBytes = ( neededBytes & ~(1024) ) + 1024;
 
        tmpBuf = (unsigned char *) malloc( neededBytes );
        bufferLength = neededBytes;

        if( dataLength )
        {
            memcpy( tmpBuf, bufferPtr, dataLength );
            free( bufferPtr );
            bufferPtr = NULL;
        }

        bufferPtr = tmpBuf;
    }

    memcpy( &bufferPtr[ dataLength ], buffer, length );
    dataLength += length;

    return 0;
}

size_t 
RESTRepDataItem::appendDataToFile( const char *buffer, size_t length )
{
     FILE   *fp;
     size_t  written;

     fp = fopen( tmpfilepath.c_str(), "a" );

     written = fwrite( buffer, 1, length, fp );

     fclose( fp );
}

size_t 
RESTRepDataItem::addData( unsigned long offset, const char *buffer, size_t length )
{
    // Check if the offset is going to cause problems

    // Check if this is a file or in memory buffer.
    if( type == REST_RDI_TYPE_ENCODED_FILE )
    {
        // Add the contents to the temporary file
        return appendDataToFile( buffer, length );
    }

    // Add content to in memory buffer.
    return appendDataToBuffer( buffer, length );
}

std::string
RESTRepDataItem::getDataAsStr()
{
    std::string resultStr( (const char *)bufferPtr, dataLength );
    return resultStr;
}

RESTRepresentation::RESTRepresentation()
{
    simpleContent = NULL;
}

RESTRepresentation::~RESTRepresentation()
{
    clearURIParameters();
    clearQueryParameters();
    clearHTTPHeaders();
    clearEncodedParameters();
    clearCookies();
    clearSimpleContent();
}

bool
RESTRepresentation::setFromRepresentation( RESTRepresentation *srcRep )
{
    // Prepare for copy by clearing any residual data
    clearURIParameters();
    clearQueryParameters();
    clearHTTPHeaders();
    clearEncodedParameters();
    clearCookies();
    clearSimpleContent();

    // Now check and copy each possible component
    if( srcRep->hasURIParameters() == true )
    {
        std::map< std::string, std::string > paramMap;

        srcRep->getURIParameterMap( paramMap );

        for( std::map< std::string, std::string >::iterator pit = paramMap.begin(); pit != paramMap.end(); pit++ )
        {
            addURIParameter( pit->first, pit->second );
        }
    }

    // REST_RDI_TYPE_HTTP_HEADER
    if( srcRep->hasHTTPHeaders() == true )
    {
        std::map< std::string, std::string > paramMap;

        srcRep->getHTTPHeaderMap( paramMap );

        for( std::map< std::string, std::string >::iterator pit = paramMap.begin(); pit != paramMap.end(); pit++ )
        {
            addHTTPHeader( pit->first, pit->second );
        }
    }

    // REST_RDI_TYPE_QUERY_PARAMETER
    if( srcRep->hasQueryParameters() == true )
    {
        std::map< std::string, std::string > paramMap;

        srcRep->getQueryParameterMap( paramMap );

        for( std::map< std::string, std::string >::iterator pit = paramMap.begin(); pit != paramMap.end(); pit++ )
        {
            addQueryParameter( pit->first, pit->second );
        }
    }

    // REST_RDI_TYPE_ENCODED_PARAMETER
    // REST_RDI_TYPE_ENCODED_FILE
    if( srcRep->hasEncodedParameters() == true )
    {
        std::cerr << "ERROR: NOT IMPLEMENTED" << std::endl;
    }

    // REST_RDI_TYPE_COOKIE
    if( srcRep->hasCookies() == true )
    {
        std::cerr << "ERROR: NOT IMPLEMENTED" << std::endl;
    }

    // REST_RDI_TYPE_SIMPLE_CONTENT,
    if( srcRep->hasSimpleContent() == true )
    {
        std::string    contentType;
        unsigned char *contentPtr;
        unsigned long  contentLength;

        contentPtr = srcRep->getSimpleContentPtr( contentType, contentLength );

        setSimpleContent( contentType, contentPtr, contentLength );
    }

    return false;
}

bool 
RESTRepresentation::hasURIParameters()
{
    if( uriParam.empty() == true )
        return false;

    return true;
}

void 
RESTRepresentation::clearURIParameters()
{
    RESTRepDataItem *diPtr;

    for( std::map< std::string, RESTRepDataItem * >::iterator it = uriParam.begin(); it != uriParam.end(); ++it )
    {
        delete it->second;
    }

    uriParam.clear();
}

void 
RESTRepresentation::addURIParameter( std::string name, std::string value )
{
    RESTRepDataItem *diPtr;

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_URI_PARAMETER );
    diPtr->setKey( name );

    diPtr->addData( 0, value.c_str(), value.size() );

    std::pair<std::string, RESTRepDataItem *> nvpair( name, diPtr );
    uriParam.insert( nvpair );
}

bool 
RESTRepresentation::getURIParameter( std::string name, std::string &value )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    value.clear();

    it = uriParam.find( name );

    if( it == uriParam.end() )
    {
        return true;
    }   

    value = it->second->getDataAsStr();
    
    return false;
}

void 
RESTRepresentation::getURIParameterMap( std::map< std::string, std::string > &paramMap )
{
    for( std::map<std::string, RESTRepDataItem *>::iterator it = uriParam.begin(); it != uriParam.end(); it++ )
    {
        paramMap.insert( std::pair< std::string, std::string> ( it->first, it->second->getDataAsStr() ) );
    }
}

bool 
RESTRepresentation::hasHTTPHeaders()
{
    if( httpHeader.empty() == true )
        return false;

    return true;
}

void 
RESTRepresentation::clearHTTPHeaders()
{
    RESTRepDataItem *diPtr;

    for( std::map< std::string, RESTRepDataItem * >::iterator it = httpHeader.begin(); it != httpHeader.end(); ++it )
    {
        delete it->second;
    }

    httpHeader.clear();
}

void 
RESTRepresentation::addHTTPHeader( std::string name, std::string value )
{
    RESTRepDataItem *diPtr;

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_HTTP_HEADER );
    diPtr->setKey( name );

    diPtr->addData( 0, value.c_str(), value.size() );

    std::pair<std::string, RESTRepDataItem *> nvpair( name, diPtr );
    httpHeader.insert( nvpair );

}

bool 
RESTRepresentation::getHTTPHeader( std::string name, std::string &value )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    value.clear();

    it = httpHeader.find( name );

    if( it == httpHeader.end() )
    {
        return true;
    }   

    value = it->second->getDataAsStr();
    
    return false;
}

void 
RESTRepresentation::getHTTPHeaderMap( std::map< std::string, std::string > &headerMap )
{
    for( std::map<std::string, RESTRepDataItem *>::iterator it = httpHeader.begin(); it != httpHeader.end(); it++ )
    {
        headerMap.insert( std::pair< std::string, std::string> ( it->first, it->second->getDataAsStr() ) );
    }
}

#if 0
bool 
RESTRepresentation::getHTTPHeaderNameList( std::vector< std::string > &nameList )
{
    for( std::map<std::string, RESTRepDataItem *>::iterator it = httpHeader.begin(); it != httpHeader.end(); ++it )
    {
        nameList.push_back( it->first );
    }

    return false;
}
#endif

bool 
RESTRepresentation::hasQueryParameters()
{
    if( queryParam.empty() == true )
        return false;

    return true;
}

void 
RESTRepresentation::clearQueryParameters()
{
    RESTRepDataItem *diPtr;

    for( std::map< std::string, RESTRepDataItem * >::iterator it = queryParam.begin(); it != queryParam.end(); ++it )
    {
        delete it->second;
    }

    queryParam.clear();
}

void 
RESTRepresentation::addQueryParameter( std::string name, std::string value )
{
    RESTRepDataItem *diPtr;

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_QUERY_PARAMETER );
    diPtr->setKey( name );

    diPtr->addData( 0, value.c_str(), value.size() );

    std::pair<std::string, RESTRepDataItem *> nvpair( name, diPtr );
    queryParam.insert( nvpair );
}

bool 
RESTRepresentation::getQueryParameter( std::string name, std::string &value )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    value.clear();

    it = queryParam.find( name );

    if( it == queryParam.end() )
    {
        return true;
    }   

    value = it->second->getDataAsStr();
    
    return false;
}

void 
RESTRepresentation::getQueryParameterMap( std::map< std::string, std::string > &paramMap )
{
    for( std::map<std::string, RESTRepDataItem *>::iterator it = queryParam.begin(); it != queryParam.end(); it++ )
    {
        paramMap.insert( std::pair< std::string, std::string > ( it->first, it->second->getDataAsStr() ) );
    }
}

bool 
RESTRepresentation::hasEncodedParameters()
{
    if( encodedParam.empty() == true )
        return false;

    return true;
}

void 
RESTRepresentation::clearEncodedParameters()
{
    RESTRepDataItem *diPtr;

    for( std::map< std::string, RESTRepDataItem * >::iterator it = encodedParam.begin(); it != encodedParam.end(); ++it )
    {
        delete it->second;
    }

    encodedParam.clear();
}

void 
RESTRepresentation::addEncodedParameter( std::string key, std::string contentType, const char *data, unsigned long offset, unsigned long size )
{
    printf("addEncodedParameter -- key: %s, off: %ld, size: %ld, contentType: %s\n", key.c_str(), offset, size, contentType.c_str());

    RESTRepDataItem *diPtr;

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_ENCODED_PARAMETER );
    diPtr->setContentType( contentType );

    diPtr->addData( offset, (const char*)data, size );

    std::pair<std::string, RESTRepDataItem *> nvpair( key, diPtr );
    encodedParam.insert( nvpair );
}

void 
RESTRepresentation::addEncodedFile( std::string key, std::string filename, std::string contentType, const char *data, unsigned long offset, unsigned long size )
{
    printf("addEncodedFile -- key: %s, off: %ld, size: %ld, filename: %s, contentType: %s\n", key.c_str(), offset, size, filename.c_str(), contentType.c_str());

    RESTRepDataItem *diPtr;

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_ENCODED_FILE );
    diPtr->setContentType( contentType );

    diPtr->setFilename( filename );
    diPtr->generateTmpFilePath();

    diPtr->addData( offset, (const char*)data, size );

    std::pair<std::string, RESTRepDataItem *> nvpair( key, diPtr );
    encodedParam.insert( nvpair );
}

void 
RESTRepresentation::updateEncodedData( std::string key, const char* data, unsigned long offset, unsigned long size )
{
    printf("updateEncodedData -- key: %s, off: %ld, size: %ld\n", key.c_str(), offset, size);

    std::map<std::string, RESTRepDataItem *>::iterator it;

    it = encodedParam.find( key );

    if( it == encodedParam.end() )
    {
        return;
    }   

    RESTRepDataItem *diPtr = it->second;

    diPtr->addData( offset, (const char*)data, size );
}
 
bool 
RESTRepresentation::getEncodedParamInfo( std::string key, bool &isFile, std::string &contentType, unsigned long &contentLength )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    it = encodedParam.find( key );

    if( it == encodedParam.end() )
    {
        return true;
    }   

    RESTRepDataItem *diPtr = it->second;

    // Check if this is a file.
    isFile = (diPtr->getType() == REST_RDI_TYPE_ENCODED_FILE) ? true : false;

    // Copy over other data
    contentType   = diPtr->getContentType();
    contentLength = diPtr->getLength(); 
    
    // Success
    return false;
}

bool 
RESTRepresentation::getEncodedFileInfo( std::string key, std::string &filename, std::string &localpath )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    it = encodedParam.find( key );

    if( it == encodedParam.end() )
    {
        return true;
    }   

    RESTRepDataItem *diPtr = it->second;

    if( diPtr->getType() != REST_RDI_TYPE_ENCODED_FILE )
    {
        return true;
    }

    // Copy over file info
    filename  = diPtr->getFilename();
    localpath = diPtr->getTempFilePath(); 

    return false;
}

bool 
RESTRepresentation::getEncodedDataAsStr( std::string key, std::string &value )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;

    it = encodedParam.find( key );

    if( it == encodedParam.end() )
    {
        return true;
    }   

    RESTRepDataItem *diPtr = it->second;

    value = diPtr->getDataAsStr();

    return false;
}

unsigned char *
RESTRepresentation::getEncodedDataAsPtr( std::string key, unsigned long offset, unsigned long windowLength )
{
    std::map<std::string, RESTRepDataItem *>::iterator it;
    unsigned char *dataPtr;

    it = encodedParam.find( key );

    if( it == encodedParam.end() )
    {
        return NULL;
    }   

    RESTRepDataItem *diPtr = it->second;

    // Bounds Check
    if( offset > diPtr->getLength() )
        return NULL;

    // Bounds Check
    if( (offset + windowLength) > diPtr->getLength() )
        return NULL;

    // Get the raw pointer
    dataPtr = diPtr->getBuffer();

    // Move by offset
    dataPtr += offset;

    // Return the pointer
    return dataPtr;
}

bool 
RESTRepresentation::hasCookies()
{
    if( cookieMap.empty() == true )
        return false;

    return true;
}

void 
RESTRepresentation::clearCookies()
{
    RESTRepDataItem *diPtr;

    for( std::map< std::string, RESTRepDataItem * >::iterator it = cookieMap.begin(); it != cookieMap.end(); ++it )
    {
        delete it->second;
    }

    cookieMap.clear();
}

void 
RESTRepresentation::addCookie( std::string name, std::string cookie )
{

}

bool 
RESTRepresentation::getCookie( std::string name, std::string &cookie )
{

}

bool 
RESTRepresentation::hasSimpleContent()
{
    if( simpleContent != NULL )
        return true;

    return false;
}

void 
RESTRepresentation::clearSimpleContent()
{
    if( simpleContent != NULL )
        delete simpleContent;

    simpleContent = NULL;
}

void 
RESTRepresentation::setSimpleContent( std::string contentType )
{
    RESTRepDataItem *diPtr;

    if( simpleContent == NULL )
    {
        diPtr = new RESTRepDataItem();
        if( diPtr == NULL )
        {
            return;
        }

        diPtr->setType( REST_RDI_TYPE_SIMPLE_CONTENT );
        diPtr->setContentType( contentType );

        simpleContent = diPtr;    
    }

    simpleContent->setContentType( contentType );  

    addHTTPHeader( "Content-Type", contentType );
}

void 
RESTRepresentation::setSimpleContent( unsigned char *contentPtr, unsigned long contentLength )
{
    std::string contentType;

    getHTTPHeader( "Content-Type", contentType );

    setSimpleContent( contentType, contentPtr, contentLength ); 
}

void 
RESTRepresentation::setSimpleContent( std::string contentType, unsigned char *contentPtr, unsigned long contentLength )
{
    RESTRepDataItem *diPtr;

    printf("setSimpleContent: %ld\n", contentLength);

    diPtr = new RESTRepDataItem();
    if( diPtr == NULL )
    {
        return;
    }

    diPtr->setType( REST_RDI_TYPE_SIMPLE_CONTENT );
    diPtr->setContentType( contentType );
    diPtr->addData( 0, (const char*)contentPtr, contentLength );

    simpleContent = diPtr;

    addHTTPHeader( "Content-Type", contentType );
}

void 
RESTRepresentation::appendSimpleContent( unsigned char *contentPtr, unsigned long contentLength )
{
    if( simpleContent == NULL )
        return;

    //printf( "appendSimpleContent -- new Length: %ld\n", contentLength );
    //printf( "appendSimpleContent -- cur Length: %ld\n", simpleContent->getLength() );

    simpleContent->addData( simpleContent->getLength(), (const char *)contentPtr, contentLength ); 
}

unsigned long 
RESTRepresentation::getSimpleContentLength()
{
    if( simpleContent == NULL )
        return 0;

    return simpleContent->getLength();
}

unsigned char* 
RESTRepresentation::getSimpleContentPtr( std::string &contentType, unsigned long &contentLength )
{
    contentType.clear();
    contentLength = 0;

    if( simpleContent == NULL )
        return NULL;

    contentType   = simpleContent->getContentType();
    contentLength = simpleContent->getLength();
    return simpleContent->getBuffer();
}






