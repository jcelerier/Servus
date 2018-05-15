
/* Copyright (c) 2012-2016, Stefan Eilemann <eile@eyescale.ch>
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

#include <servus/listener.h>

#include <cstring>
#include <map>
#include <unordered_set>

// for NI_MAXHOST
#ifdef _WIN32
#  include <ws2tcpip.h>
#else
#  include <netdb.h>
#  include <unistd.h>
#endif

namespace servus
{
#define ANNOUNCE_TIMEOUT 1000 /*ms*/

namespace detail
{
static const std::string empty_;
typedef std::map< std::string, std::string > ValueMap;
typedef std::map< std::string, ValueMap > InstanceMap;
typedef ValueMap::const_iterator ValueMapCIter;
typedef InstanceMap::const_iterator InstanceMapCIter;
typedef std::unordered_set< Listener* > Listeners;

class Servus
{
public:
    explicit Servus( const std::string& name ) : _name( name ) {}
    virtual ~Servus() {}

    virtual std::string getClassName() const = 0;

    const std::string& getName() const { return _name; }

    void set( const std::string& key, const std::string& value )
    {
        _data[ key ] = value;
        _updateRecord();
    }

    Strings getKeys() const
    {
        Strings keys;
        for( ValueMapCIter i = _data.begin(); i != _data.end(); ++i )
            keys.push_back( i->first );
        return keys;
    }

    const std::string& get( const std::string& key ) const
    {
        ValueMapCIter i = _data.find( key );
        if( i != _data.end( ))
            return i->second;
        return empty_;
    }

    virtual servus::Result announce( const unsigned short port,
                                             const std::string& instance ) =0;
    virtual void withdraw() = 0;
    virtual bool isAnnounced() const = 0;

    virtual servus::Result beginBrowsing(
        const servus::Interface interface_ ) = 0;
    virtual servus::Result browse( const int32_t timeout ) = 0;

    virtual void endBrowsing() = 0;
    virtual bool isBrowsing() const = 0;
    virtual Strings discover( const servus::Interface interface_,
                              const unsigned browseTime ) = 0;

    Strings getInstances() const
    {
        Strings instances;
        for( InstanceMapCIter i = _instanceMap.begin();
             i != _instanceMap.end(); ++i )
        {
            instances.push_back( i->first );
        }
        return instances;
    }

    Strings getKeys( const std::string& instance ) const
    {
        Strings keys;
        InstanceMapCIter i = _instanceMap.find( instance );
        if( i == _instanceMap.end( ))
            return keys;

        const ValueMap& values = i->second;
        for( ValueMapCIter j = values.begin(); j != values.end(); ++j )
            keys.push_back( j->first );
        return keys;
    }

    bool containsKey( const std::string& instance, const std::string& key )
        const
    {
        InstanceMapCIter i = _instanceMap.find( instance );
        if( i == _instanceMap.end( ))
            return false;

        const ValueMap& values = i->second;
        ValueMapCIter j = values.find( key );
        if( j == values.end( ))
            return false;
        return true;
    }

    const std::string& get( const std::string& instance,
                            const std::string& key ) const
    {
        InstanceMapCIter i = _instanceMap.find( instance );
        if( i == _instanceMap.end( ))
            return detail::empty_;

        const ValueMap& values = i->second;
        ValueMapCIter j = values.find( key );
        if( j == values.end( ))
            return detail::empty_;
        return j->second;
    }

    void addListener( Listener* listener )
    {
        if( listener )
            _listeners.insert( listener );
    }

    void removeListener( Listener* listener )
    {
        if( listener )
             _listeners.erase( listener );
    }

    void getData( servus::Data& data ) const
    {
        data = _instanceMap;
    }

protected:
    const std::string _name;
    InstanceMap _instanceMap; //!< last discovered data
    ValueMap _data;   //!< self data to announce
    Listeners _listeners;

    virtual void _updateRecord() = 0;
};

}
/** @return the local hostname. */
inline
std::string getHostname()
{
    char hostname[NI_MAXHOST+1] = {0};
    gethostname( hostname, NI_MAXHOST );
    hostname[NI_MAXHOST] = '\0';
    return std::string( hostname );
}
}

