/* Copyright (c) 2012-2015, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 *
 * This file is part of Servus <https://github.com/HBPVIS/Servus>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef SERVUS_SERVUS_H
#define SERVUS_SERVUS_H

#include <servus/result.h> // nested base class
#include <servus/types.h>
#include <servus/servus_impl.h>

// Impls need detail interface definition above
#ifdef SERVUS_USE_DNSSD
#  include "dnssd/servus.h"
#elif defined(SERVUS_USE_AVAHI_CLIENT)
#  include "avahi/servus.h"
#endif
#include "none/servus.h"
#include <map>

namespace servus
{
namespace detail { class Servus; }

/**
 * Simple wrapper for ZeroConf key/value pairs.
 *
 * The servus class allows simple announcement and discovery of key/value pairs
 * using ZeroConf networking. The same instance can be used to announce and/or
 * to browse a ZeroConf service. If the Servus library is compiled without
 * zeroconf support (@sa isAvailable()), this class does not do anything useful.
 *
 * Example: @include tests/servus.cpp
 */ 
class Servus
{
public:
    /**
     * The ZeroConf operation result code.
     *
     * The result code is either one of kDNSServiceErr_ or one of static
     * constants defined by this class
     */
    using Result = servus::Result;

    /** @return true if a usable implementation is available. */
    static constexpr bool isAvailable()
    {
    #if defined(SERVUS_USE_DNSSD) || defined(SERVUS_USE_AVAHI_CLIENT)
        return true;
    #endif
        return false;
    }

    /**
     * Create a new service handle.
     *
     * @param name the service descriptor, e.g., "_hwsd._tcp"
     * @version 1.1
     */
    explicit Servus( const std::string& name )
#ifdef SERVUS_USE_DNSSD
    : _impl( new dnssd::Servus( name ))
#elif defined(SERVUS_USE_AVAHI_CLIENT)
    : _impl( new avahi::Servus( name ))
#else
    : _impl( new none::Servus( name ))
#endif
   {
   }

    /** Destruct this service. @version 1.1 */
    ~Servus()
    {
        delete _impl;
    }

    /** @return the service name. @version 1.1 */
    const std::string& getName() const
    {
        return _impl->getName();
    }

    /**
     * Set a key/value pair to be announced.
     *
     * Keys should be at most eight characters, and values are truncated to 255
     * characters. The total length of all keys and values cannot exceed 65535
     * characters. Setting a value on an announced service causes an update
     * which needs some time to propagate after this function returns, that is,
     * calling discover() immediately afterwards will very likely not contain
     * the new key/value pair.
     *
     * @version 1.1
     */
    void set( const std::string& key, const std::string& value )
    {
        _impl->set( key, value );
    }

    /** @return all (to be) announced keys. @version 1.1 */
    Strings getKeys() const
    {
        return _impl->getKeys();
    }

    /** @return the value to the given (to be) announced key. @version 1.1 */
    const std::string& get( const std::string& key ) const
    {
        return _impl->get( key );
    }

    /**
     * Start announcing the registered key/value pairs.
     *
     * @param port the service IP port in host byte order.
     * @param instance a host-unique instance name, hostname is used if empty.
     * @return the success status of the operation.
     * @version 1.1
     */
    Result announce( const unsigned short port,
                                const std::string& instance )
    {
        return _impl->announce( port, instance );
    }

    /** Stop announcing the registered key/value pairs. @version 1.1 */
    void withdraw()
    {
        _impl->withdraw();
    }

    /** @return true if the local data is announced. @version 1.1 */
    bool isAnnounced() const
    {
        return _impl->isAnnounced();
    }

    /**
     * Discover all announced key/value pairs.
     *
     * @param addr the scope of the discovery
     * @param browseTime the browse time, in milliseconds, to wait for new
     *                   records.
     * @return all instance names found during discovery.
     * @sa beginBrowsing(), browse(), endBrowsing()
     * @version 1.1
     */
    Strings discover( const Interface addr,
                                 const unsigned browseTime )
    {
        return _impl->discover( addr, browseTime );
    }

    /**
     * Begin the discovery of announced key/value pairs.
     *
     * @param addr the scope of the discovery
     * @return the success status of the operation.
     * @version 1.1
     */
    Result beginBrowsing( const servus::Interface addr )
    {
        return _impl->beginBrowsing( addr );
    }

    /**
     * Browse and process discovered key/value pairs.
     *
     * @param timeout The time to spend browsing.
     * @return the success status of the operation.
     * @version 1.1
     */
    Result browse( int32_t timeout = -1 )
    {
        return _impl->browse( timeout );
    }

    /** Stop a discovery process and return all results. @version 1.1 */
    void endBrowsing()
    {
        _impl->endBrowsing();
    }

    /** @return true if the local data is browsing. @version 1.1 */
    bool isBrowsing() const
    {
        return _impl->isBrowsing();
    }

    /** @return all instances found during the last discovery. @version 1.1 */
    Strings getInstances() const
    {
        return _impl->getInstances();
    }

    /** @return all keys discovered on the given instance. @version 1.1 */
    Strings getKeys( const std::string& instance ) const
    {
        return _impl->getKeys( instance );
    }

    /** @return the host corresponding to the given instance. @version 1.3 */
    const std::string& getHost( const std::string& instance ) const
    {
        return get( instance, "servus_host" );
    }

    /** @return true if the given key was discovered. @version 1.1 */
    bool containsKey( const std::string& instance,
                                 const std::string& key ) const
    {
        return _impl->containsKey( instance, key );
    }

    /** @return the value of the given key and instance. @version 1.1 */
    const std::string& get( const std::string& instance,
                                       const std::string& key ) const
    {
        return _impl->get( instance, key );
    }

    /**
     * Add a listener which is invoked according to its supported callbacks.
     *
     * @param listener the listener to be added, must not be nullptr
     * @version 1.2
     */
    void addListener( Listener* listener )
    {
        _impl->addListener( listener );
    }

    /**
     * Remove a listener to stop invokation on its supported callbacks.
     *
     * @param listener the listener to be removed, must not be nullptr
     * @version 1.2
     */
    void removeListener( Listener* listener )
    {
        _impl->removeListener( listener );
    }

    /** @internal */
    void getData( Data& data )
    {
      _impl->getData( data );
    }

private:
    Servus( const Servus& );
    Servus& operator=( const Servus& );
    detail::Servus* const _impl;
    friend std::ostream& operator << ( std::ostream&,
                                                  const Servus& );
};


inline
std::string Result::getString() const
{
  const int32_t code = getCode();
  switch( code )
  {
#ifdef SERVUS_USE_DNSSD
  case kDNSServiceErr_Unknown:           return "unknown error";
  case kDNSServiceErr_NoSuchName:        return "name not found";
  case kDNSServiceErr_NoMemory:          return "out of memory";
  case kDNSServiceErr_BadParam:          return "bad parameter";
  case kDNSServiceErr_BadReference:      return "bad reference";
  case kDNSServiceErr_BadState:          return "bad state";
  case kDNSServiceErr_BadFlags:          return "bad flags";
  case kDNSServiceErr_Unsupported:       return "unsupported";
  case kDNSServiceErr_NotInitialized:    return "not initialized";
  case kDNSServiceErr_AlreadyRegistered: return "already registered";
  case kDNSServiceErr_NameConflict:      return "name conflict";
  case kDNSServiceErr_Invalid:           return "invalid value";
  case kDNSServiceErr_Firewall:          return "firewall";
  case kDNSServiceErr_Incompatible:
    return "client library incompatible with daemon";
  case kDNSServiceErr_BadInterfaceIndex: return "bad interface index";
  case kDNSServiceErr_Refused:           return "refused";
  case kDNSServiceErr_NoSuchRecord:      return "no such record";
  case kDNSServiceErr_NoAuth:            return "no authentication";
  case kDNSServiceErr_NoSuchKey:         return "no such key";
  case kDNSServiceErr_NATTraversal:      return "NAT traversal";
  case kDNSServiceErr_DoubleNAT:         return "double NAT";
  case kDNSServiceErr_BadTime:           return "bad time";
#endif

  case PENDING:          return "operation pending";
  case NOT_SUPPORTED:    return "Servus compiled without ZeroConf support";
  case POLL_ERROR:       return "Error polling for events";
  default:
    if( code > 0 )
      return ::strerror( code );
    return "Unknown error";
  }
}


/** Output the servus instance in human-readable format. */
inline
std::ostream& operator << ( std::ostream& os, const Servus& servus )
{
    os << "Servus instance"
       << (servus.isAnnounced() ? " " : " not ") << "announced"
       << (servus.isBrowsing() ? " " : " not ") << "browsing, implementation"
       << servus._impl->getClassName();

    const Strings& keys = servus.getKeys();
    for( Strings::const_iterator i = keys.begin(); i != keys.end(); ++i )
        os << std::endl << "    " << *i << " = " << servus.get( *i );

    return os;
}

/** Output the servus interface in human-readable format. */
inline
std::ostream& operator << ( std::ostream& os , const servus::Interface& addr )
{
    switch( addr )
    {
    case Interface::IF_ALL: return os << " all ";
    case Interface::IF_LOCAL: return os << " local ";
    }
    return os;
}

}

#endif // SERVUS_SERVUS_H
