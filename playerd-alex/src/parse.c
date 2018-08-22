#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>

#include <json-c/json.h>
#include <json-c/json_tokener.h>

#include "config.h"

#define STRING_ACTION "action"
#define STRING_PARAM "param"
#define STRING_FLAG "flag"


int  parse_cmd(const char *data)
{
	json_object *json = NULL;
	json_object *obj_action = NULL;
	json_object *obj_param = NULL;
	json_object *obj_flag = NULL;
	char *action;
	int flag;
	char *param;
	enum json_tokener_error error;
	char cmd[20480];
	int playing = 0;

	if (data == NULL) {
		PERROR("data is null\n");
		return -1;
	} else {
		PINFO("data: [%s]\n", data);
	}

	json = json_tokener_parse_verbose(data, &error);

	if (json == NULL) {
		PERROR("json is null: %s\n", json_tokener_error_desc(error));
		return -1;
	}

	if (json_object_object_get_ex(json, STRING_ACTION, &obj_action) == TRUE) {
		action = json_object_get_string(obj_action);
		PDEBUG("got action is : [%s]\n", action);
	} else {
		PERROR("cannot get action");
		goto out;
	}

	if (json_object_object_get_ex(json, STRING_FLAG, &obj_flag) == TRUE) {
		flag = json_object_get_int(obj_flag);
		PDEBUG("got flag is : [%d]\n", flag);
	} else {
		PERROR("cannot get flag");
		goto out;
	}

	if (json_object_object_get_ex(json, STRING_PARAM, &obj_param) == TRUE) {
		param = json_object_to_json_string(obj_param);
		PDEBUG("got param is : [%s]\n", param);
	} else {
		PERROR("cannot get param");
		goto out;
	}

	if (strcmp(action, "play") == 0) {
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/playmusic.sh %s", param);
	} else if (strcmp(action, "playlist") == 0 || strcmp(action, "playlisturl") == 0) {
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/playlist.sh %s %s %d", action, param, flag);
	} else if (strcmp(action, "toggle") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc toggle");
	} else if (strcmp(action, "resume") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc play");
	} else if (strcmp(action, "pause") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc pause");
	} else if (strcmp(action, "stop") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc stop");
	} else if (strcmp(action, "prev") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc prev");
	} else if (strcmp(action, "next") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc next");
	} else if (strcmp(action, "volumeplus") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc volume +5");
	} else if (strcmp(action, "volumeminus") == 0) {
		snprintf(cmd, sizeof(cmd), "mpc volume -5");
	} else if (strcmp(action, "setvolume") == 0) {
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "mpc volume %s", param);
	} else if (strcmp(action, "getvolume") == 0) {
		int vol = output_mpd_get_volume();
		snprintf(cmd, sizeof(cmd), "{'volume':'%d'}", vol);
		report_to_server(cmd);
		goto out;
	} else if (strcmp(action, "getstate") == 0) {
		update_status_to_server();
		goto out;
	} else if (strcmp(action, "updatestatus") == 0) {
		set_update_status_time(flag);
		update_status_to_server();
		goto out;
	} else if (strcmp(action, "command") == 0) {
		param = json_object_get_string(obj_param);
		PINFO("recieved command: %s\n", param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/btools command %s", param);
	} else if (strcmp(action, "enableap") == 0) {
		param = json_object_to_json_string(obj_param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/enable_ap.sh %s", param);
		PINFO("enableap: %s\n", param);
	} else if (strcmp(action, "updatesubscribe") == 0) {
		param = json_object_get_string(obj_param);
//		snprintf(cmd, sizeof(cmd), "/usr/bin/btools updatesubscribe %s", param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/sync_contents.sh %s &", param);
		system(cmd);
		sleep(1);
		goto out;
	} else if (strcmp(action, "start_write_nfc_card") == 0) {
		PINFO("start_write_nfc_card\n");
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "killall -USR1 nfcd; echo '%s' > /tmp/nfc_request_write.data; echo %d > /tmp/nfc_write_card.flag", param, flag);
	} else if (strcmp(action, "stop_write_nfc_card") == 0) {
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "rm -f /tmp/nfc_write_card.flag; killall -USR1 nfcd");
	} else if (strcmp(action, "play_tts") == 0) {
		param = json_object_get_string(obj_param);
		PINFO("tts: '%s'\n", param);
		snprintf(cmd, sizeof(cmd), "/usr/bin/playtts.sh '%s' %d &>/var/log/playtts.log", param, flag);
	} else if (strcmp(action, "do_test_echo") == 0) {
		PINFO("do test echo\n");
		param = json_object_get_string(obj_param);
		snprintf(cmd, sizeof(cmd), "curl -L -k -X POST '%s' 1>/var/log/do_test_echo.log 2>&1", param);
	} else if (strcmp(action, "poweroff") == 0) {
		PINFO("poweroff by playerd\n");
		/* FIXME: 安全性保证 */
		snprintf(cmd, sizeof(cmd), "killall -USR2 guid");
	} else {
		PERROR("unsurppose action: %s\n", action);
		goto out;
	}

	PDEBUG("do cmd: %s\n", cmd);
	system(cmd);

out:
	if (obj_action != NULL)
		json_object_put(obj_action);
	if (obj_param != NULL)
		json_object_put(obj_param);
	if (obj_flag != NULL)
		json_object_put(obj_flag);
	if (json != NULL)
		json_object_put(json);
}

/*
int main(int argc, char **argv)
{
	if (argc > 1) {
		printf("argv[1] = %s\n", argv[1]);
		parse_cmd(argv[1]);
	}
}
*/
