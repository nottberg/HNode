#include <string>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <stdio.h>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

#include "hnode-rest.hpp"

       
RESTResourcePE::RESTResourcePE()
{
    matchType = REST_RPE_MATCH_NOTSET;
}

RESTResourcePE::~RESTResourcePE()
{

}

void
RESTResourcePE::setMatch( REST_RPE_MATCH_T mType, std::string elemStr )
{
    matchType  = mType;
    patternStr = elemStr;
}

REST_RPE_MATCH_T
RESTResourcePE::getMatchType()
{
    return matchType;
}

std::string
RESTResourcePE::getMatchElement()
{
    return patternStr;
}

bool
RESTResourcePE::isMatch( std::string testElem )
{
    if( (matchType == REST_RPE_MATCH_STRING) && (testElem == patternStr) )
    {
        // This element matches
        return true;
    }
    
    // Not a match
    return false;
}

RESTResource::RESTResource()
{
    wildcard = false;
}

RESTResource::~RESTResource()
{

}

void 
RESTResource::setURLPattern( std::string pattern, REST_RMETHOD_T methodFlags )
{
    RESTResourcePE elem;

    urlPattern = pattern;
    methods = methodFlags;

    // Start off assuming no wildcard
    wildcard = false;

    boost::char_separator<char> sep("/");
    boost::tokenizer< boost::char_separator<char> > tokens(urlPattern, sep);
    unsigned int index = 0;
    BOOST_FOREACH (const std::string& t, tokens) 
    {
        boost::regex parameterRE("\\{([A-Za-z0-9-]+)\\}");
        boost::regex wildcardRE("([\\*])");
        boost::smatch what;

        if( regex_match(t, what, parameterRE) )
        {
            std::cout << "Adding URL parameter: " << what[1] << std::endl; 
            elem.setMatch( REST_RPE_MATCH_PARAM, what[1] );
            patternElements.push_back( elem );
            index += 1;
        }
        else if( regex_match(t, what, wildcardRE) )
        {
            std::cout << "Adding Terminating Wildcard: " << what[1] << std::endl; 
            elem.setMatch( REST_RPE_MATCH_WILDCARD, what[1] );
            patternElements.push_back( elem );
            wildcard = true;
            index += 1;
         
            // Wildcard is terminal by definition, done
            return;
        }
        else
        {
            std::cout << "Adding pattern: " << t << "." << index << std::endl;
            elem.setMatch( REST_RPE_MATCH_STRING, t );
            patternElements.push_back( elem );
            index += 1;
        }
    }

}

bool 
RESTResource::linkRequest( RESTRequest *request )
{
    RESTResourcePE *elem;

    std::cout << "Process Request Start: " << request->getURL() << std::endl;

    // Check that the requested operation is supported by this resource.
    if( (request->getMethod() & methods) == 0 )
    {
        std::cout << "Process Request Result -- method not supported" << std::endl;
        return false;
    }

    // Setup for wildcard matching.
    bool wildcardMatch = false;
    request->clearWildcardElements();

    // Walk through the request URL to make sure its a match.
    boost::char_separator<char> sep("/");
    boost::tokenizer< boost::char_separator<char> > tokens(request->getURL(), sep);
    unsigned int index = 0;
    BOOST_FOREACH (const std::string& t, tokens) 
    {
        std::cout << "Process Request:" << t << "." << index << std::endl;

        // If a wildcard has already occurred
        // then collect the rest of the URL
        // for later use.
        if( wildcardMatch )
        {
            request->appendWildcardElement( t );
            continue;
        }

        // If the request has more elements than the 
        // pattern then this is not a match
        if( index >= patternElements.size() )
        {
            std::cout << "Process Request Result -- request > pattern" << std::endl;
            return false;
        }

        // Get the current pattern element
        elem = &patternElements[ index ];

        // Determine what to do with this
        // element
        switch( elem->getMatchType() )
        {
            case REST_RPE_MATCH_STRING:
                if( elem->isMatch( t ) == false )
                {
                    std::cout << "Process Request Result -- mismatched element" << std::endl;
                    return false;
                }
            break;

            case REST_RPE_MATCH_PARAM:
                std::cout << "Process Request Result -- param match" << std::endl;
                request->setParameter( elem->getMatchElement(), t );
            break;

            case REST_RPE_MATCH_WILDCARD:
                std::cout << "Process Request Result -- wildcard match" << std::endl;
                // This is a match so set all of the remaining elements
                // as part of the extended URL
                wildcardMatch = true;
                request->appendWildcardElement( t );
            break;

        }

        // Examine the next element
        index += 1;
    }

    // Consider the wildcard match as a match
    if( wildcardMatch )
    {
        // Setup the link for future request handling.
        request->setLink( this );

        // Request handled.
        return true;
    }

    // If everything matched but the request
    // and pattern didn't have the same number
    // of elements, then it is not a match.
    if( index != (patternElements.size() - (wildcard ? 1 : 0)) )
    {
        std::cout << "Process Request Result -- request < pattern" << std::endl;
        return false;
    }

    // Setup the link for future request handling.
    request->setLink( this );

    // Request handled.
    return true;
}

unsigned int
RESTResource::getMatchCount()
{
    return patternElements.size();
}

bool
RESTResource::hasWildcardMatch()
{
    return wildcard;
}

bool 
RESTResource::executeRequest( RESTRequest *request )
{
    // Match process was successful,
    // execute the request.
    switch( request->getMethod() )
    {
        case REST_RMETHOD_GET:
            std::cout << "Process Request: Get" << std::endl;
            restGet( request );
        break;

        case REST_RMETHOD_PUT:
            std::cout << "Process Request: Put" << std::endl;
            restPut( request );
        break;

        case REST_RMETHOD_POST:
            std::cout << "Process Request: Post" << std::endl;
            restPost( request );
        break;

        case REST_RMETHOD_DELETE:
            std::cout << "Process Request: Delete" << std::endl;
            restDelete( request );
        break;
    }

    // Request handled.
    return true;
}

void 
RESTResource::restGet( RESTRequest *request )
{
    std::cout << "RESTResource::restGet" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTResource::restPut( RESTRequest *request )
{
    std::cout << "RESTResource::restPut" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTResource::restPost( RESTRequest *request )
{
    std::cout << "RESTResource::restPost" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTResource::restDelete( RESTRequest *request )
{
    std::cout << "RESTResource::restDelete" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

