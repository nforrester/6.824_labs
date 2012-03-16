#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include <pthread.h>
#include "lock_protocol.h"
#include "lock_server.h"
#include "rpc.h"

class lock_record_cache_srv {
	public:
		bool locked;
		std::string client;
		pthread_cond_t cv;
		pthread_mutex_t revoke_mutex;
		int waiting;
		lock_record_cache_srv();
		~lock_record_cache_srv();
};

class lock_server_cache {
	private:
		map<lock_protocol::lockid_t, lock_record_cache_srv*> locks_held;
		pthread_mutex_t locks_held_mutex;

		int nacquire;
	public:
		lock_server_cache();
		~lock_server_cache();
		lock_protocol::status stat(lock_protocol::lockid_t, int &);
		int acquire(lock_protocol::lockid_t, std::string id, int &);
		int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
