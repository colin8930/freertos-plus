#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"
#include "romfs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include <math.h>

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void history_command(int, char **);

extern int his_handle;
extern int fibonacci(int x);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(history, "show command history")
};

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){

	char* fs_list [20];
	int size=fs_ls(fs_list);

	for(int i=0; i<size; i++){
		 fio_printf(1,"\r\n%s",fs_list[i]);	 
		}
 	fio_printf(2,"\r\n");
}

int filedump(const char *filename){
	char buf[128];


	int fd=fs_open(filename, 0, O_RDONLY);
	
	if(fd<0)
	
		return 0;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
	}

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");

/********************/
		return;
	}
	
	if(!filedump(argv[1])) fio_printf(2, "\r\n%s no such file or directory.\r\n", argv[1]);


}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

	if(!filedump(buf))
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i=0;i<sizeof(cl)/sizeof(cl[0]); ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}


void history_command(int n,char *argv[]){

	int handle, error;
	handle = host_action(SYS_OPEN, "output/history", 0);
	
	if(handle == -1) {
        fio_printf(1, "Open file error!\n");
        return;
    }
	int len;
	
	char buf[512] = {0};
	len=host_action(SYS_FLEN, handle);


	error = host_action(SYS_READ, handle, (void *)buf, len);
	
	if(error !=0) {
            fio_printf(1, "read error.\n\r");
            
            return;
        }
	fio_printf(1, "\r\n%s\r\n",buf);
	host_action(SYS_CLOSE, handle);
}



void test_command(int n, char *argv[]) {
	int handle;
	int error;

	int input=strtoint(argv[1]);	
	handle = host_action(SYS_OPEN, "output/fib", 8);
	
	if(handle == -1) {
		fio_printf(1, "Open file error!\n\r");
		return;
	}
	int fib = fibonacci(input);
	
	error = host_action(SYS_WRITE, handle,&fib, sizeof(int));
	if(error != 0) {
        		fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
		host_action(SYS_CLOSE, handle);
		return;
	}

   	 host_action(SYS_CLOSE, handle);
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}

int strtoint(char * arg)
{
	int len = sizeof(arg)/2;
	int result=0;
	for(int i=0; i<len; i++){
		result+=(arg[i]-48)*power(10,len-i-1);
	}
	return result;
}

int power(int base, int exp)
{
	int result=1;
	for(int i=0; i<exp; i++){
		result*=base;
	}
	return result;
}
