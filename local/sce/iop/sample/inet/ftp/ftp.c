/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                           - ftp -
 *
 *                        Version 1.0.0
 *                          Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * ftp.c - main file for ftp sample program on IOP
 *
 * $Id: ftp.c,v 1.4 2001/06/19 01:39:35 ywashizu Exp $
 * $Author: ywashizu $
 *
 */


#include "ftp.h"

static int sendFtpCmd(CmdArg* cmd_arg, char* buffer);
static int execGet(CmdArg* cmd_arg, char* buffer);
static int execPut(CmdArg* cmd_arg, char* buffer);
static int execLs(CmdArg* cmd_arg, char* buffer);
static int execQ(CmdArg* cmd_arg, char* buffer);
static int execLls(CmdArg* cmd_arg, char* buffer);
static int execLmkdir(CmdArg* cmd_arg, char* buffer);
static int execLpwd(CmdArg* cmd_arg, char* buffer);
static int execLcd(CmdArg* cmd_arg, char* buffer);
static int execGet(CmdArg* cmd_arg, char* buffer);
static int execPut(CmdArg* cmd_arg, char* buffer);

static CmdType commands[] = {
    {"?", "?", execQ},
    {"help", "help", sendFtpCmd},
    {"quit", "quit", sendFtpCmd},
    {"ascii", "type a", sendFtpCmd},
    {"a", "type a", sendFtpCmd},
    {"binary", "type i", sendFtpCmd},
    {"bin", "type i", sendFtpCmd},
    {"bi", "type i", sendFtpCmd},
    {"b", "type i", sendFtpCmd},
    {"get", "retr", execGet},
    {"put", "stor", execPut},
    {"pwd", "pwd", sendFtpCmd},
    {"dir", "list", sendFtpCmd},
    {"ls", "list", execLs},
    {"mkdir", "mkd", sendFtpCmd},
    {"cd", "cwd", sendFtpCmd},
    {"rmdir", "rmd", sendFtpCmd},
    {"rm", "dele", sendFtpCmd},
    {"lcd", "lcd", execLcd},
    {"lpwd", "lpwd", execLpwd},
    {"lls", "lls", execLls},
    {"lmkdir", "lmkdir", execLmkdir},
};

static sceInetParam_t ctrl_param;
static sceInetParam_t data_param;

static int ready_sema;
static int ctrl_cid;
static int data_cid;

static int recv_thread_id;
static int send_thread_id;
static int data_thread_id;

static char recv_buffer[BUFFER_SIZE];
static char send_buffer[BUFFER_SIZE];
static char data_buffer[BUFFER_SIZE];

static char cwd[MAX_PATH];

static int cmd_flg = 0;
static int reply_code = 0;
static int accessflg = 0;

static char file_name[256];

static int usage(void)
{
    puts("Usage: ftp [-options] <remote host address>\n");
    puts("options:\n");
    puts("\t-h          : show this help message\n");
    puts("\t-f          : format HDD\n");
	return(-1);
}

static int check_inet_flags(int flags)
{
    return (flags & sceINETF_FIN) ? 1 : 0;
}

static int sendFtpCmd(CmdArg* cmd_arg, char* buffer)
{
    int cnt;
    int result;
    memset(buffer, 0, sizeof(buffer));
    strcat(buffer, cmd_arg->v[0]);
    for(cnt = 1;cnt < cmd_arg->c;cnt++){
	strcat(buffer, " ");
	strcat(buffer, cmd_arg->v[cnt]);
    }
    strcat(buffer, "\r\n");
    result = sceInetSend(ctrl_cid, buffer, strlen(buffer), 0, INET_DEFAULT_TIMEOUT);
    if (result < 0) {
	printf("send: sceInetSend failed with %d\n", result);
	return -1;
    }
    return 0;
}

static void execLsData()
{
    int flags = 0;
    int result;
    int quit = 0;

    while(!quit){
	result = sceInetRecv(data_cid, data_buffer, BUFFER_SIZE,
			     &flags, INET_INFINITE_TIMEOUT);
	if (result < 0) {
	    printf("result is negative %d\n", result);
	}
	if (result > 0) {
	    data_buffer[result] = '\0';
	    fdputs(data_buffer, 0);
	}
	quit = check_inet_flags(flags);
    }
}

static int execLs(CmdArg* cmd_arg, char* buffer)
{
    if(sendFtpCmd(cmd_arg, buffer) == -1)
	return -1;
    cmd_flg = CMD_FLG_LS;
    return 0;
}

static int execGetData()
{
    int fd;
    int quit = 0;
    int flags = 0;
    int result;
    int ret;
    
    if((fd = open(file_name, O_CREAT|O_RDWR, SCE_STM_RUGO|SCE_STM_WUGO)) < 0){
	printf("cannot open %s with return %d\n", file_name, fd);
	return -1;
    }
    while(!quit){
	result = sceInetRecv(data_cid, data_buffer, BUFFER_SIZE, 
			     &flags, INET_INFINITE_TIMEOUT);
	if (result < 0) {
	    printf("result is negative %d\n", result);
	}
	if (result > 0) {
	    if((ret = write(fd, data_buffer, result)) < 0)
		printf("write failed %d\n", ret);
	}
	putchar('#');
	quit = check_inet_flags(flags);
    }
    close(fd);
    putchar('\n');
    return 0;
}

static int execGet(CmdArg* cmd_arg, char* buffer)
{
    if(sendFtpCmd(cmd_arg, buffer) == -1)
	return -1;
    cmd_flg = CMD_FLG_GET;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "pfs0:/%s/%s", cwd, cmd_arg->v[1]);
    return 0;
}

static int execPutData()
{
    int result;
    int fd;
    int rest;
    int ret;
    int filesize;
    int flags;

    if((fd = open(file_name, O_RDONLY)) < 0){
	printf("cannot open %s with return %d\n", file_name, fd);
	return fd;
    }
    if((filesize = lseek(fd, 0, SEEK_END)) < 0){
	printf("cannot seek\n");
	return filesize;
    }
    printf("transfer %d fd %d\n", filesize, fd);
    if((result = lseek(fd, 0, SEEK_SET)) < 0){
	printf("cannot seek\n");
	return result;
    }

    rest = filesize;
    while(rest){
	if((ret = read(fd, data_buffer, min(rest, BUFFER_SIZE))) < 0){
	    printf("read failed %d\n", ret);
	    return ret;
	}
	result = sceInetSend(data_cid, data_buffer, ret, 
			     &flags, INET_INFINITE_TIMEOUT);
	if (result < 0) {
	    printf("result is negative %d\n", result);
	    return result;
	}
	rest -= result;
	putchar('#');
    }
    close(fd);
    putchar('\n');
    return 0;
}

static int execPut(CmdArg* cmd_arg, char* buffer)
{
    if(sendFtpCmd(cmd_arg, buffer) == -1)
	return -1;
    cmd_flg = CMD_FLG_PUT;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "pfs0:/%s/%s", cwd, cmd_arg->v[1]);
    return 0;
}


static int execQ(CmdArg* cmd_arg, char* buffer){
    int ncmd = sizeof(commands)/sizeof(commands[0]);
    int cnt = 0;
    puts("Current supported commands are:\n\n");
    while(cnt < ncmd){
	printf("%s\t\t", commands[cnt++].input_cmd);
	if(cnt%5 == 0)putchar('\n');
    }
    putchar('\n');
    return 0;
}

static int execLls(CmdArg* cmd_arg, char* buffer)
{
    char tmp[256];
    sprintf(tmp, "pfs0:/%s", cwd);
    showHddDir(tmp);
    return 0;
}

static int execLmkdir(CmdArg* cmd_arg, char* buffer)
{
    char dir_name[256];
    sprintf(dir_name, "pfs0:/%s/%s", cwd, cmd_arg->v[1]);  // syntax for mkdir is "mkdir dir_name"
    mkdir(dir_name, SCE_STM_RWXUGO);
    return 0;
}

static int execLpwd(CmdArg* cmd_arg, char* buffer)
{
    char tmp[256];
    sprintf(tmp, "pfs0:/%s", cwd);
    printf("%s\n", tmp);
    return 0;
}

static int execLcd(CmdArg* cmd_arg, char* buffer)
{
    char tmp_buff[1024];
    char dir_name[1024+7];
    memcpy(tmp_buff, cwd, 1024);
    updateCwd(cmd_arg->v[1], tmp_buff);                     // syntax for cd is "cd dir_name"
    sprintf(dir_name, "pfs0:/%s", tmp_buff);
    if(chdir(dir_name) >= 0)memcpy(cwd, tmp_buff, 1024);
    return 0;
}


static int readCmdLine(CmdArg* cmd_arg)
{
    static char cmd_buff[256];
    int i;
    int cnt, begin;

    while(1){              // exit when '\r' has been typed
	memset(cmd_buff, 0, sizeof(cmd_buff));
	for(i = 0;i < 256;i++){
	    read(0, cmd_buff+i, 1);// read the input data from the console
	    if((cmd_buff[i] == '\b') || (cmd_buff[i] == DEL)){
		if(i != 0){
		    putchar('\b');
		    putchar(' ');
		    putchar('\b');
		    cmd_buff[i--] = '\0';
		    cmd_buff[i--] = '\0';
		}
		else{
		    cmd_buff[i--] = '\0';
		}
		continue;
	    }
	    else if(cmd_buff[i] == '\r'){ // if it's carriage return, put \n and write and then put \0 
		putchar('\n');
		if(i == 0){
		    puts("ftp> ");
		    break; // return to while loop
		}
		begin = cnt = 0;
		memset(cmd_arg, 0, sizeof(cmd_arg));
		while(cmd_buff[cnt]){
		    if(isspace(cmd_buff[cnt])){
			cmd_buff[cnt] = '\0';
			cmd_arg->v[cmd_arg->c++] = &cmd_buff[begin];
			begin = cnt + 1;
		    }
		    cnt++;
		}
		return 0;
	    }
	    else
		write(0, cmd_buff+i, 1);
	}
    }    
}


static void terminateFtp()
{
    int result;

    if ((result = sceInetClose(ctrl_cid, INET_DEFAULT_TIMEOUT)) < 0) {
	printf("recv: sceInetClose failed with %d\n", result);
    }
    printf("cmd recv: closed.\n");

    releaseHdd("pfs0:");

    DeleteSema(ready_sema);
    TerminateThread(send_thread_id);
    TerminateThread(data_thread_id);

    ExitThread();
}

static int runThread(void (*func)(void))
{
    struct ThreadParam tparam;
    int tid, r;
    
    tparam.attr = TH_C;
    tparam.entry = func;
    tparam.initPriority = USER_LOWEST_PRIORITY;
    tparam.stackSize = 0x4000;/* 16KB */
    tparam.option = 0;
    
    if ((tid = CreateThread(&tparam)) <= 0) {
	printf("CreateThread failed with %d\n", tid);
	return tid;
    }
    if ((r = StartThread(tid, 0) != KE_OK)) {
	printf("StartThread failed with %d\n", r); 
	DeleteThread(tid);
	return r;
    }
    
    return tid;
}

static int openInet(int type, sceInetParam_t* iparam)
{
    int cid, result;
    
    iparam->type = type;

    if ((cid = sceInetCreate(iparam)) < 0) {
        printf("sceInetCreate failed with %d\n", cid);
        return cid;
    }
    accessflg = cid;

    if ((result = sceInetOpen(cid, (type == sceINETT_LISTEN) ? 
			      INET_INFINITE_TIMEOUT : INET_DEFAULT_TIMEOUT)) < 0) {
        printf("sceInetOpen failed with %d\n", result);
        return result;
    }

    return cid;
}


static void reCreateDataConnection()
{
    char buf[256];
    char sbuf[256];
    int len = sizeof(buf);
    sceInetInfo_t info;
    int result;

    if ((data_cid = sceInetCreate(&data_param)) < 0) {
	printf("sceInetCreate failed with %d\n", accessflg);

    }

    sceInetControl(ctrl_cid, sceINETC_CODE_GET_INFO, &info, sizeof(info));
    
    sceInetAddress2String(buf, len, &info.local_adr);
    sceInetControl(data_cid, sceINETC_CODE_GET_INFO, &info, sizeof(info));	

    {
	int cnt;
	for(cnt = 0;buf[cnt] != '\0';cnt++){
	    if(buf[cnt] == '.')buf[cnt] = ',';
	}
    }
    sprintf(sbuf, "PORT %s,%d,%d\r\n", buf, 
	    (info.local_port&0xff00)>>8,info.local_port&0x00ff);  
    
    len = strlen(sbuf);
    result = sceInetSend(ctrl_cid, sbuf, len, 0, INET_DEFAULT_TIMEOUT);
    
    if (result < 0) {
	printf("send: sceInetSend failed with %d\n", result);
    }
    if ((result = sceInetOpen(data_cid, INET_INFINITE_TIMEOUT)) < 0) {
	printf("sceInetOpen failed with %d\n", result);
    }
}

static void dataThread(void)
{
    while(1){                       	// main loop for this data thread, exit when it gets error
	switch(cmd_flg){
	case CMD_FLG_PUT:
	    execPutData();
	    break;
	case CMD_FLG_GET:
	    execGetData();
	    break;
	case CMD_FLG_LS:
	    execLsData();
	    break;
	default:
	    puts("unknown flg\n");
	    break;
	}
	cmd_flg = 0;
	sceInetClose(data_cid, 0);
	reCreateDataConnection();      // wait until data connection has been established
    }

    terminateFtp();
}

static int checkReplyCode(char* buffer)
{
    reply_code = (atoi(buffer)/100);
    WakeupThread(send_thread_id);

    return reply_code;
}

static int recvLoginReply(void)
{
    int result;
    int flags = 0;
    int quit = 0;
    while (!quit) {
	result = sceInetRecv(ctrl_cid, recv_buffer, BUFFER_SIZE, &flags, INET_INFINITE_TIMEOUT);
        if (result < 0) {
	    printf("cannot recv correct login reply %d\n", result);
	    return -1;
        }
        if (result > 0) {
            recv_buffer[result] = '\0';
            fdputs(recv_buffer, 0);
        }
	if(checkReplyCode(recv_buffer) == 2){
	    return 0;
	}
	result = 0;

	quit = check_inet_flags(flags);
    }

    return -1;
}

static int recvMesgFromServer(void)
{
    int result;
    int flags = 0;

    result = sceInetRecv(ctrl_cid, recv_buffer, BUFFER_SIZE, &flags, INET_INFINITE_TIMEOUT);
    if (result < 0) {
	printf("recv:something wrong\n");
    }
    if (result > 0) {
	recv_buffer[result] = '\0';
	putchar('\r');
	fdputs(recv_buffer, 0);
    }
    return check_inet_flags(flags);
}


static int isServerReady()
{
    int result;
    int flags = 0;
    int quit = 0;
    while (!quit) {
	result = sceInetRecv(ctrl_cid, recv_buffer, BUFFER_SIZE, &flags, INET_INFINITE_TIMEOUT);
        if (result < 0) {
	    printf("cannot recv correct ready reply %d\n", result);
	    return -1;
        }
        if (result > 0) {
            recv_buffer[result] = '\0';
            fdputs(recv_buffer, 0);
        }
	if(atoi(recv_buffer) == FTP_REPLY_CODE_SERVICE_READY);
	    return 0;
	quit = check_inet_flags(flags);
    }
    return -1;
}

static void recvThread(void)
{
    int quit = 0;
    if(isServerReady() != 0)
	terminateFtp();
    SignalSema(ready_sema);

    if(recvLoginReply() < 0)
	terminateFtp();

    while (!quit) {      // main loop in this receive thread, exit when it gets FIN flag.
	puts("ftp> ");
	quit = recvMesgFromServer();
    }
    
    terminateFtp();
}


static void sendUserData(char* cmd)
{
    char bufferptr[256];
    int result;
    int i;
    int len;
    int isPass = 0;

    if(!strcmp(cmd, "PASS "))isPass = 1;

    sprintf(bufferptr, cmd);
    
    len = strlen(cmd);
    for(i = len;i < 256;i++){
	read(0, bufferptr+i, 1);
	if(bufferptr[i] == '\r'){ // if it's carriage return, put \n and write and then put \0 
	    bufferptr[++i] = '\n';
	    write(0, bufferptr+i, 1);
	    bufferptr[++i] = '\0';
	    break;
	}
	else if((bufferptr[i] == '\b') || (bufferptr[i] == DEL)){
	    if(i > len){
		if(!isPass){
		    putchar('\b');
		    putchar(' ');
		    putchar('\b');
		}
		bufferptr[i--] = '\0';
		bufferptr[i--] = '\0';
	    }
	    else{
		bufferptr[i--] = '\0';
	    }
	    continue;
	}
	else{
	    if(!isPass)
		write(0, bufferptr+i, 1);
	}
    }
    result = sceInetSend(ctrl_cid, bufferptr, strlen(bufferptr), 0, INET_DEFAULT_TIMEOUT);
    
    if (result < 0) {
	printf("send: sceInetSend failed with %d\n", result);
    } 
}

static void ftpLogin(voi)
{
    int breakFlg = 0;
    while(1){
	puts("Name: ");
	sendUserData("USER ");
	SleepThread();
	switch(reply_code){
	case 2:
	    breakFlg = 1;
	    break;
	case 3:
	    break;
	case 1:
	case 4:
	case 5:
	default:
	    continue;
	}
	if(breakFlg)break;

	puts("Password: ");
	sendUserData("PASS ");
	SleepThread();
	switch(reply_code){
	case 2:
	    breakFlg = 1;
	    break;
	case 1:
	case 3:
	case 4:
	case 5:
	default:
	    break;
	}
	if(breakFlg)break;
    }
}

static void setDataPort(void)
{
    char buf[256];
    char sbuf[256];
    int len = sizeof(buf);
    sceInetInfo_t info;
    int result;

    sceInetControl(ctrl_cid, sceINETC_CODE_GET_INFO, &info, sizeof(info));
    
    sceInetAddress2String(buf, len, &info.local_adr);
    sceInetControl(accessflg, sceINETC_CODE_GET_INFO, &info, sizeof(info));	
    {
	int cnt;
	for(cnt = 0;buf[cnt] != '\0';cnt++){
	    if(buf[cnt] == '.')buf[cnt] = ',';
	}
    }
    sprintf(sbuf, "PORT %s,%d,%d\r\n", buf, (info.local_port&0xff00)>>8,info.local_port&0x00ff);  
    
    len = strlen(sbuf);
    result = sceInetSend(ctrl_cid,  sbuf, len, 0, INET_DEFAULT_TIMEOUT);
    
    if (result < 0) {
	printf("send: sceInetSend failed with %d\n", result);
    }
}


static void sendThread(void)
{
    CmdArg cmd_arg;
    int ncmd = sizeof(commands)/sizeof(commands[0]);
    int k;

    WaitSema(ready_sema);
    ftpLogin();
    setDataPort();

    while (1){    // main loop of send thread
	readCmdLine(&cmd_arg);
	for(k = 0;k < ncmd;k++){
	    if(!strcmp(cmd_arg.v[0], commands[k].input_cmd)){
		cmd_arg.v[0] = commands[k].ftp_cmd;
		commands[k].func(&cmd_arg, send_buffer);
		break;
	    }
	}
	if(k == ncmd){
	    puts("?Invalid command. Type ? for supported commands.\n");
	}
	puts("ftp> ");
    }
    
    terminateFtp();
}

static void parentThread(void)
{
    printf("\n---------- FTP client start! ----------\n\n");
    
    if ((ctrl_cid = openInet(sceINETT_CONNECT, &ctrl_param)) < 0) {
        printf("open CONTROL connection failed with %d\n", ctrl_cid);
        ExitThread();
    }
    if ((recv_thread_id = runThread(recvThread)) < 0) {
        printf("recv_thread start failed with %d\n", recv_thread_id);
        ExitThread();
    }
    if ((send_thread_id = runThread(sendThread)) < 0) {
        printf("send_thread start failed with %d\n", send_thread_id);
        ExitThread();
    }
    if ((data_cid = openInet(sceINETT_LISTEN, &data_param)) < 0) {
        printf("open DATA connection failed with %d\n", data_cid);
        ExitThread();
    }
    if ((data_thread_id = runThread(dataThread)) < 0) {
        printf("data_thread start failed with %d\n", data_thread_id);
        ExitThread();
    }
}

static int prepareOpenConnection(char* addr, sceInetParam_t* inet_param, int port_num)
{
    inet_param->local_port = sceINETP_AUTO;
    inet_param->remote_port = port_num;
    return sceInetName2Address(0, &inet_param->remote_addr, addr,
			       INET_INFINITE_TIMEOUT, INET_DEFAULT_NRETRY);
}

static int prepareReadySema()
{
    struct SemaParam sp;
    sp.attr      = SA_THFIFO;
    sp.initCount = 0;
    sp.maxCount = 1;
    return CreateSema(&sp);
}

int start(int argc, char *argv[])
{
    int i;
    int formatFlg = 0;
    char* remote_addr = NULL;

    for (i=1; i<argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
	    case 'f':
		formatFlg = 1;
		break;

            case 'h':
            case '\0':
            default:
                usage();
                return NO_RESIDENT_END;
            }
        } 
	else{
	    remote_addr = argv[i];
        }
    }
    if(!remote_addr){
	usage();
	return NO_RESIDENT_END;
    }

    if((ready_sema = prepareReadySema()) < 0){
	printf("cannot prepare sema %d\n", ready_sema);
	return NO_RESIDENT_END;
    }
    printf("sema id %d\n", ready_sema);
    if(prepareOpenConnection(remote_addr, &ctrl_param, FTP_PORT) < 0){
	puts("cannot prepare open connection\n");
	return NO_RESIDENT_END;
    }
    if(prepareOpenConnection(remote_addr, &data_param, FTP_DATA_PORT) < 0){
	puts("cannot prepare open connection\n");
	return NO_RESIDENT_END;
    }

    if(setupHdd("pfs0:", cwd, formatFlg) < 0){
	puts("cannot setup HDD\n");
	return NO_RESIDENT_END;
    }

    if (runThread(parentThread) < 0) {
	return NO_RESIDENT_END;
    }

    return RESIDENT_END;
}
