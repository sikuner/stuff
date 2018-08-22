
#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdbool.h>

typedef void (*network_connected_handler)(void);

void network_set_connected_handler(network_connected_handler h); // ���������ӳɹ�,�ᴥ���¼�

void network_daemon(bool on);

bool network_get_connected(void);

#endif // __NETWORK_H__

