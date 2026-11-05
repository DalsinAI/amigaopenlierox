/////////////////////////////////////////
//
//   OpenLieroX for AmigaOS: networking
//
//   code under LGPL, like OpenLieroX
//
/////////////////////////////////////////

// The Amiga port has no network play, no server list and no HawkNL. But
// OpenLieroX plays a local game through its network code: the menu starts
// a server on a UDP port and the client connects to 127.0.0.1 there. So
// this file replaces src/common/Networking.cpp with UDP sockets that live
// in memory: a datagram written to 127.0.0.1:port is queued on the socket
// bound to that port in this program, and a Read() takes it off again and
// makes its sender the socket's remote address, as HawkNL's UDP does.
// Datagrams to any other address are dropped as if sent. TCP, broadcast
// and name lookups fail at once, as on a machine with no network.
//
// The AmiCradle Team.

#include <string.h>
#include <stdio.h>
#include <deque>
#include <map>
#include <string>

#include "LieroX.h"
#include "Debug.h"
#include "Networking.h"
#include "StringUtils.h"
#include "Mutex.h"

namespace {

struct LoopAddr {
	bool valid;
	unsigned long ip; // host order
	unsigned short port;
	LoopAddr() : valid(false), ip(0), port(0) {}
};

const unsigned long LOOPBACK_IP = 0x7F000001UL;

enum {
	ERR_NONE = 0,
	ERR_MESSAGE_END = 1,
	ERR_NO_NETWORK = 2,
	ERR_PORT_IN_USE = 3,
	ERR_BAD_ADDR = 4
};

int lastError = ERR_NONE;

struct Datagram {
	std::string data;
	unsigned short fromPort;
};

struct Endpoint {
	NetworkSocket* owner;
	unsigned short port;
	LoopAddr remote;
	std::deque<Datagram> queue;
};

Mutex* netMutex = NULL;
std::map<unsigned short, Endpoint*> boundPorts;
unsigned short nextEphemeral = 40000;

Mutex& lockObj() {
	if(!netMutex) netMutex = new Mutex();
	return *netMutex;
}

unsigned short allocPort(unsigned short wanted) {
	if(wanted != 0)
		return boundPorts.count(wanted) ? 0 : wanted;
	for(int tries = 0; tries < 20000; tries++) {
		unsigned short p = nextEphemeral++;
		if(nextEphemeral >= 60000) nextEphemeral = 40000;
		if(!boundPorts.count(p)) return p;
	}
	return 0;
}

} // namespace


struct NetworkSocket::InternSocket {
	Endpoint* ep;
	InternSocket() : ep(NULL) {}
};

struct NetworkSocket::EventHandler {};


// ---------------- addresses ----------------

bool AreNetworkAddrEqual(const NetworkAddr& addr1, const NetworkAddr& addr2);
DECLARE_INTERNDATA_CLASS( NetworkAddr, LoopAddr );

static LoopAddr& la(NetworkAddr& a) { return NetworkAddrData(a); }
static const LoopAddr& la(const NetworkAddr& a) { return NetworkAddrData(a); }

bool IsNetAddrValid(const NetworkAddr& addr) { return la(addr).valid; }
bool IsNetAddrAvailable(const NetworkAddr& addr) { return la(addr).valid && la(addr).ip == LOOPBACK_IP; }
bool SetNetAddrValid(NetworkAddr& addr, bool valid) { la(addr).valid = valid; return true; }
void ResetNetAddr(NetworkAddr& addr) { la(addr) = LoopAddr(); }
bool IsNetAddrV6(const NetworkAddr&) { return false; }
bool IsNetAddrV6(const std::string& s) { return s.size() > 0 && s[0] == '['; }

// accepts "%i.%i.%i.%i[:%i]"
bool StringToNetAddr(const std::string& string, NetworkAddr& addr, std::string* errorStr) {
	unsigned int a, b, c, d, port = 0;
	char tail = 0;
	std::string s = string;
	TrimSpaces(s);
	int n = sscanf(s.c_str(), "%u.%u.%u.%u:%u%c", &a, &b, &c, &d, &port, &tail);
	if(n < 4 || n == 6 || a > 255 || b > 255 || c > 255 || d > 255 || port > 65535) {
		la(addr).valid = false;
		lastError = ERR_BAD_ADDR;
		if(errorStr) *errorStr = "not an IPv4 address";
		return false;
	}
	if(n == 4) {
		// "a.b.c.d" alone, or followed by something that isn't ":port"
		char buf[32];
		snprintf(buf, sizeof(buf), "%u.%u.%u.%u", a, b, c, d);
		if(s != buf) {
			la(addr).valid = false;
			lastError = ERR_BAD_ADDR;
			if(errorStr) *errorStr = "not an IPv4 address";
			return false;
		}
	}
	la(addr).valid = true;
	la(addr).ip = ((unsigned long)a << 24) | ((unsigned long)b << 16) | ((unsigned long)c << 8) | d;
	la(addr).port = (unsigned short)port;
	return true;
}

NetworkAddr StringToNetAddr(const std::string& string) {
	NetworkAddr ret;
	StringToNetAddr(string, ret);
	return ret;
}

bool NetAddrToString(const NetworkAddr& addr, std::string& string) {
	const LoopAddr& l = la(addr);
	char buf[32];
	snprintf(buf, sizeof(buf), "%lu.%lu.%lu.%lu:%u",
		(l.ip >> 24) & 255, (l.ip >> 16) & 255, (l.ip >> 8) & 255, l.ip & 255, (unsigned)l.port);
	string = buf;
	return true;
}

std::string NetAddrToString(const NetworkAddr& addr) {
	std::string ret;
	NetAddrToString(addr, ret);
	return ret;
}

unsigned short GetNetAddrPort(const NetworkAddr& addr) { return la(addr).port; }

bool SetNetAddrPort(NetworkAddr& addr, unsigned short port, std::string*) {
	la(addr).port = port;
	return true;
}

bool AreNetAddrEqual(const NetworkAddr& addr1, const NetworkAddr& addr2) {
	const LoopAddr& a = la(addr1);
	const LoopAddr& b = la(addr2);
	if(!a.valid || !b.valid) return a.valid == b.valid;
	return a.ip == b.ip && a.port == b.port;
}

bool AreNetworkAddrEqual(const NetworkAddr& addr1, const NetworkAddr& addr2) {
	return AreNetAddrEqual(addr1, addr2);
}

// No name lookups: only numeric addresses resolve.
bool GetNetAddrFromNameAsync(const std::string& name) {
	NetworkAddr addr;
	return StringToNetAddr(name, addr);
}

bool GetFromDnsCache(const std::string& name, NetworkAddr& addr4, NetworkAddr& addr6) {
	ResetNetAddr(addr6);
	if(StringToNetAddr(name, addr4)) return true;
	ResetNetAddr(addr4);
	return false;
}

Event<> onDnsReady;


// ---------------- system ----------------

bool InitNetworkSystem() {
	lockObj();
	notes << "Networking: local (loopback) games only in the Amiga port" << endl;
	return true;
}

bool QuitNetworkSystem() {
	return true;
}

int GetSocketErrorNr() { return lastError; }

std::string GetSocketErrorStr(int errnr) {
	switch(errnr) {
		case ERR_NONE: return "no error";
		case ERR_MESSAGE_END: return "no more data";
		case ERR_NO_NETWORK: return "no network in the Amiga port";
		case ERR_PORT_IN_USE: return "port in use";
		case ERR_BAD_ADDR: return "bad address";
	}
	return "unknown error";
}

std::string GetLastErrorStr() { return GetSocketErrorStr(lastError); }
bool IsMessageEndSocketErrorNr(int errnr) { return errnr == ERR_MESSAGE_END; }
void ResetSocketError() { lastError = ERR_NONE; }


// ---------------- sockets ----------------

NetworkSocket::NetworkSocket() : m_type(NST_INVALID), m_state(NSS_NONE), m_withEvents(false), m_socket(NULL) {
	m_socket = new InternSocket();
}

NetworkSocket::~NetworkSocket() {
	Clear();
	delete m_socket;
	m_socket = NULL;
}

void NetworkSocket::swap(NetworkSocket& sock) {
	Mutex::ScopedLock lock(lockObj());
	std::swap(m_type, sock.m_type);
	std::swap(m_state, sock.m_state);
	std::swap(m_withEvents, sock.m_withEvents);
	std::swap(m_socket, sock.m_socket);
	if(m_socket && m_socket->ep) m_socket->ep->owner = this;
	if(sock.m_socket && sock.m_socket->ep) sock.m_socket->ep->owner = &sock;
}

// Events are pushed when a datagram arrives (see Write); nothing runs here.
void NetworkSocket::setWithEvents(bool v) { m_withEvents = v; }
void NetworkSocket::checkEventHandling() {}

std::string NetworkSocket::debugString() const {
	if(!isOpen()) return "Closed";
	return TypeStr(m_type) + "/" + StateStr(m_state) + " loopback port " + itoa(localPort());
}

bool NetworkSocket::OpenReliable(Port) {
	lastError = ERR_NO_NETWORK;
	return false;
}


bool NetworkSocket::OpenUnreliable(Port port) {
	if(isOpen()) Close();
	Mutex::ScopedLock lock(lockObj());
	unsigned short p = allocPort(port);
	if(p == 0) {
		lastError = ERR_PORT_IN_USE;
		return false;
	}
	Endpoint* ep = new Endpoint();
	ep->owner = this;
	ep->port = p;
	boundPorts[p] = ep;
	m_socket->ep = ep;
	m_type = NST_UDP;
	m_state = NSS_NONE;
	return true;
}

// The menu won't start without its LAN socket: a broadcast socket is a
// UDP one here, and its broadcasts (to a LAN address) go nowhere.
bool NetworkSocket::OpenBroadcast(Port port) {
	if(!OpenUnreliable(port)) return false;
	m_type = NST_UDPBROADCAST;
	return true;
}

bool NetworkSocket::Connect(const NetworkAddr&) {
	lastError = ERR_NO_NETWORK;
	return false;
}

// HawkNL lets a UDP socket "listen" (the server does); it changes nothing.
bool NetworkSocket::Listen() {
	if(!isOpen() || m_type == NST_TCP) {
		lastError = ERR_NO_NETWORK;
		return false;
	}
	m_state = NSS_LISTENING;
	return true;
}

void NetworkSocket::Close() {
	if(!isOpen()) return;
	Mutex::ScopedLock lock(lockObj());
	Endpoint* ep = m_socket->ep;
	if(ep) {
		boundPorts.erase(ep->port);
		delete ep;
	}
	m_socket->ep = NULL;
	m_type = NST_INVALID;
	m_state = NSS_NONE;
}

bool NetworkSocket::isReady() const {
	return isOpen() && m_socket->ep != NULL;
}

int NetworkSocket::Write(const void* buffer, int nbytes) {
	if(!isOpen() || !m_socket->ep) {
		lastError = ERR_NO_NETWORK;
		return -1;
	}
	NetworkSocket* wake = NULL;
	{
		Mutex::ScopedLock lock(lockObj());
		Endpoint* ep = m_socket->ep;
		if(!ep->remote.valid) {
			lastError = ERR_BAD_ADDR;
			return -1;
		}
		if(ep->remote.ip == LOOPBACK_IP) {
			std::map<unsigned short, Endpoint*>::iterator it = boundPorts.find(ep->remote.port);
			if(it != boundPorts.end() && it->second->queue.size() < 1024) {
				Datagram d;
				d.data.assign((const char*)buffer, nbytes);
				d.fromPort = ep->port;
				it->second->queue.push_back(d);
				if(it->second->owner && it->second->owner->withEvents())
					wake = it->second->owner;
			}
		}
	}
	// Events go to the main loop's queue, which is ours (one thread).
	if(wake) wake->OnNewData.pushToMainQueue(EventData(wake));
	return nbytes;
}

int NetworkSocket::Read(void* buffer, int nbytes) {
	if(!isOpen() || !m_socket->ep) {
		lastError = ERR_NO_NETWORK;
		return -1;
	}
	Mutex::ScopedLock lock(lockObj());
	Endpoint* ep = m_socket->ep;
	if(ep->queue.empty()) {
		lastError = ERR_MESSAGE_END;
		return -1;
	}
	Datagram& d = ep->queue.front();
	int n = (int)d.data.size();
	if(n > nbytes) n = nbytes; // UDP: the rest of the datagram is lost
	memcpy(buffer, d.data.data(), n);
	ep->remote.valid = true;
	ep->remote.ip = LOOPBACK_IP;
	ep->remote.port = d.fromPort;
	ep->queue.pop_front();
	return n;
}

bool NetworkSocket::isDataAvailable() {
	if(!isOpen() || !m_socket->ep) return false;
	Mutex::ScopedLock lock(lockObj());
	return !m_socket->ep->queue.empty();
}

void NetworkSocket::WaitForSocketWrite(int) {}

void NetworkSocket::WaitForSocketRead(int timeout) {
	Uint32 start = SDL_GetTicks();
	while(!isDataAvailable() && timeout > 0 && SDL_GetTicks() - start < (Uint32)timeout)
		SDL_Delay(2);
}

void NetworkSocket::WaitForSocketReadOrWrite(int) {}

NetworkSocket::Port NetworkSocket::localPort() const {
	return (isOpen() && m_socket->ep) ? m_socket->ep->port : 0;
}

NetworkSocket::Port NetworkSocket::remotePort() const {
	return (isOpen() && m_socket->ep) ? m_socket->ep->remote.port : 0;
}

NetworkAddr NetworkSocket::localAddress() const {
	NetworkAddr addr;
	if(isOpen() && m_socket->ep) {
		la(addr).valid = true;
		la(addr).ip = LOOPBACK_IP;
		la(addr).port = m_socket->ep->port;
	}
	return addr;
}

NetworkAddr NetworkSocket::remoteAddress() const {
	NetworkAddr addr;
	if(isOpen() && m_socket->ep) {
		Mutex::ScopedLock lock(lockObj());
		la(addr) = m_socket->ep->remote;
	}
	return addr;
}

bool NetworkSocket::setRemoteAddress(const NetworkAddr& addr) {
	if(!isOpen() || !m_socket->ep) return false;
	Mutex::ScopedLock lock(lockObj());
	m_socket->ep->remote = la(addr);
	return true;
}

void NetworkSocket::reapplyRemoteAddress() {}
