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

RESTResourceRelationship::RESTResourceRelationship()
{

}

RESTResourceRelationship::~RESTResourceRelationship()
{

}

void 
RESTResourceRelationship::setListID( std::string listID )
{
    list = listID;
}

void 
RESTResourceRelationship::setChildParamID( std::string urlID )
{
    paramID = urlID;
}

std::string 
RESTResourceRelationship::getListID()
{
    return list;
}

std::string 
RESTResourceRelationship::getChildParamID()
{
    return paramID;
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
                request->setURIParameter( elem->getMatchElement(), t );
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


RESTContentManagerResource::RESTContentManagerResource( RESTContentManager &mgr )
: contentMgr( mgr )
{


}

RESTContentManagerResource::~RESTContentManagerResource()
{

}

void 
RESTContentManagerResource::clearRelationships()
{
    relationshipStack.clear();
}

void 
RESTContentManagerResource::appendRelationship( std::string listName, std::string childParamID )
{
    RESTResourceRelationship relObj;

    relObj.setListID( listName );
    relObj.setChildParamID( childParamID );

    relationshipStack.push_back( relObj );
}

bool
RESTContentManagerResource::checkObjectRelationships( RESTRequest *request, std::string &terminalID )
{
    std::string paramID;
    std::string curRootID = "root";

    // Havent found anything yet.
    terminalID.clear();

    // Walk though any relationships and find the terminating ID
    for( std::vector<RESTResourceRelationship>::iterator it = relationshipStack.begin(); it != relationshipStack.end(); ++it )
    {
        if( request->getURIParameter( it->getChildParamID(), paramID ) )
        {
            printf("Failed to look up ruleid parameter\n");
            return true;
        }

        std::cout << "checkObjectRelationships: " << curRootID << ", " << paramID << ", " << it->getListID() << std::endl;

        if( contentMgr.hasRelationship( curRootID, paramID, it->getListID() ) == false )
        {
            printf("Failed to look up ruleid parameter\n");
            return true;
        }

        // Take the next step
        curRootID = paramID;

        printf( "checkObjectRelationships: %s\n", curRootID.c_str() );
    }

    // Report back the last match
    terminalID = curRootID;

    // Success
    return false;
}

void 
RESTContentManagerResource::restGet( RESTRequest *request )
{
    std::cout << "RESTResource::restGet" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTContentManagerResource::restPut( RESTRequest *request )
{
    std::cout << "RESTResource::restPut" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTContentManagerResource::restPost( RESTRequest *request )
{
    std::cout << "RESTResource::restPost" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

void 
RESTContentManagerResource::restDelete( RESTRequest *request )
{
    std::cout << "RESTResource::restDelete" << std::endl;
    request->setResponseCode( REST_HTTP_RCODE_NOT_IMPLEMENTED );
    request->sendResponse();
}

RESTResourceRESTContentList::RESTResourceRESTContentList( std::string pattern, RESTContentManager &mgr, unsigned int type, std::string list, std::string prefix )
: RESTContentManagerResource( mgr )
{
    setURLPattern( pattern, (REST_RMETHOD_T)( REST_RMETHOD_GET | REST_RMETHOD_POST ) );

    objType      = type;
    listElement  = list;
    objPrefix    = prefix;
}

RESTResourceRESTContentList::~RESTResourceRESTContentList()
{

}

void
RESTResourceRESTContentList::restGet( RESTRequest *request )
{
    std::string        rspData;
    RESTContentHelper *helper;
    RESTContentNode   *objNode;
    RESTContentNode *curNode;

    std::cout << "RESTResourceRESTContentList::restGet" << std::endl;

    // All of the routines below will throw a SMException if they encounter an error
    // during processing.
    try{

        // Parse the content
        helper = RESTContentHelperFactory::getResponseSimpleContentHelper( request->getInboundRepresentation() ); 

        // Get a pointer to the root node
        objNode = helper->getRootNode();

        // Create the root object
        objNode->setAsArray( listElement );

        // Resolve relationships to a terminalID and its parent
        std::string parentID;

        if( checkObjectRelationships( request, parentID ) )
        {
            throw new RCMException( 1004, "Could not resolve parent ID" );
        }

        // Enumerate the zone-group list
        std::vector< std::string > idList;

        contentMgr.getIDListForRelationship( parentID, listElement, idList );
        //contentMgr.getIDVectorByType( objType, idList );

        for( std::vector< std::string >::iterator it = idList.begin() ; it != idList.end(); ++it)
        {
            std::cout << "RESTResourceRESTContentList::restGet - " << *it << std::endl;

            curNode = new RESTContentNode();
            curNode->setAsID( *it );
            curNode->setID( *it );
            objNode->addChild( curNode );
        }

        // Make sure we have the expected object
        helper->generateContentRepresentation( request->getOutboundRepresentation() );

    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }    

    request->setResponseCode( REST_HTTP_RCODE_OK );
    request->sendResponse();
}

void
RESTResourceRESTContentList::restPost( RESTRequest *request )
{
    RESTContentHelper   *helper;
    RESTContentTemplate *templateNode;
    RESTContentNode     *objNode;
    std::string          objID;

    std::cout << "RESTResourceRESTContentList::restPost" << std::endl;

    // All of the routines below will throw a SMException if they encounter an error
    // during processing.
    try
    {
        // Generate a template for acceptable data
        templateNode = contentMgr.getContentTemplateForType( objType );

        // Allocate the appropriate type of helper to parse the content
        helper = RESTContentHelperFactory::getRequestSimpleContentHelper( request->getInboundRepresentation() );

        // Parse the content based on the template ( throws an exception for missing content )
        helper->parseWithTemplate( templateNode, request->getInboundRepresentation() ); 

        // Resolve relationships to a terminalID and its parent
        std::string parentID;

        if( checkObjectRelationships( request, parentID ) )
        {
            throw new RCMException( 1004, "Could not resolve parent ID" );
        }

        // Create the new object
        contentMgr.createObj( objType, objPrefix, objID );
        contentMgr.updateObj( objID, helper->getRootNode() );
        contentMgr.addRelationship( listElement, parentID, objID );

        // Notify that an update occurred.
        contentMgr.notifyCfgChange();
    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }

    // Build a response, including the new object id
    request->sendResourceCreatedResponse( objID );
}


RESTResourceRESTContentObject::RESTResourceRESTContentObject(  std::string pattern, RESTContentManager &mgr, unsigned int type )
: RESTContentManagerResource( mgr )
{
    setURLPattern( pattern, (REST_RMETHOD_T)( REST_RMETHOD_GET | REST_RMETHOD_PUT | REST_RMETHOD_DELETE ) );
 
    objType  = type;
}

RESTResourceRESTContentObject::~RESTResourceRESTContentObject()
{

}

void
RESTResourceRESTContentObject::restGet( RESTRequest *request )
{
    RESTContentHelper *helper;
    RESTContentNode   *outNode;
    RESTContentNode   *objNode;
    std::string        objID;

#if 0
    if( request->getURIParameter( objUrlID, objID ) )
    {
        printf("Failed to look up ruleid parameter\n");
        request->setResponseCode( REST_HTTP_RCODE_BAD_REQUEST );
        request->sendResponse();
        return;
    }
#endif

    // Resolve relationships to a terminalID and its parent
    if( checkObjectRelationships( request, objID ) )
    {
        throw new RCMException( 1004, "Could not resolve object relationships" );
    }

    printf( "URL objID: %s\n", objID.c_str() );

    // All of the routines below will throw a SMException if they encounter an error
    // during processing.
    try
    {
        // Parse the content
        helper = RESTContentHelperFactory::getResponseSimpleContentHelper( request->getInboundRepresentation() ); 

        // Get a pointer to the root node
        outNode = helper->getRootNode();

        // Lookup the rule object
        objNode = contentMgr.getObjectByID( objID );

        // Generate the list
        objNode->setContentNodeFromFields( outNode );           

        // Make sure we have the expected object
        helper->generateContentRepresentation( request->getOutboundRepresentation() );
    
    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }

    request->setResponseCode( REST_HTTP_RCODE_OK );
    request->sendResponse();
}

void 
RESTResourceRESTContentObject::restPut( RESTRequest *request )
{
    std::string          objID;
    RESTContentHelper    *helper;
    RESTContentTemplate  *templateNode;
//    RESTContentNode      *objNode;

    std::cout << "RESTResourceRESTContentObject::restPut" << std::endl;

    // All of the routines below will throw a SMException if they encounter an error
    // during processing.
    try{

#if 0
        if( request->getURIParameter( objUrlID, objID ) )
        {
            request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 0, "A valid id must be provided as part of the URL." ); 
            return;
        }
#endif

        // Resolve relationships to a terminalID and its parent
        if( checkObjectRelationships( request, objID ) )
        {
            throw new RCMException( 1004, "Could not resolve object relationships" );
        }

        printf( "RESTResourceRESTContentObject - PUT: %s\n", objID.c_str() );

        // Generate a template for acceptable data
        templateNode = contentMgr.getContentTemplateForType( objType );

        // Allocate the appropriate type of helper to parse the content
        helper = RESTContentHelperFactory::getRequestSimpleContentHelper( request->getInboundRepresentation() );

        // Parse the content based on the template ( throws an exception for missing content )
        helper->parseWithTemplate( templateNode, request->getInboundRepresentation() );

        // Create the new object
        contentMgr.updateObj( objID, helper->getRootNode() );

        // Notify that an update occurred.
        contentMgr.notifyCfgChange();
    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }

    // Success
    request->setResponseCode( REST_HTTP_RCODE_OK );
    request->sendResponse();
}

void 
RESTResourceRESTContentObject::restDelete( RESTRequest *request )
{
    std::string objID;

    try
    {
#if 0
        // extract the ruleid parameter
        if( request->getURIParameter( objUrlID, objID ) )
        {
            request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 0, "A valid id must be provided as part of the URL." );
            return;
        }
#endif
        // Resolve relationships to a terminalID and its parent
        if( checkObjectRelationships( request, objID ) )
        {
            throw new RCMException( 1004, "Could not resolve object relationships" );
        }

        printf( "RESTResourceRESTContentObject - DELETE: %s\n", objID.c_str() );

        // Attempt the delete operation.
        contentMgr.deleteObjByID( objID );

        // Notify that an update occurred.
        contentMgr.notifyCfgChange();
    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }

    // Success
    request->setResponseCode( REST_HTTP_RCODE_OK );
    request->sendResponse();
}

RESTResourceRESTStatusProvider::RESTResourceRESTStatusProvider(  std::string pattern, RESTContentManager &mgr, unsigned int dataID )
: RESTContentManagerResource( mgr )
{
    setURLPattern( pattern, (REST_RMETHOD_T)( REST_RMETHOD_GET | REST_RMETHOD_PUT | REST_RMETHOD_DELETE ) );
 
    id = dataID;
}

RESTResourceRESTStatusProvider::~RESTResourceRESTStatusProvider()
{

}

void
RESTResourceRESTStatusProvider::restGet( RESTRequest *request )
{
    RESTContentHelper *helper;
    RESTContentNode   *outNode;
    std::map< std::string, std::string > paramMap;

    // All of the routines below will throw a SMException if they encounter an error
    // during processing.
    try
    {
        // Get parameters from the request
        request->getInboundRepresentation()->getQueryParameterMap( paramMap );

        // Parse the content
        helper = RESTContentHelperFactory::getResponseSimpleContentHelper( request->getInboundRepresentation() ); 

        // Get a pointer to the root node
        outNode = helper->getRootNode();

        // Lookup the rule object
        contentMgr.populateContentNodeFromStatusProvider( id, outNode, paramMap );

        // Make sure we have the expected object
        helper->generateContentRepresentation( request->getOutboundRepresentation() );
    }
    catch( RCMException& me )
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, me.getErrorCode(), me.getErrorMsg() ); 
        return;
    }
    catch(...)
    {
        request->sendErrorResponse( REST_HTTP_RCODE_BAD_REQUEST, 10000, "Internal Controller Error" ); 
        return;
    }

    request->setResponseCode( REST_HTTP_RCODE_OK );
    request->sendResponse();
}

