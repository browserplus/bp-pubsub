//////////////////////////////
// pubsub.cpp 
//
// Implements the Native PubSub service.
//
#include <list>
#include <string>
#include <vector>
#include "bpservice/bpservice.h"
#include "bpservice/bpcallback.h"
#include "bputil/bpstrutil.h"
#include "bputil/bpsync.h"
#include "bputil/bpurl.h"
#include "listener.h"


using namespace std;
using namespace bplus;
using namespace bplus::service;
using namespace bplus::sync;
using namespace bplus::url;


class PubSub : public Service
{
public:    
    BP_SERVICE( PubSub );

    PubSub();
    ~PubSub();
    
    void    finalConstruct();

    void    addListener( const Transaction& tran, const Map& args );

    void    postMessage( const Transaction& tran, const Map& args );
    
private:
    // We define a "subscriber" as a PubSub instance with at least one listener.
    static list<PubSub*>    s_lSubscribers;
    static Mutex            s_mtxSubscribers;

private:    
    static bool             isSafeToPublish( const Object& oData );

    const string&           origin();

    // Notification handler, called by other PubSub instances.
    void                    onNotify( const Map& mData );

private:    
    string                  m_sOrigin;
    vector<Listener>        m_vListeners;
    Mutex                   m_mtxListeners;
};


BP_SERVICE_DESC( PubSub, "PubSub", "0.0.1",
                 "A cross document message service that allows JavaScript to "
                 "send and receive messages between web pages within "
                 "one or more browsers (cross document + cross process)." )
  ADD_BP_METHOD( PubSub, addListener,
                 "Subscribe to the pubsub mechanism." )
    ADD_BP_METHOD_ARG( addListener, "receiver", CallBack, true,
                       "JavaScript function that is notified of a message. "
                       "The value passed to the callback contains "
                       "{data:(Any), origin:(String)}" )
    ADD_BP_METHOD_ARG( addListener, "origin", String, false,
                       "Optional string that specifies the domain "
                       "e.g. (\"http://example.com\") to accept messages from. "
                       "Defaults to all (\"*\"). "
                       "This is not part of the HTML5 spec but allows "
                       "automatic filtering of events so JavaScript listener "
                       "does not have to manually check event.origin." )
          
  ADD_BP_METHOD( PubSub, postMessage,
                 "Post a message.  The message posted is associated with "
                 "the domain of the sender.  Receivers may elect to filter "
                 "messages based on the domain." )
    ADD_BP_METHOD_ARG( postMessage, "data", Any, true,
                       "The data object (Object, Array, String, Boolean, "
                       "Integer, Float, Boolean, Null) that is posted to all "
                       "interested subscribers.  All other data types are "
                       "stripped out of the passed object." )
    ADD_BP_METHOD_ARG( postMessage, "targetOrigin", String, true,
                       "The origin specifies where to send the message to. "
                       "Options are either an URI like \"http://example.org\" "
                       "or \"*\" to pass it to all listeners." )
END_BP_SERVICE_DESC


          
////////////////////////////////////////////////////////////////////////////////
// Implementation
//

list<PubSub*>   PubSub::s_lSubscribers;
Mutex           PubSub::s_mtxSubscribers;
          

PubSub::PubSub() : Service()
{
    
}


void PubSub::finalConstruct()
{
    // Setup our "origin", which is a massaged version of our uri.
    Url url;
    if (!url.parse( context( "uri" ) )) {
        log( BP_ERROR, "Could not parse context uri." );
        return;
    }

    string sDomain;
    string sScheme = url.scheme();
    if (sScheme == "file") {
        sDomain = "";
    } else {
        vector<string> vsComps = strutil::split( url.host(), "." );
        if (vsComps.size() == 2) {
            vsComps.insert( vsComps.begin(), "www" );
        }

        sDomain = strutil::join( vsComps, "." );
    }

    m_sOrigin = sScheme + "://" + sDomain;
}


PubSub::~PubSub()
{
    // We're going down.  Remove us as a subscriber, if we're there.
    Lock lock( s_mtxSubscribers );
    s_lSubscribers.remove( this );
}


void PubSub::addListener( const Transaction& tran, const Map& args )
{
    // Pull "accept origin" out of args if it's there.
    string sAcceptOrigin = "*";
    const Object* po = args.get( "origin" );
    if (po) {
        sAcceptOrigin = string( *po );
    }
    
    Listener lstnr( tran, args["receiver"], sAcceptOrigin );

    Lock listenerLock( m_mtxListeners );
    m_vListeners.push_back( lstnr );

    // If this is our first listener, add us as a subscriber.
    if (m_vListeners.size() == 1)
    {
        Lock subscriberLock( s_mtxSubscribers );
        s_lSubscribers.push_back( this );
    }
}


void PubSub::postMessage( const Transaction& tran, const Map& args )
{
    const Object& oData = args["data"];
    if (!isSafeToPublish( oData )) {
        tran.error( "DataTransferError",
                    "Objects of that type cannot be sent through postMessage" );
    }

    Map mData;
    mData.add( "data", oData.clone() );
    mData.add( "origin", String( origin() ).clone() );

    string sTargetOrigin = args["targetOrigin"];

    Lock lock( s_mtxSubscribers );

    for (list<PubSub*>::iterator it = s_lSubscribers.begin();
         it != s_lSubscribers.end(); ++it) {
        PubSub* pSub = *it;

        if (sTargetOrigin == "*" || pSub->origin() == sTargetOrigin) {
            pSub->onNotify( mData );
        }
    }

    tran.complete( bplus::Bool( true ) );
}


bool PubSub::isSafeToPublish( const Object& oData )
{
    switch (oData.type())
    {
        case BPTNull:       return true; 
        case BPTBoolean:    return true;
        case BPTInteger:    return true;
        case BPTDouble:     return true;
        case BPTString:     return true; 
        case BPTMap:        {
                                Map& m = (Map&) oData;
                                Map::Iterator it(m);
                                const char* key = NULL;
                                while ((key = it.nextKey()) != NULL) {
                                    if (!isSafeToPublish( *m.value(key) ))
                                          return false;
                                }
                                return true;
                            }
        case BPTList:       {
                                List& l = (List&) oData;
                                for (unsigned int i = 0; i < l.size(); i++) {
                                    if (!isSafeToPublish( *l.value(i) ))
                                        return false;
                                }
                                return true;
                            }
        case BPTCallBack:   return false;
        case BPTPath:       return false;
        case BPTAny:        return false;
        default:            return false;
    }
}


void PubSub::onNotify( const Map& mData )
{
    for (vector<Listener>::iterator it = m_vListeners.begin();
         it != m_vListeners.end(); ++it)
    {
        it->onNotify( mData );
    }
}


const string& PubSub::origin()
{
    return m_sOrigin;
}

