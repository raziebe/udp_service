
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include "hslog.h"

/* Max size of string */
#define MAXMSG 8196
#define BIGSTR 4098

void hslog_prepare_output(const char* pStr, const HSlogDate *pDate, int nType, int nColor, char* pOut, int nSize);

static HSlogConfig g_hslogCfg;
static hslog_prepare_output_func prepare_output_func = hslog_prepare_output;
static HSlogTag g_HSlogTags[] =
{
    { 0, "NONE", NULL },
    { 1, "LIVE", CLR_NORMAL },
    { 2, "INFO", CLR_GREEN },
    { 3, "WARN", CLR_YELLOW },
    { 4, "DEBUG", CLR_BLUE },
    { 5, "ERROR", CLR_RED},
    { 6, "FATAL", CLR_RED },
    { 7, "PANIC", CLR_WHITE }
};

void hslog_sync_lock()
{
    if (g_hslogCfg.nTdSafe)
    {
        if (pthread_mutex_lock(&g_hslogCfg.hslogLock))
        {
            printf("[ERROR] Slog can not lock mutex: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
}

void hslog_sync_unlock()
{
    if (g_hslogCfg.nTdSafe) 
    {
        if (pthread_mutex_unlock(&g_hslogCfg.hslogLock))
        {
            printf("[ERROR] Slog can not unlock mutex: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
}

void hslog_sync_init()
{
    if (g_hslogCfg.nTdSafe)
    {
        /* Init mutex attribute */
        pthread_mutexattr_t m_attr;
        if (pthread_mutexattr_init(&m_attr) ||
            pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE) ||
            pthread_mutex_init(&g_hslogCfg.hslogLock, &m_attr) ||
            pthread_mutexattr_destroy(&m_attr))
        {
            printf("[ERROR] Slog can not initialize mutex: %d\n", errno);
            g_hslogCfg.nTdSafe = 0;
            g_hslogCfg.nSync = 0;
        }

        g_hslogCfg.nSync = 1;
    }
}

void hslog_sync_destroy()
{
    if (pthread_mutex_destroy(&g_hslogCfg.hslogLock))
    {
        printf("[ERROR] Can not deinitialize mutex: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    g_hslogCfg.nSync = 0;
    g_hslogCfg.nTdSafe = 0;
}

#ifdef DARWIN
static inline int clock_gettime(int clock_id, struct timespec *ts)
{
    struct timeval tv;

    if (clock_id != CLOCK_REALTIME) 
    {
        errno = EINVAL;
        return -1;
    }
    if (gettimeofday(&tv, NULL) < 0) 
    {
        return -1;
    }
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
    return 0;
}
#endif /* DARWIN */

void hslog_get_date(HSlogDate *pDate)
{
    struct tm timeinfo;
    time_t rawtime = time(NULL);
    localtime_r(&rawtime, &timeinfo);

    /* Get System Date */
    pDate->year = timeinfo.tm_year+1900;
    pDate->mon = timeinfo.tm_mon+1;
    pDate->day = timeinfo.tm_mday;
    pDate->hour = timeinfo.tm_hour;
    pDate->min = timeinfo.tm_min;
    pDate->sec = timeinfo.tm_sec;

    /* Get micro seconds */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pDate->usec = now.tv_nsec / 10000000;
}

const char* hslog_version(int nMin)
{
    static char sVersion[128];

    /* Version short */
    if (nMin) sprintf(sVersion, "%d.%d.%d", 
        HSLOGVERSION_MAJOR, HSLOGVERSION_MINOR, HSLOGBUILD_NUM);

    /* Version long */
    else sprintf(sVersion, "%d.%d build %d (%s)", 
        HSLOGVERSION_MAJOR, HSLOGVERSION_MINOR, HSLOGBUILD_NUM, __DATE__);

    return sVersion;
}

void hslog_prepare_output(const char* pStr, const HSlogDate *pDate, int nType, int nColor, char* pOut, int nSize)
{
    char sDate[32];
    snprintf(sDate, sizeof(sDate), "%02d.%02d.%02d-%02d:%02d:%02d.%02d", 
                    pDate->year, pDate->mon, pDate->day, pDate->hour, 
                    pDate->min, pDate->sec, pDate->usec);

    /* Walk throu */
    int i;
    for (i = 0;; i++)
    //for (int i = 0;; i++)
    {
        if ((nType == HSLOG_NONE) && !g_HSlogTags[i].nType && (g_HSlogTags[i].pDesc == NULL))
        {
            if(strlen(g_hslogCfg.sModuleName) > 0)
                snprintf(pOut, nSize, "%s - %s - %s", sDate, g_hslogCfg.sModuleName, pStr);
            else
                snprintf(pOut, nSize, "%s - %s", sDate, pStr);
            return;
        }

        if ((nType != HSLOG_NONE) && (g_HSlogTags[i].nType == nType) && (g_HSlogTags[i].pDesc != NULL))
        {
            if (nColor){
                if(strlen(g_hslogCfg.sModuleName) > 0)
                    snprintf(pOut, nSize, "%s - %s - [%s%s%s] %s", sDate, g_hslogCfg.sModuleName, g_HSlogTags[i].pColor, g_HSlogTags[i].pDesc, CLR_RESET, pStr);
                else
                    snprintf(pOut, nSize, "%s - [%s%s%s] %s", sDate, g_HSlogTags[i].pColor, g_HSlogTags[i].pDesc, CLR_RESET, pStr);
            }
            else{
                if(strlen(g_hslogCfg.sModuleName) > 0)
                    snprintf(pOut, nSize, "%s - %s - [%s] %s", sDate, g_hslogCfg.sModuleName, g_HSlogTags[i].pDesc, pStr);
                else
                    snprintf(pOut, nSize, "%s - [%s] %s", sDate, g_HSlogTags[i].pDesc, pStr);
            }

            return;
        }
    }
}

void hslog_prepare_output_xml_format(const char* pStr, const HSlogDate *pDate, int nType, int nColor, char* pOut, int nSize)
{
    char sDate[32];
    int i;
    snprintf(sDate, sizeof(sDate), "%02d.%02d.%02d-%02d:%02d:%02d.%02d", 
                    pDate->year, pDate->mon, pDate->day, pDate->hour, 
                    pDate->min, pDate->sec, pDate->usec);

    /* Walk throu */
    for (i = 0;; i++)
    {
        if ((nType == HSLOG_NONE) && !g_HSlogTags[i].nType && (g_HSlogTags[i].pDesc == NULL))
        {
            if(strlen(g_hslogCfg.sModuleName) > 0)
                snprintf(pOut, nSize, "<logmessage><Date>%s</Date><Module>%s</Module><message>%s</message></logmessage>", sDate, g_hslogCfg.sModuleName, pStr);
            else
                snprintf(pOut, nSize, "<logmessage><Date>%s</Date><message>%s</message></logmessage>", sDate, pStr);
            return;
        }

        if ((nType != HSLOG_NONE) && (g_HSlogTags[i].nType == nType) && (g_HSlogTags[i].pDesc != NULL))
        {
            if (nColor){
                if(strlen(g_hslogCfg.sModuleName) > 0)
                    snprintf(pOut, nSize, "<logmessage><Date>%s</Date><Module>%s</Module><Severity>[%s%s%s]</Severity><message>%s</message></logmessage>",
                        sDate, g_hslogCfg.sModuleName, g_HSlogTags[i].pColor, g_HSlogTags[i].pDesc, CLR_RESET, pStr);
                else
                    snprintf(pOut, nSize, "<logmessage><Date>%s</Date><Severity>[%s%s%s]</Severity><message>%s</message></logmessage>", sDate,
                        g_HSlogTags[i].pColor, g_HSlogTags[i].pDesc, CLR_RESET, pStr);
            }
            else{
                if(strlen(g_hslogCfg.sModuleName) > 0)
                    snprintf(pOut, nSize, "<logmessage><Date>%s</Date><Module>%s</Module><Severity>[%s]</Severity><message>%s</message></logmessage>",
                     sDate, g_hslogCfg.sModuleName, g_HSlogTags[i].pDesc, pStr);
                else
                    snprintf(pOut, nSize, "<logmessage><Date>%s</Date><Severity>[%s]</Severity><message>%s</message></logmessage>", sDate,
                        g_HSlogTags[i].pDesc, pStr);
            }
            return;
        }
    }                                   
}

void hslog_to_file(char *pStr, const char *pFile, HSlogDate *pDate)
{
    char sFileName[PATH_MAX];
    memset(sFileName, 0, sizeof(sFileName));
    snprintf(sFileName, sizeof(sFileName), "%s.log", pFile);

    FILE *fp = fopen(sFileName, "a");
    if (fp == NULL) return;

    fprintf(fp, "%s\n", pStr);
    fclose(fp);
}

int hslog_parse_config(const char *pConfig)
{
    if (pConfig == NULL) return 0;

    FILE *pFile = fopen(pConfig, "r");
    if(pFile == NULL) return 0;

    char sArg[256], sName[32];
    while(fscanf(pFile, "%s %[^\n]\n", sName, sArg) == 2)
    {
        if ((strlen(sName) > 0) && (sName[0] == '#'))
        {
            /* Skip comment */
            continue;
        }

        if (strcmp(sName, "LOGLEVEL") == 0)
        {
            g_hslogCfg.nLogLevel = atoi(sArg);
        }

        if (strcmp(sName, "LOGFILELEVEL") == 0)
        {
            g_hslogCfg.nFileLevel = atoi(sArg);
        }

        if (strcmp(sName, "LOGTOFILE") == 0)
        {
            g_hslogCfg.nToFile = atoi(sArg);
        }

        if (strcmp(sName, "ERRORLOG") == 0)
        {
            g_hslogCfg.nErrLog = atoi(sArg);
        }

        if (strcmp(sName, "PRETTYLOG") == 0)
        {
            g_hslogCfg.nPretty = atoi(sArg);
        }
        
	if (strcmp(sName, "LOGFILESIZE") == 0)
        {
            g_hslogCfg.nLogFileSize = atoi(sArg);
        }
        
	if (strcmp(sName, "MULTICAST") == 0)
        {
	    int t = atoi(sArg);
	    if (t)
              enable_network_logging();
	   else
              disable_network_logging();
        }
        
	if (strcmp(sName, "UNICAST") == 0)
        {
	    int t = atoi(sArg);
	    if (t)
              enable_unicast_logging();
	   else
              disable_unicast_logging();
        }
    }

    fclose(pFile);
    return 1;
}

bool is_network_logging_enabled(void);
void hslog_to_network(const char* pStr, const HSlogDate *pDate, int nType, char *module);
void hslog(int nFlag, const char *pMsg, ...)
{
    hslog_sync_lock();

    if (g_hslogCfg.nSilent && 
        (nFlag == HSLOG_DEBUG || 
        nFlag == HSLOG_LIVE)) 
    {
        hslog_sync_unlock();
        return;
    }

    char sInput[MAXMSG];
    memset(sInput, 0, sizeof(sInput));

    va_list args;
    va_start(args, pMsg);
    vsprintf(sInput, pMsg, args);
    va_end(args);

    /* Check logging levels */
    if(nFlag >= g_hslogCfg.nLogLevel || nFlag >= g_hslogCfg.nFileLevel)
    {
        HSlogDate date;
        hslog_get_date(&date);

        char sMessage[MAXMSG];
        memset(sMessage, 0, sizeof(sMessage));

        prepare_output_func(sInput, &date, nFlag, g_hslogCfg.nPretty, sMessage, sizeof(sMessage));
        if (nFlag >= g_hslogCfg.nLogLevel) printf("%s\n", sMessage);

        /* Save log in the file */
        if ((g_hslogCfg.nToFile && nFlag >= g_hslogCfg.nFileLevel))
            hslog_to_file(sMessage, g_hslogCfg.sFileName, &date);

        // Send message on network if requested.
        hslog_to_network(sInput, &date, nFlag, g_hslogCfg.sModuleName);
    }

    hslog_sync_unlock();
}
 
void hslog_config_get(HSlogConfig *pCfg)
{
    hslog_sync_lock();
    memset(pCfg->sFileName, 0, sizeof(pCfg->sFileName));
    strcpy(pCfg->sFileName, g_hslogCfg.sFileName);
    memset(pCfg->sModuleName, 0, sizeof(pCfg->sModuleName));
    strcpy(pCfg->sModuleName, g_hslogCfg.sModuleName);

    pCfg->nFileLevel = g_hslogCfg.nFileLevel;
    pCfg->nLogLevel = g_hslogCfg.nLogLevel;
    pCfg->nToFile = g_hslogCfg.nToFile;
    pCfg->nPretty = g_hslogCfg.nPretty;
    pCfg->nTdSafe = g_hslogCfg.nTdSafe;
    pCfg->nErrLog = g_hslogCfg.nErrLog;
    pCfg->nSilent = g_hslogCfg.nSilent;
    pCfg->nLogFileSize = g_hslogCfg.nLogFileSize;
    hslog_sync_unlock();
}

void hslog_config_set(HSlogConfig *pCfg)
{
    hslog_sync_lock();
    memset(g_hslogCfg.sFileName, 0, sizeof(g_hslogCfg.sFileName));
    strcpy(g_hslogCfg.sFileName, pCfg->sFileName);
    memset(g_hslogCfg.sModuleName, 0, sizeof(g_hslogCfg.sModuleName));
    strcpy(g_hslogCfg.sModuleName, pCfg->sModuleName);

    g_hslogCfg.nFileLevel = pCfg->nFileLevel;
    g_hslogCfg.nLogLevel = pCfg->nLogLevel;
    g_hslogCfg.nToFile = pCfg->nToFile;
    g_hslogCfg.nPretty = pCfg->nPretty;
    g_hslogCfg.nTdSafe = pCfg->nTdSafe;
    g_hslogCfg.nErrLog = pCfg->nErrLog;
    g_hslogCfg.nSilent = pCfg->nSilent;
    g_hslogCfg.nLogFileSize = pCfg->nLogFileSize;

    if (g_hslogCfg.nTdSafe && !g_hslogCfg.nSync)
    {
        hslog_sync_init();
        hslog_sync_lock();
    }
    else if (!g_hslogCfg.nTdSafe && g_hslogCfg.nSync)
    {
        g_hslogCfg.nTdSafe = 1;
        hslog_sync_unlock();
        hslog_sync_destroy();
    }

    hslog_sync_unlock();
}

void hslog_init(const char* pName, const char* pConf, int nLogLevel, int nTdSafe)
{
    /* Set up default values */
    memset(g_hslogCfg.sFileName, 0, sizeof(g_hslogCfg.sFileName));
    strcpy(g_hslogCfg.sFileName, pName);
    memset(g_hslogCfg.sModuleName, 0, sizeof(g_hslogCfg.sModuleName));

    g_hslogCfg.nLogLevel = nLogLevel;
    g_hslogCfg.nTdSafe = nTdSafe;
    g_hslogCfg.nFileLevel = 0;
    g_hslogCfg.nErrLog = 0;
    g_hslogCfg.nSilent = 0;
    g_hslogCfg.nToFile = 0;
    g_hslogCfg.nPretty = 0;
    g_hslogCfg.nSync = 0;
    g_hslogCfg.nLogFileSize = 1024*1024*5; // 5MB default size limit for log rotation

    /* Init mutex sync */
    hslog_sync_init();

    /* Parse config file */
    hslog_parse_config(pConf);
}

void hslog_set_output_func(hslog_prepare_output_func output_function)
{
    prepare_output_func = output_function;
}

char *get_logfilename(void)
{
    return g_hslogCfg.sFileName;
}

int get_logfilesize(void)
{
    return g_hslogCfg.nLogFileSize;
}

void hslog_set_module(const char *str)
{
    if(str){
        memset(g_hslogCfg.sModuleName, 0, sizeof(g_hslogCfg.sModuleName));
        strcpy(g_hslogCfg.sModuleName, str);
    }
}
