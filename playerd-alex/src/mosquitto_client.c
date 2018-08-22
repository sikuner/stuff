#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include <iwinfo.h>

#define VERSION "0.2.0"

#define LOGD(fmt, args...) syslog(LOG_SYSLOG, "[PLAYERD](D)"fmt, ##args)

#include "config.h"

#include <mosquitto.h>

/* This struct is used to pass data to callbacks.
 * An instance "ud" is created in main() and populated, then passed to
 * mosquitto_new(). */
struct userdata {
	char **topics;
	int topic_count;
	int topic_qos;
	char *username;
	char *password;
	int verbose;
	bool quiet;
	bool no_retain;
};

static char *get_report_info(const char *ip, const char *ssid);
static int get_ip_ssid(char *ip, char *ssid);
static int get_ip(char *ip);

static struct mosquitto *mosq = NULL;
static char *id = NULL;
static char lan_ip[20] = {'\0'};

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct userdata *ud;

	assert(obj);
	ud = (struct userdata *)obj;

	if(message->retain && ud->no_retain) return;

	if(ud->verbose){
		if(message->payloadlen){
			PDEBUG("%s ", message->topic);
			fwrite(message->payload, 1, message->payloadlen, stdout);
			PDEBUG("\n");
		}else{
			PDEBUG("%s (null)\n", message->topic);
		}
		fflush(stdout);
	}else{
		if(message->payloadlen){
/*
			char cmd[256];
			fwrite(message->payload, 1, message->payloadlen, stdout);
			printf("\n");
			fflush(stdout);
			sprintf(cmd, "/usr/bin/playmusic.sh %s", message->payload);
			system(cmd);
*/
//			parse_cmd(message->payload);
			msg_put(message->payload);
		}
	}
}

void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct userdata *ud;

	assert(obj);
	ud = (struct userdata *)obj;

	if(!result){
		for(i=0; i<ud->topic_count; i++){
			mosquitto_subscribe(mosq, NULL, ud->topics[i], ud->topic_qos);
		}
	}else{
		if(result && !ud->quiet){
			PERROR("%s\n", mosquitto_connack_string(result));
		}
	}
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	char ip[20] = {'\0'};

	if (get_ip(ip) < 0) {
		syslog(LOG_SYSLOG, "===== get ip failed\n");
		return;
	}

	syslog(LOG_SYSLOG, "=====  : %s %d: ip=%s, lan_ip=%s", __func__, __LINE__, ip, lan_ip);

	if (strncmp(ip, lan_ip, sizeof(ip)) != 0) {
		PERROR("ip is changed, exit ...\n");
		exit(0);
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	struct userdata *ud;

	assert(obj);
	ud = (struct userdata *)obj;

	if(!ud->quiet) printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		if(!ud->quiet) printf(", %d", granted_qos[i]);
	}
	if(!ud->quiet) printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	PDEBUG("%s\n", str);
}

void print_usage(void)
{
	int major, minor, revision;

	mosquitto_lib_version(&major, &minor, &revision);
	printf("playerd is a simple mqtt client that will subscribe to a single topic and print all messages it receives.\n");
	printf("playerd version %s running on libmosquitto %d.%d.%d.\n\n", VERSION, major, minor, revision);
	printf("Usage: playerd [-c] [-h host] [-k keepalive] [-p port] [-q qos] [-R] [-v] -t topic ...\n");
	printf("                     [-A bind_address]\n");
	printf("                     [-i id] [-I id_prefix]\n");
	printf("                     [-d] [--quiet]\n");
	printf("                     [-u username [-P password]]\n");
	printf("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
#ifdef WITH_TLS
	printf("                     [{--cafile file | --capath dir} [--cert file] [--key file] [--insecure]]\n");
#ifdef WITH_TLS_PSK
	printf("                     [--psk hex-key --psk-identity identity]\n");
#endif
#endif
	printf("       mosquitto_sub --help\n\n");
	printf(" -A : bind the outgoing socket to this host/ip address. Use to control which interface\n");
	printf("      the client communicates over.\n");
	printf(" -c : disable 'clean session' (store subscription and pending messages when client disconnects).\n");
	printf(" -d : enable debug messages.\n");
	printf(" -h : mqtt host to connect to. Defaults to localhost.\n");
	printf(" -i : id to use for this client. Defaults to mosquitto_sub_ appended with the process id.\n");
	printf(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
	printf("      broker is using the clientid_prefixes option.\n");
	printf(" -k : keep alive in seconds for this client. Defaults to 60.\n");
	printf(" -p : network port to connect to. Defaults to 1883.\n");
	printf(" -q : quality of service level to use for the subscription. Defaults to 0.\n");
	printf(" -R : do not print stale messages (those with retain set).\n");
	printf(" -t : mqtt topic to subscribe to. May be repeated multiple times.\n");
	printf(" -u : provide a username (requires MQTT 3.1 broker)\n");
	printf(" -v : print published messages verbosely.\n");
	printf(" -P : provide a password (requires MQTT 3.1 broker)\n");
	printf(" --help : display this message.\n");
	printf(" --quiet : don't print error messages.\n");
	printf(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
	printf("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
	printf("                  length message will be sent.\n");
	printf(" --will-qos : QoS level for the client Will.\n");
	printf(" --will-retain : if given, make the client Will retained.\n");
	printf(" --will-topic : the topic on which to publish the client Will.\n");
#ifdef WITH_TLS
	printf(" --cafile : path to a file containing trusted CA certificates to enable encrypted\n");
	printf("            certificate based communication.\n");
	printf(" --capath : path to a directory containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --cert : client certificate for authentication, if required by server.\n");
	printf(" --key : client private key for authentication, if required by server.\n");
	printf(" --tls-version : TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1.\n");
	printf("                 Defaults to tlsv1.2 if available.\n");
	printf(" --insecure : do not check that the server certificate hostname matches the remote\n");
	printf("              hostname. Using this option means that you cannot be sure that the\n");
	printf("              remote host is the server you wish to connect to and so is insecure.\n");
	printf("              Do not use this option in a production environment.\n");
#ifdef WITH_TLS_PSK
	printf(" --psk : pre-shared-key in hexadecimal (no leading 0x) to enable TLS-PSK mode.\n");
	printf(" --psk-identity : client identity string for TLS-PSK mode.\n");
#endif
#endif
}

int main(int argc, char *argv[])
{
	char *id_prefix = NULL;
	int i;
	char *host = "localhost";
	int port = 1883;
	char *bind_address = NULL;
	int keepalive = 120; //60;
	bool clean_session = true;
	bool debug = false;
	int rc;
	char hostname[256];
	char err[1024];
	struct userdata ud;
	int len;
	
	char *will_payload = NULL;
	long will_payloadlen = 0;
	int will_qos = 0;
	bool will_retain = false;
	char *will_topic = NULL;

	bool insecure = false;
	char *cafile = NULL;
	char *capath = NULL;
	char *certfile = NULL;
	char *keyfile = NULL;
	char *tls_version = NULL;

	char *psk = NULL;
	char *psk_identity = NULL;

	memset(&ud, 0, sizeof(struct userdata));

	for(i=1; i<argc; i++){
		if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")){
			if(i==argc-1){
				fprintf(stderr, "Error: -p argument given but no port specified.\n\n");
				print_usage();
				return 1;
			}else{
				port = atoi(argv[i+1]);
				if(port<1 || port>65535){
					fprintf(stderr, "Error: Invalid port given: %d\n", port);
					print_usage();
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "-A")){
			if(i==argc-1){
				fprintf(stderr, "Error: -A argument given but no address specified.\n\n");
				print_usage();
				return 1;
			}else{
				bind_address = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--disable-clean-session")){
			clean_session = false;
		}else if(!strcmp(argv[i], "--cafile")){
			if(i==argc-1){
				fprintf(stderr, "Error: --cafile argument given but no file specified.\n\n");
				print_usage();
				return 1;
			}else{
				cafile = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--capath")){
			if(i==argc-1){
				fprintf(stderr, "Error: --capath argument given but no directory specified.\n\n");
				print_usage();
				return 1;
			}else{
				capath = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--cert")){
			if(i==argc-1){
				fprintf(stderr, "Error: --cert argument given but no file specified.\n\n");
				print_usage();
				return 1;
			}else{
				certfile = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")){
			debug = true;
		}else if(!strcmp(argv[i], "--help")){
			print_usage();
			return 0;
		}else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host")){
			if(i==argc-1){
				fprintf(stderr, "Error: -h argument given but no host specified.\n\n");
				print_usage();
				return 1;
			}else{
				host = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--insecure")){
			insecure = true;
		}else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--id")){
			if(id_prefix){
				fprintf(stderr, "Error: -i and -I argument cannot be used together.\n\n");
				print_usage();
				return 1;
			}
			if(i==argc-1){
				fprintf(stderr, "Error: -i argument given but no id specified.\n\n");
				print_usage();
				return 1;
			}else{
				id = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-I") || !strcmp(argv[i], "--id-prefix")){
			if(id){
				fprintf(stderr, "Error: -i and -I argument cannot be used together.\n\n");
				print_usage();
				return 1;
			}
			if(i==argc-1){
				fprintf(stderr, "Error: -I argument given but no id prefix specified.\n\n");
				print_usage();
				return 1;
			}else{
				id_prefix = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-k") || !strcmp(argv[i], "--keepalive")){
			if(i==argc-1){
				fprintf(stderr, "Error: -k argument given but no keepalive specified.\n\n");
				print_usage();
				return 1;
			}else{
				keepalive = atoi(argv[i+1]);
				if(keepalive>65535){
					fprintf(stderr, "Error: Invalid keepalive given: %d\n", keepalive);
					print_usage();
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "--key")){
			if(i==argc-1){
				fprintf(stderr, "Error: --key argument given but no file specified.\n\n");
				print_usage();
				return 1;
			}else{
				keyfile = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--psk")){
			if(i==argc-1){
				fprintf(stderr, "Error: --psk argument given but no key specified.\n\n");
				print_usage();
				return 1;
			}else{
				psk = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--psk-identity")){
			if(i==argc-1){
				fprintf(stderr, "Error: --psk-identity argument given but no identity specified.\n\n");
				print_usage();
				return 1;
			}else{
				psk_identity = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-q") || !strcmp(argv[i], "--qos")){
			if(i==argc-1){
				fprintf(stderr, "Error: -q argument given but no QoS specified.\n\n");
				print_usage();
				return 1;
			}else{
				ud.topic_qos = atoi(argv[i+1]);
				if(ud.topic_qos<0 || ud.topic_qos>2){
					fprintf(stderr, "Error: Invalid QoS given: %d\n", ud.topic_qos);
					print_usage();
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "--quiet")){
			ud.quiet = true;
		}else if(!strcmp(argv[i], "-R")){
			ud.no_retain = true;
		}else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--topic")){
			if(i==argc-1){
				fprintf(stderr, "Error: -t argument given but no topic specified.\n\n");
				print_usage();
				return 1;
			}else{
				ud.topic_count++;
				ud.topics = realloc(ud.topics, ud.topic_count*sizeof(char *));
				ud.topics[ud.topic_count-1] = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--tls-version")){
			if(i==argc-1){
				fprintf(stderr, "Error: --tls-version argument given but no version specified.\n\n");
				print_usage();
				return 1;
			}else{
				tls_version = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-u") || !strcmp(argv[i], "--username")){
			if(i==argc-1){
				fprintf(stderr, "Error: -u argument given but no username specified.\n\n");
				print_usage();
				return 1;
			}else{
				ud.username = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")){
			ud.verbose = 1;
		}else if(!strcmp(argv[i], "-P") || !strcmp(argv[i], "--pw")){
			if(i==argc-1){
				fprintf(stderr, "Error: -P argument given but no password specified.\n\n");
				print_usage();
				return 1;
			}else{
				ud.password = argv[i+1];
			}
			i++;
		}else if(!strcmp(argv[i], "--will-payload")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-payload argument given but no will payload specified.\n\n");
				print_usage();
				return 1;
			}else{
				will_payload = argv[i+1];
				will_payloadlen = strlen(will_payload);
			}
			i++;
		}else if(!strcmp(argv[i], "--will-qos")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-qos argument given but no will QoS specified.\n\n");
				print_usage();
				return 1;
			}else{
				will_qos = atoi(argv[i+1]);
				if(will_qos < 0 || will_qos > 2){
					fprintf(stderr, "Error: Invalid will QoS %d.\n\n", will_qos);
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "--will-retain")){
			will_retain = true;
		}else if(!strcmp(argv[i], "--will-topic")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-topic argument given but no will topic specified.\n\n");
				print_usage();
				return 1;
			}else{
				will_topic = argv[i+1];
			}
			i++;
		}else{
			fprintf(stderr, "Error: Unknown option '%s'.\n",argv[i]);
			print_usage();
			return 1;
		}
	}

	if(clean_session == false && (id_prefix || !id)){
		if(!ud.quiet) fprintf(stderr, "Error: You must provide a client id if you are using the -c option.\n");
		return 1;
	}

	if(ud.topic_count == 0){
		fprintf(stderr, "Error: You must specify a topic to subscribe to.\n");
		print_usage();
		return 1;
	}
	if(will_payload && !will_topic){
		fprintf(stderr, "Error: Will payload given, but no will topic given.\n");
		print_usage();
		return 1;
	}
	if(will_retain && !will_topic){
		fprintf(stderr, "Error: Will retain given, but no will topic given.\n");
		print_usage();
		return 1;
	}
	if(ud.password && !ud.username){
		if(!ud.quiet) fprintf(stderr, "Warning: Not using password since username not set.\n");
	}
	if((certfile && !keyfile) || (keyfile && !certfile)){
		fprintf(stderr, "Error: Both certfile and keyfile must be provided if one of them is.\n");
		print_usage();
		return 1;
	}
	if((cafile || capath) && psk){
		if(!ud.quiet) fprintf(stderr, "Error: Only one of --psk or --cafile/--capath may be used at once.\n");
		return 1;
	}
	if(psk && !psk_identity){
		if(!ud.quiet) fprintf(stderr, "Error: --psk-identity required if --psk used.\n");
		return 1;
	}

	mosquitto_lib_init();

	if(id_prefix){
		id = malloc(strlen(id_prefix)+10);
		if(!id){
			if(!ud.quiet) fprintf(stderr, "Error: Out of memory.\n");
			mosquitto_lib_cleanup();
			return 1;
		}
		snprintf(id, strlen(id_prefix)+10, "%s%d", id_prefix, getpid());
	}else if(!id){
		hostname[0] = '\0';
		gethostname(hostname, 256);
		hostname[255] = '\0';
		len = strlen("mosqsub/-") + 6 + strlen(hostname);
		id = malloc(len);
		if(!id){
			if(!ud.quiet) fprintf(stderr, "Error: Out of memory.\n");
			mosquitto_lib_cleanup();
			return 1;
		}
		snprintf(id, len, "mosqsub/%d-%s", getpid(), hostname);
		if(strlen(id) > MOSQ_MQTT_ID_MAX_LENGTH){
			/* Enforce maximum client id length of 23 characters */
			id[MOSQ_MQTT_ID_MAX_LENGTH] = '\0';
		}
	}

	char myid[2048] = { '\0' };
	while (1) {
		char ssid[IWINFO_ESSID_MAX_SIZE+1] = { 0 };
		if (get_ip_ssid(lan_ip, ssid) < 0) {
			LOGD("ip is null\n");
			sleep(2);
			continue;
		}

		char *info = get_report_info(lan_ip, ssid);
		strncat(myid, id, sizeof(myid));
		strncat(myid, "@", sizeof(myid));
		strncat(myid, info, sizeof(myid));
		LOGD("id is [%s]\n", myid);
		break;
	}

	mosq = mosquitto_new(myid, clean_session, &ud);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!ud.quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!ud.quiet) fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	if(debug){
		mosquitto_log_callback_set(mosq, my_log_callback);
	}
	if(will_topic && mosquitto_will_set(mosq, will_topic, will_payloadlen, will_payload, will_qos, will_retain)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting will.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(ud.username && mosquitto_username_pw_set(mosq, ud.username, ud.password)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting username and password.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if((cafile || capath) && mosquitto_tls_set(mosq, cafile, capath, certfile, keyfile, NULL)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(insecure && mosquitto_tls_insecure_set(mosq, true)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS insecure option.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(psk && mosquitto_tls_psk_set(mosq, psk, psk_identity, NULL)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS-PSK options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(tls_version && mosquitto_tls_opts_set(mosq, 1, tls_version, NULL)){
		if(!ud.quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	if(debug){
		mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	}

	rc = mosquitto_connect_bind(mosq, host, port, keepalive, bind_address);
	if(rc){
		if(!ud.quiet){
			if(rc == MOSQ_ERR_ERRNO){
#ifndef WIN32
				strerror_r(errno, err, 1024);
#else
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, 1024, NULL);
#endif
				fprintf(stderr, "Error: %s\n", err);
			}else{
				fprintf(stderr, "Unable to connect (%d).\n", rc);
			}
		}
		mosquitto_lib_cleanup();
		return rc;
	}

	output_mpd_init();
	msg_init();

	rc = mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	if(rc){
		if(rc == MOSQ_ERR_ERRNO){
			fprintf(stderr, "Error: %s\n", strerror(errno));
		}else{
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		}
	}
	return rc;
}

char *get_device_id()
{
	return id;
}

void report_to_server(const char *msg)
{
	int rc;
	int qos = 2;
	char topic[256];

	if (mosq == NULL) {
		PERROR("Error: mosq is null\n");
		return;
	}

	snprintf(topic, sizeof(topic), "S_%c", id[0]);

	rc = mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, qos, false);

	if (rc != MOSQ_ERR_SUCCESS) {
		PERROR("Error: publish failed : %d\n", rc);
	}
}

#define VERSION_FILE "/etc/beeba_version"
static int read_line_from_file(const char *file, char *buf, int buf_size)
{
	struct stat st;
	FILE *fd;
	int len;

	memset(&st,0,sizeof(st));

	if (stat(file, &st) < 0) {
		goto out;
	}

	if (st.st_size <= 0) {
		goto out;
	}

	if ((fd=fopen(file, "rb")) != NULL) {
		memset(buf, 0, buf_size);
		len = fread((void *)buf, 1, buf_size-1, fd);
		fclose(fd);
		return len;
	}

out:
	PERROR("cannot load buf from file: %s, %s\n", file, strerror(errno));
	return -1;
}

static void get_version(char *ver)
{
	read_line_from_file(VERSION_FILE, ver, 6);
	ver[5] = '\0';
}

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int get_ip_by_ifname(const char *fname, char *ip)
{
	struct ifaddrs * ifAddrStruct=NULL;
	void * tmpAddrPtr=NULL;
	int ret = -1;

	getifaddrs(&ifAddrStruct);

	while (ifAddrStruct != NULL) {
		if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4 // is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
			if (strcmp(ifAddrStruct->ifa_name, fname) == 0) {
				strncpy(ip, addressBuffer, 17);
				printf("got ip: %s\n", ip);
				ret = 0;
				break;
			}
		}
		ifAddrStruct=ifAddrStruct->ifa_next;
	}

	return ret;
}

#include <uci.h>

static void get_device_name(const char **name)
{
	struct uci_context *ctx = NULL;
	//struct uci_element *e;
	struct uci_ptr ptr;
	char str[64];

	ctx = uci_alloc_context();
	if (!ctx) {
		return;
	}

	strcpy(str, "device.beeba.name");

	if (uci_lookup_ptr(ctx, &ptr, str, true) != UCI_OK) {
		return;
	}

	*name = strdup(ptr.o->v.string);
}

static void get_ssid(char *ssid)
{
	const struct iwinfo_ops *iw;
	const char *ifname = "wlan0";

	iw = iwinfo_backend(ifname);

	if (!iw) {
		PERROR("No such wireless device: %s\n", ifname);
		return;
	}

	if (iw->ssid(ifname, ssid))
		memset(ssid, 0, sizeof(ssid));

	iwinfo_finish();
}

static int get_ip(char *ip)
{
	if (get_ip_by_ifname("wlan0", ip) < 0) {
		if (get_ip_by_ifname("eth1", ip) == 0) {
			return 0;
		}
	} else {
		return 0;
	}

	return -1;
}

static int get_ip_ssid(char *ip, char *ssid)
{
	if (get_ip_by_ifname("wlan0", ip) < 0) {
		if (get_ip_by_ifname("eth1", ip) == 0) {
			strcpy(ssid, "wan_is_eth1");
			return 0;
		}
	} else {
		get_ssid(ssid);
		return 0;
	}

	return -1;
}

static char *get_report_info(const char *ip, const char *ssid)
{
	char ver[6] = {'\0'};
	char data[1024] = {'\0'};
	char data2[2048] = {'\0'};
	char *name = NULL;

	get_version(ver);
	get_device_name(&name);

	snprintf(data, sizeof(data), "%s|%s|%s|%s", ver, ip, ssid, name);
	syslog(LOG_SYSLOG, "===== net_info : %s", data);

	if (name != NULL)
		free(name);

	base64_in(data, data2, strlen(data));

	return strdup(data2);
}

