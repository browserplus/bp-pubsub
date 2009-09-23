//////////////////////////////
// listener.cpp 
//
// 
#include "listener.h"

using namespace bplus;
using namespace bplus::service;
using namespace std;


Listener::Listener( const Transaction& tran,
                    const Object& cb,
                    const std::string& sAcceptOrigin ) :
    m_cb( tran, cb ),
    m_sAcceptOrigin( sAcceptOrigin )
{

}


void Listener::onNotify( const bplus::Map& mData )
{
    string sSourceDomain = mData["origin"];

    // Extra filtering above and beyond html5.
    if (m_sAcceptOrigin == "*" || m_sAcceptOrigin == sSourceDomain) {
        m_cb.invoke( mData );
    }
}


