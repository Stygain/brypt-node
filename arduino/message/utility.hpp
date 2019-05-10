#ifndef UTILITY_HPP
#define UTILITY_HPP

// #include <string>

// Communication Type Constants
// const int DIRECT = 0;
// const int BLE = 1;
// const int LORA = 2;
// const int WEBSOCKET = 3;

enum DeviceOperation { ROOT, BRANCH, LEAF, NO_OPER };

enum TechnologyType { DIRECT_TYPE, BLE_TYPE, LORA_TYPE, WEBSOCKET_TYPE, TCP_TYPE, STREAMBRIDGE_TYPE, NO_TECH };

enum CommandType { INFORMATION_TYPE, QUERY_TYPE, ELECTION_TYPE, TRANSFORM_TYPE, CONNECT_TYPE, NO_CMD };

enum DeviceSocketCapability { MASTER, SLAVE };

// Super Secure NET_KEY
const String NET_KEY PROGMEM = "01234567890123456789012345678901";
const unsigned int NET_NONCE PROGMEM = 998;

const String ID_SEPERATOR = ";";
/*
// Central Authority Connection Constants
const String CA_DOMAIN PROGMEM = "brypt.com";
const String CA_SUBDOMAIN PROGMEM = "bridge";
const String CA_PORT PROGMEM = "8080";
const String CA_PROTOCOL PROGMEM = "https://";
const unsigned int PORT_GAP PROGMEM = 16;


struct Options {
    bool run_tests;
    TechnologyType technology;
    DeviceOperation operation;
    String id;
    String addr;
    String port;

    String peer_name;
    String peer_addr;
    String peer_port;
};

// inline char * cast_string( String s ) {
// 	char * cs = new char[ s.length() ];
// 	memset( cs, '\0', s.length() );
// 	strcpy( cs, s.c_str() );
//
// 	return cs;
// }
*/
#endif
