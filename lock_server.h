// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <map>
#include <pthread.h>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

using namespace std;

class lock_record {
	public:
		bool locked;
		int client;
		pthread_cond_t cv;
		int waiting;
		lock_record();
		~lock_record();
};

class lock_server {
	private:
		map<lock_protocol::lockid_t, lock_record*> locks_held;
		pthread_mutex_t locks_held_mutex;

	protected:
		int nacquire;

	public:
		lock_server();
		~lock_server();
		lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
		lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
		lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 
