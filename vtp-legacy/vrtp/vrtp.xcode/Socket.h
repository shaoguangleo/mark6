/*
 *  Socket.h
 *  vrtp
 *
 *  Created by David Lapsley on Fri Feb 20 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */


#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <SocketBuffer.h>

#ifndef _SOCKET_H_
#define _SOCKET_H_

using namespace std;

class SocketException {
private:
    int _n;
    string _message;
public:
        SocketException(int n) {
            _n=n;
        }
    SocketException(int n, string s) {
        _n=n;
        _message=s;
    }
    ~SocketException()
    {
    }
    const char* what() {
        const char* s=(char*)_message.data();
        return (s);
    }
};

// Stream socket.
class Socket {
protected:
    int _sockd;
public:
    virtual void close() = 0;
#ifdef _DAVE_DEBUG
    virtual int test(void*);
    virtual int dump(void*);
#endif
};

#endif

