
#ifndef __USRV_H__
#define __USRV_H__

#include "task.h"

int notify_progress(TaskInfo *ti);

int notify_status(TaskRecord *task_rec);

int notify_complete(TaskRecord *task_rec, TaskRecord *complete_rec);

int  ubus_init(void);

void ubus_done(void);

void ubus_term(void);

#endif // __USRV_H__

