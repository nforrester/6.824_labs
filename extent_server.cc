// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

extent::extent() {
	unsigned int now = time(NULL);
	a.atime = now;
	a.mtime = now;
	a.ctime = now;
	a.size = 0;
}

extent::extent(std::string buffer, extent_protocol::attr attributes) {
	buf = buffer;
	a = attributes;
}

extent_server::extent_server() {
	pthread_mutex_init(&extents_mutex, NULL);
	pthread_mutex_lock(&extents_mutex);
	extent ext;
	extents[ROOT_DIRECTORY_ID] = ext;
	pthread_mutex_unlock(&extents_mutex);
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &) {
	unsigned int now = time(NULL);
	extent_protocol::attr a;
	a.atime = now;
	a.mtime = now;
	a.ctime = now;
	a.size = buf.size();
	extent ext(buf, a);
	pthread_mutex_lock(&extents_mutex);
	extents[id] = ext;
	pthread_mutex_unlock(&extents_mutex);
	return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf) {
	unsigned int now = time(NULL);
	pthread_mutex_lock(&extents_mutex);
	if (extents.count(id) == 1) {
		buf = extents[id].buf;
		extents[id].a.atime = now;
		extents[id].a.ctime = now;
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::OK;
	} else {
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::NOENT;
	}
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a) {
	pthread_mutex_lock(&extents_mutex);
	if (extents.count(id) == 1) {
		a.size = extents[id].a.size;
		a.atime = extents[id].a.atime;
		a.mtime = extents[id].a.mtime;
		a.ctime = extents[id].a.ctime;
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::OK;
	} else {
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::NOENT;
	}
}

int extent_server::remove(extent_protocol::extentid_t id, int &) {
	pthread_mutex_lock(&extents_mutex);
	if (extents.count(id) == 1) {
		extents.erase(id);
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::OK;
	} else {
		pthread_mutex_unlock(&extents_mutex);
		return extent_protocol::NOENT;
	}
}

