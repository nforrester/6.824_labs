#if LAB < 5
	// RPC stubs for clients to talk to extent_server
#else
	// A caching extent client
	#include "extent_server.h"
#endif

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst) {
	sockaddr_in dstsock;
	make_sockaddr(dst.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() != 0) {
		printf("extent_client: bind failed\n");
	}
#if LAB >= 5
	pthread_mutex_init(&cache_mutex, NULL);
#endif
}

extent_protocol::status extent_client::get(extent_protocol::extentid_t eid, std::string &buf) {
	extent_protocol::status ret = extent_protocol::OK;
#if LAB < 5
	ret = cl->call(extent_protocol::get, eid, buf);
#else
	pthread_mutex_lock(&cache_mutex);
	int extent_known = cached_extents.count(eid);
	if (extent_known && cached_extents[eid].buf_valid) {
		// cache hit
		buf.assign(cached_extents[eid].buf);
	} else {
		// cache miss
		ret = cl->call(extent_protocol::get, eid, buf);
		if (ret == extent_protocol::OK) {
			if (extent_known) {
				cached_extents[eid].buf = buf;
				cached_extents[eid].buf_valid = true;
				cached_extents[eid].buf_dirty = false;
			} else {
				extent_protocol::attr a;
				extent ext(buf, a);
				ext.buf_valid = true;
				ext.buf_dirty = false;
				ext.a_valid = false;
				ext.a_dirty = false;
				cached_extents[eid] = ext;
			}
		}
	}
	pthread_mutex_unlock(&cache_mutex);
#endif
	return ret;
}

extent_protocol::status extent_client::getattr(extent_protocol::extentid_t eid, extent_protocol::attr &attr) {
	extent_protocol::status ret = extent_protocol::OK;
#if LAB < 5
	ret = cl->call(extent_protocol::getattr, eid, attr);
#else
	pthread_mutex_lock(&cache_mutex);
	int extent_known = cached_extents.count(eid);
	if (extent_known && cached_extents[eid].a_valid) {
		// cache hit
		attr.atime = cached_extents[eid].a.atime;
		attr.mtime = cached_extents[eid].a.mtime;
		attr.ctime = cached_extents[eid].a.ctime;
		attr.size = cached_extents[eid].a.size;
	} else {
		// cache miss
		ret = cl->call(extent_protocol::getattr, eid, attr);
		if (ret == extent_protocol::OK) {
			if (extent_known) {
				cached_extents[eid].a = attr;
				cached_extents[eid].a_valid = true;
				cached_extents[eid].a_dirty = false;
			} else {
				std::string buf;
				extent ext(buf, attr);
				ext.buf_valid = false;
				ext.buf_dirty = false;
				ext.a_valid = true;
				ext.a_dirty = false;
				cached_extents[eid] = ext;
			}
		}
	}
	pthread_mutex_unlock(&cache_mutex);
#endif
	return ret;
}

extent_protocol::status extent_client::put(extent_protocol::extentid_t eid, std::string buf) {
	extent_protocol::status ret = extent_protocol::OK;
#if LAB < 5
	int r;
	ret = cl->call(extent_protocol::put, eid, buf, r);
#else
	pthread_mutex_lock(&cache_mutex);

	unsigned int now = time(NULL);
	extent_protocol::attr a;
	a.atime = now;
	a.mtime = now;
	a.ctime = now;
	a.size = buf.size();

	extent ext(buf, a);
	cached_extents[eid] = ext;

	cached_extents[eid].buf_valid = true;
	cached_extents[eid].buf_dirty = true;

	cached_extents[eid].a_valid = true;
	cached_extents[eid].a_dirty = true;

	pthread_mutex_unlock(&cache_mutex);
#endif
	return ret;
}

extent_protocol::status extent_client::remove(extent_protocol::extentid_t eid) {
	extent_protocol::status ret = extent_protocol::OK;
	int r;
	ret = cl->call(extent_protocol::remove, eid, r);
#if LAB >= 5
	if (ret == extent_protocol::NOENT) {
		// Well it must never have gotten flushed.
		// That's ok, we wanted it to not be there, and so it's not, so everything is hunky dory.
		ret = extent_protocol::OK;
	}

	pthread_mutex_lock(&cache_mutex);
	if (cached_extents.count(eid)) {
		cached_extents.erase(eid);
	}
	pthread_mutex_unlock(&cache_mutex);
#endif
	return ret;
}
