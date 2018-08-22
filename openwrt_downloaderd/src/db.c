
#include "db.h"

static sqlite3 *down_db = NULL;

int db_init(void)
{
	int rc = 0;
	char sql[MAX_LINE] = { 0 };
	
	rc = sqlite3_open(DOWN_DB, &down_db);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't open database:%s, rc:%s \n", DOWN_DB, rc);
		return -1;
	}
	
	sprintf(sql, "CREATE TABLE IF NOT EXISTS %s(taskId INTEGER PRIMARY KEY, status INTEGER, errorno INTEGER, received INTEGER, total INTEGER, filename NVARCHAR(64), vurl NVARCHAR(1024), url NVARCHAR(1024))", TASK_TABLE);
	rc = sqlite3_exec(down_db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't exec sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	sprintf(sql, "CREATE TABLE IF NOT EXISTS %s(taskId INTEGER PRIMARY KEY, status INTEGER, errorno INTEGER, received INTEGER, total INTEGER, filename NVARCHAR(64), vurl NVARCHAR(1024), url NVARCHAR(1024))", COMPLETE_TABLE);
	rc = sqlite3_exec(down_db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't exec sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	DBG_PRINTF("The database is successfully initialized! \n");
	
	return 0;
}

void db_release(void)
{
	if(NULL != down_db)
	{
		sqlite3_close(down_db);
		down_db = NULL;
	}
}

int db_task_insert(const char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	char *vurl = NULL;
	char *filename = NULL;
	
	vurl = url_strip_param(url);
	filename = get_md5sum(vurl);
	
	sprintf(sql, "INSERT INTO %s(taskId, status, errorno, received, total, filename, vurl, url) VALUES(NULL, ?, 0, 0, 0, ?, ?, ?)", TASK_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 1, DT_READY);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 2, filename, strlen(filename), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d filename:%s \n", rc, filename);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 3, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, vurl:%s \n", rc, vurl);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 4, url, strlen(url), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, url:%s \n", rc, url);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_DONE != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_task_del(const char *url)
{
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	char *vurl = NULL;
	
	vurl = url_strip_param(url);
	sprintf(sql, "DELETE FROM %s WHERE vurl=?", TASK_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 1, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, vurl:%s \n", rc, vurl);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_DONE != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_task_update(const TaskRecord *task_rec)
{
	if(NULL == task_rec)
	{
		return -1;
	}
	
	int rc = -1;
	char sql[MAX_LINE] = { 0 };
	char *errmsg = NULL;
	
	sprintf(sql, "UPDATE %s SET status=%d, errorno=%d, received=%d, total=%d WHERE filename='%s'", 
		TASK_TABLE, task_rec->status, task_rec->errorno, task_rec->received, task_rec->total, task_rec->filename);
	DBG_PRINTF("UPDATE sql= %s \n", sql);
	
	rc = sqlite3_exec(down_db, sql, NULL, NULL, &errmsg);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't update the database! errmsg:%s", errmsg);
		return -1;
	}
	
	return 0;
}

int db_task_select(const char *url, TaskRecord *task_rec)
{
	if(NULL==url || 0==strlen(url) || NULL==task_rec)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	char *vurl = NULL;
	
	vurl = url_strip_param(url);
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s WHERE vurl=?", TASK_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 1, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	task_rec->taskId = sqlite3_column_int(stmt, 0);
	task_rec->status = sqlite3_column_int(stmt, 1);
	task_rec->errorno = sqlite3_column_int(stmt, 2);
	task_rec->received = sqlite3_column_int(stmt, 3);
	task_rec->total = sqlite3_column_int(stmt, 4);
	strcpy(task_rec->filename, (char*)sqlite3_column_text(stmt, 5));
	strcpy(task_rec->url, (char*)sqlite3_column_text(stmt, 6));
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_task_del_all(void)
{
	char sql[MAX_LINE] = { 0 };
	sprintf(sql, "DELETE FROM %s;", TASK_TABLE);
	
	int rc = sqlite3_exec(down_db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't update the database! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_task_select_all(struct list_head *head)
{
	char sql[MAX_LINE] = { 0 };
	int rc = -1;
	char **result = NULL;
	int rows = 0;
	int cols = 0;
	char *errmsg = NULL;
	TaskRecord *pRec = NULL;
	int r = 0; 
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s", TASK_TABLE);
	DBG_PRINTF(" sql: %s \n", sql);
	
	rc = sqlite3_get_table(down_db, sql, &result, &rows, &cols, &errmsg);
	if(rc != SQLITE_OK)
    {
		DBG_PRINTF("Can't get table! rc:%d, errmsg:%s \n", rc, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return -1;
    }
	
	for(r = 1; r < rows+1; r++)
	{
		pRec = (TaskRecord*)malloc(sizeof(TaskRecord));
		memset(pRec, 0, sizeof(TaskRecord));
		
		pRec->taskId = atoi(result[r*cols+0]);
		pRec->status = atoi(result[r*cols+1]);
		pRec->errorno = atoi(result[r*cols+2]);
		pRec->received = atoi(result[r*cols+3]);
		pRec->total = atoi(result[r*cols+4]);
		strcpy(pRec->filename, result[r*cols+5]);
		strcpy(pRec->url, result[r*cols+6]);
		
		list_add_tail(&pRec->list, head);
	}
	
	sqlite3_free_table(result);
	
	return 0;
}

int db_task_range(int begin, int length, struct list_head *head)
{
	char sql[MAX_LINE] = { 0 };
	int rc = -1;
	char **result = NULL;
	int rows = 0;
	int cols = 0;
	char *errmsg = NULL;
	TaskRecord *pRec = NULL;
	int r = 0; 
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s LIMIT %d, %d", TASK_TABLE, begin, length);
	DBG_PRINTF(" sql: %s \n", sql);
	
	rc = sqlite3_get_table(down_db, sql, &result, &rows, &cols, &errmsg);
	if(rc != SQLITE_OK)
    {
		DBG_PRINTF("Can't get table! rc:%d, errmsg:%s \n", rc, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return -1;
    }
	
	for(r = 1; r < rows+1; r++)
	{
		pRec = (TaskRecord*)malloc(sizeof(TaskRecord));
		memset(pRec, 0, sizeof(TaskRecord));
		
		pRec->taskId = atoi(result[r*cols+0]);
		pRec->status = atoi(result[r*cols+1]);
		pRec->errorno = atoi(result[r*cols+2]);
		pRec->received = atoi(result[r*cols+3]);
		pRec->total = atoi(result[r*cols+4]);
		strcpy(pRec->filename, result[r*cols+5]);
		strcpy(pRec->url, result[r*cols+6]);
		
		list_add_tail(&pRec->list, head);
	}
	
	sqlite3_free_table(result);
	
	return 0;
}

int  db_task_count(int *count)
{	
	if(NULL == count)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	sprintf(sql, "SELECT COUNT(*) FROM %s", TASK_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	*count = sqlite3_column_int(stmt, 0);
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_get_ready_task(TaskRecord *task_rec)
{
	if(NULL == task_rec)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s WHERE status=? OR status=? ORDER BY taskId ASC LIMIT 1", TASK_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d \n", TASK_TABLE, rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 1, DT_READY);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, status:%s", rc, DT_READY);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 2, DT_DOWNLOAD);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, status:%s", rc, DT_DOWNLOAD);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	task_rec->taskId = sqlite3_column_int(stmt, 0);
	task_rec->status = sqlite3_column_int(stmt, 1);
	task_rec->errorno = sqlite3_column_int(stmt, 2);
	task_rec->received = sqlite3_column_int(stmt, 3);
	task_rec->total = sqlite3_column_int(stmt, 4);
	strcpy(task_rec->filename, (char*)sqlite3_column_text(stmt, 5));
	strcpy(task_rec->url, (char*)sqlite3_column_text(stmt, 6));
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d", rc);
		return -1;
	}
	
	return 0;
}

int db_complete_insert(const TaskRecord *task_rec) // ºöÂÔlist/taskId×Ö¶Î,ÆäËû×Ö¶Î²åÈë
{
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	char *vurl = NULL;
	
	vurl = url_strip_param(task_rec->url);
	sprintf(sql, "INSERT INTO %s(taskId, status, errorno, received, total, filename, vurl, url) VALUES(NULL, ?, ?, ?, ?, ?, ?, ?)", COMPLETE_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 1, task_rec->status);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 2, task_rec->errorno);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 3, task_rec->received);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_bind_int(stmt, 4, task_rec->total);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 5, task_rec->filename, strlen(task_rec->filename), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d filename:%s \n", rc, task_rec->filename);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 6, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, vurl:%s \n", rc, vurl);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 7, task_rec->url, strlen(task_rec->url), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, url:%s \n", rc, task_rec->url);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_DONE != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_complete_del(const char *url)
{
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	char *vurl = NULL;
	vurl = url_strip_param(url);
	
	sprintf(sql, "DELETE FROM %s WHERE vurl=?", COMPLETE_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 1, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, vurl:%s \n", rc, vurl);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_DONE != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int  db_complete_delete(const char *filename)
{
	if(NULL == filename)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	sprintf(sql, "DELETE FROM %s WHERE filename=?", COMPLETE_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 1, filename, strlen(filename), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d, filename:%s \n", rc, filename);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_DONE != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_complete_select(const char *url, TaskRecord *task_rec)
{
	if(NULL==url || 0==strlen(url) || NULL==task_rec)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	char *vurl = NULL;
	vurl = url_strip_param(url);
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s WHERE vurl=?", COMPLETE_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_bind_text(stmt, 1, vurl, strlen(vurl), NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't bind for sql! rc:%d \n", rc);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	task_rec->taskId = sqlite3_column_int(stmt, 0);
	task_rec->status = sqlite3_column_int(stmt, 1);
	task_rec->errorno = sqlite3_column_int(stmt, 2);
	task_rec->received = sqlite3_column_int(stmt, 3);
	task_rec->total = sqlite3_column_int(stmt, 4);
	strcpy(task_rec->filename, (char*)sqlite3_column_text(stmt, 5));
	strcpy(task_rec->url, (char*)sqlite3_column_text(stmt, 6));
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_complete_del_all(void)
{
	char sql[MAX_LINE] = { 0 };
	sprintf(sql, "DELETE FROM %s;", COMPLETE_TABLE);
	
	int rc = sqlite3_exec(down_db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete the database! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

int db_complete_select_all(struct list_head *head)
{
	char sql[MAX_LINE] = { 0 };
	int rc = -1;
	char **result = NULL;
	int rows = 0;
	int cols = 0;
	char *errmsg = NULL;
	TaskRecord *pRec = NULL;
	int r = 0; 
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s", COMPLETE_TABLE);
	DBG_PRINTF(" sql: %s \n", sql);
	
	rc = sqlite3_get_table(down_db, sql, &result, &rows, &cols, &errmsg);
	if(rc != SQLITE_OK)
	{
		DBG_PRINTF("Can't get table! rc:%d, errmsg:%s \n", rc, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return -1;
	}
	
	for(r = 1; r < rows+1; r++)
	{
		pRec = (TaskRecord*)malloc(sizeof(TaskRecord));
		memset(pRec, 0, sizeof(TaskRecord));
		
		pRec->taskId = atoi(result[r*cols+0]);
		pRec->status = atoi(result[r*cols+1]);
		pRec->errorno = atoi(result[r*cols+2]);
		pRec->received = atoi(result[r*cols+3]);
		pRec->total = atoi(result[r*cols+4]);
		strcpy(pRec->filename, result[r*cols+5]);
		strcpy(pRec->url, result[r*cols+6]);
		
		list_add_tail(&pRec->list, head);
	}
	
	sqlite3_free_table(result);
	
	return 0;
}

int db_complete_range(int begin, int length, struct list_head *head)
{
	char sql[MAX_LINE] = { 0 };
	int rc = -1;
	char **result = NULL;
	int rows = 0;
	int cols = 0;
	char *errmsg = NULL;
	TaskRecord *pRec = NULL;
	int r = 0; 
	
	sprintf(sql, "SELECT taskId, status, errorno, received, total, filename, url FROM %s LIMIT %d, %d", COMPLETE_TABLE, begin, length);
	DBG_PRINTF(" sql: %s \n", sql);
	
	rc = sqlite3_get_table(down_db, sql, &result, &rows, &cols, &errmsg);
	if(rc != SQLITE_OK)
    {
		DBG_PRINTF("Can't get table! rc:%d, errmsg:%s \n", rc, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
		return -1;
    }
	
	for(r = 1; r < rows+1; r++)
	{
		pRec = (TaskRecord*)malloc(sizeof(TaskRecord));
		memset(pRec, 0, sizeof(TaskRecord));
		
		pRec->taskId = atoi(result[r*cols+0]);
		pRec->status = atoi(result[r*cols+1]);
		pRec->errorno = atoi(result[r*cols+2]);
		pRec->received = atoi(result[r*cols+3]);
		pRec->total = atoi(result[r*cols+4]);
		strcpy(pRec->filename, result[r*cols+5]);
		strcpy(pRec->url, result[r*cols+6]);
		
		list_add_tail(&pRec->list, head);
	}
	
	sqlite3_free_table(result);
	
	return 0;
}

int  db_complete_count(int *count)
{	
	if(NULL == count)
	{
		return -1;
	}
	
	int rc = -1;
	sqlite3_stmt *stmt = NULL;	
	char sql[MAX_LINE] = { 0 };
	
	sprintf(sql, "SELECT COUNT(*) FROM %s", COMPLETE_TABLE);
	
	rc = sqlite3_prepare(down_db, sql, -1, &stmt, NULL);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't prepare for sql! database:%s, rc:%d \n", DOWN_DB, rc);
		return -1;
	}
	
	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		DBG_PRINTF("Can't sqlite3_step a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	*count = sqlite3_column_int(stmt, 0);
	
	rc = sqlite3_finalize(stmt);
	if(SQLITE_OK != rc)
	{
		DBG_PRINTF("Can't delete a prepared statement! rc:%d \n", rc);
		return -1;
	}
	
	return 0;
}

