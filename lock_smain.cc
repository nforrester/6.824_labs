#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#if LAB <= 3
	#include "lock_server.h"
#else
	#include "lock_server_cache.h"
#endif

#include "jsl_log.h"

#if LAB <= 3
	// Main loop of lock_server
#else
	// Main loop of lock_server_cache
#endif

// lock_smain.cc eh? I like the name lock_sfoils_in_attack_position.cc better.

int main(int argc, char *argv[]) {
	int count = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	srandom(getpid());

	if(argc != 2){
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(1);
	}

	char *count_env = getenv("RPC_COUNT");
	if(count_env != NULL){
		count = atoi(count_env);
	}

	//jsl_set_debug(2);

#ifndef RSM
	#if LAB <= 3
		lock_server ls;
		rpcs server(atoi(argv[1]), count);
		server.reg(lock_protocol::stat, &ls, &lock_server::stat);
		server.reg(lock_protocol::acquire, &ls, &lock_server::acquire);
		server.reg(lock_protocol::release, &ls, &lock_server::release);
	#else
		lock_server_cache ls;
		rpcs server(atoi(argv[1]), count);
		server.reg(lock_protocol::stat, &ls, &lock_server_cache::stat);
		server.reg(lock_protocol::acquire, &ls, &lock_server_cache::acquire);
		server.reg(lock_protocol::release, &ls, &lock_server_cache::release);
	#endif
#endif


	while(1){
		sleep(1000);
	}
}
