//////////////////////////////
// listener.h
//
#ifndef LISTENER_H_
#define LISTENER_H_

#include "bpservice/bpcallback.h"


class Listener
{
public:
        
    // Note: we accept type Object& for caller convenience, but
    //       the arg must be of type CallBack& at runtime.
    Listener( const bplus::service::Transaction& tran,
              const bplus::Object& cb,
              const std::string& sAcceptOrigin = "*" );

    void onNotify( const bplus::Map& mData );
    
private:
    bplus::service::Callback    m_cb;
    std::string                 m_sAcceptOrigin;
};



inline Listener::Listener( const bplus::service::Transaction& tran,
                           const bplus::Object& cb,
                           const std::string& sAcceptOrigin ) :
    m_cb( tran, cb ),
    m_sAcceptOrigin( sAcceptOrigin )
{
    
}


inline void
Listener::onNotify( const bplus::Map& mData )
{
    std::string sSourceDomain = mData["origin"];

    // Extra filtering above and beyond html5.
    if (m_sAcceptOrigin == "*" || m_sAcceptOrigin == sSourceDomain) {
        m_cb.invoke( mData );
    }
}



#endif
