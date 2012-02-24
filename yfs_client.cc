// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fuse/fuse_lowlevel.h>
#include <sstream>

#define MAX_FILENAME_LEN 2048

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst) {
	ec = new extent_client(extent_dst);
}

yfs_client::inum yfs_client::n2i(std::string n) {
	std::istringstream ist(n);
	unsigned long long finum;
	ist >> finum;
	return finum;
}

std::string yfs_client::filename(inum inum) {
	std::ostringstream ost;
	ost << inum;
	return ost.str();
}

bool yfs_client::isfile(inum inum) {
	if (inum & 0x80000000) {
		return true;
	} else {
		return false;
	}
}

bool yfs_client::isdir(inum inum) {
	return ! isfile(inum);
}

int yfs_client::getfile(inum inum, fileinfo &fin) {
	int r = OK;

	printf("getfile %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}

	fin.atime = a.atime;
	fin.mtime = a.mtime;
	fin.ctime = a.ctime;
	fin.size = a.size;
	printf("getfile %016llx -> sz %llu\n", inum, fin.size);

	release:

	return r;
}

int yfs_client::getdir(inum inum, dirinfo &din) {
	int r = OK;

	printf("getdir %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;

	release:

	return r;
}

bool yfs_client::lookup(inum parent, const char *name, fuse_entry_param *e) {
	std::string dir_contents;
	if (ec->get(parent, dir_contents) == extent_protocol::OK) {
		std::istringstream dc(dir_contents, std::istringstream::in);
		inum finum = 0;
		char fname[MAX_FILENAME_LEN];
		fname[0] = 0;
		while (!dc.eof()) {
			dc >> finum;
			dc.getline(fname, MAX_FILENAME_LEN);
			if (strncmp(name, fname, MAX_FILENAME_LEN) == 0) {
				printf("lookup finum: %llu\n", (unsigned long long) finum);
				e->ino = finum;
				extent_protocol::attr a;
				if (ec->getattr(finum, a) == extent_protocol::OK) {
					e->attr.st_atime = a.atime;
					e->attr.st_mtime = a.mtime;
					e->attr.st_ctime = a.ctime;
					e->attr.st_size = a.size;
					return true;
				}
				break;
			}
		}
	}
	return false;
}


yfs_client::status yfs_client::create(inum parent, const char *name, fuse_entry_param *e) {
	fuse_entry_param e_tmp;
	extent_protocol::attr a_tmp;
	inum finum;

	if (lookup(parent, name, &e_tmp)) {
		printf("file exists!\n");
		return EXIST;
	}

	do {
		finum = random();
		finum |= 0x80000000;
	} while (ec->getattr(finum, a_tmp) != extent_protocol::NOENT);

	if (ec->put(finum, std::string()) != extent_protocol::OK) {
		printf("failed to create extent!\n");
		return NOENT;
	}

	std::string dir_contents;
	if (ec->get(parent, dir_contents) != extent_protocol::OK) {
		printf("failed to get parent directory contents!\n");
		if (ec->remove(finum) != extent_protocol::OK) {
			printf("failed to remove orphan extent! FUCK!\n");
		}
		return NOENT;
	}

	std::ostringstream dc(dir_contents, std::ostringstream::out | std::ostringstream::app);
	dc << finum << std::endl;
	dc << name << std::endl;

	if (ec->put(parent, dc.str()) != extent_protocol::OK) {
		printf("failed to put parent directory contents!\n");
		if (ec->remove(finum) != extent_protocol::OK) {
			printf("failed to remove orphan extent! FUCK!\n");
		}
		return NOENT;
	}
	
	printf("create finum: %llu\n", (unsigned long long) finum);
	e->ino = finum;
	printf("create e->ino: %lu\n", (unsigned long) e->ino);
	if (ec->getattr(finum, a_tmp) != extent_protocol::OK) {
		printf("failed to getattr new file!\n");
		return NOENT;
	}

	e->attr.st_atime = a_tmp.atime;
	e->attr.st_mtime = a_tmp.mtime;
	e->attr.st_ctime = a_tmp.ctime;
	e->attr.st_size = a_tmp.size;
	printf("create e->atrr.st_size: %lu\n", (unsigned long) e->attr.st_size);

	printf("everything is hunky dory!\n");
	return OK;
}

yfs_client::status yfs_client::readdir(void (*dirbuf_add)(struct dirbuf*, const char*, fuse_ino_t), struct dirbuf *b, inum dir_inum) {
	std::string dir_contents;
	if (ec->get(dir_inum, dir_contents) != extent_protocol::OK) {
		printf("failed to get directory contents!\n");
		return NOENT;
	}

	std::istringstream dc(dir_contents, std::istringstream::in);
	inum finum = 0;
	char fname[MAX_FILENAME_LEN];
	fname[0] = 0;
	while (!dc.eof()) {
		dc >> finum;
		dc.getline(fname, MAX_FILENAME_LEN);
		(*dirbuf_add)(b, fname, finum);
	}
	return OK;
}
