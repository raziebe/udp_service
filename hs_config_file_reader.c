#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "hslog/hslog.h"

char *get_config_param(const char *config_file, const char *param_name)
{
	FILE *file;
	char *ptr = NULL;
	static char value[512];
	
	file = fopen(config_file, "r");
	if(!file){
        // Failed to open file.
		return ptr;
	}
	
	while(!feof(file)){
		
		// Reset buffer
		memset(value, 0, sizeof(value));
		ptr = fgets(value, sizeof(value), file);
		
		// check if read was success.
		if(!ptr && feof(file)){
            // configuration parameter not found.
			goto FAIL_1;
		}
		else if(!ptr && ferror(file)){
            // some error , use ferror(file)
			goto FAIL_1;
		}
		
		// check if line feed is present, if so remove the line feed. fgets inserts the line feed for text mode files
		value[strcspn(value, "\n\r")] = '\0';
		
		ptr = strtok(value, ":");
		 if(!ptr){
            // Invalid string format present in file
			goto FAIL_1;
		 }
		 
		 if(!strcmp(ptr, param_name)){
			 
			 // Get the value field
			 ptr = strtok(NULL, ":");
			 if(!ptr){
                 // Invalid configuration format
                 goto FAIL_1;
			 }
			 
			 // Check for starting spaces after ':'
			 while(*ptr == ' ') ptr++;			 

			 fclose(file);
			 return ptr;
		 }
	}
	
	FAIL_1:
	fclose(file);
	return ptr;
}
 
#define CONFIGURATION_FILE      "/etc/hs-udp-service.conf"

char ConfigFileName[256] = CONFIGURATION_FILE;

void SetConfigFileName(char *fname)
{
	strcpy(ConfigFileName, fname);
	hslog_error("Using %s\n",ConfigFileName);
}

char* GetConfigFileName(void)
{
	return ConfigFileName;
}
