// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <pthread.h>
#include "extent_protocol.h"
#include "rpc.h"

#if LAB >= 5
	#include "extent_server.h"
#endif

class extent_client {
	private:
		rpcc *cl;

#if LAB >= 5
		std::map<extent_protocol::extentid_t, extent> cached_extents;
		pthread_mutex_t cache_mutex;
#endif

	public:
		extent_client(std::string dst);

		extent_protocol::status get(extent_protocol::extentid_t eid, std::string &buf);
		extent_protocol::status getattr(extent_protocol::extentid_t eid, extent_protocol::attr &a);
		extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
		extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

