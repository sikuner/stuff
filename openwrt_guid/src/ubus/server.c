
#include "log.h"
#include "conf.h"
#include "server.h"
#include "srv_proc.h"
#include "common.h"
#include "types.h"

#include <pthread.h>

#include <libubus.h>
#include <json/json_object.h>
#include <libubox/blobmsg_json.h>

static struct ubus_context *ctx;
static struct blob_buf b;
static struct ubus_subscriber down_event;

static void uloop_alive_cb(struct uloop_timeout *timeout)
{
//	static int counter = 0;
//	fprintf(stderr, "uloop_alive_cb  counter = %d \n", counter++);
	uloop_timeout_set(timeout, TIMER_ALIVE_UNIT);
}
static struct uloop_timeout uloop_alive_timer = {
	.cb = uloop_alive_cb,
};

static void uloop_end_cb(struct uloop_timeout *timeout)
{
	uloop_end(); // uloop_cancelled = true;	
}
static struct uloop_timeout uloop_end_timer = {
	.cb = uloop_end_cb,
};
static const struct blobmsg_policy shutdown_policy[] = { {0} };
static int gui_shutdown(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = do_shutdown();
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	int ret = ubus_send_reply(ctx, req, b.head);	
	uloop_timeout_set(&uloop_end_timer, TIMER_ALIVE_UNIT);
	
	return ret;
}

static void down_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s, uint32_t id)
{
	fprintf(stderr, "Object %08x went away\n", id);
}
static int down_notify(struct ubus_context *ctx, struct ubus_object *obj,
	    struct ubus_request_data *req, const char *method,
	    struct blob_attr *msg)
{
//	char *str = blobmsg_format_json(msg, true);
//	fprintf(stderr, "Received notification '%s': %s\n", method, str);
//	free(str);
	
	fprintf(stderr, "down_notify method: %s \n", method);
	if(0 == strcmp(method, "progress"))
	{
		do_notify_progress();
	}
	
	return 0;
}
static const struct blobmsg_policy watch_down_policy[] = { {0} };
static int gui_watch_down(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int ret = -1;
	uint32_t watch_id = -1;
	
	ret = ubus_lookup_id(ctx, "down", &watch_id);
	if (ret) 
	{
		fprintf(stderr, "Failed to look up test object : %s \n", ubus_strerror(ret));
		goto reply;
	}
	
	down_event.remove_cb = down_handle_remove;
	down_event.cb = down_notify;
	ret = ubus_subscribe(ctx, &down_event, watch_id);
	fprintf(stderr, "Watching object %08x: %s\n", watch_id, ubus_strerror(ret));
	
reply:
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", 0-ret);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_HELLO_INDEX,
	
	__GUI_HELLO_MAX
};
static const struct blobmsg_policy hello_policy[] = {
	[GUI_HELLO_INDEX] = { .name = "index", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_hello(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	int index = 0;
	
	struct blob_attr *tb[__GUI_HELLO_MAX];
	
	blobmsg_parse(hello_policy, ARRAY_SIZE(hello_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_HELLO_INDEX])
		index = blobmsg_get_u32(tb[GUI_HELLO_INDEX]);
	
	int rc = do_hello(index);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_AIRPLAY_ACT,
	
	__GUI_AIRPLAY_MAX
};
static const struct blobmsg_policy airplay_policy[] = {
	[GUI_AIRPLAY_ACT] = { .name = "act", .type = BLOBMSG_TYPE_STRING }
};
static int gui_airplay(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	char *act = 0;
	
	struct blob_attr *tb[__GUI_AIRPLAY_MAX];
	
	blobmsg_parse(airplay_policy, ARRAY_SIZE(airplay_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_AIRPLAY_ACT])
		act = blobmsg_data(tb[GUI_AIRPLAY_ACT]);
	
	DBG_PRINTF("act: %s", act);
	int rc = 0;
	
	if(NULL == act)
	{
		rc = -2;
	}
	else
	{
		rc = do_airplay(act);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_WIFI_STATE,
	GUI_WIFI_TWINKLE,
	
	__GUI_WIFI_MAX
};
static const struct blobmsg_policy wifi_policy[] = {
	[GUI_WIFI_STATE] = { .name = "state", .type = BLOBMSG_TYPE_INT32 },
	[GUI_WIFI_TWINKLE] = { .name = "twinkle", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_wifi(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	int state = 0x0000FFFF;
	int twinkle = 0;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_WIFI_MAX];
	
	blobmsg_parse(wifi_policy, ARRAY_SIZE(wifi_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_WIFI_STATE])
		state = blobmsg_get_u32(tb[GUI_WIFI_STATE]);
	if (tb[GUI_WIFI_TWINKLE])
		twinkle = blobmsg_get_u32(tb[GUI_WIFI_TWINKLE]);
	
	DBG_PRINTF("state = %d", state);
	if(0x0000FFFF == state)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_wifi(state, twinkle);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_DOWN_SHOW,
	
	__GUI_DOWN_MAX
};
static const struct blobmsg_policy down_policy[] = {
	[GUI_DOWN_SHOW] = { .name = "show", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_down(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int show = 0x00FFFFFF;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_DOWN_MAX];
	
	blobmsg_parse(down_policy, ARRAY_SIZE(down_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_DOWN_SHOW])
		show = blobmsg_get_u32(tb[GUI_DOWN_SHOW]);
	
	DBG_PRINTF("show = 0x%08x", show);
	if(0x00FFFFFF == show)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_down(show);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_LOCK_SHOW,
	
	__GUI_LOCK_MAX
};
static const struct blobmsg_policy lock_policy[] = {
	[GUI_LOCK_SHOW] = { .name = "show", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_lock(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int show = 0x00FFFFFF;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_LOCK_MAX];
	
	blobmsg_parse(lock_policy, ARRAY_SIZE(lock_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_LOCK_SHOW])
		show = blobmsg_get_u32(tb[GUI_LOCK_SHOW]);
	
	DBG_PRINTF("show = %d", show);
	if(0x00FFFFFF == show)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_lock(show);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_BLUETOOTH_STATE,
	
	__GUI_BLUETOOTH_MAX
};
static const struct blobmsg_policy bluetooth_policy[] = {
	[GUI_BLUETOOTH_STATE] = { .name = "state", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_bluetooth(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	int state = 0x0000FFFF;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_BLUETOOTH_MAX];
	
	blobmsg_parse(bluetooth_policy, ARRAY_SIZE(bluetooth_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_BLUETOOTH_STATE])
		state = blobmsg_get_u32(tb[GUI_BLUETOOTH_STATE]);
	
	DBG_PRINTF("state = %d", state);
	if(0x0000FFFF == state)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_bluetooth(state);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_ALARM_SHOW,
	
	__GUI_ALARM_MAX
};
static const struct blobmsg_policy alarm_policy[] = {
	[GUI_ALARM_SHOW] = { .name = "show", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_alarm(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	int show = 0x00FFFFFF;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_ALARM_MAX];
	
	blobmsg_parse(alarm_policy, ARRAY_SIZE(alarm_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_ALARM_SHOW])
		show = blobmsg_get_u32(tb[GUI_ALARM_SHOW]);
	
	DBG_PRINTF("show = 0x%08x", show);
	if(0x00FFFFFF == show)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_alarm(show);
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_TIME_SHOW,
	
	__GUI_TIME_MAX
};
static const struct blobmsg_policy time_policy[] = {
	[GUI_TIME_SHOW] = { .name = "show", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_time(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	int show = 0x00FFFFFF;
	int rc = 0;
	
	struct blob_attr *tb[__GUI_TIME_MAX];
	
	blobmsg_parse(time_policy, ARRAY_SIZE(time_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_TIME_SHOW])
		show = blobmsg_get_u32(tb[GUI_TIME_SHOW]);
	
	DBG_PRINTF("show = 0x%08x", show);
	if(0x00FFFFFF == show)
	{
		rc = -2; // 参数传递错误
	}
	else
	{
		rc = do_time(show);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_NETERR_STR,
	
	__GUI_NETERR_MAX
};
static const struct blobmsg_policy neterr_policy[] = {
	[GUI_NETERR_STR] = { .name = "str", .type = BLOBMSG_TYPE_STRING }
};
static int gui_neterr(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	char *str = 0;
	
	struct blob_attr *tb[__GUI_NETERR_MAX];
	
	blobmsg_parse(neterr_policy, ARRAY_SIZE(neterr_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_NETERR_STR])
		str = blobmsg_data(tb[GUI_NETERR_STR]);
	
	DBG_PRINTF("str: %s", str);
	int rc = 0;
	
	if(NULL == str)
	{
		rc = -2;
	}
	else
	{
		rc = do_neterr(str);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_LINE1_STR,
	
	__GUI_LINE1_MAX
};
static const struct blobmsg_policy line1_policy[] = {
	[GUI_LINE1_STR] = { .name = "str", .type = BLOBMSG_TYPE_STRING }
};
static int gui_line1(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	char *str = 0;
	
	struct blob_attr *tb[__GUI_LINE1_MAX];
	
	blobmsg_parse(line1_policy, ARRAY_SIZE(line1_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_LINE1_STR])
		str = blobmsg_data(tb[GUI_LINE1_STR]);
	
	DBG_PRINTF("str: %s", str);
	int rc = 0;
	
	if(NULL == str)
	{
		rc = -2;
	}
	else
	{
		rc = do_line1(str);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_LINE2_STR,
	
	__GUI_LINE2_MAX
};
static const struct blobmsg_policy line2_policy[] = {
	[GUI_LINE2_STR] = { .name = "str", .type = BLOBMSG_TYPE_STRING }
};
static int gui_line2(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{	
	char *str = 0;
	
	struct blob_attr *tb[__GUI_LINE2_MAX];
	
	blobmsg_parse(line2_policy, ARRAY_SIZE(line2_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_LINE2_STR])
		str = blobmsg_data(tb[GUI_LINE2_STR]);
	
	int rc = 0;
	
	if(NULL == str)
	{
		rc = -2;
	}
	else
	{
		rc = do_line2(str);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_LINE3_STR,
	
	__GUI_LINE3_MAX
};
static const struct blobmsg_policy line3_policy[] = {
	[GUI_LINE3_STR] = { .name = "str", .type = BLOBMSG_TYPE_STRING }
};
static int gui_line3(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *str = 0;
	
	struct blob_attr *tb[__GUI_LINE3_MAX];
	
	blobmsg_parse(line3_policy, ARRAY_SIZE(line3_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_LINE3_STR])
		str = blobmsg_data(tb[GUI_LINE3_STR]);
	
	int rc = 0;
	
	if(NULL == str)
	{
		rc = -2;
	}
	else
	{
		rc = do_line3(str);		
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

enum {
	GUI_TOAST_STR,
	GUI_TOAST_DUR,
	GUI_TOAST_STYLE,
	
	__GUI_TOAST_MAX
};
static const struct blobmsg_policy toast_policy[] = {
	[GUI_TOAST_STR]   = { .name = "str",   .type = BLOBMSG_TYPE_STRING },
	[GUI_TOAST_DUR]   = { .name = "dur",   .type = BLOBMSG_TYPE_INT32 },
	[GUI_TOAST_STYLE] = { .name = "style", .type = BLOBMSG_TYPE_INT32 }
};
static int gui_toast(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	int rc = 0;
	
	char *str = 0;
	int dur = 0;
	int style = 0;
	
	struct blob_attr *tb[__GUI_TOAST_MAX];
	
	blobmsg_parse(toast_policy, ARRAY_SIZE(toast_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[GUI_TOAST_STR])
		str = blobmsg_data(tb[GUI_TOAST_STR]);
	if (tb[GUI_TOAST_DUR])
		dur = blobmsg_get_u32(tb[GUI_TOAST_DUR]);
	if (tb[GUI_TOAST_STYLE])
		style = blobmsg_get_u32(tb[GUI_TOAST_STYLE]);
	
	DBG_PRINTF("str: %s, dur: %d, style: %d \n", str, dur, style);
	
	if(!str)
		rc = -2;
	else
		rc = do_toast(str,  dur,  style);
	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", rc);
	
	return ubus_send_reply(ctx, req, b.head);
}

static const struct ubus_method gui_methods[] = {

	UBUS_METHOD("hello",  	gui_hello,  hello_policy),
	UBUS_METHOD("shutdown", gui_shutdown, shutdown_policy),
	
	UBUS_METHOD("watch_down", gui_watch_down, watch_down_policy),
	
	UBUS_METHOD("airplay",  gui_airplay,  airplay_policy),
	UBUS_METHOD("wifi",  	gui_wifi,  wifi_policy),
	UBUS_METHOD("bluetooth",gui_bluetooth, bluetooth_policy),
	UBUS_METHOD("down", 	gui_down,  down_policy),	
	UBUS_METHOD("lock",		gui_lock,	lock_policy),
	UBUS_METHOD("alarm",	gui_alarm,	alarm_policy),	
	UBUS_METHOD("time", 	gui_time, time_policy),
	
	UBUS_METHOD("neterr", 	gui_neterr, neterr_policy),
	UBUS_METHOD("line1",	gui_line1, line1_policy),
	UBUS_METHOD("line2", 	gui_line2, line2_policy),
	UBUS_METHOD("line3",	gui_line3, line3_policy),
	UBUS_METHOD("toast",	gui_toast, toast_policy),
};

static struct ubus_object_type gui_object_type =
	UBUS_OBJECT_TYPE("gui", gui_methods);

static struct ubus_object gui_object = {
	.name = "gui",
	.type = &gui_object_type,
	.methods = gui_methods,
	.n_methods = ARRAY_SIZE(gui_methods)
};

static void watch_down_cb(struct uloop_timeout *timeout)
{
	int ret = -1;
	uint32_t watch_id = -1;
	
	ret = ubus_lookup_id(ctx, "down", &watch_id);
	if (ret) 
	{
		fprintf(stderr, "Failed to look up test object : %s \n", ubus_strerror(ret));
		return;
	}
	
	down_event.remove_cb = down_handle_remove;
	down_event.cb = down_notify;
	ret = ubus_subscribe(ctx, &down_event, watch_id);
	fprintf(stderr, " xxxWatchDownxxx Watching object %08x: %s\n", watch_id, ubus_strerror(ret));
}
static struct uloop_timeout watch_down_timer = {
	.cb = watch_down_cb
};

int ubus_init(void)
{
	ctx = ubus_connect(NULL);
	if (!ctx) 
	{
		fprintf(stderr, "Failed to connect to ubus ctx \n");
		return -1;
	}
	ubus_add_uloop(ctx);
	
	int ret = ubus_add_object(ctx, &gui_object);
	if (ret)
	{
		fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
		return -1;
	}
	
	ret = ubus_register_subscriber(ctx, &down_event);
	if (ret)
		fprintf(stderr, "Failed to add watch handler: %s\n", ubus_strerror(ret));
	
	uloop_timeout_set(&uloop_alive_timer, TIMER_ALIVE_UNIT); // 保活心跳定时器
	uloop_timeout_set(&watch_down_timer, WATCH_DOWN_PERIOD); // 订阅down消息
	
	return 0;
}

void ubus_done(void)
{
	ubus_free(ctx);	
}

void ubus_term(void)
{
	do_shutdown();
	uloop_end();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

static down_exist_handler_t down_exist_async_result_cb = NULL;

enum {
	EXIST_RC,
	EXIST_URL,
	
	__EXIST_MAX
};
static const struct blobmsg_policy exist_policy[] = {
	[EXIST_RC]      = { .name = "rc",  .type = BLOBMSG_TYPE_INT32 },
	[EXIST_URL]     = { .name = "url", .type = BLOBMSG_TYPE_STRING }
};
static void down_exist_async_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
	if (!msg)
		return;
	
	int rc = 0;
	char *url = NULL;
	
	struct blob_attr *tb[__EXIST_MAX];
	
	blobmsg_parse(exist_policy, ARRAY_SIZE(exist_policy), tb, blob_data(msg), blob_len(msg));
	
	if (tb[EXIST_RC])
		rc = blobmsg_get_u32(tb[EXIST_RC]);
	if (tb[EXIST_URL])
		url = blobmsg_data(tb[EXIST_URL]);
	
	fprintf(stderr, "  down_exist_async_result_data.rc     : %d \n", rc);
	fprintf(stderr, "  down_exist_async_result_data.url    : %s \n", url);
	
	if(down_exist_async_result_cb)
	{
		down_exist_async_result_cb(rc, url);
	}
}

int down_exist_async(const char *url, down_exist_handler_t cb)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	down_exist_async_result_cb = cb;
	
	static struct ubus_request req;
	uint32_t id;
	
	if (ubus_lookup_id(ctx, "down", &id)) 
	{
		fprintf(stderr, "Failed to look up down object \n");
		return -1;
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "url", url);
	
	ubus_invoke_async(ctx, id, "exist", b.head, &req);
	req.data_cb = down_exist_async_result_data;
	ubus_complete_request_async(ctx, &req);
	
	return 0;
}

static void down_create_async_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
	char *str;
	if (!msg)
		return;
	
	str = blobmsg_format_json_indent(msg, true, 0);
	
	printf("down_create_async_result_data: \n");
	printf("%s \n", str);
	
	free(str);
}
int down_create_async(const char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	fprintf(stderr, "down_create_async url: %s \n", url);
	char url_trim[MAX_URL] = { 0 };
	strcpy(url_trim, url);
	trim(url_trim);
	fprintf(stderr, "down_create_async url_trim: %s \n", url_trim);
	
	static struct ubus_request req;
	uint32_t id;
	
	if (ubus_lookup_id(ctx, "down", &id)) 
	{
		fprintf(stderr, "Failed to look up down object \n");
		return -1;
	}
	
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "url", url_trim);
	
	ubus_invoke_async(ctx, id, "create", b.head, &req);
	req.data_cb = down_create_async_result_data;
	ubus_complete_request_async(ctx, &req);
	
	return 0;
}

