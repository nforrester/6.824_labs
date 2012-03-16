// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"

char* state_to_string(int state) {
	static char buf[5];
	if (state == NONE) {
		strncpy(buf, "NONE", 5);
	} else if (state == ACQU) {
		strncpy(buf, "ACQU", 5);
	} else if (state == LOCK) {
		strncpy(buf, "LOCK", 5);
	} else if (state == FREE) {
		strncpy(buf, "FREE", 5);
	} else if (state == RELE) {
		strncpy(buf, "RELE", 5);
	} else if (state == RACK) {
		strncpy(buf, "RACK", 5);
	} else {
		strncpy(buf, "EROR", 5);
	}
	return buf;
}

lock_record_cache_clt::lock_record_cache_clt() {
	state = NONE;
	pthread_cond_init(&cv, NULL);
	pthread_mutex_init(&acquire_mutex, NULL);
}

lock_record_cache_clt::~lock_record_cache_clt() {
	pthread_cond_destroy(&cv);
	pthread_mutex_lock(&acquire_mutex);
	pthread_mutex_destroy(&acquire_mutex);
}

lock_client_cache::lock_client_cache(std::string xdst, class lock_release_user *_lu)
	: lock_client(xdst), lu(_lu) {
	rpcs *rlsrpc = new rpcs(0);
	rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
	rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

	const char *hname;
	hname = "127.0.0.1";
	std::ostringstream host;
	host << hname << ":" << rlsrpc->port();
	id = host.str();

	pthread_mutex_init(&locks_held_mutex, NULL);
}

lock_client_cache::~lock_client_cache() {
	std::map<lock_protocol::lockid_t, lock_record_cache_clt*>::iterator lock_iter;

	pthread_mutex_lock(&locks_held_mutex);
	pthread_mutex_destroy(&locks_held_mutex);

	for (lock_iter = locks_held.begin(); lock_iter != locks_held.end(); lock_iter++) {
		pthread_cond_destroy(&lock_iter->second->cv);
	}
}

lock_protocol::status lock_client_cache::acquire(lock_protocol::lockid_t lid) {
	return acquire_(lid, 0);
}

lock_protocol::status lock_client_cache::acquire_(lock_protocol::lockid_t lid, int depth) {
	int r;

	//printf("(C%d ACQUIRE %llu %lu %s)\n", depth, lid, pthread_self(), id.c_str());
	if (depth == 0) {
		printf("Client %s wants %llu\n", id.c_str(), lid);
	}

	pthread_mutex_lock(&locks_held_mutex);
	printf("                                              c %s lhm acquire lock1\n", id.c_str());

	if (locks_held.count(lid) == 0) {
		locks_held[lid] = new lock_record_cache_clt();
	}

	//printf("(C%d ACQUIRE %llu %lu %s %s) BEFORE\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));

	if (locks_held[lid]->state == FREE) {
		locks_held[lid]->state = LOCK;
		locks_held[lid]->owner = pthread_self();
	} else if (locks_held[lid]->state == NONE) {
		if (0 == pthread_mutex_trylock(&locks_held[lid]->acquire_mutex)) {
			locks_held[lid]->state = ACQU;
			locks_held[lid]->owner = pthread_self();
			printf("                                              c %s lhm acquire unlock2\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			lock_protocol::status ret = cl->call(lock_protocol::acquire, lid, id, r);
			VERIFY (ret == lock_protocol::OK);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm acquire lock2\n", id.c_str());
			pthread_mutex_unlock(&locks_held[lid]->acquire_mutex);
			locks_held[lid]->state = LOCK;
			pthread_cond_signal(&locks_held[lid]->cv);
		} else {
			printf("                                              c %s lhm acquire unlock3\n", id.c_str());
			pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
			printf("                                              c %s lhm acquire lock3\n", id.c_str());
			printf("                                              c %s lhm acquire unlock4\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			acquire_(lid, depth + 1);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm acquire lock4\n", id.c_str());
		}
	} else if (locks_held[lid]->state == ACQU) {
		while (locks_held[lid]->state == ACQU) {
			printf("                                              c %s lhm acquire unlock5\n", id.c_str());
			pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
			printf("                                              c %s lhm acquire lock5\n", id.c_str());
		}
		printf("                                              c %s lhm acquire unlock6\n", id.c_str());
		pthread_mutex_unlock(&locks_held_mutex);
		acquire_(lid, depth + 1);
		pthread_mutex_lock(&locks_held_mutex);
		printf("                                              c %s lhm acquire lock6\n", id.c_str());
	} else if (locks_held[lid]->state == LOCK) {
		if (locks_held[lid]->owner != pthread_self()){
			while (locks_held[lid]->state == LOCK) {
				printf("                                              c %s lhm acquire unlock9\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm acquire lock9\n", id.c_str());
			}
			printf("                                              c %s lhm acquire unlock10\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			acquire_(lid, depth + 1);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm acquire lock10\n", id.c_str());
		} else {
			// no need to do anything
		}
	} else if (locks_held[lid]->state == RELE) {
		if (locks_held[lid]->owner != pthread_self()){
			while (locks_held[lid]->state == RELE) {
				printf("                                              c %s lhm acquire unlock11\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm acquire lock11\n", id.c_str());
			}
			printf("                                              c %s lhm acquire unlock12\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			acquire_(lid, depth + 1);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm acquire lock12\n", id.c_str());
		} else {
			// no need to do anything,
			// even though we are going to release it sometime,
			// we currently have it locked.
		}
	} else if (locks_held[lid]->state == RACK) {
		while (locks_held[lid]->state == RACK) {
			printf("                                              c %s lhm acquire unlock7\n", id.c_str());
			pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
			printf("                                              c %s lhm acquire lock7\n", id.c_str());
		}
		printf("                                              c %s lhm acquire unlock8\n", id.c_str());
		pthread_mutex_unlock(&locks_held_mutex);
		acquire_(lid, depth + 1);
		pthread_mutex_lock(&locks_held_mutex);
		printf("                                              c %s lhm acquire lock8\n", id.c_str());
	} else {
		// this should never happen, this is an invalid state
		VERIFY (0);
	}

	pthread_cond_broadcast(&locks_held[lid]->cv);
	printf("                                              c %s lhm acquire unlock1\n", id.c_str());
	pthread_mutex_unlock(&locks_held_mutex);

	//printf("(C%d ACQUIRE %llu %lu %s %s) RETURN\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));
	if (depth == 0) {
		printf("Client %s Thread %lu locked %llu\n", id.c_str(), pthread_self(), lid);
	}

	return r;
}

lock_protocol::status lock_client_cache::release(lock_protocol::lockid_t lid) {
	return release_(lid, 0);
}

lock_protocol::status lock_client_cache::release_(lock_protocol::lockid_t lid, int depth) {
	int r = 1;

	//printf("(C%d RELEASE %llu %lu %s)\n", depth, lid, pthread_self(), id.c_str());
	if (depth == 0) {
		printf("Client %s no longer wants %llu\n", id.c_str(), lid);
	}

	pthread_mutex_lock(&locks_held_mutex);
	printf("                                              c %s lhm release lock1\n", id.c_str());

	if (locks_held.count(lid) > 0) {
		//printf("(C%d RELEASE %llu %lu %s %s)\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));
		if (locks_held[lid]->state == FREE) {
			// no need to do anything
		} else if (locks_held[lid]->state == NONE) {
			// no need to do anything
		} else if (locks_held[lid]->state == ACQU) {
			while (locks_held[lid]->state == ACQU) {
				printf("                                              c %s lhm release unlock2\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm release lock2\n", id.c_str());
			}
			printf("                                              c %s lhm release unlock3\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			release_(lid, depth + 1);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm release lock3\n", id.c_str());
		} else if (locks_held[lid]->state == LOCK) {
			locks_held[lid]->state = FREE;
		} else if (locks_held[lid]->state == RELE) {
			locks_held[lid]->state = RACK;
		} else if (locks_held[lid]->state == RACK) {
			// no need to do anything
		} else {
			// this should never happen, this is an invalid state
			VERIFY (0);
		}
	}

	pthread_cond_broadcast(&locks_held[lid]->cv);
	printf("                                              c %s lhm release unlock1\n", id.c_str());

	if (locks_held[lid]->state == FREE) {
		if (depth == 0) {
			printf("Client %s cached %llu\n", id.c_str(), lid);
		}
	} else if (locks_held[lid]->state == RACK) {
		if (depth == 0) {
			printf("Client %s released %llu but isn't telling anyone\n", id.c_str(), lid);
		}
	} else {
		if (depth == 0) {
			printf("Client %s WTF WTF WTF WTF %llu %s\n", id.c_str(), lid, state_to_string(locks_held[lid]->state));
		}
	}

	pthread_mutex_unlock(&locks_held_mutex);

	//printf("(C%d RELEASE %llu %lu %s %s) RETURN\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));
	return r;
}

rlock_protocol::status lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, int &r) {
	return revoke_handler_(lid, r, 0);
}

rlock_protocol::status lock_client_cache::revoke_handler_(lock_protocol::lockid_t lid, int &r, int depth) {
	int ret = rlock_protocol::OK;
	r = 1;

	//printf("(C%d REVOKE %llu %lu %s)\n", depth, lid, pthread_self(), id.c_str());
	if (depth == 0) {
		printf("Client %s had %llu revoked\n", id.c_str(), lid);
	}

	pthread_mutex_lock(&locks_held_mutex);
	printf("                                              c %s lhm revoke lock1\n", id.c_str());

	if (locks_held.count(lid) > 0) {
		//printf("(C%d REVOKE %llu %lu %s %s) BEFORE\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));
		if (locks_held[lid]->state == FREE) {
			locks_held[lid]->state = NONE;
		} else if (locks_held[lid]->state == NONE) {
			// no need to do anything
		} else if (locks_held[lid]->state == ACQU) {
			while (locks_held[lid]->state == ACQU) {
				printf("                                              c %s lhm revoke unlock2\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm revoke lock2\n", id.c_str());
			}
			printf("                                              c %s lhm revoke unlock3\n", id.c_str());
			pthread_mutex_unlock(&locks_held_mutex);
			revoke_handler_(lid, r, depth + 1);
			pthread_mutex_lock(&locks_held_mutex);
			printf("                                              c %s lhm revoke lock3\n", id.c_str());
		} else if (locks_held[lid]->state == LOCK) {
			locks_held[lid]->state = RELE;
			while (locks_held[lid]->state != RACK) {
				printf("                                              c %s lhm revoke unlock4\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm revoke lock4\n", id.c_str());
			}
			locks_held[lid]->state = NONE;
			pthread_cond_broadcast(&locks_held[lid]->cv);
		} else if (locks_held[lid]->state == RELE) {
			while (locks_held[lid]->state != NONE) {
				printf("                                              c %s lhm revoke unlock5\n", id.c_str());
				pthread_cond_wait(&locks_held[lid]->cv, &locks_held_mutex);
				printf("                                              c %s lhm revoke lock5\n", id.c_str());
			}
		} else if (locks_held[lid]->state == RACK) {
			locks_held[lid]->state = NONE;
			pthread_cond_broadcast(&locks_held[lid]->cv);
		} else {
			// this should never happen, this is an invalid state
			VERIFY (0);
		}
	}

	printf("                                              c %s lhm revoke unlock1\n", id.c_str());
	pthread_mutex_unlock(&locks_held_mutex);

	//printf("(C%d REVOKE %llu %lu %s %s) RETURN\n", depth, lid, pthread_self(), id.c_str(), state_to_string(locks_held[lid]->state));
	if (depth == 0) {
		printf("Client %s released %llu publicly\n", id.c_str(), lid);
	}

	return ret;
}

rlock_protocol::status lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &r) {
	int ret = rlock_protocol::OK;
	r = 1;
	return ret;
}
