
#ifndef __SERVER_H__
#define __SERVER_H__

int ubus_init(void);
void ubus_done(void);

void ubus_term(void);

typedef void (*down_exist_handler_t)(int rc, char *url);
int down_exist_async(const char *url, down_exist_handler_t cb);

int down_create_async(const char *url);

#endif // __SERVER_H__

