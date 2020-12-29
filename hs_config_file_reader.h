#ifndef __HS_CONFIG_FILE_READER_H
#define __HS_CONFIG_FILE_READER_H

char *get_config_param(const char *config_file, const char *param_name);
char* GetConfigFileName(void);
void   SetConfigFileName(char *fname);
#endif // __HS_CONFIG_FILE_READER_H
