// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

lock_record_cache_srv::lock_record_cache_srv() {
	locked = 0;
	client = -1;
	pthread_cond_init(&cv, NULL);
	pthread_mutex_init(&revoke_mutex, NULL);
	waiting = 0;
}

lock_record_cache_srv::~lock_record_cache_srv() {
	pthread_cond_destroy(&cv);
	pthread_mutex_lock(&revoke_mutex);
	pthread_mutex_destroy(&revoke_mutex);
}

lock_server_cache::lock_server_cache()
	: nacquire(0) {
	pthread_mutex_init(&locks_held_mutex, NULL);
}

lock_server_cache::~lock_server_cache() {
	map<lock_protocol::lockid_t, lock_record_cache_srv*>::iterator lock_iter;

	pthread_mutex_lock(&locks_held_mutex);
	pthread_mutex_destroy(&locks_held_mutex);

	for (lock_iter = locks_held.begin(); lock_iter != locks_held.end(); lock_iter++) {
		delete &lock_iter->second;
	}
}

int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &r) {
	lock_protocol::status ret = lock_protocol::OK;
	r = 1;

	//printf("(S ACQUIRE %llu %s)\n", lid, id.c_str());
	
	handle *h = NULL;
	rpcc *cl = NULL;
	std::string cl_id = "";

	pthread_mutex_lock(&locks_held_mutex);

	if (locks_held.count(lid) == 0) {
		locks_held[lid] = new lock_record_cache_srv();
	}

	//printf("(S ACQUIRE %llu %s %d %s) BEFORE\n", lid, id.c_str(), locks_held[lid]->locked, locks_held[lid]->client.c_str());

	locks_held[lid]->waiting++;
	while (locks_held[lid]->locked == 1) {
		if (!cl || locks_held[lid]->client.compare(cl_id) != 0) {
			if (h) {
				delete h;
			}
			cl_id = locks_held[lid]->client;
			h = new handle(cl_id);
			pthread_mutex_unlock(&locks_held_mutex);
			cl = h->safebind();
			pthread_mutex_lock(&locks_held_mutex);
		} else {
			if (0 == pthread_mutex_trylock(&locks_held[lid]->revoke_mutex)) {
				//printf("(S ACQUIRE %llu %s %d) REVOKE %s\n", lid, id.c_str(), locks_held[lid]->locked, cl_id.c_str());
				pthread_mutex_unlock(&locks_held_mutex);
				lock_protocol::status ret = cl->call(rlock_protocol::revoke, lid, r);
				VERIFY (ret == lock_protocol::OK);
				pthread_mutex_lock(&locks_held_mutex);
				pthread_mutex_unlock(&locks_held[lid]->revoke_mutex);
				//printf("(S ACQUIRE %llu %s %d) REVOKED %s\n", lid, id.c_str(), locks_held[lid]->locked, cl_id.c_str());
				locks_held[lid]->locked = 0;
				pthread_cond_signal(&locks_held[lid]->cv);
			} else {
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
			}
		}
	}
	locks_held[lid]->waiting--;

	locks_held[lid]->locked = 1;
	locks_held[lid]->client = id;

	pthread_mutex_unlock(&locks_held_mutex);

	//printf("(S ACQUIRE %llu %s %d %s) RETURN\n", lid, id.c_str(), locks_held[lid]->locked, locks_held[lid]->client.c_str());

	return ret;
}

int lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &r) {
	lock_protocol::status ret = lock_protocol::OK;
	r = 1;

	//printf("(WTF? WTF? WTF?)\n");

/*	pthread_mutex_lock(&locks_held_mutex);

	if (locks_held.count(lid) > 0) {
		if (locks_held[lid]->locked && locks_held[lid]->client.compare(id) == 0) {
			locks_held[lid]->locked = 0;
			if (locks_held[lid]->waiting > 0) {
				pthread_cond_signal(&locks_held[lid]->cv);
			}
		}
	}

	pthread_mutex_unlock(&locks_held_mutex);*/

	return ret;
}

lock_protocol::status lock_server_cache::stat(lock_protocol::lockid_t lid, int &r) {
	tprintf("stat request\n");
	r = nacquire;
	return lock_protocol::OK;
}

