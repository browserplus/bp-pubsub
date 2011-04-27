/**
 * ***** BEGIN LICENSE BLOCK *****
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is BrowserPlus (tm).
 * 
 * The Initial Developer of the Original Code is Yahoo!.
 * Portions created by Yahoo! are Copyright (C) 2006-2009 Yahoo!.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 * ***** END LICENSE BLOCK ***** */

#include "bpservice/bpservice.h"
#include "bpservice/bpcallback.h"
#include "bputil/bpsync.h"
#include "bputil/bpurl.h"

class PubSub : public bplus::service::Service {
    class Listener {
    public:
        // Note: we accept type Object& for caller convenience, but
        //       the arg must be of type CallBack& at runtime.
        Listener(const bplus::service::Transaction& tran, const bplus::Object& cb, const std::string& sAcceptOrigin = "*") :
            m_cb(tran, cb),
            m_sAcceptOrigin(sAcceptOrigin) {
        }
        void onNotify(const bplus::Map& mData) {
            std::string sSourceDomain = mData["origin"];
            // Extra filtering above and beyond html5.
            if (m_sAcceptOrigin == "*" || m_sAcceptOrigin == sSourceDomain) {
                m_cb.invoke(mData);
            }
        }
    private:
        bplus::service::Callback m_cb;
        std::string m_sAcceptOrigin;
    };
public:    
BP_SERVICE(PubSub);
    PubSub();
    ~PubSub();
    void finalConstruct();
    void addListener(const bplus::service::Transaction& tran, const bplus::Map& args);
    void postMessage(const bplus::service::Transaction& tran, const bplus::Map& args);
private:
    // We define a "subscriber" as a PubSub instance with at least one listener.
    static std::list<PubSub*> s_lSubscribers;
    static bplus::sync::Mutex s_mtxSubscribers;
private:    
    static bool isSafeToPublish(const bplus::Object& oData);
    const std::string& origin();
    // Notification handler, called by other PubSub instances.
    void onNotify(const bplus::Map& mData);
private:    
    std::string m_sOrigin;
    std::vector<Listener> m_vListeners;
    bplus::sync::Mutex m_mtxListeners;
};

BP_SERVICE_DESC(PubSub, "PubSub", "0.1.0",
                "A cross document message service that allows JavaScript to "
                "send and receive messages between web pages within "
                "one or more browsers (cross document + cross process).")
ADD_BP_METHOD(PubSub, addListener,
              "Subscribe to the pubsub mechanism.")
ADD_BP_METHOD_ARG(addListener, "receiver", CallBack, true,
                  "JavaScript function that is notified of a message. "
                  "The value passed to the callback contains "
                  "{data:(Any), origin:(String)}")
ADD_BP_METHOD_ARG(addListener, "origin", String, false,
                  "Optional string that specifies the domain "
                  "e.g. (\"http://example.com\") to accept messages from. "
                  "Defaults to all (\"*\"). "
                  "This is not part of the HTML5 spec but allows "
                  "automatic filtering of events so JavaScript listener "
                  "does not have to manually check event.origin.")
ADD_BP_METHOD(PubSub, postMessage,
              "Post a message.  The message posted is associated with "
              "the domain of the sender.  Receivers may elect to filter "
              "messages based on the domain.")
ADD_BP_METHOD_ARG(postMessage, "data", Any, true,
                  "The data object (Object, Array, String, Boolean, "
                  "Integer, Float, Boolean, Null) that is posted to all "
                  "interested subscribers.  All other data types are "
                  "stripped out of the passed object.")
ADD_BP_METHOD_ARG(postMessage, "targetOrigin", String, true,
                  "The origin specifies where to send the message to. "
                  "Options are either an URI like \"http://example.org\" "
                  "or \"*\" to pass it to all listeners.")
END_BP_SERVICE_DESC

////////////////////////////////////////////////////////////////////////////////
// Implementation
//
std::list<PubSub*> PubSub::s_lSubscribers;
bplus::sync::Mutex PubSub::s_mtxSubscribers;

PubSub::PubSub() : bplus::service::Service() {
}

PubSub::~PubSub() {
    // We're going down.  Remove us as a subscriber, if we're there.
    bplus::sync::Lock lock(s_mtxSubscribers);
    s_lSubscribers.remove(this);
}

void PubSub::finalConstruct() {
    // Setup our "origin", which is a massaged version of our uri.
    bplus::url::Url url;
    if (!url.parse(clientUri())) {
        log(BP_ERROR, "Could not parse context uri.");
        return;
    }
    std::string sDomain;
    std::string sScheme = url.scheme();
    if (sScheme == "file") {
        sDomain = "";
    } else {
        std::vector<std::string> vsComps = bplus::strutil::split(url.host(), ".");
        if (vsComps.size() == 2) {
            vsComps.insert(vsComps.begin(), "www");
        }
        sDomain = bplus::strutil::join(vsComps, ".");
    }
    m_sOrigin = sScheme + "://" + sDomain;
}

void PubSub::addListener(const bplus::service::Transaction& tran, const bplus::Map& args) {
    // Pull "accept origin" out of args if it's there.
    std::string sAcceptOrigin = "*";
    const bplus::Object* po = args.get("origin");
    if (po) {
        sAcceptOrigin = std::string(*po);
    }
    Listener lstnr(tran, args["receiver"], sAcceptOrigin);
    bplus::sync::Lock listenerLock(m_mtxListeners);
    m_vListeners.push_back(lstnr);
    // If this is our first listener, add us as a subscriber.
    if (m_vListeners.size() == 1) {
        bplus::sync::Lock subscriberLock(s_mtxSubscribers);
        s_lSubscribers.push_back(this);
    }
}

void PubSub::postMessage(const bplus::service::Transaction& tran, const bplus::Map& args) {
    const bplus::Object& oData = args["data"];
    if (!isSafeToPublish(oData)) {
        tran.error("DataTransferError", "Objects of that type cannot be sent through postMessage");
    }
    bplus::Map mData;
    mData.add("data", oData.clone());
    mData.add("origin", bplus::String(origin()).clone());
    std::string sTargetOrigin = args["targetOrigin"];
    bplus::sync::Lock lock(s_mtxSubscribers);
    for (std::list<PubSub*>::iterator it = s_lSubscribers.begin(); it != s_lSubscribers.end(); ++it) {
        PubSub* pSub = *it;
        if (sTargetOrigin == "*" || pSub->origin() == sTargetOrigin) {
            pSub->onNotify(mData);
        }
    }
    tran.complete(bplus::Bool(true));
}

bool PubSub::isSafeToPublish(const bplus::Object& oData) {
    switch (oData.type()) {
        case BPTNull:
        case BPTBoolean:
        case BPTInteger:
        case BPTDouble:
        case BPTString:
            return true; 
        case BPTCallBack:
        case BPTNativePath:
        case BPTWritableNativePath:
        case BPTAny:
            return false;
        case BPTMap: {
            bplus::Map& m = (bplus::Map&)oData;
            bplus::Map::Iterator it(m);
            const char* key = NULL;
            while ((key = it.nextKey()) != NULL) {
                if (!isSafeToPublish(*m.value(key)))
                    return false;
            }
            return true;
        }
        case BPTList: {
            bplus::List& l = (bplus::List&)oData;
            for (unsigned int i = 0; i < l.size(); i++) {
                if (!isSafeToPublish(*l.value(i)))
                    return false;
            }
            return true;
        }
        default:
            return false;
    }
}

void PubSub::onNotify(const bplus::Map& mData) {
    for (std::vector<Listener>::iterator it = m_vListeners.begin(); it != m_vListeners.end(); ++it) {
        it->onNotify(mData);
    }
}

const std::string& PubSub::origin() {
    return m_sOrigin;
}

