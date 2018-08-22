
#ifndef __SRV_PROC_H__
#define __SRV_PROC_H__

int do_shutdown();
int do_hello(int index);

int do_notify_progress(void);

int do_airplay(char *act);

int do_wifi(int state, int twinkle);
int do_bluetooth(int state);
int do_down(int show);
int do_lock(int show);
int do_alarm(int show);
int do_time(int show);

int do_neterr(char *str);
int do_line1(char *str);
int do_line2(char *str);
int do_line3(char *str);

int do_toast(char *str, int dur, int style);

#endif // __SRV_PROC_H__

