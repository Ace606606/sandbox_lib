#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

struct PortConfig
{
     std::string pci_address;
     uint16_t mtu;
     uint8_t socket_id;
};

struct StreamProfile
{
     std::string name;
     uint64_t pps_rate;
     uint64_t packet_size;
};

struct PacketHeader
{
     uint32_t src_ip;
     uint32_t dst_ip;
     std::array< uint8_t, 6 > src_mac;
     std::array< uint8_t, 6 > dst_mac;
};

struct Configuration
{
     PortConfig port_config;
     StreamProfile stream_profile;
     PacketHeader packet_header;
};

bool parse_ipv4( const std::string& ip_str, uint32_t& out_ip )
{
     return ::inet_pton( AF_INET, ip_str.c_str(), &out_ip ) == 1;
}

bool parse_mac( const std::string& mac_str, std::array< uint8_t, 6 >& out_mac )
{
     struct ether_addr temp_mac;
     if( ::ether_aton_r( mac_str.c_str(), &temp_mac ) == nullptr )
     {
          return false;
     }
     std::memcpy( out_mac.data(), &temp_mac, 6 );
     return true;
}

std::optional< Configuration > load_configuration( std::string_view path_to_toml )
{
     try
     {
          auto tbl = toml::parse_file( path_to_toml );
          Configuration config;

          auto pci_opt = tbl[ "port_configuration" ][ "pci_address" ].value< std::string >();
          auto socket_opt = tbl[ "port_configuration" ][ "socket_id" ].value< uint8_t >();
          auto mtu_opt = tbl[ "port_configuration" ][ "mtu" ].value< uint16_t >();

          if( !pci_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'port_configuration.pci_address'\n";
               return std::nullopt;
          }
          if( !socket_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'port_configuration.socket_id'\n";
               return std::nullopt;
          }
          if( !mtu_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'port_configuration.mtu'\n";
               return std::nullopt;
          }

          config.port_config.pci_address = std::move( *pci_opt );
          config.port_config.socket_id = *socket_opt;
          config.port_config.mtu = *mtu_opt;

          auto name_opt = tbl[ "stream_profile" ][ "name" ].value< std::string >();
          auto pps_rate_opt = tbl[ "stream_profile" ][ "pps_rate" ].value< uint64_t >();
          auto packet_size_opt = tbl[ "stream_profile" ][ "packet_size" ].value< uint64_t >();

          if( !name_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'stream_profile.name'\n";
               return std::nullopt;
          }
          if( !pps_rate_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'stream_profile.pps_rate'\n";
               return std::nullopt;
          }
          if( !packet_size_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'stream_profile.packet_size'\n";
               return std::nullopt;
          }

          config.stream_profile.name = std::move( *name_opt );
          config.stream_profile.pps_rate = *pps_rate_opt;
          config.stream_profile.packet_size = *packet_size_opt;

          auto src_ip_opt = tbl[ "packet_header" ][ "src_ip" ].value< std::string >();
          auto dst_ip_opt = tbl[ "packet_header" ][ "dst_ip" ].value< std::string >();
          auto src_mac_opt = tbl[ "packet_header" ][ "src_mac" ].value< std::string >();
          auto dst_mac_opt = tbl[ "packet_header" ][ "dst_mac" ].value< std::string >();

          if( !src_ip_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'packet_header.src_ip'\n";
               return std::nullopt;
          }
          if( !dst_ip_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'packet_header.dst_ip'\n";
               return std::nullopt;
          }
          if( !src_mac_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'packet_header.src_mac'\n";
               return std::nullopt;
          }
          if( !dst_mac_opt )
          {
               std::cerr << "[ERROR] Missing or invalid 'packet_header.dst_mac'\n";
               return std::nullopt;
          }

          if( !parse_ipv4( *src_ip_opt, config.packet_header.src_ip ) )
          {
               std::cerr << "[ERROR] Invalid IPv4 format in 'src_ip'\n";
               return std::nullopt;
          }
          if( !parse_ipv4( *dst_ip_opt, config.packet_header.dst_ip ) )
          {
               std::cerr << "[ERROR] Invalid IPv4 format in 'dst_ip'\n";
               return std::nullopt;
          }
          if( !parse_mac( *src_mac_opt, config.packet_header.src_mac ) )
          {
               std::cerr << "[ERROR] Invalid MAC format in 'src_mac'\n";
               return std::nullopt;
          }
          if( !parse_mac( *dst_mac_opt, config.packet_header.dst_mac ) )
          {
               std::cerr << "[ERROR] Invalid MAC format in 'dst_mac'\n";
               return std::nullopt;
          }

          return config;
     }
     catch( const toml::parse_error& err )
     {
          std::cerr << "[ERROR] TOML syntax error: " << err << '\n';
          return std::nullopt;
     }
     catch( const std::exception& ex )
     {
          std::cerr << "[ERROR] Unexpected error: " << ex.what() << '\n';
          return std::nullopt;
     }
}

int main()
{
     std::optional< Configuration > config_opt = load_configuration( "config.toml" );
     if( !config_opt.has_value() )
     {
          return 1;
     }

     auto config = config_opt.value();

     std::cout << "Port config: " << '\n'
               << config.port_config.pci_address << '\n'
               << static_cast< int >( config.port_config.socket_id ) << '\n'
               << config.port_config.mtu << '\n';
     std::cout << "Stream profile: " << '\n'
               << config.stream_profile.name << '\n'
               << config.stream_profile.pps_rate << '\n'
               << config.stream_profile.packet_size << '\n';

     std::cout << "Packet header:" << '\n' << config.packet_header.src_ip << '\n' << config.packet_header.dst_ip << '\n';

     const auto& src_mac = config.packet_header.src_mac;
     printf( "%02X-%02X-%02X-%02X-%02X-%02X\n", src_mac[ 0 ], src_mac[ 1 ], src_mac[ 2 ], src_mac[ 3 ], src_mac[ 4 ],
          src_mac[ 5 ] );

     const auto& dst_mac = config.packet_header.dst_mac;
     printf( "%02X-%02X-%02X-%02X-%02X-%02X\n", dst_mac[ 0 ], dst_mac[ 1 ], dst_mac[ 2 ], dst_mac[ 3 ], dst_mac[ 4 ],
          dst_mac[ 5 ] );

     return 0;
}