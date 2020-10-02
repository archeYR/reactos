#include <ws2_32.h>

/***********************************************************************
*              inet_pton                      (WS2_32.@)
*/
INT WINAPI WS_inet_pton( INT family, PCSTR addr, PVOID buffer)
{
#ifdef HAVE_INET_PTON
    int unixaf, ret;

    if (!addr || !buffer)
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    unixaf = convert_af_w2u(family);
    if (unixaf != AF_INET && unixaf != AF_INET6)
    {
        SetLastError(WSAEAFNOSUPPORT);
        return SOCKET_ERROR;
    }

    ret = inet_pton(unixaf, addr, buffer);
    if (ret == -1) SetLastError(wsaErrno());
    return ret;
#else
    SetLastError( WSAEAFNOSUPPORT );
    return SOCKET_ERROR;
#endif
}
