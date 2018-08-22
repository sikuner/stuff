#ifndef _PLAYERD_CONFIG_H_
#define _PLAYERD_CONFIG_H_

#define PERROR(fmt, args...) syslog(LOG_SYSLOG, "[PLAYERD](E)"fmt, ##args)
#if 0
#define PDEBUG(fmt, args...) syslog(LOG_SYSLOG, "[PLAYERD](D)"fmt, ##args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PINFO(fmt, args...) syslog(LOG_SYSLOG, "[PLAYERD](I)"fmt, ##args)

#endif /* _PLAYERD_CONFIG_H_ */
