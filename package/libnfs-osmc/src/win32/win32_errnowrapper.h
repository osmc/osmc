#ifndef WIN32_ERRNOWRAPPER_H_
#define WIN32_ERRNOWRAPPER_H_

#undef errno
#define errno WSAGetLastError()
#undef EAGAIN
#undef EWOULDBLOCK
#undef EINTR
#undef EINPROGRESS

#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN		WSAEWOULDBLOCK					//same on windows
#define EINTR       WSAEINTR
#define EINPROGRESS WSAEINPROGRESS
#endif //WIN32_ERRNOWRAPPER_H_
