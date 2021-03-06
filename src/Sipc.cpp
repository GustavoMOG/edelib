/*
 * $Id: Sipc.cpp 3191 2011-11-28 17:02:00Z karijes $
 *
 * A very simple IPC
 * Copyright (c) 2008 edelib authors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <edelib/Sipc.h>
#include <edelib/Listener.h>
#include <edelib/Debug.h>
#include <edelib/List.h>

#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif

/* max message length without ending '\n' */
#define MSG_LEN_MAX 1024

EDELIB_NS_BEGIN

typedef list<SipcClientPrivate*> ConnectionList;
typedef list<SipcClientPrivate*>::iterator ConnectionListIter;

static void server_cb(int fd, void* data);

struct SipcClientPrivate {
	int   fd;
	char *path;

	~SipcClientPrivate() {
		if(fd > -1)
			close(fd);
		free(path);
	}
};

struct SipcServerPrivate {
	int            fd;
	char          *path;
	SipcCallback   cb;
	void          *arg;
	ConnectionList accepted_connections;

	~SipcServerPrivate();
};

SipcServerPrivate::~SipcServerPrivate() {
	ConnectionListIter it = accepted_connections.begin(), it_end = accepted_connections.end();
	for(; it != it_end; ++it) {
		listener_remove_fd((*it)->fd);
		close((*it)->fd);
		free((*it)->path);
	}

	if(fd > -1)
		close(fd);
	unlink(path);
	free(path);
}

static char* get_username(void) {
	char* name = getenv("USER");
	if(!name) {
		struct passwd* pw = getpwuid(getuid());
		if(!pw)
			name = (char*)"__unknown__";
		else { 
			// should this be copied ?
			name = pw->pw_name;
		}
	}
	return name;
}

static char* make_socket_filename(const char* prefix) {
	char buf[UNIX_PATH_MAX];

	snprintf(buf, sizeof(buf), "/tmp/.%s.%s.socket", prefix, get_username());
	return strdup(buf);
}

static bool file_is_socket(const char* path) {
	struct stat s;

	if(stat(path, &s) == -1)
		return false;

	if(S_ISSOCK(s.st_mode))
		return true;

	return false;
}

static void accept_new_connection(SipcServerPrivate* p) {
	socklen_t alen;

	SipcClientPrivate* cp = new SipcClientPrivate;
	cp->path = 0;
	cp->fd = accept(p->fd, NULL, &alen);

	p->accepted_connections.push_back(cp);

	/* listen accepted clients */
	listener_add_fd(cp->fd, server_cb, p);
}

static void server_cb(int fd, void* data) {
	SipcServerPrivate* priv = (SipcServerPrivate*)data;

	if(priv->fd == fd) {
		accept_new_connection(priv);
		return;
	}

	char c;
	int  nc, i = 0;
	char buf[MSG_LEN_MAX];
	memset(buf, 0, sizeof(buf));

	nc = ::read(fd, &c, 1);
	while(nc > 0 && c != '\n' && i < MSG_LEN_MAX) {
		buf[i] = c;
		i = i + nc;
		nc = ::read(fd, &c, 1);
	}

	if(nc <= 0) {
		ConnectionListIter it = priv->accepted_connections.begin();
		ConnectionListIter it_end = priv->accepted_connections.end();

		for(; it != it_end; ++it) {
			if((*it)->fd == fd) {
				listener_remove_fd(fd);
				close(fd);
				free((*it)->path);
				delete *it;
				priv->accepted_connections.erase(it);
				break;
			}
		}

		return;
	}

	buf[i] = '\0';

	if(priv->cb)
		priv->cb(buf, priv->arg);
}

SipcServer::SipcServer() : priv(0) { }

SipcServer::~SipcServer() {
	delete priv;
}

bool SipcServer::request_name(const char* prefix) {
	if(priv)
		delete priv;
   
	char* sname = make_socket_filename(prefix);
	if(!sname)
		return false;

	if(file_is_socket(sname)) {
		free(sname);
		return false;
	}

	priv = new SipcServerPrivate;
	priv->path = sname;
	priv->cb = 0;
	priv->arg = 0;

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, priv->path); /* TODO strncpy */
	priv->fd = socket(PF_UNIX, SOCK_STREAM, 0);

	if(bind(priv->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		priv->fd = -1;
		return false;
	}

	::listen(priv->fd, 5);

	listener_add_fd(priv->fd, server_cb, priv);
	return true;
}

void SipcServer::callback(SipcCallback cb, void* data) {
	E_RETURN_IF_FAIL(priv != NULL);

	priv->cb = cb;
	priv->arg = data;
}

SipcClient::SipcClient() : priv(0) { }

SipcClient::~SipcClient() {
	delete priv;
}

bool SipcClient::connect(const char* prefix) {
	if(priv)
		delete priv;
   
	char* sname = make_socket_filename(prefix);
	if(!sname)
		return false;

	// file must exists
	if(!file_is_socket(sname)) {
		free(sname);
		return false;
	}

	priv = new SipcClientPrivate;
	priv->path = sname;

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, priv->path); // TODO strncpy
	priv->fd = socket(PF_UNIX, SOCK_STREAM, 0);

	if(::connect(priv->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		priv->fd = -1;
		return false;
	}

	return true;
}

void SipcClient::send(const char* msg) {
	E_RETURN_IF_FAIL(priv != NULL);
	E_RETURN_IF_FAIL(priv->fd != -1);

	int len = strlen(msg);
	if(len > MSG_LEN_MAX)
		len = MSG_LEN_MAX;

	::write(priv->fd, msg, len);
	::write(priv->fd, "\n", 1);
}

EDELIB_NS_END
