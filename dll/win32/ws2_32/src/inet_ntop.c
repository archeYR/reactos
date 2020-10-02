#include <ws2_32.h>

/***********************************************************************
 *              inet_ntop                      (WS2_32.@)
 */
PCSTR WINAPI WS_inet_ntop( INT family, PVOID addr, PSTR buffer, SIZE_T len )
{
#ifdef HAVE_INET_NTOP
    struct WS_in6_addr *in6;
    struct WS_in_addr  *in;
    PCSTR pdst;

    if (!buffer)
    {
        SetLastError( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    switch (family)
    {
    case WS_AF_INET:
    {
        in = addr;
        pdst = inet_ntop( AF_INET, &in->WS_s_addr, buffer, len );
        break;
    }
    case WS_AF_INET6:
    {
        in6 = addr;
        pdst = inet_ntop( AF_INET6, in6->WS_s6_addr, buffer, len );
        break;
    }
    default:
        SetLastError( WSAEAFNOSUPPORT );
        return NULL;
    }

    if (!pdst) SetLastError( STATUS_INVALID_PARAMETER );
    return pdst;
#else
    SetLastError( WSAEAFNOSUPPORT );
    return NULL;
#endif
}
