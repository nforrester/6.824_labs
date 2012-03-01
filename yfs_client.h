#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <fuse/fuse_lowlevel.h>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
		extent_client *ec;

	public:
		typedef unsigned long long inum;
		enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
		typedef int status;

		struct fileinfo {
			unsigned long long size;
			unsigned long atime;
			unsigned long mtime;
			unsigned long ctime;
		};
		struct dirinfo {
			unsigned long atime;
			unsigned long mtime;
			unsigned long ctime;
		};
		struct dirent {
			std::string name;
			yfs_client::inum inum;
		};

	private:
		static std::string filename(inum);
		static inum n2i(std::string);

	public:
		yfs_client(std::string, std::string);

		bool isfile(inum);
		bool isdir(inum);

		int getfile(inum, fileinfo &);
		int getdir(inum, dirinfo &);

		bool lookup(inum, const char*, fuse_entry_param*);
		status create(inum, const char*, fuse_entry_param*, int);
		status readdir(void (*dirbuf_add)(struct dirbuf*, const char*, fuse_ino_t), struct dirbuf*, inum);
		status setsize(inum, unsigned long long size);
		std::string read(inum, unsigned long long size, unsigned long long offset);
		void write(inum, unsigned long long size, unsigned long long offset, const char*);
};

#endif 
