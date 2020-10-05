/* Copyright (c) 2014-2016, Stefan.Eilemann@epfl.ch
 *                          Juan Hernando <jhernando@fi.upm.es>
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

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <stdexcept>

#include <cassert>
#include <mutex>

#include <dlfcn.h>
using ScopedLock = std::unique_lock< std::mutex >;
namespace chrono = std::chrono;

#define WARN std::cerr << __FILE__ << ":" << __LINE__ << ": "

// http://stackoverflow.com/questions/14430906
//   Proper way of doing this is using the threaded polling in avahi
namespace
{
static std::mutex _mutex;

int64_t _elapsedMilliseconds(
    const chrono::high_resolution_clock::time_point& startTime )
{
  const chrono::high_resolution_clock::time_point& endTime =
      chrono::high_resolution_clock::now();
  return chrono::nanoseconds( endTime - startTime ).count() / 1000000;
}

}

namespace servus
{
namespace avahi
{
namespace
{

class dylib_loader
{
public:
  explicit dylib_loader(const char* const so)
  {
    impl = dlopen(so, RTLD_LAZY | RTLD_LOCAL | RTLD_NODELETE);
  }

  dylib_loader(const dylib_loader&) noexcept = delete;
  dylib_loader& operator=(const dylib_loader&) noexcept = delete;
  dylib_loader(dylib_loader&& other)
  {
    impl = other.impl;
    other.impl = nullptr;
  }

  dylib_loader& operator=(dylib_loader&& other) noexcept
  {
    impl = other.impl;
    other.impl = nullptr;
    return *this;
  }

  ~dylib_loader()
  {
    if (impl)
    {
      dlclose(impl);
    }
  }

  template <typename T>
  T symbol(const char* const sym) const noexcept
  {
    return (T)dlsym(impl, sym);
  }

  operator bool() const { return bool(impl); }

private:
  void* impl{};
};

class libavahi
{
public:
  decltype(&::avahi_strerror) strerror{};

  decltype(&::avahi_simple_poll_new) simple_poll_new{};
  decltype(&::avahi_simple_poll_get) simple_poll_get{};
  decltype(&::avahi_simple_poll_quit) simple_poll_quit{};
  decltype(&::avahi_simple_poll_free) simple_poll_free{};
  decltype(&::avahi_simple_poll_iterate) simple_poll_iterate{};

  decltype(&::avahi_client_new) client_new{};
  decltype(&::avahi_client_free) client_free{};
  decltype(&::avahi_client_errno) client_errno{};

  decltype(&::avahi_entry_group_new) entry_group_new{};
  decltype(&::avahi_entry_group_reset) entry_group_reset{};
  decltype(&::avahi_entry_group_is_empty) entry_group_is_empty{};
  decltype(&::avahi_entry_group_commit) entry_group_commit{};
  decltype(&::avahi_entry_group_add_service_strlst) entry_group_add_service_strlst{};

  decltype(&::avahi_service_browser_new) service_browser_new{};
  decltype(&::avahi_service_browser_free) service_browser_free{};

  decltype(&::avahi_service_resolver_new) service_resolver_new{};
  decltype(&::avahi_service_resolver_free) service_resolver_free{};

  decltype(&::avahi_string_list_add_pair) string_list_add_pair{};
  decltype(&::avahi_string_list_free) string_list_free{};

  static const libavahi& instance() {
    static const libavahi self;
    return self;
  }

private:
  dylib_loader library;

  libavahi()
    : library("libavahi-client.so")
  {
    strerror = library.symbol<decltype(&::avahi_strerror)>("avahi_strerror");

    simple_poll_new = library.symbol<decltype(&::avahi_simple_poll_new)>("avahi_simple_poll_new");
    simple_poll_get = library.symbol<decltype(&::avahi_simple_poll_get)>("avahi_simple_poll_get");
    simple_poll_quit = library.symbol<decltype(&::avahi_simple_poll_quit)>("avahi_simple_poll_quit");
    simple_poll_free = library.symbol<decltype(&::avahi_simple_poll_free)>("avahi_simple_poll_free");
    simple_poll_iterate = library.symbol<decltype(&::avahi_simple_poll_iterate)>("avahi_simple_poll_iterate");

    client_new = library.symbol<decltype(&::avahi_client_new)>("avahi_client_new");
    client_free = library.symbol<decltype(&::avahi_client_free)>("avahi_client_free");
    client_errno = library.symbol<decltype(&::avahi_client_errno)>("avahi_client_errno");

    entry_group_new = library.symbol<decltype(&::avahi_entry_group_new)>("avahi_entry_group_new");
    entry_group_reset = library.symbol<decltype(&::avahi_entry_group_reset)>("avahi_entry_group_reset");
    entry_group_is_empty = library.symbol<decltype(&::avahi_entry_group_is_empty)>("avahi_entry_group_is_empty");
    entry_group_commit = library.symbol<decltype(&::avahi_entry_group_commit)>("avahi_entry_group_commit");
    entry_group_add_service_strlst = library.symbol<decltype(&::avahi_entry_group_add_service_strlst)>("avahi_entry_group_add_service_strlst");

    service_browser_new = library.symbol<decltype(&::avahi_service_browser_new)>("avahi_service_browser_new");
    service_browser_free = library.symbol<decltype(&::avahi_service_browser_free)>("avahi_service_browser_free");

    service_resolver_new = library.symbol<decltype(&::avahi_service_resolver_new)>("avahi_service_resolver_new");
    service_resolver_free = library.symbol<decltype(&::avahi_service_resolver_free)>("avahi_service_resolver_free");

    string_list_add_pair = library.symbol<decltype(&::avahi_string_list_add_pair)>("avahi_string_list_add_pair");
    string_list_free = library.symbol<decltype(&::avahi_string_list_free)>("avahi_string_list_free");

    assert(simple_poll_new);
    assert(client_new);
    assert(simple_poll_get);
    assert(strerror);
    assert(client_free);
    assert(simple_poll_free);
    assert(simple_poll_iterate);
    assert(entry_group_reset);
    assert(entry_group_is_empty);
    assert(service_browser_new);
    assert(client_errno);
    assert(service_browser_free);
    assert(simple_poll_quit);
    assert(service_resolver_new);
    assert(service_resolver_free);
    assert(entry_group_new);
    assert(string_list_add_pair);
    assert(entry_group_add_service_strlst);
    assert(string_list_free);
    assert(entry_group_commit);
  }
};


AvahiSimplePoll* _newSimplePoll()
{
  ScopedLock lock( _mutex );
  return libavahi::instance().simple_poll_new();
}
}

class Servus final : public detail::Servus
{
  const libavahi& avahi = libavahi::instance();
public:
  explicit Servus( const std::string& name )
    : detail::Servus( name )
    , _poll( _newSimplePoll( ))
    , _client( 0 )
    , _browser( 0 )
    , _group( 0 )
    , _result( servus::Result::PENDING )
    , _port( 0 )
    , _announcable( false )
    , _scope( servus::Interface::IF_ALL )
  {
    if( !_poll )
      throw std::runtime_error( "Can't setup avahi poll device" );

    int error = 0;
    ScopedLock lock( _mutex );
    _client = avahi.client_new( avahi.simple_poll_get( _poll ),
                                (AvahiClientFlags)(0), _clientCBS, this,
                                &error );
    if( !_client )
      throw std::runtime_error(
          std::string( "Can't setup avahi client: " ) +
          avahi.strerror( error ));
  }

  virtual ~Servus()
  {
    withdraw();
    endBrowsing();

    ScopedLock lock( _mutex );
    if( _client )
      avahi.client_free( _client );
    if( _poll )
      avahi.simple_poll_free( _poll );
  }

  std::string getClassName() const override { return "avahi"; }

  servus::Result announce( const unsigned short port,
                                   const std::string& instance ) final override
  {

    ScopedLock lock( _mutex );

    _result = servus::Result::PENDING;
    _port = port;
    if( instance.empty( ))
      _announce = getHostname();
    else
      _announce = instance;

    if( _announcable )
      _createServices();
    else
    {
      const chrono::high_resolution_clock::time_point& startTime =
          chrono::high_resolution_clock::now();
      while( !_announcable &&
             _result == servus::Result::PENDING &&
             _elapsedMilliseconds( startTime ) < ANNOUNCE_TIMEOUT )
      {
        avahi.simple_poll_iterate( _poll, ANNOUNCE_TIMEOUT );
      }
    }

    return servus::Result( _result );
  }

  void withdraw() final override
  {
    ScopedLock lock( _mutex );
    _announce.clear();
    _port = 0;
    if( _group )
      avahi.entry_group_reset( _group );
  }

  bool isAnnounced() const final override
  {
    ScopedLock lock( _mutex );
    return ( _group && !avahi.entry_group_is_empty( _group ));
  }

  servus::Result beginBrowsing(
      const ::servus::Interface addr ) final override
  {
    if( _browser )
      return servus::Result( servus::Result::PENDING );

    ScopedLock lock( _mutex );
    _scope = addr;
    _instanceMap.clear();
    _result = servus::Result::SUCCESS;
    _browser = avahi.service_browser_new( _client, AVAHI_IF_UNSPEC,
                                          AVAHI_PROTO_UNSPEC, _name.c_str(),
                                          0, (AvahiLookupFlags)(0),
                                          _browseCBS, this );
    if( _browser )
      return servus::Result( _result );

    _result = avahi.client_errno( _client );
    WARN << "Failed to create browser for " << _name << ": "
         << avahi.strerror( _result ) << std::endl;
    return servus::Result( _result );
  }

  servus::Result browse( const int32_t timeout ) final override
  {
    ScopedLock lock( _mutex );
    _result = servus::Result::PENDING;
    const chrono::high_resolution_clock::time_point& startTime =
        chrono::high_resolution_clock::now();

    size_t nErrors = 0;
    do
    {
      if( avahi.simple_poll_iterate( _poll, timeout ) != 0 )
      {
        if( ++nErrors < 10 )
          continue;

        _result = servus::Result::POLL_ERROR;
        break;
      }
    }
    while( _elapsedMilliseconds( startTime ) < timeout );

    if( _result != servus::Result::POLL_ERROR )
      _result = servus::Result::SUCCESS;

    return servus::Result( _result );
  }

  void endBrowsing() final override
  {
    ScopedLock lock( _mutex );
    if( _browser )
      avahi.service_browser_free( _browser );
    _browser = 0;
  }

  bool isBrowsing() const final override { return _browser; }

  Strings discover( const ::servus::Interface addr,
                    const unsigned browseTime ) final override
  {
    const servus::Result& result = beginBrowsing( addr );
    if( !result && result != servus::Result::PENDING )
      return getInstances();

    assert( _browser );
    browse( browseTime );
    if( result != servus::Result::PENDING )
      endBrowsing();
    return getInstances();
  }

private:
  AvahiSimplePoll* const _poll;
  AvahiClient* _client;
  AvahiServiceBrowser* _browser;
  AvahiEntryGroup* _group;
  int32_t _result;
  std::string _announce;
  unsigned short _port;
  bool _announcable;
  servus::Interface _scope;

  // Client state change
  static void _clientCBS( AvahiClient*, AvahiClientState state,
                          void* servus )
  {
    ((Servus*)servus)->_clientCB( state );
  }

  void _clientCB( AvahiClientState state )
  {
    switch (state)
    {
      case AVAHI_CLIENT_S_RUNNING:
        _announcable = true;
        if( !_announce.empty( ))
          _createServices();
        break;

      case AVAHI_CLIENT_FAILURE:
        _result = avahi.client_errno( _client );
        WARN << "Client failure: " << avahi.strerror( _result )
             << std::endl;
        avahi.simple_poll_quit( _poll );
        break;

      case AVAHI_CLIENT_S_COLLISION:
        // Can't setup client
        _result = EEXIST;
        avahi.simple_poll_quit( _poll );
        break;

      case AVAHI_CLIENT_S_REGISTERING:
        // The server records are now being established. This might be
        // caused by a host name change. We need to wait for our own records
        // to register until the host name is properly established.
        throw std::runtime_error(
              "Unimplemented AVAHI_CLIENT_S_REGISTERING event" );
        // withdraw & _createServices ?
        break;

      case AVAHI_CLIENT_CONNECTING:
        /*nop*/;
    }
  }

  // Browsing
  static void _browseCBS( AvahiServiceBrowser*, AvahiIfIndex ifIndex,
                          AvahiProtocol protocol, AvahiBrowserEvent event,
                          const char* name, const char* type,
                          const char* domain, AvahiLookupResultFlags,
                          void* servus )
  {
    ((Servus*)servus)->_browseCB( ifIndex, protocol, event, name, type,
                                  domain );
  }

  void _browseCB( const AvahiIfIndex ifIndex, const AvahiProtocol protocol,
                  const AvahiBrowserEvent event, const char* name,
                  const char* type, const char* domain )
  {
    switch( event )
    {
      case AVAHI_BROWSER_FAILURE:
      {
        _result = avahi.client_errno( _client );
        WARN << "Browser failure: " << avahi.strerror( _result )
             << std::endl;
        avahi.simple_poll_quit( _poll );
        break;
      }

      case AVAHI_BROWSER_NEW:
      {
        // We ignore the returned resolver object. In the callback function
        // we free it. If the server is terminated before the callback
        // function is called the server will free the resolver for us.
        if( !avahi.service_resolver_new( _client, ifIndex, protocol, name,
                                         type, domain, AVAHI_PROTO_UNSPEC,
                                         (AvahiLookupFlags)(0),
                                         _resolveCBS, this ))
        {
          _result = avahi.client_errno( _client );
          WARN << "Error creating resolver: "
               << avahi.strerror( _result ) << std::endl;
          avahi.simple_poll_quit( _poll );
        }
        break;
      }

      case AVAHI_BROWSER_REMOVE:
      {
        _instanceMap.erase( name );
        for( detail::Listeners::iterator i = _listeners.begin();
             i != _listeners.end(); ++i )
        {
          (*i)->instanceRemoved( name );
        }
        break;
      }

      case AVAHI_BROWSER_ALL_FOR_NOW:
      case AVAHI_BROWSER_CACHE_EXHAUSTED:
        _result = servus::Result::SUCCESS;
        break;
    }
  }

  static void _resolveCBS( AvahiServiceResolver* resolver,
                           AvahiIfIndex, AvahiProtocol,
                           AvahiResolverEvent event, const char* name,
                           const char*, const char*,
                           const char* host, const AvahiAddress* addr,
                           uint16_t port, AvahiStringList *txt,
                           AvahiLookupResultFlags flags, void* servus )
  {
    ((Servus*)servus)->_resolveCB( resolver, event, name, host, addr, port, txt, flags );
  }

  void _resolveCB( AvahiServiceResolver* resolver,
                   const AvahiResolverEvent event, const char* name,
                   const char* host, const AvahiAddress* addr, uint16_t port, AvahiStringList *txt,
                   const AvahiLookupResultFlags flags)
  {
    // If browsing through the local interface,
    // consider only the local instances
    if(_scope == servus::Interface::IF_LOCAL &&
       !(flags & AVAHI_LOOKUP_RESULT_LOCAL) )
      return;

    switch( event )
    {
      case AVAHI_RESOLVER_FAILURE:
        _result = avahi.client_errno( _client );
        WARN << "Resolver error: " << avahi.strerror( _result )
             << std::endl;
        break;

      case AVAHI_RESOLVER_FOUND:
      {
        detail::ValueMap& values = _instanceMap[ name ];
        if(addr->proto == AVAHI_PROTO_INET)
        {
          values[ "servus_proto" ] = "ipv4";

          struct in_addr ip_addr;
          ip_addr.s_addr = addr->data.ipv4.address;
          values[ "servus_ip" ] = inet_ntoa(ip_addr);
        }
        else if(addr->proto == AVAHI_PROTO_INET6)
        {
          values[ "servus_proto" ] = "ipv6";
          /*
          values[ "servus_ip" ] = addr->data.ipv6.address;
          struct in_addr ip_addr;
          ip_addr.s_addr = addr->data.ipv6.address;
          values[ "servus_ip" ] = inet_ntop(addr);
          */
        }

        values[ "servus_host" ] = host;
        values[ "servus_port" ] = std::to_string((int)port);
        for( ; txt; txt = txt->next )
        {
          const std::string entry(
                reinterpret_cast< const char* >( txt->text ),
                txt->size );
          const size_t pos = entry.find_first_of( "=" );
          const std::string key = entry.substr( 0, pos );
          const std::string value = entry.substr( pos + 1 );
          values[ key ] = value;
        }
        for( detail::Listeners::iterator i = _listeners.begin();
             i != _listeners.end(); ++i )
        {
          (*i)->instanceAdded( name );
        }
      } break;
    }

    avahi.service_resolver_free( resolver );
  }

  void _updateRecord() final override
  {
    if( _announce.empty() || !_announcable )
      return;

    if( _group )
      avahi.entry_group_reset( _group );
    _createServices();
  }

  void _createServices()
  {
    if( !_group )
      _group = avahi.entry_group_new( _client, _groupCBS, this );
    else
      avahi.entry_group_reset( _group );

    if( !_group )
      return;

    AvahiStringList* data = 0;
    for( detail::ValueMapCIter i = _data.begin(); i != _data.end(); ++i )
      data = avahi.string_list_add_pair( data, i->first.c_str(),
                                         i->second.c_str( ));

    _result = avahi.entry_group_add_service_strlst(
          _group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
          (AvahiPublishFlags)(0), _announce.c_str(), _name.c_str(), 0, 0,
          _port, data );

    if( data )
      avahi.string_list_free( data );

    if( _result != servus::Result::SUCCESS )
    {
      avahi.simple_poll_quit( _poll );
      return;
    }

    _result = avahi.entry_group_commit( _group );
    if( _result != servus::Result::SUCCESS )
      avahi.simple_poll_quit( _poll );
  }

  static void _groupCBS( AvahiEntryGroup*, AvahiEntryGroupState state,
                         void* servus )
  {
    ((Servus*)servus)->_groupCB( state );
  }

  void _groupCB( const AvahiEntryGroupState state )
  {
    switch( state )
    {
      case AVAHI_ENTRY_GROUP_ESTABLISHED:
        break;

      case AVAHI_ENTRY_GROUP_COLLISION:
      case AVAHI_ENTRY_GROUP_FAILURE:
        _result = EEXIST;
        avahi.simple_poll_quit( _poll );
        break;

      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
        /*nop*/ ;
    }
  }
};

}
}
