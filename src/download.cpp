/*****************************************
To complete the upgrade process
*****************************************/
#include "download.h"
#include "file.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include<time.h>
#include <stdlib.h>
#include "os_linux.h"
#include "serialif.h"
#include "qcn.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
static pid_t udhcpc_pid = 0;

static FILE * ql_popen(const char *program, const char *type)
{
    FILE *iop;
    int pdes[2];
    pid_t pid;
    char *argv[20];
    int argc = 0;
    char *dup_program = strdup(program);
    char *pos = dup_program;

    while (*pos != '\0')
    {
        while (isblank(*pos)) *pos++ = '\0';
        if (*pos != '\0')
        {
            argv[argc++] = pos;
            while (*pos != '\0' && !isblank(*pos)) pos++;
            //dbg_time("argv[%d] = %s", argc-1, argv[argc-1]);
        }
    }
    argv[argc++] = NULL;

    if (pipe(pdes) < 0) {
        return (NULL);
    }

    switch (pid = fork()) {
    case -1:            /* Error. */
        (void)close(pdes[0]);
        (void)close(pdes[1]);
        return (NULL);
        /* NOTREACHED */
    case 0:                /* Child. */
        {
        if (*type == 'r') {
            (void) close(pdes[0]);
            if (pdes[1] != STDOUT_FILENO) {
                (void)dup2(pdes[1], STDOUT_FILENO);
                (void)close(pdes[1]);
            }
        } else {
            (void)close(pdes[1]);
            if (pdes[0] != STDIN_FILENO) {
                (void)dup2(pdes[0], STDIN_FILENO);
                (void)close(pdes[0]);
            }
        }
        execvp(argv[0], argv);
        _exit(127);
        /* NOTREACHED */
        }
           break;
       default:
            udhcpc_pid = pid;
            free(dup_program);
       break;
    }

    /* Parent; assume fdopen can't fail. */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        (void)close(pdes[1]);
    } else {
        iop = fdopen(pdes[1], type);
        (void)close(pdes[0]);
    }

    return (iop);
}

static int ql_pclose(FILE *iop)
{
    (void)fclose(iop);
    udhcpc_pid = 0;
    return 0;
}
//++++++++++++++++++++++
FILE *log_file;  //Log Handle define
int g_port_temp=0;
int g_upgrade_fastboot=0;
int g_upgrade_fastboot_last=0;
int create_log(void) {
    struct tm *ptm;
    long ts;
    int y, m, d, h, n, s;
    ts = time(NULL);
    ptm = localtime(&ts);
    y = ptm->tm_year + 1900; //�?
    m = ptm->tm_mon + 1; //�?
    d = ptm->tm_mday; //�?
    h = ptm->tm_hour; //�?
    n = ptm->tm_min; //�?
    s = ptm->tm_sec; //�?
    char filename[200];
    sprintf(filename, "UPGRADE%d%02d%02d%02d%02d%02d.log", y, m, d, h, n, s);
    printf("Log name is %s\n", filename);
    log_file = fopen(filename, "wt");
    return (log_file != NULL);
}

int close_log(void) {
    if (log_file != NULL)
        fclose(log_file);
    log_file = NULL;
    return TRUE;
}
int save_log(const char *fmt,...)
{
	va_list args;
	int len;
	int result = false;
	struct tm *ptm;
	long ts;
	int y, m, d, h, n, s;
	ts = time(NULL);
	ptm = localtime(&ts);
	y = ptm->tm_year + 1900; //�?
	m = ptm->tm_mon + 1; //�?
	d = ptm->tm_mday; //�?
	h = ptm->tm_hour; //�?
	n = ptm->tm_min; //�?
	s = ptm->tm_sec; //�?
	char* newfmt = (char*) malloc(strlen(fmt) + 30);
	sprintf(newfmt, "[%d-%02d-%02d %02d:%02d:%02d]%s", y, m, d, h, n, s, fmt);
	va_start(args, fmt);
	char* buffer = (char*) malloc(500);
	vsprintf(buffer, newfmt, args);
	strcat(buffer, "\r\n");
	va_end(args);
	free(newfmt);
	newfmt = NULL;

	if (buffer == NULL)
		return result;

        //printf("%s", buffer);   
    
	if (log_file != NULL) {
		int result = fwrite((void *) buffer, sizeof(char),
				strlen((const char *) buffer), log_file);
		free(buffer);
		buffer = NULL;
		fflush(log_file);
		result = true;
	}
	return result;
}

int ProcessInit(qdl_context *pQdlContext) {
    pQdlContext->logfile_cb = save_log;
    //create_log();
    
    if (!image_read(pQdlContext)) {
        printf("Parse file error\n");
        return 0;
    }
    return 1;
}

int ProcessUninit(qdl_context *pQdlContext) {
	close_log();
	image_close();
	return 1;
}

int module_state(qdl_context *pQdlContext)
{
	int timeout = 5;
	while (timeout--) {
		pQdlContext->TargetState = send_sync();
		if (pQdlContext->TargetState == STATE_UNSPECIFIED) {
			if (timeout == 0) {
				pQdlContext->text_cb(
						"Module status is unspecified, download failed!");
				pQdlContext->logfile_cb(
						"Module status is unspecified, download failed!");
				return FALSE;
			}
			pQdlContext->logfile_cb("Module state is unspecified, try again");
			qdl_sleep(2000);
		} else {
			break;
		}
	}
	return TRUE;
}
int open_port_operate()
{
	int timeout = 5;
	while (timeout--) {
		qdl_sleep(1000);
		if (!openport()) {
			qdl_sleep(1000);
			if (timeout == 0) {

				return 0;
			} else
				continue;
		} else {
			return 1;
			break;
		}
	}
	return 1;
}

static int do_fastboot(const char *cmd, const char *partion, const char *filepath) {
    char *program = (char *) malloc(MAX_PATH + MAX_PATH + 32);
    char *line = (char *) malloc(MAX_PATH);
    char *self_path = (char *) malloc(MAX_PATH);
    FILE * fpin;
	
    int self_count = 0;
    int recv_okay = 0;
    int recv_9615 = 0;

    if (!program || !line || !self_path) {
        printf("fail to malloc memory for %s %s %s\n", cmd, partion, filepath);
        return 0;
    }

    self_count = readlink("/proc/self/exe", self_path, MAX_PATH - 1);
    if (self_count > 0) {
        self_path[self_count] = 0;
    } else {
        printf("fail to readlink /proc/self/exe for %s %s %s\n", cmd, partion, filepath);
        return 0;
    }
    
    if (!strcmp(cmd, "flash")) {
        if (!partion || !partion[0] || !filepath || !filepath[0] || access(filepath, R_OK)) {
            free(program);;free(line);free(self_path);
            return 0;
        }
        sprintf(program, "%s fastboot %s %s %s", self_path, cmd, partion, filepath);
	  // sprintf(program, "%s fastboot %s %s %s 1>2 2>./rfastboot", self_path, cmd, partion, filepath);
        
    } else {
        sprintf(program, "%s fastboot %s", self_path, cmd);
		//sprintf(program, "%s fastboot %s 1>./rfastboot", self_path, cmd);
    }

    printf("%s\n", program);

    fpin = ql_popen(program, "r");


    if (!fpin) {
		printf("popen failed\n");
		printf("popen strerror: %s\n", strerror(errno));
        return 0;
    }

    while (fgets(line, MAX_PATH - 1, fpin) != NULL) {
        printf("%s", line);
        if (strstr(line, "OKAY")) {
            recv_okay++;
        } else if (strstr(line, "MDM9615")) {
            recv_9615++;
        }
    }
    
    ql_pclose(fpin);
	
   
	//use sysytem() to excuse shell
	/*system(program);
	 	if ((fpin = fopen("rfastboot", "r")) == NULL) {
		printf("Read File Error\n");
		return FALSE;
	}
	while (fgets(line, 1024, fpin) != NULL) {
		printf("%s", line);
		if (strstr(line, "MDM9615")) {
			recv_9615++;
		}
		if (strstr(line, "OKAY")) {
			recv_okay++;
		}
		memset(line, 0, 1024);
	}
	fclose(fpin);
	system("rm -f rfastboot");*/
	//+++++++++++++++++++++++++++++
	 free(program);free(line);free(self_path);
    if (!strcmp(cmd, "flash"))
        return (recv_okay == 2);
    else if (!strcmp(cmd, "devices"))
        return (recv_9615 == 1);
   else if (!strcmp(cmd, "continue"))
        return (recv_okay == 1);
   else
        return (recv_okay > 0);
    
    return 0;
}

static int do_flash_mbn(const char *partion, const char *filepath) {
    char *program = (char *) malloc(MAX_PATH + MAX_PATH + 32);
    int result;
    byte *filebuf;
    uint32 filesize;

    if (!program) {
        printf("fail to malloc memory for %s %s\n", partion, filepath);
        return 0;
    }

    sprintf(program, "flash %s %s", partion, filepath);
    printf("%s\n", program);

    if (!partion || !filepath || !filepath[0] || access(filepath, R_OK)) {
        free(program);
        return 0;
    }

    filebuf = open_file(filepath, &filesize);
    if (filebuf == NULL) {
        free(program);
        return FALSE;
    }
 
    strcpy(program, partion);
    result = handle_openmulti(strlen(partion) + 1, (byte *)program);
    if (result == false) {
        printf("%s open failed\n", partion);
        goto __fail;
    }

    sprintf(program, "sending '%s' (%dKB)", partion, (int)(filesize/1024));
    printf("%s\n", program);
  
    result = handle_write(filebuf, filesize);
    if (result == false) {
        QdlContext->text_cb("");
        QdlContext->text_cb("%s download failed", partion);
        goto __fail;
    }
    
    result = handle_close();
    if (result == false) {
        QdlContext->text_cb("%s close failed", partion);
        goto __fail;
    }

    printf("OKAY\n");
    
    free(program);
    free_file(filebuf, filesize);
    return TRUE;

__fail:
    free(program);
    free_file(filebuf, filesize);
    return FALSE;
}

int BFastbootModel() {
    return do_fastboot("devices", NULL, NULL);
}


int downloadfastboot(qdl_context *pQdlContext) {
	/***********upgrade fastboot==>start**********/
	//++++++++++++++

	if (pQdlContext->update_method == 4|| pQdlContext->update_method == 1) {
		
       
		do_fastboot("flash", "sbl2", pQdlContext->sbl2_path);
		do_fastboot("flash", "aboot", pQdlContext->appsboot_path);
		do_fastboot("flash", "dsp1", pQdlContext->dsp1_path);
        do_fastboot("flash", "dsp2", pQdlContext->dsp2_path);
        do_fastboot("flash", "dsp3", pQdlContext->dsp3_path);
        do_fastboot("flash", "system", pQdlContext->system_path);
        do_fastboot("flash", "userdata", pQdlContext->userdata_path);
        do_fastboot("flash", "recoveryfs", pQdlContext->recoveryfs_path);
		do_fastboot("flash", "boot", pQdlContext->boot_path);
        do_fastboot("flash", "recovery", pQdlContext->recovery_path);
        //do_fastboot("continue", NULL, NULL);reboot
		do_fastboot("reboot", NULL, NULL);
    }
    
	/***********upgrade fastboot==>end************/
	return TRUE;
}
static int GetModuleState(qdl_context *pQdlContext)
{
	int sync_timeout=15;
	while (sync_timeout--) {
		pQdlContext->TargetState = send_sync();
		if (pQdlContext->TargetState != STATE_UNSPECIFIED) 
		{
			break;
		}
		else if (sync_timeout > 0)
		{
			closeport(g_hCom);
			qdl_sleep(4000);
			if (open_port_operate() == 0) 
			{
				pQdlContext->text_cb("Start to open port, Failed!");
				return FALSE;
			}
			continue;
		}
		else if (sync_timeout == 0)
		{
			pQdlContext->text_cb("Sync Timeout, Failed!");
			return FALSE;
		}
	}
	return TRUE;
}
static int ReOpenPort()
{
	int timeout = 10;
	closeport(g_hCom);
	qdl_sleep(2000);
	while (timeout--) {
		closeport(g_hCom);
		qdl_sleep(1000);
		if (!openport()) {
			qdl_sleep(2000);
			if (timeout == 0) {
				printf("Start to open port, Failed!\n");
				return FALSE;
			}
			else
				continue;
		}
		break;
	}
	return TRUE;
}
static int BackUpQQB(qdl_context *pQdlContext)
{
	if (pQdlContext->update_method == 4|| pQdlContext->update_method == 2) 
	{
		if (handle_send_sp_code() != 1) {
			pQdlContext->text_cb("Sp unlocked failed");
			qdl_sleep(200);
			return FALSE;
		}
		pQdlContext->text_cb("Backup QQB...");
		if(SaveQqbThreadFunc(pQdlContext)== false)
		{
			remove("backup.qqb");
			pQdlContext->text_cb("");
			pQdlContext->text_cb("Backup failed");
			return FALSE;
		}
	}
	return TRUE;
}
static int ReStoreQQB(qdl_context *pQdlContext)
{
	if (pQdlContext->qqb_EC20 != NULL||(pQdlContext->update_method == 4|| pQdlContext->update_method == 3)) {
		if (handle_switch_target_ftm() != 1)
		{
			pQdlContext->text_cb("Switch offline mode failed");
			qdl_sleep(200);
			return FALSE;
		}
		if (handle_send_sp_code() != 1) {
			pQdlContext->text_cb("Sp unlocked failed");
			qdl_sleep(200);
			return FALSE;
		}
		if(pQdlContext->qqb_EC20 != NULL&& pQdlContext->update_method != 3)
		{
			pQdlContext->text_cb("QQB downloading...");
			if (Import_Qqb_Func(pQdlContext) == false) {
				pQdlContext->text_cb("QQB download failed");
				return FALSE;
			}
		}
		if (pQdlContext->update_method == 4|| pQdlContext->update_method == 3) 
		{
			pQdlContext->text_cb("Restore QQB...");
			if (RestoreQqbThreadFunc(pQdlContext) == false) 
			{
				pQdlContext->text_cb("");
				pQdlContext->text_cb("Restore QQB fail");
				return FALSE;
			}
		}
	}
	return TRUE;
}
//MDM9615	fastboot
int downloading(qdl_context *pQdlContext)
{
	int download_sbl2_flag=0;
	pQdlContext->text_cb("Module Status Detection");
	if (BFastbootModel()) {
		goto fastboot_download;
	}
start_download:
	if (open_port_operate() == 0) {
		pQdlContext->text_cb("Start to open port, Failed!");
		return FALSE;	
	}
	if(GetModuleState(pQdlContext)==FALSE)
		return FALSE;
	if(pQdlContext->TargetState == STATE_NORMAL_MODE)
	{
		if(BackUpQQB(pQdlContext)==false)
			return FALSE;
		if (pQdlContext->update_method == 2)
			return TRUE;
		if(ReStoreQQB(pQdlContext)==false)
			return FALSE;
		if (pQdlContext->update_method == 3)
			return TRUE;
		pQdlContext->text_cb("Switch to PRG status");
		if (switch_to_dload() == false) 
		{
			pQdlContext->text_cb("Switch to PRG status failed");
		}
		if(ReOpenPort()==FALSE)
			return FALSE;
		if(GetModuleState(pQdlContext)==FALSE)
			return FALSE;
	}
	if (pQdlContext->TargetState == STATE_DLOAD_MODE)
	{
		pQdlContext->text_cb("In PRG Status");
		if(send_nop()== false)
		{
			pQdlContext->text_cb("Send nop command failed");
			return FALSE;
		}
		if (!preq_cmd()) {
			pQdlContext->text_cb("Send preq command failed");
			return FALSE;
		}
		if(download_sbl2_flag==1)
		{
			pQdlContext->text_cb("Send %s", pQdlContext->ENPRG9x15_path);
			auto_ehex_download(pQdlContext);
			if (write_32bit_cmd_emerg() == false) 
			{
				pQdlContext->text_cb("Send %s", pQdlContext->NPRG9x15_path);
				auto_hex_download(pQdlContext);
				qdl_sleep(500);
				if (write_32bit_cmd() == false)
				{
					return FALSE;
				} 
			}
		}
		else
		{
			auto_hex_download(pQdlContext);
			pQdlContext->text_cb("Send %s", pQdlContext->NPRG9x15_path);
			if (write_32bit_cmd() == false) {
				pQdlContext->text_cb("Send %s", pQdlContext->ENPRG9x15_path);
				auto_ehex_download(pQdlContext);
				qdl_sleep(500);
				if (write_32bit_cmd_emerg() == false) 
				{
				
				  return FALSE;
				}
			}
		}
		
		qdl_sleep(1000);
		if (go_cmd() == false) {
			pQdlContext->text_cb("Send go command failed");
			return FALSE;
		}
		if(ReOpenPort()==FALSE)
			return FALSE;
		if(GetModuleState(pQdlContext)==FALSE)
			return FALSE;
	}
	if (pQdlContext->TargetState == STATE_GOING_MODE) {
		pQdlContext->text_cb("Start to download firmware");
		qdl_sleep(3000);
		if (handle_hello() == false) {
			pQdlContext->text_cb("Send hello command fail");
			return FALSE;
		}
		if (handle_security_mode(1) == false) {
			pQdlContext->text_cb("Send trust command fail");
			return FALSE;
		}
		if (handle_parti_tbl(0) == false) {
			pQdlContext->text_cb("Partitbl mismatched");
		}
		if (pQdlContext->TargetPlatform == TARGET_PLATFORM_9615) {
			if(download_sbl2_flag==1)
			{
				do_flash_mbn("0:SBL2", pQdlContext->sbl2_path);
				//do_flash_mbn("0:SBL1", pQdlContext->sbl1_path);
				//do_flash_mbn("0:RPM", pQdlContext->rpm_path);
				//do_flash_mbn("0:APPSBL", pQdlContext->appsboot_path);
			}
			else{
				do_flash_mbn("0:SBL1", pQdlContext->sbl1_path);
				do_flash_mbn("0:SBL2", "sbl2_tmp.mbn");
				do_flash_mbn("0:RPM", pQdlContext->rpm_path);
				do_flash_mbn("0:APPSBL", "appsboot_tmp.mbn");
			}
			
		}
	}
	qdl_sleep(1000);
	/*reset the module*/
	if (handle_reset() == false) {
		pQdlContext->text_cb("Send reset command failed");
		return FALSE;
	}
	closeport(g_hCom); 
	qdl_sleep(2000);
fastboot_download:
	if(download_sbl2_flag==0&&BFastbootModel())
	{
		if(downloadfastboot(pQdlContext)== FALSE)
		{
			return FALSE;
		}
		download_sbl2_flag=1;
		goto start_download;
	}
	pQdlContext->text_cb("restart...");
	/*qdl_sleep(5000);
	if(ReOpenPort()==FALSE)
		return FALSE;
	normal_reset();
	pQdlContext->text_cb("restart...");
	qdl_sleep(2000);*/
	return TRUE;
}


