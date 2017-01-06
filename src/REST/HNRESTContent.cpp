#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <list>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "hnode-rest.hpp"

RESTContentFieldDef::RESTContentFieldDef()
{

}

RESTContentFieldDef::~RESTContentFieldDef()
{

}

void
RESTContentFieldDef::setName( std::string nameStr )
{
    name = nameStr;
}

std::string
RESTContentFieldDef::getName()
{
    return name;
}


RESTContentField::RESTContentField()
{

}

RESTContentField::~RESTContentField()
{

}

void
RESTContentField::setName( std::string nameStr )
{
    name = nameStr;
}

std::string
RESTContentField::getName()
{
    return name;
}

void
RESTContentField::setValue( std::string valueStr )
{
    value = valueStr;
}

std::string
RESTContentField::getValue()
{
    return value;
}

RESTContentReferenceDef::RESTContentReferenceDef()
{

}

RESTContentReferenceDef::~RESTContentReferenceDef()
{

}

void
RESTContentReferenceDef::setName( std::string nameStr )
{
    name = nameStr;
}

std::string
RESTContentReferenceDef::getName()
{
    return name;
}

#if 0
RESTContentReference::RESTContentReference()
{

}

RESTContentReference::~RESTContentReference()
{

}

void
RESTContentReference::setName( std::string nameStr )
{
    name = nameStr;
}

std::string
RESTContentReference::getName()
{
    return name;
}

void
RESTContentReference::setValue( std::string valueStr )
{
    value = valueStr;
}

std::string
RESTContentReference::getValue()
{
    return value;
}
#endif

RESTContentBase::RESTContentBase()
{
    type   = RCNT_NOTSET;
    parent = NULL;
}

RESTContentBase::~RESTContentBase()
{

}

void 
RESTContentBase::setAsArray( std::string nameStr )
{
    type = RCNT_ARRAY;
    setName( nameStr );
}

void 
RESTContentBase::setAsID( std::string idValue )
{
    type = RCNT_ID;
}

void 
RESTContentBase::setAsObject( std::string nameStr )
{
    type = RCNT_OBJECT;
    setName( nameStr );
}

RCN_TYPE 
RESTContentBase::getType()
{
    return type;
}

bool 
RESTContentBase::isArray()
{
    if( type == RCNT_ARRAY )
        return true;

    return false;
}

bool 
RESTContentBase::isID()
{
    if( type == RCNT_ID )
        return true;

    return false;
}

bool 
RESTContentBase::isObj()
{
    if( type == RCNT_OBJECT )
        return true;

    return false;
}

void 
RESTContentBase::setName( std::string nameValue )
{
    name = nameValue;
}

bool 
RESTContentBase::hasName()
{
    if( name.empty() )
        return false;

    return true;
}

std::string 
RESTContentBase::getName()
{
    return name;
}

void 
RESTContentBase::setParent( RESTContentBase *parentPtr )
{
    parent = parentPtr;
}

RESTContentBase *
RESTContentBase::getParent()
{
    return parent;
}

void 
RESTContentBase::addChild( RESTContentBase *childPtr )
{
    childPtr->setParent( this );
    children.push_back( childPtr );
}

unsigned long 
RESTContentBase::getChildCount()
{
    return children.size();
}

RESTContentBase *
RESTContentBase::getChildByIndex( unsigned long index )
{
    if( index >= children.size() )
        return NULL;

    return children[ index ];
}

RESTContentBase *
RESTContentBase::getChildByName( std::string name )
{
    for( std::vector< RESTContentBase * >::const_iterator it = children.begin(); it != children.end(); ++it )
    {
        if( (*it)->getName() == name )
        {
            return *it;
        }
    }

    return NULL;
}

RESTContentTemplate::RESTContentTemplate()
{
    staticFlag = false;
}

RESTContentTemplate::~RESTContentTemplate()
{

}

void 
RESTContentTemplate::setFactoryType( std::string type )
{
    factoryType = type;
}

std::string 
RESTContentTemplate::getFactoryType()
{
    return factoryType;
}

void 
RESTContentTemplate::setTemplateID( unsigned int id )
{
    templateID = id;
}

unsigned int 
RESTContentTemplate::getTemplateID()
{
    return templateID;
}

bool 
RESTContentTemplate::isStatic()
{
    return staticFlag;
}

void 
RESTContentTemplate::setStatic()
{
    staticFlag = true;
}

void 
RESTContentTemplate::clearStatic()
{
    staticFlag = false;
}

void 
RESTContentTemplate::defineField( std::string name, bool required )
{
    RESTContentFieldDef field;

    if( fieldDefs.count( name ) > 0 )
    {
        fieldDefs[name] = field;
        return;
    }

    RESTContentFieldDef newField;
    newField.setName( name );   
    fieldDefs.insert( std::pair< std::string, RESTContentFieldDef >( name, newField ) );
}

void 
RESTContentTemplate::defineRef( std::string name, bool required )
{
    RESTContentReferenceDef ref;

    if( referenceDefs.count( name ) > 0 )
    {
        referenceDefs[name] = ref;
        return;
    }

    RESTContentReferenceDef newRef;
    newRef.setName( name );   
    referenceDefs.insert( std::pair< std::string, RESTContentReferenceDef >( name, newRef ) );
}

bool 
RESTContentTemplate::getFieldInfo( std::string name, bool &required )
{

}

std::vector< RESTContentFieldDef* >
RESTContentTemplate::getFieldList()
{
    std::vector< RESTContentFieldDef* > resultList;

    for( std::map< std::string, RESTContentFieldDef >::const_iterator it = fieldDefs.begin(); it != fieldDefs.end(); ++it )
    {
        resultList.push_back( (RESTContentFieldDef *) &(it->second) );      
    }

    return resultList;
}

bool 
RESTContentTemplate::checkForFieldMatch( std::string name )
{
    if( fieldDefs.count( name ) )
        return true;

    return false;
}

bool 
RESTContentTemplate::checkForRefMatch( std::string name )
{
    if( referenceDefs.count( name ) )
        return true;

    return false;
}

bool 
RESTContentTemplate::checkForTagMatch( std::string name )
{
    if( fieldDefs.count( name ) )
        return false;

    return true;
}

bool 
RESTContentTemplate::checkForChildMatch( std::string name, RESTContentTemplate **cnPtr )
{
    RESTContentTemplate *cnPtr2;

    cnPtr2 = (RESTContentTemplate *) getChildByName( name );

    if( cnPtr2 == NULL )
        return false;

    *cnPtr = cnPtr2;

    return true;
}

RESTContentNode::RESTContentNode()
{
//    type = RCNT_NOTSET;
}

RESTContentNode::~RESTContentNode()
{

}

unsigned int
RESTContentNode::getObjType()
{
    return -1;
}

void 
RESTContentNode::setID( std::string idValue )
{
    setField( "id", idValue );
}

std::string 
RESTContentNode::getID()
{
    std::string returnStr;

    getField( "id", returnStr );

    return returnStr;
}

void 
RESTContentNode::setField( std::string name, std::string value )
{
    if( fieldValues.count( name ) > 0 )
    {
        fieldValues[name].setValue( value );
        return;
    }

    RESTContentField newField;
    newField.setName( name );
    newField.setValue( value );
    fieldValues.insert( std::pair< std::string, RESTContentField >( name, newField ) );
}

bool 
RESTContentNode::clearField( std::string name )
{
    fieldValues.erase( name );
}

bool 
RESTContentNode::getField( std::string name, std::string &value )
{
    if( fieldValues.count( name ) > 0 )
    {
       value = fieldValues[ name ].getValue();
       return true;
    }

    return false;
}

std::vector< RESTContentField* >
RESTContentNode::getFieldList()
{
    std::vector< RESTContentField* > resultList;

    for( std::map< std::string, RESTContentField >::const_iterator it = fieldValues.begin(); it != fieldValues.end(); ++it )
    {
        resultList.push_back( (RESTContentField *) &(it->second) );      
    }

    return resultList;
}

#if 0
void 
RESTContentNode::setRef( std::string name, std::string value )
{
    if( refValues.count( name ) > 0 )
    {
        refValues[name].setValue( value );
        return;
    }

    RESTContentReference newRef;
    newRef.setName( name );
    newRef.setValue( value );
    refValues.insert( std::pair< std::string, RESTContentReference >( name, newRef ) );
}

bool 
RESTContentNode::clearRef( std::string name )
{
    refValues.erase( name );
}

bool 
RESTContentNode::getRef( std::string name, std::string &value )
{
    if( refValues.count( name ) > 0 )
    {
       value = refValues[ name ].getValue();
       return true;
    }

    return false;
}

std::vector< RESTContentReference* >
RESTContentNode::getRefList()
{
    std::vector< RESTContentReference* > resultList;

    for( std::map< std::string, RESTContentReference >::const_iterator it = refValues.begin(); it != refValues.end(); ++it )
    {
        resultList.push_back( (RESTContentReference *) &(it->second) );      
    }

    return resultList;
}
#endif

void 
RESTContentNode::setVertex( refGraphVertex_t newVertex )
{
    vertex = newVertex;
}

refGraphVertex_t 
RESTContentNode::getVertex()
{
    return vertex;
}

void 
RESTContentNode::setFieldsFromContentNode( RESTContentNode *objCN )
{

}

void 
RESTContentNode::setContentNodeFromFields( RESTContentNode *objCN )
{

}

#if 0
RESTContentRef::RESTContentRef()
{

}

RESTContentRef::~RESTContentRef()
{

}

void 
RESTContentRef::setID( std::string newID )
{
    refID = newID;
}

std::string 
RESTContentRef::getID()
{
    return refID;
}
#endif

RESTContentEdge::RESTContentEdge()
{

}

RESTContentEdge::~RESTContentEdge()
{

}

void 
RESTContentEdge::setEdgeData( std::string srcNodeID, std::string dstNodeID, std::string type )
{
    srcID    = srcNodeID;
    dstID    = dstNodeID;
    edgeType = type;
}

std::string 
RESTContentEdge::getSrcID()
{
    return srcID;
}

std::string 
RESTContentEdge::getDstID()
{
    return dstID;
}

std::string 
RESTContentEdge::getEdgeType()
{
    return edgeType;
}

RESTContentManager::RESTContentManager()
{
    nextID = 0;

    init();
}

RESTContentManager::~RESTContentManager()
{

}

void
RESTContentManager::init()
{
    RESTContentNode *node;

    node = new RESTContentNode();
    node->setID( "root" );

    struct RESTCMVertex rootVProp;
    rootVProp.objID = node->getID();

    refGraphVertex_t vertex = add_vertex( rootVProp, refGraph );
    node->setVertex( vertex );

    std::pair< std::string, RESTContentNode* > insPair( node->getID(), node );
    objMap.insert( insPair );
}

void 
RESTContentManager::notifyCfgChange()
{

}

std::string 
RESTContentManager::getUniqueObjID( std::string prefix )
{
    char idStr[128];

    // Create a unique id
    do
    {
        nextID += 1;
        sprintf( idStr, "%s%d", prefix.c_str(), (int) nextID );
    }while( objMap.find( idStr ) != objMap.end() );

    return idStr;
}

void 
RESTContentManager::notifyClear()
{

}

void
RESTContentManager::clear()
{
    // Tell upper layers first (If they care)
    notifyClear();

    // Free the existing objects
    for( std::map< std::string, RESTContentNode* >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        // Call the allocator to free the object
        freeObject( it->second );
    }

    // Clear out the object map
    objMap.clear();

    // Get rid of all of the graph elements
    refGraph.clear();

    // Reset the base configuraiton
    init();
}

void 
RESTContentManager::createObj( unsigned int type, std::string idPrefix, std::string &objID )
{
    RESTContentNode *node;

    objID.clear();

    // Generate a new object ID
    objID = getUniqueObjID( idPrefix );

    // Add the object
    addObj( type, objID );
}

void 
RESTContentManager::addObj( unsigned int type, std::string objID )
{
    RESTContentNode *node;

    // create the new object
    node = newObject( type );

    if( node == NULL )
        return;

    // Assign it's unique id
    node->setID( objID );

    // Add it to the vertex list
    struct RESTCMVertex rootVProp;

    rootVProp.objID = node->getID();

    refGraphVertex_t vertex = add_vertex( rootVProp, refGraph );

    // Record the vertext in the node object
    node->setVertex( vertex );

    // Add it to the object map
    std::pair< std::string, RESTContentNode* > insPair( node->getID(), node );
    objMap.insert( insPair );
}

void 
RESTContentManager::updateObj( std::string objID, RESTContentNode *inputCN )
{
    std::map< std::string, RESTContentNode* >::iterator it;

    std::cout << "RESTContentManager::updateObj: " << objID << std::endl;

    // Find and validate the object
    it = objMap.find( objID );

    if( it != objMap.end() )
    {
        // Remove the vertex
        it->second->setFieldsFromContentNode( inputCN );
    }
    else
    {
        std::cout << "RESTContentManager::updateObj - Not found" << std::endl;
    }
}


void 
RESTContentManager::removeObjByID( std::string objID )
{
    std::map< std::string, RESTContentNode* >::iterator it;

    // Find and validate the object
    it = objMap.find( objID );

    if( it != objMap.end() )
    {
        // Remove the vertex
        refGraphVertex_t vertex = it->second->getVertex();
        clear_vertex( vertex, refGraph );
        remove_vertex( vertex, refGraph );

        // Remove the object from the map
        objMap.erase( it );
        return;
    }

    return;
}

void 
RESTContentManager::deleteObjByID( std::string objID )
{
    std::map< std::string, RESTContentNode* >::iterator it;

    // Find and validate the object
    it = objMap.find( objID );

    if( it != objMap.end() )
    {
        // Remove the vertex
        refGraphVertex_t vertex = it->second->getVertex();
        clear_vertex( vertex, refGraph );
        remove_vertex( vertex, refGraph );

        // Remove the object from the map
        RESTContentNode *objPtr = it->second;

        // Erase the object
        objMap.erase( it );
        delete objPtr;
        return;
    }

    return;
}

void 
RESTContentManager::removeAllObjects()
{
    refGraph.clear();
    objMap.clear();
}

void 
RESTContentManager::deleteAllObjects()
{
    // Perform the delete on the actual objects
    for( std::map< std::string, RESTContentNode* >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        // Remove the object from the map
        RESTContentNode *objPtr = it->second;

        // Erase the object
        delete objPtr;       
    }

    // Clear the data structures
    refGraph.clear();
    objMap.clear();
}

void
RESTContentManager::addRelationship( std::string relID, std::string parentID, std::string childID )
{
    std::map< std::string, RESTContentNode* >::iterator it;
    RESTContentNode *parentObj;
    RESTContentNode *childObj;
    struct RESTCMEdge eProp;

    std::cout << "addRelationship: " << relID << ", " << parentID << ", " << childID << std::endl;

    // Find the parent object
    it = objMap.find( parentID );

    if( it == objMap.end() )
        return;
    
    parentObj = it->second;

    // Find the child object
    it = objMap.find( childID );

    if( it == objMap.end() )
        return;
    
    childObj = it->second;

    // Fill in the link metadata
    eProp.relationID = relID;

    // Add the new edge
    add_edge( parentObj->getVertex(), childObj->getVertex(), eProp, refGraph );
}

void
RESTContentManager::removeRelationship( std::string relID, std::string parentID, std::string childID )
{

}

#if 0
template < class Graph > struct exercise_vertex 
{
//...
    void operator()( const Vertex& v ) const
    {
        typedef graph_traits<Graph> GraphTraits;
        typename property_map<Graph, vertex_index_t>::type index = get(vertex_index, g);

        std::cout << "out-edges: ";
        typename GraphTraits::out_edge_iterator out_i, out_end;
        typename GraphTraits::edge_descriptor e;
        for( tie( out_i, out_end ) = out_edges( v, g ); out_i != out_end; ++out_i ) 
        {
            e = *out_i;
            Vertex src = source(e, g), targ = target(e, g);
            std::cout << "(" << index[src] << "," << index[targ] << ") ";
        }
      
        std::cout << std::endl;
        //...
    }
    //...
};
#endif

bool 
RESTContentManager::hasRelationship( std::string parentID, std::string childID, std::string listID )
{
    std::cout << "hasRelationship: " << parentID << ", " << listID << ", " << childID << std::endl;

    std::map< std::string, RESTContentNode* >::iterator it;

    it = objMap.find( parentID );

    if( it == objMap.end() )
    {
        return false;
    }

    // Get a list of outgoing edges from vertex 1
    typedef boost::graph_traits < refGraph_t >::out_edge_iterator out_edge_iterator;
    std::pair< out_edge_iterator, out_edge_iterator > outEdges = boost::out_edges( it->second->getVertex(), refGraph );
 
    for( ; outEdges.first != outEdges.second; ++outEdges.first )
    {
        if( ( refGraph[*outEdges.first].relationID == listID ) && ( refGraph[ target(*outEdges.first, refGraph) ].objID == childID ) )
        {
            return true;
        }
    }

    return false;
}


RESTContentNode* 
RESTContentManager::getObjectByID( std::string objID )
{
    std::map< std::string, RESTContentNode* >::iterator it;

    it = objMap.find( objID );

    if( it != objMap.end() )
    {
        return it->second;
    }

    return NULL;
}

void 
RESTContentManager::getIDListForRelationship( std::string parentID, std::string relID, std::vector< std::string > &idList )
{
    //std::cout << "getRelationList: " << parentID << ", " << relID << std::endl;

    std::map< std::string, RESTContentNode* >::iterator it;

    it = objMap.find( parentID );

    if( it == objMap.end() )
    {
        return;
    }

    // Get a list of outgoing edges from vertex 1
    typedef boost::graph_traits < refGraph_t >::out_edge_iterator out_edge_iterator;
    std::pair< out_edge_iterator, out_edge_iterator > outEdges = boost::out_edges( it->second->getVertex(), refGraph );
 
    for( ; outEdges.first != outEdges.second; ++outEdges.first )
    {
        if( refGraph[*outEdges.first].relationID == relID )
        {
            idList.push_back( refGraph[ target(*outEdges.first, refGraph) ].objID );
        }
    }
}


#if 0
RESTContentNode* 
RESTContentManager::getObjectFromRef( RESTContentRef &ref )
{
    return getObjectByID( ref.getID() );
}
#endif

unsigned int 
RESTContentManager::getObjectCountByType( unsigned int type )
{
    unsigned int count = 0;
    for( std::map< std::string, RESTContentNode * >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        if( it->second->getType() == type )
            count += 1; 
    }

    return count;
}

void 
RESTContentManager::getIDVectorByType( unsigned int type, std::vector< std::string > &rtnVector )
{
    for( std::map< std::string, RESTContentNode * >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        std::cout << "RESTContentManager::getIDVectorByType - " << it->first << ":" << it->second->getObjType() << std::endl;
        if( it->second->getObjType() == type )
        {
            rtnVector.push_back( it->second->getID() );
        }
    }
}

void 
RESTContentManager::getObjectVectorByType( unsigned int type, std::vector< RESTContentNode* > &rtnVector )
{
    for( std::map< std::string, RESTContentNode * >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        if( it->second->getObjType() == type )
        {
            rtnVector.push_back( it->second );
        }
    }
}

void 
RESTContentManager::getObjectList( std::list< RESTContentNode* > &objList )
{
    for( std::map< std::string, RESTContentNode * >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        objList.push_back( it->second );
    }
}

void 
RESTContentManager::getObjectRepresentationList( std::vector< RESTContentNode > &objList )
{
    // Clear any existing entries
    objList.clear();

    // Preallocate for all of the objects, plus a root object for the content manager itself
    objList.resize( objMap.size() + 1 );

    // Build the root object
    objList[ 0 ].setAsObject( "rcm-root" );
    objList[ 0 ].setID( "root" );
//    objCN->setField( "name", getName() );
 
    // Initialize the record for each object
    unsigned int objIndex = 1;
    for( std::map< std::string, RESTContentNode * >::iterator it = objMap.begin(); it != objMap.end(); ++it )
    {
        it->second->setContentNodeFromFields( &objList[ objIndex ] );

        objIndex += 1;
    } 
}

void 
RESTContentManager::getEdgeRepresentationList( std::vector< RESTContentEdge > &edgeList )
{
    RESTContentEdge newEdge;

    // Clear any existing entries
    edgeList.clear();

    // ...
    boost::graph_traits< refGraph_t >::edge_iterator ei, ei_end;
    for( tie( ei, ei_end ) = edges( refGraph ); ei != ei_end; ++ei)
    {
        std::string srcNodeID = refGraph[ source( *ei, refGraph ) ].objID;
        std::string edgeType  = refGraph[ *ei ].relationID;
        std::string dstNodeID = refGraph[ target( *ei, refGraph ) ].objID;
        std::cout << "(" << srcNodeID << "," << edgeType << "," << dstNodeID << ") " << std::endl;
        newEdge.setEdgeData( srcNodeID, dstNodeID, edgeType );
        edgeList.push_back( newEdge );
    }
}

RESTContentHelper::RESTContentHelper()
{
    // Initialize root node
    rootNode = NULL;
}

RESTContentHelper::~RESTContentHelper()
{
    // Cleanup
    clearRootNode();
}

void 
RESTContentHelper::clearRootNode()
{
    // If we have an object then delete it (and its children)
    if( rootNode )
    {
        delete rootNode;
    }

    // Object is no longer
    rootNode = NULL;
}

RESTContentNode *
RESTContentHelper::getRootNode()
{
    // Check if we already have a root node object
    if( rootNode == NULL )
    {
        rootNode = new RESTContentNode();
    }

    // Return object pointer.
    return rootNode;
}

RESTContentNode *
RESTContentHelper::detachRootNode()
{
    RESTContentNode *returnPtr;

    // Check if we already have a root node object
    if( rootNode == NULL )
    {
        returnPtr = new RESTContentNode();
    }   
    else
    {
        returnPtr = rootNode;
    }

    // Forget about the datastructure
    rootNode = NULL;

    // Return the detached datastructure
    return returnPtr;
}

void 
RESTContentHelper::freeDetachedRootNode( RESTContentNode *objPtr )
{
    delete objPtr;
}

bool 
RESTContentHelper::hasRootObject( std::string objectName, RESTContentNode **objPtr )
{
    // Clear the object pointer
    *objPtr = NULL;

    if( rootNode == NULL )
        return false;

    if( rootNode->getName() != objectName )
        return false;

    *objPtr = rootNode;

    return true;
}

RESTContentNode *
RESTContentHelper::getObject( std::string objectName )
{
    // FIX ME -- implement search
    return rootNode;
}

RESTContentHelperXML::RESTContentHelperXML()
{

}

RESTContentHelperXML::~RESTContentHelperXML()
{

}

bool
RESTContentHelperXML::getAttribute( void *docPtr, void *nodePtr, std::string attrName, std::string &result )
{
    xmlChar *attrValue;
    xmlNode *elem = (xmlNode *) nodePtr;

    // Start with a clear result
    result.clear();

    // Get the type attribute
    attrValue = xmlGetProp( elem, (xmlChar *)attrName.c_str() );

    if( attrValue == NULL )
    {
        return true;
    }    

    result = (char*)attrValue;

    xmlFree( attrValue );

    return false;
}

bool
RESTContentHelperXML::getChildContent( void *docPtr, void *nodePtr, std::string childName, std::string &result )
{
    xmlChar *contentValue;
    xmlNode *curElem;
    xmlNode *childElem;
    xmlNode *elem = (xmlNode *) nodePtr;

    // Start with a clear result
    result.clear();

    // Find the address element
    childElem = NULL;
    for( curElem = elem->children; curElem; curElem = curElem->next ) 
    {
        if( (curElem->type == XML_ELEMENT_NODE) && (strcmp( (char *)curElem->name, childName.c_str() ) == 0) ) 
        {
            childElem = curElem;
            break;
        }
    }
 
    if(childElem == NULL )
    {
        return true;
    }

    // Get the type attribute
    contentValue = xmlNodeGetContent( childElem );

    result = (char*)contentValue;

    xmlFree( contentValue );

    return false;
}

void
RESTContentHelperXML::addFieldValues( void *docPtr, void *nodePtr, RESTContentNode *cnPtr )
{
    xmlChar *contentValue;
    xmlNode *curElem;
    xmlDocPtr doc = (xmlDocPtr) docPtr;
    xmlNode *elem = (xmlNode *) nodePtr;

    // Find the address element
    for( curElem = elem->children; curElem; curElem = curElem->next ) 
    {
        if( ( curElem->type == XML_ELEMENT_NODE ) && ( xmlChildElementCount( curElem ) == 0 ) ) 
        {
            // Get the node contents
            contentValue = xmlNodeGetContent( curElem );
              
            // Add this name value pair
            cnPtr->setField( (const char *)curElem->name, (const char *)contentValue );

            // Cleanup temporary copy
            xmlFree( contentValue );
        }
    }
}

void 
RESTContentHelperXML::parseTree( void *docPtr, void *nodePtr, RESTContentNode *cnPtr )
{
    xmlChar *contentValue;
    xmlNode *curElem;
    RESTContentNode *curCNPtr;
    xmlDocPtr doc = (xmlDocPtr) docPtr;
    xmlNode *elem = (xmlNode *) nodePtr;

    // Give it the element name
    cnPtr->setName( (const char *) elem->name );

    // Add any terminal children that represent name value pairs
    addFieldValues( doc, elem, cnPtr );

    // Add any child elements and recurse on those
    // Find the address element
    for( curElem = elem->children; curElem; curElem = curElem->next ) 
    {
        if( ( curElem->type == XML_ELEMENT_NODE ) && ( xmlChildElementCount( curElem ) != 0 ) )  
        {
            // A new object for the child
            curCNPtr = new RESTContentNode();

            // Setup the relationships
            curCNPtr->setParent( cnPtr );
            cnPtr->addChild( curCNPtr );

            // Recurse to build the tree
            parseTree( docPtr, nodePtr, curCNPtr );
        }
    }    
}

bool 
RESTContentHelperXML::supportsContentDecode( std::string contentType )
{
    if( contentType != "application/xml" )
    {
        return true;
    }

    return false;
}

bool 
RESTContentHelperXML::supportsContentCreate( std::string contentType )
{
    if( contentType != "application/xml" )
    {
        return true;
    }

    return false;
}

bool 
RESTContentHelperXML::parseRawData( RESTRepresentation *repPtr )
{
    xmlDocPtr      doc;
    xmlNode       *rootElem;
    unsigned char *dataPtr;
    unsigned long dataLength;
    std::string   contentType;
   
    //std::cout << "RESTContentHelperXML::parseRepSimple -- start" << std::endl;

    // Find the databuffer to parse
    dataPtr = repPtr->getSimpleContentPtr( contentType, dataLength );

    //printf( "Data(%d):\n%*.*s\n", dataLength, dataLength, dataLength, dataPtr );
 
    // Run the simple content through the xml parser
    doc = xmlParseMemory( (const char *)dataPtr, dataLength );
    if (doc == NULL) 
    {
        return true;
    }

    // Get the root element for the document
    rootElem = xmlDocGetRootElement( doc );

    //std::cout << "Root Elem Name: " << rootElem->name << std::endl;

    // Clean up any old root element
    clearRootNode(); 

    // Create the root element
    RESTContentNode *rootCN = getRootNode();  

    // Parse all of the object out of the tree
    parseTree( doc, rootElem, rootCN );

    return false;
}

bool
RESTContentHelperXML::findFieldValue( std::string fieldName, RESTContentNode *cnPtr, void *docPtr, void *nodePtr )
{
    xmlChar *contentValue;
    xmlNode *curElem;
    xmlDocPtr doc = (xmlDocPtr) docPtr;
    xmlNode *elem = (xmlNode *) nodePtr;

    // Find the address element
    for( curElem = elem->children; curElem; curElem = curElem->next ) 
    {
        if( ( curElem->type == XML_ELEMENT_NODE ) && ( xmlChildElementCount( curElem ) == 0 ) ) 
        {
            if( fieldName == (const char *)curElem->name )
            {
                // Get the node contents
                contentValue = xmlNodeGetContent( curElem );
              
                // Add this name value pair
                cnPtr->setField( (const char *)curElem->name, (const char *)contentValue );

                // Cleanup temporary copy
                xmlFree( contentValue );

                return true;
            }
        }
    }

    return false;
}

bool 
RESTContentHelperXML::parseWithTemplate( RESTContentTemplate *templateCN, RESTRepresentation *repPtr )
{
    xmlDocPtr      doc;
    xmlNode       *rootElem;
    unsigned char *dataPtr;
    unsigned long dataLength;
    std::string   contentType;
   
    std::cout << "RESTContentHelperXML::parseWithTemplate -- start" << std::endl;

    // Find the databuffer to parse
    dataPtr = repPtr->getSimpleContentPtr( contentType, dataLength );

    std::cout << dataPtr << std::endl;

    // Run the simple content through the xml parser
    doc = xmlParseMemory( (const char *)dataPtr, dataLength );
    if (doc == NULL) 
    {
        return true;
    }

    // Get the root element for the document
    rootElem = xmlDocGetRootElement( doc );

    std::cout << "Root Elem Name: " << rootElem->name << std::endl;

    // Clean up any old root element
    clearRootNode(); 

    // Create the root element
    RESTContentNode *rootCN = getRootNode();

    if( templateCN->isID() )
    {
        std::cout << "RESTContentHelperXML::parseWithTemplate - id" << std::endl;
        //generateIDContent( childCN, repPtr );
    }
    else if( templateCN->isObj() )
    {
        std::cout << "RESTContentHelperXML::parseWithTemplate - object" << std::endl;

        // Make sure the root level object matches
        if( templateCN->getName() != (const char *)rootElem->name )
        {
            std::cout << "Object name did not match.  Expecting " + templateCN->getName() + " found " + (const char *)rootElem->name << std::endl;
            throw new RESTContentException( 1000, "Object name did not match.  Expecting " + templateCN->getName() + " found " + (const char *)rootElem->name );
        }

        // Set the name in the generated object
        rootCN->setName( templateCN->getName() );

        // Go through the fields in the template and try to match those.
        std::vector< RESTContentFieldDef* > fieldList = templateCN->getFieldList();

        for( std::vector< RESTContentFieldDef* >::const_iterator it = fieldList.begin(); it != fieldList.end(); ++it )
        {
            std::cout << "Field Name: " << (*it)->getName() << std::endl;
            if( findFieldValue( (*it)->getName(), rootCN, doc, rootElem ) == false )
            {
                std::cout << "Field Not Found!" << std::endl;
            }
            else
            {
                std::string tmpStr;
                rootCN->getField( (*it)->getName(), tmpStr );
                std::cout << "Field Found: " << tmpStr << std::endl;
            }
        }
    }
    else if( templateCN->isArray() )
    {
        std::cout << "RESTContentHelperXML::parseWithTemplate - array" << std::endl;
        //generateArrayContent( childCN, repPtr );
    }

    // Parse all of the object out of the tree
    //parseTree( doc, rootElem, rootCN );

    return false;
}

bool
RESTContentHelperXML::generateChildContent(  RESTContentNode *childCN, RESTRepresentation *repPtr )
{

    std::cout << "RESTContentHelperXML::generateChildContent - start" << std::endl;

    if( childCN->isID() )
    {
        std::cout << "RESTContentHelperXML::generateChildContent - id" << std::endl;
        generateIDContent( childCN, repPtr );
    }
    else if( childCN->isObj() )
    {
        std::cout << "RESTContentHelperXML::generateChildContent - object" << std::endl;
        generateObjectContent( childCN, repPtr );
    }
    else if( childCN->isArray() )
    {
        std::cout << "RESTContentHelperXML::generateChildContent - array" << std::endl;
        generateArrayContent( childCN, repPtr );
    }

    return false;
}

bool
RESTContentHelperXML::generateArrayContent( RESTContentNode *arrCN, RESTRepresentation *repPtr )
{
    std::string rspData;

    rspData = "<" + arrCN->getName() + ">";
    repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 

    for( int indx = 0; indx < arrCN->getChildCount(); indx++ )
    {
        RESTContentNode *childCN = (RESTContentNode *)arrCN->getChildByIndex( indx );
        
        generateChildContent( childCN, repPtr );
    }

    rspData = "</" + arrCN->getName() + ">";
    repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 

}

bool
RESTContentHelperXML::generateObjectContent( RESTContentNode *objCN, RESTRepresentation *repPtr )
{
    std::string rspData;

    rspData = "<" + objCN->getName() + ">";
    repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 

    std::vector< RESTContentField* > fieldList = objCN->getFieldList();

    for( std::vector< RESTContentField* >::const_iterator it = fieldList.begin(); it != fieldList.end(); ++it )
    {
        rspData = "<" + (*it)->getName() + ">" + (*it)->getValue() + "</" + (*it)->getName() + ">"; 
        repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 
    }

#if 0
    for( int findx = 0; findx < objCN->getFieldCount(); findx++ )
    {
        RESTContentField *field;

        field = objCN->getFieldByIndex( findx );
        
        rspData = "<" + field->getName() + ">" + field->getValue() + "</" + field->getName() + ">"; 
        repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 
    }
#endif

    for( int indx = 0; indx < objCN->getChildCount(); indx++ )
    {
        RESTContentNode *childCN = (RESTContentNode *)objCN->getChildByIndex( indx );
        
        generateChildContent( childCN, repPtr );
    }

    rspData = "</" + objCN->getName() + ">";
    repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 
}

bool
RESTContentHelperXML::generateIDContent( RESTContentNode *idCN, RESTRepresentation *repPtr )
{
    std::string rspData;

    rspData = "<id>";
    rspData += idCN->getID();
    rspData += "</id>";    

    repPtr->appendSimpleContent( (unsigned char*) rspData.c_str(), rspData.size() ); 

    return false;
}

bool 
RESTContentHelperXML::generateContentRepresentation( RESTRepresentation *repPtr )
{
    RESTContentNode *rootCN = getRootNode();
    std::string rspData;

    std::cout << "RESTContentHelperXML::generateContentRepresentation" << std::endl;

    repPtr->setSimpleContent( "application/xml" );

    generateChildContent( rootCN, repPtr );

    return false;
}


RESTContentHelperJSON::RESTContentHelperJSON()
{

}

RESTContentHelperJSON::~RESTContentHelperJSON()
{

}

#if 0
RESTContentHelper *
RESTContentHelperFactory::decodeSimpleContent( RESTRepresentation *repPtr )
{
    RESTContentHelper *chPtr;
    unsigned char     *dataPtr;
    std::string        contentType;
    unsigned long      dataLength;

    // Makes sure we recognize the content
    if( repPtr->hasSimpleContent() == false )
    {
        return NULL;
    }

    dataPtr = repPtr->getSimpleContentPtr( contentType, dataLength );

    // Determine which type of decoder we will use
    if( contentType == "application/xml" )
    {
        chPtr = new RESTContentHelperXML();
        chPtr->parseRawData( dataPtr, dataLength );
        return chPtr;
    }

    return NULL;
}

RESTContentHelper *
RESTContentHelperFactory::decodeSimpleContent( RESTContentNode *templateCN, RESTRepresentation *repPtr )
{
    RESTContentHelper *chPtr;
    unsigned char     *dataPtr;
    std::string        contentType;
    unsigned long      dataLength;

    // Makes sure we recognize the content
    if( repPtr->hasSimpleContent() == false )
    {
        return NULL;
    }

    dataPtr = repPtr->getSimpleContentPtr( contentType, dataLength );

    // Determine which type of decoder we will use
    if( contentType == "application/xml" )
    {
        chPtr = new RESTContentHelperXML();
        chPtr->parseRawData( dataPtr, dataLength );
        return chPtr;
    }

    return NULL;
}
#endif

RESTContentNode *
RESTContentHelperFactory::newContentNode()
{
    return new RESTContentNode();
}

void 
RESTContentHelperFactory::freeContentNode( RESTContentNode *nodePtr )
{
    delete nodePtr;
}

RESTContentTemplate *
RESTContentHelperFactory::newContentTemplate()
{
    return new RESTContentTemplate();
}

void 
RESTContentHelperFactory::freeContentTemplate( RESTContentTemplate *nodePtr )
{
    delete nodePtr;
}

RESTContentHelper *
RESTContentHelperFactory::getRequestSimpleContentHelper( RESTRepresentation *repPtr )
{
    // Default to XML
    return new RESTContentHelperXML();
}

RESTContentHelper *
RESTContentHelperFactory::getResponseSimpleContentHelper( RESTRepresentation *repPtr )
{
    // Default to XML
    return new RESTContentHelperXML();
}

void 
RESTContentHelperFactory::freeContentHelper( RESTContentHelper *chPtr )
{
    delete chPtr;
}

