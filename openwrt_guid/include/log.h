
#ifndef __LOG_H__
#define __LOG_H__

/* 信息的调试级别,数值起小级别越高 */
#define	APP_EMERG				"<0>"	/* system is unusable			*/
#define	APP_ALERT				"<1>"	/* action must be taken immediately	*/
#define	APP_CRIT				"<2>"	/* critical conditions			*/
#define	APP_ERR	    			"<3>"	/* error conditions			*/
#define	APP_WARNING				"<4>"	/* warning conditions			*/
#define	APP_NOTICE				"<5>"	/* normal but significant condition	*/
#define	APP_INFO				"<6>"	/* informational			*/
#define	APP_DEBUG				"<7>"	/* debug-level messages			*/

/* 信息的默认调试级别 */
#define DEFAULT_DEBUG_LEVEL 	4

#define DBG_PRINTF(format, args...) dbg_printf(__FILE__, __LINE__, __FUNCTION__, format, ##args)
void dbg_printf(const char *file, int line, const char *func, const char *format, ...); 

#endif // __LOG_H__

