#ifndef NETDBG_H_
#define NETDBG_H_

#define AVA_NETDBG_PORT 0xDEAD

/** 
 * Initialize network console on port 57005.
 * This is a no-op if console is already initialized.
 * 
 * @param ip_addr: Target IP address. Broadcast assumed when NULL.
 * @returns 0 on success, -1 otherwise.
 **/
extern int NetDbg_Initialize(const char* ip_addr);

/**
 * Check if NetDbg module was initialized.
 * 
 * @returns 1 if initialized, 0 otherwise.
 **/
extern int NetDbg_WasInit(void);

/**
 * Echo a formatted string through the console.
 * This is a no-op if console was not initialized.
 * 
 * If the formatted string is longer than 576 bytes
 * it will be segmented and sent out non-atomically.
 * 
 * @param fmt: printf-like format.
 * @param ...: printf arguments.
 * 
 * @returns Length of the formatted string or -1 on error.
 **/
extern int NetDbg_Echo(const char* fmt, ...);

/**
 * Destroy a previously created console. 
 * This is a no-op if console was not initialized.
 **/
extern void NetDbg_Destroy(void);

#endif // NETDBG_H_