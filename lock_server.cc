// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

lock_record::lock_record() {
	locked = 0;
	client = -1;
	pthread_cond_init(&cv, NULL);
	waiting = 0;
}

lock_record::~lock_record() {
	pthread_cond_destroy(&cv);
}

lock_server::lock_server()
	: nacquire(0) {
	pthread_mutex_init(&locks_held_mutex, NULL);
}

lock_server::~lock_server() {
	map<lock_protocol::lockid_t, lock_record*>::iterator lock_iter;

	pthread_mutex_lock(&locks_held_mutex);
	pthread_mutex_destroy(&locks_held_mutex);

	for (lock_iter = locks_held.begin(); lock_iter != locks_held.end(); lock_iter++) {
		pthread_cond_destroy(&lock_iter->second->cv);
	}
}

lock_protocol::status lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r) {
	lock_protocol::status ret = lock_protocol::OK;
	printf("stat request from clt %d\n", clt);
	r = nacquire;
	return ret;
}

lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r) {
	lock_protocol::status ret = lock_protocol::OK;
	printf("acquire request from clt %d on lock %llu\n", clt, lid);
	r = 1;

	pthread_mutex_lock(&locks_held_mutex);

	if (locks_held.count(lid) == 0) {
		locks_held[lid] = new lock_record();
	}

	locks_held[lid]->waiting++;
	while (locks_held[lid]->locked == 1) {
		pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
	}
	locks_held[lid]->waiting--;

	locks_held[lid]->locked = 1;
	locks_held[lid]->client = clt;

	pthread_mutex_unlock(&locks_held_mutex);

	return ret;
}

lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid, int &r) {
	lock_protocol::status ret = lock_protocol::OK;
	printf("acquire request from clt %d on lock %llu\n", clt, lid);
	r = 1;

	pthread_mutex_lock(&locks_held_mutex);

	if (locks_held.count(lid) > 0) {
		if (locks_held[lid]->locked && locks_held[lid]->client == clt) {
			locks_held[lid]->locked = 0;
			if (locks_held[lid]->waiting > 0) {
				pthread_cond_signal(&locks_held[lid]->cv);
			}
		}
	}

	pthread_mutex_unlock(&locks_held_mutex);

	return ret;
}
