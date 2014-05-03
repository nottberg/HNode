#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hnode-rest.hpp"

RESTRepresentation::RESTRepresentation()
{
    rspDataBufferLength = 0;
    rspDataLength       = 0;
    rspData             = NULL;

    contentLength       = 0;
}

RESTRepresentation::~RESTRepresentation()
{
    if( rspDataBufferLength )
        free( rspData );
}

void 
RESTRepresentation::setContentType( std::string cType )
{
    contentType = cType;
}

std::string 
RESTRepresentation::getContentType()
{
    return contentType;
}

void 
RESTRepresentation::setContentLength( std::string contentLength )
{
    contentLength = strtol( contentLength.c_str(), NULL, 0 );
}

unsigned long 
RESTRepresentation::getContentLength()
{
    return contentLength;
}

void 
RESTRepresentation::addParameter( std::string name, std::string value )
{
    std::pair<std::string, std::string> nvpair(name,value);
    printf( "RESTRepresentation::addParameter -- name: %s, value: %s\n", name.c_str(), value.c_str() );

    paramMap.insert( nvpair );
}

unsigned long
RESTRepresentation::getLength()
{
    return rspDataLength;
}

unsigned char *
RESTRepresentation::getBuffer()
{
    return rspData;
}

void 
RESTRepresentation::resetData()
{
    rspDataLength = 0;
}

size_t 
RESTRepresentation::appendData( const char *buffer, size_t length )
{
    unsigned char *tmpBuf;
    unsigned long  neededBytes;

    printf("RESTRepresentation::appendData -- size: %ld\n", length);
    
    // Calculate the number of bytes needed
    neededBytes = rspDataLength + length;

    // Check if there is already enough space in the buffer
    if( neededBytes > rspDataBufferLength )
    {
        // Buffer needs to be increased in size.  Calculate the size of
        // new buffer.
        neededBytes = ( neededBytes & ~(1024) ) + 1024;
 
        tmpBuf = (unsigned char *) malloc( neededBytes );
        rspDataBufferLength = neededBytes;

        if( rspDataLength )
        {
            memcpy( tmpBuf, rspData, rspDataLength );
            free( rspData );
            rspData = NULL;
        }

        rspData = tmpBuf;
    }

    memcpy( &tmpBuf[rspDataLength], buffer, length );
    rspDataLength += length;

    return 0;
}





