
#include "usrv.h"
#include "common.h"
#include "manager.h"

#include <json/json_object.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

static struct ubus_context *ctx;
static struct blob_buf b;

static void uloop_alive_cb(struct uloop_timeout *timeout)
{
	uloop_timeout_set(timeout, TIMER_ALIVE_UNIT);
}
static struct uloop_timeout uloop_alive_timer = {
	.cb = uloop_alive_cb,
};

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
	char *str;
	if (!msg)
		return;
	
	str = blobmsg_format_json_indent(msg, true, 0);
	
	printf("\n");
	printf("down call gui watch_down result: \n");
	printf("%s \n", str);
	printf("\n");
	
	free(str);
}
static void watch_down_cb(struct uloop_timeout *timeout)
{
	static struct ubus_request req;
	uint32_t id;
	
	if(ubus_lookup_id(ctx, "gui", &id))
	{
		fprintf(stderr, "Failed to look up gui object \n");
		return;
	}
	
	blob_buf_init(&b, 0);
	ubus_invoke_async(ctx, id, "watch_down", b.head, &req);
	req.data_cb = receive_call_result_data;
	ubus_complete_request_async(ctx, &req);
}
static struct uloop_timeout watch_down_timer = {
	.cb = watch_down_cb
};

static const struct blobmsg_policy test_policy[] = { { 0 } };
static int __down_test(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	DBG_PRINTF(" __down_test \n");
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", 0);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	HELLO_MSG,
	
	__HELLO_MAX
};
static const struct blobmsg_policy hello_policy[] = {
	[HELLO_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING }
};
static int __down_hello(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *str = 0;
	
	struct blob_attr *tb[__HELLO_MAX];
	
	blobmsg_parse(hello_policy, ARRAY_SIZE(hello_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[HELLO_MSG])
		str = blobmsg_data(tb[HELLO_MSG]);
	
	int rc = 0;
	
	if(NULL == str)
	{
		rc = -2;
	}
	else
	{
		rc = down_hello(str);		
	}
	
	DBG_PRINTF("__down_hello rc: %d \n", rc);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	blobmsg_add_string(&b, "msg", str);
	
	return ubus_send_reply(ctx, req, b.head);
}

static void uloop_end_cb(struct uloop_timeout *timeout)
{
	uloop_end(); // uloop_cancelled = true;	
}
static struct uloop_timeout uloop_end_timer = {
	.cb = uloop_end_cb,
};
static const struct blobmsg_policy shutdown_policy[] = { {0} };
static int __down_shutdown(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = down_shutdown();
	DBG_PRINTF("__down_shutdown rc: %d \n", rc);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	uloop_timeout_set(&uloop_end_timer, TIMER_ALIVE_UNIT);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	REMOVE_PATH,
	
	__REMOVE_MAX
};
static const struct blobmsg_policy remove_policy[] = {
	[REMOVE_PATH] = { .name = "path", .type = BLOBMSG_TYPE_STRING },
};
static int __down_remove(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__REMOVE_MAX];
	char *path = NULL;
	int   rc = -1;
	
	blobmsg_parse(remove_policy, ARRAY_SIZE(remove_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[REMOVE_PATH])
		path = blobmsg_data(tb[REMOVE_PATH]);
	
	path = strduptrim(path);
	rc = down_remove(path);
	
	DBG_PRINTF("__down_remove rc: %d, path: %s \n", rc, path);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	RMDIRX_PATH,
	
	__RMDIRX_MAX
};
static const struct blobmsg_policy rmdirx_policy[] = {
	[RMDIRX_PATH] = { .name = "dir", .type = BLOBMSG_TYPE_STRING },
};
static int __down_rmdirx(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__REMOVE_MAX];
	char *path = NULL;
	int   rc = -1;
	
	blobmsg_parse(rmdirx_policy, ARRAY_SIZE(rmdirx_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[RMDIRX_PATH])
		path = blobmsg_data(tb[RMDIRX_PATH]);
	
	path = strduptrim(path);
	rc = down_rmdirx(path);
	
	DBG_PRINTF("__down_rmdirx rc: %d, path: %s \n", rc, path);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}


enum {
	LENGTH_URL,
	
	__LENGTH_MAX
};
static const struct blobmsg_policy length_policy[] = {
	[LENGTH_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_length(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__LENGTH_MAX];
	int rc = -1;
	unsigned int length = 0;
	
	blobmsg_parse(length_policy, ARRAY_SIZE(length_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[LENGTH_URL])
		url = blobmsg_data(tb[LENGTH_URL]);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_length(url, &length);
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", json_object_new_int(rc));
	json_object_object_add(json_root, "length", json_object_new_int(length));
	json_object_object_add(json_root, "url", json_object_new_string(url));
	
	fprintf(stderr, "  __down_length.rc     : %d \n", rc);
	fprintf(stderr, "  __down_length.length : %d \n", length);
	fprintf(stderr, "  __down_length.url    : %s \n", url);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	CHECK_URL,
	
	__CHECK_MAX
};
static const struct blobmsg_policy check_policy[] = {
	[CHECK_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_check(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__CHECK_MAX];
	char *url = NULL;
	
	int rc = -1;
	unsigned int length = 0;
	
	blobmsg_parse(check_policy, ARRAY_SIZE(check_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[CHECK_URL])
		url = blobmsg_data(tb[CHECK_URL]);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_check(url, &length);
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", 	json_object_new_int(rc));
	json_object_object_add(json_root, "length", json_object_new_int(length));
	json_object_object_add(json_root, "url", 	json_object_new_string(url));
	
	fprintf(stderr, "  __down_length.rc         : %d \n", rc);
	fprintf(stderr, "  __down_length.length     : %d \n", length);
	fprintf(stderr, "  __down_length.url        : %s \n", url);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	EXIST_URL,
	
	__EXIST_MAX
};
static const struct blobmsg_policy exist_policy[] = {
	[EXIST_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_exist(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__EXIST_MAX];
	char *url = NULL;
	
	int rc = -1;
	
	blobmsg_parse(exist_policy, ARRAY_SIZE(exist_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[EXIST_URL])
		url = blobmsg_data(tb[EXIST_URL]);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_exist(url);
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", 	json_object_new_int(rc));
	json_object_object_add(json_root, "url", 	json_object_new_string(url));
	
	fprintf(stderr, "  __down_length.rc         : %d \n", rc);
	fprintf(stderr, "  __down_length.url        : %s \n", url);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	QUERY_URL,
	
	__QUERY_MAX
};
static const struct blobmsg_policy query_policy[] = {
	[QUERY_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_query(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__QUERY_MAX];
	int rc = -1;
	TaskRecord task_rec = { {0} };
	
	blobmsg_parse(query_policy, ARRAY_SIZE(query_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[QUERY_URL])
		url = blobmsg_data(tb[QUERY_URL]);
	
	fprintf(stderr, "    __down_query.url        : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_query(url, &task_rec);
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", json_object_new_int(rc));
	
	json_object *json_task = json_object_new_object();
	json_object_object_add(json_task, "taskId", json_object_new_int(task_rec.taskId));
	json_object_object_add(json_task, "status", json_object_new_int(task_rec.status));
	json_object_object_add(json_task, "errorno", json_object_new_int(task_rec.errorno));
	json_object_object_add(json_task, "received", json_object_new_int(task_rec.received));
	json_object_object_add(json_task, "total", json_object_new_int(task_rec.total));
	json_object_object_add(json_task, "filename", json_object_new_string(task_rec.filename));
	json_object_object_add(json_task, "url", json_object_new_string(task_rec.url));
	
	json_object_object_add(json_root, "task", json_task);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	CREATE_URL,
	
	__CREATE_MAX
};
static const struct blobmsg_policy create_policy[] = {
	[CREATE_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_create(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__CREATE_MAX];
	int rc = -1;
	
	blobmsg_parse(create_policy, ARRAY_SIZE(create_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[CREATE_URL])
		url = blobmsg_data(tb[CREATE_URL]);
	
	fprintf(stderr, "  __task_create.url    : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_task_create(url);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	DELETE_URL,
	
	__DELETE_MAX
};
static const struct blobmsg_policy delete_policy[] = {
	[DELETE_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_delete(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__DELETE_MAX];
	int rc = -1;
	
	blobmsg_parse(delete_policy, ARRAY_SIZE(delete_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[DELETE_URL])
		url = blobmsg_data(tb[DELETE_URL]);
	
	fprintf(stderr, "    __task_delete.url        : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_task_delete(url);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	START_URL,
	
	__START_MAX
};
static const struct blobmsg_policy start_policy[] = {
	[START_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_start(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__START_MAX];
	int rc = -1;
	
	blobmsg_parse(start_policy, ARRAY_SIZE(start_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[START_URL])
		url = blobmsg_data(tb[START_URL]);
	
	fprintf(stderr, "    __task_start.url        : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_task_start(url);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	STOP_URL,
	
	__STOP_MAX
};
static const struct blobmsg_policy stop_policy[] = {
	[START_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_stop(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__STOP_MAX];
	int rc = -1;
	
	blobmsg_parse(stop_policy, ARRAY_SIZE(stop_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[STOP_URL])
		url = blobmsg_data(tb[STOP_URL]);
	
	fprintf(stderr, "    __task_stop.url        : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_task_stop(url);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy current_policy[] = { {0} };
static int __down_current(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	TaskInfo info = { 0 };
	rc = down_task_current(&info);
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", json_object_new_int(rc));
	
	json_object *json_info = json_object_new_object();
	json_object_object_add(json_info, "taskId", json_object_new_int(info.taskId));
	json_object_object_add(json_info, "status", json_object_new_int(info.status));
	json_object_object_add(json_info, "errorno", json_object_new_int(info.errorno));
	json_object_object_add(json_info, "speed", json_object_new_int(info.speed));
	json_object_object_add(json_info, "received", json_object_new_int(info.received));
	json_object_object_add(json_info, "total", json_object_new_int(info.total));	
	json_object_object_add(json_info, "filename", json_object_new_string(info.filename));
	json_object_object_add(json_info, "url", json_object_new_string(info.url));
	
	json_object_object_add(json_root, "info", json_info);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	RANGE_BEGIN,
	RANGE_LENGTH,
	
	__RANGE_MAX
};
static const struct blobmsg_policy range_policy[] = {
	[RANGE_BEGIN] = { .name = "begin", .type = BLOBMSG_TYPE_INT32 },
	[RANGE_LENGTH] = { .name = "length", .type = BLOBMSG_TYPE_INT32 },
};
static int __down_range(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int begin = -1;
	int length = -1;
	
	struct blob_attr *tb[__RANGE_MAX];
	
	blobmsg_parse(range_policy, ARRAY_SIZE(range_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[RANGE_BEGIN])
		begin = blobmsg_get_u32(tb[RANGE_BEGIN]);
	if (tb[RANGE_LENGTH])
		length = blobmsg_get_u32(tb[RANGE_LENGTH]);
	
	fprintf(stderr, "    __down_range.begin : %d \n", begin);
	fprintf(stderr, "    __down_range.length : %d \n", length);
	
	int rc = -1;	
	TaskRecord *pRec = NULL;
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	
	rc = down_task_range(begin, length, &records_head);
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", json_object_new_int(rc));
	
	json_object *json_array = json_object_new_array();
	list_for_each_entry(pRec, &records_head, list) 
	{
		json_object *json_record = json_object_new_object();
		json_object_object_add(json_record, "taskId", json_object_new_int(pRec->taskId));
		json_object_object_add(json_record, "status", json_object_new_int(pRec->status));
		json_object_object_add(json_record, "errorno", json_object_new_int(pRec->errorno));
		json_object_object_add(json_record, "received", json_object_new_int(pRec->received));
		json_object_object_add(json_record, "total", json_object_new_int(pRec->total));
		json_object_object_add(json_record, "filename", json_object_new_string(pRec->filename));
		json_object_object_add(json_record, "url", json_object_new_string(pRec->url));
		
		json_object_array_add(json_array, json_record);
	}
	
	json_object_object_add(json_root, "tasks", json_array);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	// 清理内存
	struct list_head *pos = records_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &records_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		pRec = container_of(pos, TaskRecord, list);
		free(pRec);
		pRec = NULL;
		
		pos = tmp;
	}
	
	json_object_put(json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy count_policy[] = { {0} };
static int __down_count(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	int count = -1;
	
	rc = down_task_count(&count);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	blobmsg_add_u32(&b, "count", count);
	
	return ubus_send_reply(ctx, req, b.head);	
}

enum {
	CREATE_ALL_TASKS,
	
	__CREATE_ALL_MAX
};
static const struct blobmsg_policy create_all_policy[] = {
	[CREATE_ALL_TASKS] = { .name = "tasks", .type = BLOBMSG_TYPE_ARRAY },
};
static int __down_create_all(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__CREATE_ALL_MAX], *cur, *cur2;
	int rem, rem2;
	
	int rc = 0;
	
	char *urls[1024] = { 0 };
	int sum = 0;
	int i = 0;
	
	blobmsg_parse(create_all_policy, ARRAY_SIZE(create_all_policy), tb, blob_data(msg), blob_len(msg));
	
	if(tb[CREATE_ALL_TASKS])
	{
		blobmsg_for_each_attr(cur, tb[CREATE_ALL_TASKS], rem)
		{
			if(blobmsg_type(cur)==BLOBMSG_TYPE_TABLE)
			{
				blobmsg_for_each_attr(cur2, cur, rem2)
				{
					urls[sum++] = strdup(strduptrim(blobmsg_get_string(cur2)));
				}
			}
		}
	}
	
	rc = down_task_create_all(urls);
	
	for(i = 0; urls[i] != NULL; i++)
	{
		fprintf(stderr, "urls[%d]: %s \n", i, urls[i]);
		free(urls[i]);
		urls[i] = NULL;
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy delete_all_policy[] = { {0} };
static int __down_delete_all(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	
	rc = down_task_delete_all();
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy start_all_policy[] = { {0} };
static int __down_start_all(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	
	rc = down_task_start_all();
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy stop_all_policy[] = { {0} };
static int __down_stop_all(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	
	rc = down_task_stop_all();
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	COMPLETE_DELETE_URL,
	
	__COMPLETE_DELETE_MAX
};
static const struct blobmsg_policy complete_delete_policy[] = {
	[COMPLETE_DELETE_URL] = { .name = "url", .type = BLOBMSG_TYPE_STRING },
};
static int __down_complete_delete(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *url = NULL;
	struct blob_attr *tb[__COMPLETE_DELETE_MAX];
	int rc = -1;
	
	blobmsg_parse(complete_delete_policy, ARRAY_SIZE(complete_delete_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[COMPLETE_DELETE_URL])
		url = blobmsg_data(tb[COMPLETE_DELETE_URL]);
	
	fprintf(stderr, "    __down_complete_delete.url        : %s \n", url);
	
	url = strduptrim(url);
	if(NULL==url || 0==strlen(url))
	{
		rc = -2;
	}
	else
	{
		rc = down_complete_delete(url);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy complete_delete_all_policy[] = { {0} };
static int __down_complete_delete_all(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	
	rc = down_complete_delete_all();
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	COMPLETE_RANGE_BEGIN,
	COMPLETE_RANGE_LENGTH,
	
	__COMPLETE_RANGE_MAX
};
static const struct blobmsg_policy complete_range_policy[] = {
	[COMPLETE_RANGE_BEGIN] = { .name = "begin", .type = BLOBMSG_TYPE_INT32 },
	[COMPLETE_RANGE_LENGTH] = { .name = "length", .type = BLOBMSG_TYPE_INT32 },
};
static int __down_complete_range(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int begin = -1;
	int length = -1;
	
	struct blob_attr *tb[__COMPLETE_RANGE_MAX];
	
	blobmsg_parse(complete_range_policy, ARRAY_SIZE(complete_range_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[COMPLETE_RANGE_BEGIN])
		begin = blobmsg_get_u32(tb[COMPLETE_RANGE_BEGIN]);
	if (tb[COMPLETE_RANGE_LENGTH])
		length = blobmsg_get_u32(tb[COMPLETE_RANGE_LENGTH]);
	
	fprintf(stderr, "    __down_complete_range.begin : %d \n", begin);
	fprintf(stderr, "    __down_complete_range.length : %d \n", length);
	
	int rc = -1;
	TaskRecord *pRec = NULL;
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	
	rc = down_complete_range(begin, length, &records_head);
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "rc", json_object_new_int(rc));
	
	json_object *json_array = json_object_new_array();
	list_for_each_entry(pRec, &records_head, list) 
	{
		json_object *json_record = json_object_new_object();
		json_object_object_add(json_record, "taskId", json_object_new_int(pRec->taskId));
		json_object_object_add(json_record, "status", json_object_new_int(pRec->status));
		json_object_object_add(json_record, "errorno", json_object_new_int(pRec->errorno));
		json_object_object_add(json_record, "received", json_object_new_int(pRec->received));
		json_object_object_add(json_record, "total", json_object_new_int(pRec->total));
		json_object_object_add(json_record, "filename", json_object_new_string(pRec->filename));
		json_object_object_add(json_record, "url", json_object_new_string(pRec->url));
		
		json_object_array_add(json_array, json_record);		
	}
	
	json_object_object_add(json_root, "completes", json_array);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct blobmsg_policy complete_count_policy[] = { {0} };
static int __down_complete_count(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = -1;
	int count = -1;
	
	rc = down_complete_count(&count);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	blobmsg_add_u32(&b, "count", count);
	
	return ubus_send_reply(ctx, req, b.head);	
}

static const struct ubus_method down_methods[] = {

	UBUS_METHOD("test",  				__down_test,  				test_policy),
	UBUS_METHOD("hello",  				__down_hello,  				hello_policy),
	UBUS_METHOD("shutdown", 			__down_shutdown,  			shutdown_policy),
	UBUS_METHOD("remove",				__down_remove,				remove_policy),
	UBUS_METHOD("rmdirx",				__down_rmdirx,				rmdirx_policy),
	
	UBUS_METHOD("length", 				__down_length,  			length_policy),
	UBUS_METHOD("check",				__down_check,				check_policy),
	UBUS_METHOD("exist",				__down_exist,				exist_policy),
	
	UBUS_METHOD("query",				__down_query,				query_policy),
	UBUS_METHOD("create",				__down_create,				create_policy),
	UBUS_METHOD("delete",				__down_delete,				delete_policy),
	UBUS_METHOD("start",				__down_start,				start_policy),
	UBUS_METHOD("stop",					__down_stop,				stop_policy),
	
	UBUS_METHOD("current", 				__down_current, 			current_policy),
	UBUS_METHOD("range",				__down_range,				range_policy),
	UBUS_METHOD("count",				__down_count,				count_policy),
	
	UBUS_METHOD("create_all", 			__down_create_all, 			create_all_policy),
	UBUS_METHOD("delete_all", 			__down_delete_all, 			delete_all_policy),
	UBUS_METHOD("start_all", 			__down_start_all, 			start_all_policy),
	UBUS_METHOD("stop_all", 			__down_stop_all, 			stop_all_policy),
	
	UBUS_METHOD("complete_delete", 		__down_complete_delete, 	complete_delete_policy),
	UBUS_METHOD("complete_delete_all", 	__down_complete_delete_all, complete_delete_all_policy),
	UBUS_METHOD("complete_range", 		__down_complete_range, 		complete_range_policy),	
	UBUS_METHOD("complete_count", 		__down_complete_count, 		complete_count_policy),

};

static struct ubus_object_type down_object_type =
	UBUS_OBJECT_TYPE("down", down_methods);

static struct ubus_object down_object = {
	.name 		= "down",
	.type 		= &down_object_type,
	.methods 	= down_methods,
	.n_methods 	= ARRAY_SIZE(down_methods)
};

int notify_progress(TaskInfo *info)
{
	if(NULL == info)
	{
		return -1;
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "type", json_object_new_string("progress"));
	
	json_object *json_info = json_object_new_object();
	json_object_object_add(json_info, "taskId", json_object_new_int(info->taskId));
	json_object_object_add(json_info, "status", json_object_new_int(info->status));
	json_object_object_add(json_info, "errorno", json_object_new_int(info->errorno));
	json_object_object_add(json_info, "speed", json_object_new_int(info->speed));
	json_object_object_add(json_info, "received", json_object_new_int(info->received));
	json_object_object_add(json_info, "total", json_object_new_int(info->total));
	json_object_object_add(json_info, "filename", json_object_new_string(info->filename));
	json_object_object_add(json_info, "url", json_object_new_string(info->url));
	
	json_object_object_add(json_root, "information", json_info);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_info);
	json_object_put(json_root);
	
	return ubus_notify(ctx, &down_object, "progress", b.head, -1);
}

int notify_status(TaskRecord *task_rec)
{
	if(NULL==task_rec)
	{
		return -1;
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "type", json_object_new_string("status"));
	
	json_object *json_task = json_object_new_object();
	json_object_object_add(json_task, "taskId", 	json_object_new_int(task_rec->taskId));
	json_object_object_add(json_task, "status", 	json_object_new_int(task_rec->status));
	json_object_object_add(json_task, "errorno", 	json_object_new_int(task_rec->errorno));
	json_object_object_add(json_task, "received", 	json_object_new_int(task_rec->received));
	json_object_object_add(json_task, "total", 		json_object_new_int(task_rec->total));
	json_object_object_add(json_task, "filename", 	json_object_new_string(task_rec->filename));
	json_object_object_add(json_task, "url", 		json_object_new_string(task_rec->url));
	json_object_object_add(json_root, "task", json_task);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_task);
	json_object_put(json_root);
	
	return ubus_notify(ctx, &down_object, "status", b.head, -1);
}

int notify_complete(TaskRecord *task_rec, TaskRecord *complete_rec)
{
	if(NULL==task_rec || NULL==complete_rec)
	{
		return -1;
	}
	
	json_object *json_root = json_object_new_object();
	json_object_object_add(json_root, "type", json_object_new_string("complete"));
	
	json_object *json_task = json_object_new_object();
	json_object_object_add(json_task, "taskId", 	json_object_new_int(task_rec->taskId));
	json_object_object_add(json_task, "status", 	json_object_new_int(task_rec->status));
	json_object_object_add(json_task, "errorno", 	json_object_new_int(task_rec->errorno));
	json_object_object_add(json_task, "received", 	json_object_new_int(task_rec->received));
	json_object_object_add(json_task, "total", 		json_object_new_int(task_rec->total));
	json_object_object_add(json_task, "filename", 	json_object_new_string(task_rec->filename));
	json_object_object_add(json_task, "url", 		json_object_new_string(task_rec->url));
	json_object_object_add(json_root, "task", json_task);
	
	json_object *json_complete = json_object_new_object();
	json_object_object_add(json_complete, "taskId", 	json_object_new_int(complete_rec->taskId));
	json_object_object_add(json_complete, "status", 	json_object_new_int(complete_rec->status));
	json_object_object_add(json_complete, "errorno", 	json_object_new_int(complete_rec->errorno));
	json_object_object_add(json_complete, "received", 	json_object_new_int(complete_rec->received));
	json_object_object_add(json_complete, "total", 		json_object_new_int(complete_rec->total));
	json_object_object_add(json_complete, "filename", 	json_object_new_string(complete_rec->filename));
	json_object_object_add(json_complete, "url", 		json_object_new_string(complete_rec->url));
	json_object_object_add(json_root, "complete", json_complete);
	
	blob_buf_init(&b, 0);
	blobmsg_add_object(&b, json_root);
	
	json_object_put(json_complete);
	json_object_put(json_task);
	json_object_put(json_root);
	
	return ubus_notify(ctx, &down_object, "complete", b.head, -1);
}

int ubus_init(void)
{
	ctx = ubus_connect(NULL);
	if (!ctx) 
	{
		fprintf(stderr, "Failed to connect to ubus ctx \n");
		return -1;
	}
	ubus_add_uloop(ctx);
	
	int ret = ubus_add_object(ctx, &down_object);
	if (ret)
	{
		fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
		return -1;
	}
	
	uloop_timeout_set(&uloop_alive_timer, TIMER_ALIVE_UNIT); // 保活心跳定时器
	uloop_timeout_set(&watch_down_timer, TIMER_WATCH_DOWN);
	
	return 0;
}

void ubus_done(void)
{
	ubus_free(ctx);	
}

void ubus_term(void)
{
	manager_pause();
	task_stop();
	uloop_end();
}

