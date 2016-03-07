/* Copyright (C) 2014 J.F.Dockes
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the
 *	 Free Software Foundation, Inc.,
 *	 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _DEVICE_H_X_INCLUDED_
#define _DEVICE_H_X_INCLUDED_
#include "libupnpp/config.h"

#include <pthread.h>                    // for pthread_cond_t
#include <upnp/upnp.h>                  // for Upnp_EventType, etc

#include <string>                       // for string
#include <vector>                       // for vector

#include "libupnpp/ptmutex.hxx"         // for PTMutexInit
#include "libupnpp/soaphelp.hxx"        // for SoapOutgoing, SoapIncoming

namespace UPnPP {
class LibUPnP;
}
namespace UPnPProvider {
class UpnpService;
}

namespace UPnPProvider {

typedef STD_FUNCTION<int (const UPnPP::SoapIncoming&, UPnPP::SoapOutgoing&)>
soapfun;

// Definition of a virtual directory entry: data and mime type
struct VDirContent {
    VDirContent(const std::string& ct, const std::string& mt)
        : content(ct), mimetype(mt) {}
    std::string content;
    std::string mimetype;
};

/** Define an interface to link libupnp operations to a device implementation
 */
class UpnpDevice {
public:
    /** Construct device object. Do not start it (this is done by the
     *   eventloop() call when everything is set up).
     *
     * @param deviceId uuid for device: "uuid:UUIDvalue"
     * @param files list of path/content pairs to be added to the
     *   virtual directory root. The file paths must match the SCDPURL
     *   values for the services in the description.xml document.
     *   The file paths should include a sub-directory component.
     *   The list must include the description document, but this will not
     *   be directly served out. Instead a version modified by libupnp
     *  (with URLBase added) will be served from '/'. Of course, the
     *  paths in description.xml must be consistent with the list.
     *
     */
    UpnpDevice(const std::string& deviceId,
               const STD_UNORDERED_MAP<std::string, VDirContent>& files);
    ~UpnpDevice();

    // We only ever keep one instance of a serviceId. Multiple calls will
    // only keep the last one.
    void addService(UpnpService *, const std::string& serviceId);
    void forgetService(const std::string& serviceId);

    /**
     * Add mapping from service+action-name to handler function.
     */
    void addActionMapping(const UpnpService*,
                          const std::string& actName, soapfun);

    /**
     * Main routine. To be called by main() when done with initialization.
     *
     * This loop mostly polls getEventData and generates an UPnP event if
     * there is anything to broadcast. The UPnP action calls happen in
     * other threads with which we synchronize, currently using a global lock.
     */
    void eventloop();

    /**
     * To be called from a service action callback to wake up the
     * event loop early if something needs to be broadcast without
     * waiting for the normal delay.
     *
     * Will only do something if the previous event is not too recent.
     */
    void loopWakeup(); // To trigger an early event

    /**
     * To be called to get the event loop to return
     */
    void shouldExit();

    /** Check status */
    bool ok();

private:
    class Internal;
    Internal *m;
    class InternalStatic;
    static InternalStatic *o;
};

/**
 * Upnp service implementation class. This does not do much useful beyond
 * encapsulating the service actions and event callback. In most cases, the
 * services will need full access to the device state anyway.
 */
class UpnpService {
public:
    UpnpService(const std::string& stp,const std::string& sid, UpnpDevice *dev);
    /**
     * Constructor added to avoid changing the ABI when the noevents
     * parameter was needed.
     *
     * @param noevents if set, the service will function normally except that
     *                 no calls will be made to libupnp to broadcast events.
     *                 This allows a service object to retain its possible
     *                 internal functions without being externally visible
     *                 (in conjunction with a description doc edit).
     */
    UpnpService(const std::string& stp,const std::string& sid,
                UpnpDevice *dev, bool noevents);

    virtual ~UpnpService();

    /**
     * Poll to retrieve evented data changed since last call (see
     * Device::eventLoop).
     *
     * To be implemented by the derived class if it does generate event data.
     * Also called by the library when a control point subscribes, to
     * retrieve eventable data.
     * Return name/value pairs for changed variables in the data arrays.
     *
     * @param all if true, treat all state variable as changed (return
     *            full state)
     * @param names names of returned state variable
     * @param values array parallel to names, holding the values.
     */
    virtual bool getEventData(bool all, std::vector<std::string>& names,
                              std::vector<std::string>& values);
    virtual const std::string& getServiceType() const;
    virtual const std::string& getServiceId() const;

    /** Get value of the noevents property */
    bool noevents();

protected:
    const std::string m_serviceType;
    const std::string m_serviceId;
private:
    class Internal;
    Internal *m;
};

} // End namespace UPnPProvider

#endif /* _DEVICE_H_X_INCLUDED_ */
