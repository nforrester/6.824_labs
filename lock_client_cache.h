// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include "extent_client.h"

#define NONE 0
#define ACQU 1
#define LOCK 2
#define RELE 3
#define FREE 4
#define RACK 5

class lock_record_cache_clt {
	public:
		int state;
		pthread_cond_t cv;
		pthread_mutex_t acquire_mutex;
		pthread_t owner;
		lock_record_cache_clt();
		~lock_record_cache_clt();
};

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
	public:
		virtual void dorelease(lock_protocol::lockid_t) = 0;
		virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
	private:
		class lock_release_user *lu;
		int rlock_port;
		std::string hostname;
		std::string id;

		std::map<lock_protocol::lockid_t, lock_record_cache_clt*> locks_held;
		pthread_mutex_t locks_held_mutex;
#if LAB >= 5
		extent_client *ec;
#endif
	public:
		lock_client_cache(std::string xdst, class lock_release_user *l = 0);
		virtual ~lock_client_cache();
		lock_protocol::status acquire(lock_protocol::lockid_t);
		lock_protocol::status acquire_(lock_protocol::lockid_t, int);
		lock_protocol::status release(lock_protocol::lockid_t);
		lock_protocol::status release_(lock_protocol::lockid_t, int);
		rlock_protocol::status revoke_handler(lock_protocol::lockid_t, int &);
		rlock_protocol::status revoke_handler_(lock_protocol::lockid_t, int &, int);
		rlock_protocol::status retry_handler(lock_protocol::lockid_t, int &);
#if LAB >= 5
		void set_extent_client(extent_client*);
#endif
};

#endif
