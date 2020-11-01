

#ifndef __HSLOG_H__
#define __HSLOG_H__

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/* Definations for version info */
#define HSLOGVERSION_MAJOR  1
#define HSLOGVERSION_MINOR  0
#define HSLOGBUILD_NUM      1

/* Loging flags */
#define HSLOG_NONE   0
#define HSLOG_LIVE   1
#define HSLOG_INFO   2
#define HSLOG_WARN   3
#define HSLOG_DEBUG  4
#define HSLOG_ERROR  5
#define HSLOG_FATAL  6
#define HSLOG_PANIC  7

/* Supported colors */
#define CLR_NORMAL   "\x1B[0m"
#define CLR_RED      "\x1B[31m"
#define CLR_GREEN    "\x1B[32m"
#define CLR_YELLOW   "\x1B[33m"
#define CLR_BLUE     "\x1B[34m"
#define CLR_NAGENTA  "\x1B[35m"
#define CLR_CYAN     "\x1B[36m"
#define CLR_WHITE    "\x1B[37m"
#define CLR_RESET    "\033[0m"

#define LVL1(x) #x
#define LVL2(x) LVL1(x)
#define SOURCE_THROW_LOCATION "<"__FILE__":"LVL2(__LINE__)"> -- "

/* 
 * Define macros to allow us get further informations 
 * on the corresponding erros. These macros used as wrappers 
 * for hslog() function.
 */
#define hslog_none(...) \
    hslog(HSLOG_NONE, __VA_ARGS__);

#define hslog_live(...) \
    hslog(HSLOG_LIVE, __VA_ARGS__);

#define hslog_info(...) \
    hslog(HSLOG_INFO, __VA_ARGS__);

#define hslog_warn(...) \
    hslog(HSLOG_WARN, __VA_ARGS__);

#define hslog_debug(...) \
    hslog(HSLOG_DEBUG, __VA_ARGS__);

#define hslog_none_loc(...) \
    hslog(HSLOG_NONE, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_live_loc(...) \
    hslog(HSLOG_LIVE, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_info_loc(...) \
    hslog(HSLOG_INFO, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_warn_loc(...) \
    hslog(HSLOG_WARN, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_debug_loc(...) \
    hslog(HSLOG_DEBUG, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_error(...) \
    hslog(HSLOG_ERROR, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_fatal(...) \
    hslog(HSLOG_FATAL, SOURCE_THROW_LOCATION __VA_ARGS__);

#define hslog_panic(...) \
    hslog(HSLOG_PANIC, SOURCE_THROW_LOCATION __VA_ARGS__);

/* Flags */
typedef struct {
    char sFileName[64];
    char sModuleName[64];
    unsigned long  nLogFileSize;
    unsigned short nFileLevel;
    unsigned short nLogLevel;
    unsigned short nToFile:1;
    unsigned short nPretty:1;
    unsigned short nTdSafe:1;
    unsigned short nErrLog:1;
    unsigned short nSilent:1;
    unsigned short nSync:1;
    pthread_mutex_t hslogLock;
} HSlogConfig;

/* Date variables */
typedef struct {
    int year; 
    int mon; 
    int day;
    int hour;
    int min;
    int sec;
    int usec;
} HSlogDate;

typedef struct {
    int nType;
    const char* pDesc;
    const char* pColor;
} HSlogTag;

typedef void (*hslog_prepare_output_func)(const char* pStr, const HSlogDate *pDate, int nType, int nColor, char* pOut, int nSize);
void hslog_set_output_func(hslog_prepare_output_func output_function);

const char* hslog_version(int nMin);

int init_network_log(char *multicast_groupIp, char *ttl, short port);

void hslog_config_get(HSlogConfig *pCfg);
void hslog_config_set(HSlogConfig *pCfg);
void hslog_set_module(const char *str);
void hslog_rotatelogfile(void);

void hslog_init(const char* pName, const char* pConf, int nLogLevel, int nTdSafe);
void hslog(int flag, const char *pMsg, ...);
void hslog_to_network(const char* pStr, const HSlogDate *pDate, int nType, char *module);

void enable_network_logging(void);
void disable_network_logging(void);
void enable_unicast_logging(void);
void disable_unicast_logging(void);
int start_inotify_service(char *dir,char *file);
#define	SERVICE_LOG_CFG "udpservice_log.cfg"
#define UDP_SERVICE_LOG  "udp-service.log"

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __HSLOG_H__ */
