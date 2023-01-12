#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/error.h>

#include <psp2/sysmodule.h>
#include <psp2/net/net.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "netdbg.h"

#define NET_INIT_PARAM_SIZE (16 * 1024)
#define MAX_PAYLOAD_SIZE 576

typedef struct NetDbg_State {
    SceUID mutex;
    uint32_t mutex_timeout;

    SceNetInitParam net_init_param;

    const char* ip;
    uint16_t port;

    int sock_fd;
    SceNetSockaddrIn sin;
} NetDbg_State;

static NetDbg_State* state;

static int _TryInitializeSystemModules(void)
{
    if (state == NULL)
    {
        return -1;
    }
    
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) != SCE_SYSMODULE_LOADED)
    {
        sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    }

    if (sceNetShowNetstat() == SCE_NET_ERROR_ENOTINIT)
    {
        state->net_init_param.memory = malloc(NET_INIT_PARAM_SIZE);
        memset(state->net_init_param.memory, 0, NET_INIT_PARAM_SIZE);
        state->net_init_param.size = NET_INIT_PARAM_SIZE;
        
        return sceNetInit(&state->net_init_param);
    }

    return 0;
}

static void _EchoString(char* str)
{
    if (state == NULL
        || state->sock_fd < 0
        || str == NULL)
    {
        return;
    }

    size_t datalen = strlen(str);

    if (datalen == 0)
    {
        return;
    }

    if (datalen <= MAX_PAYLOAD_SIZE)
    {
        sceNetSendto(
            state->sock_fd, 
            str, 
            datalen, 
            SCE_NET_MSG_DONTWAIT, 
            (struct SceNetSockaddr*)&state->sin, 
            sizeof(state->sin)
        );
    }
    else
    {
        int segments = datalen / MAX_PAYLOAD_SIZE;
        size_t rest = datalen - (segments * MAX_PAYLOAD_SIZE);

        for (int i = 0; i < segments; i++)
        {
            sceNetSendto(
                state->sock_fd,
                str + (i * MAX_PAYLOAD_SIZE), 
                MAX_PAYLOAD_SIZE, 
                SCE_NET_MSG_DONTWAIT, 
                (struct SceNetSockaddr*)&state->sin, 
                sizeof(state->sin)
            );
        }

        if (rest > 0)
        {
            sceNetSendto(
                state->sock_fd,
                str + (segments * MAX_PAYLOAD_SIZE), 
                rest, 
                SCE_NET_MSG_DONTWAIT, 
                (struct SceNetSockaddr*)&state->sin, 
                sizeof(state->sin)
            );
        }
    }
}

int NetDbg_Initialize(const char* ip)
{
    if (state != NULL)
    {
        return 0;
    }

    state = (NetDbg_State*)malloc(sizeof(NetDbg_State));
    memset(state, 0, sizeof(NetDbg_State));

    state->mutex = sceKernelCreateMutex("NetDbg_ConsoleMutex", 0, 0, NULL);
    state->mutex_timeout = 0xFFFFFFFF;

    state->ip = ip;
    state->port = 0xDEAD;

    if (_TryInitializeSystemModules() < 0)
    {
        NetDbg_Destroy();
        return -1;
    }

    state->sock_fd = sceNetSocket(
        "NetDbg_LogSocket", 
        SCE_NET_AF_INET, 
        SCE_NET_SOCK_DGRAM, 
        0
    );

    if (state->sock_fd < 0)
    {
        NetDbg_Destroy();
        return -1;
    }

    if (state->ip == NULL)
    {
        state->ip = "255.255.255.255";
    }

    int yes = 1;
    int sockoptret = sceNetSetsockopt(
        state->sock_fd, 
        SCE_NET_SOL_SOCKET, 
        SCE_NET_SO_BROADCAST, 
        &yes, 
        sizeof(yes)
    );

    if (sockoptret < 0)
    {
        NetDbg_Destroy();
        return -1;
    }

    state->sin.sin_family = SCE_NET_AF_INET;
    state->sin.sin_port = sceNetHtons(state->port);

    if (sceNetInetPton(SCE_NET_AF_INET, state->ip, &state->sin.sin_addr) < 0)
    {
        NetDbg_Destroy();
        return -1;
    }

    return 0;
}

int NetDbg_WasInit(void)
{
    return state != NULL;
}

int NetDbg_Echo(const char* fmt, ...)
{
    if (state == NULL)
    {
        return -1;
    }

    int ret = 0;
    sceKernelLockMutex(state->mutex, 1, &state->mutex_timeout);

    int fmt_result = 0;
    char* out_buffer;

    va_list args;
    va_start(args, fmt);
    {
        fmt_result = vasiprintf(&out_buffer, fmt, args);
    }
    va_end(args);

    if (fmt_result >= 0)
    {
        _EchoString(out_buffer);
        free(out_buffer);
    }

    sceKernelUnlockMutex(state->mutex, 1);

    return ret;
}

void NetDbg_Destroy(void)
{
    if (state == NULL)
    {
        return;
    }

    if (state->net_init_param.memory != NULL)
    {
        free(state->net_init_param.memory);
    }

    sceKernelDeleteMutex(state->mutex);
    sceNetSocketClose(state->sock_fd);

    state->sock_fd = -1;

    free(state);
    state = NULL;
}