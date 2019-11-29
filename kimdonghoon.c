
#include <stdio.h>
#include <stdlib.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>

void voiddevdong(char** cline)
{
	if (strcmp(*cline, "exit_program") == 0)
	{
		exit(1);
	}
	if (strcmp(*cline, "uname") == 0)
	{
		if (voiduname(cline) == 0)
		{
		}
		else
		{
			printf("uname error");
		}
	}
	if (strcmp(*cline, "ctime") == 0)
	{
		if (voidctime(cline) == 0)
		{
		}
		else
		{
			printf("ctime error");
		}
	}
	if (strcmp(*cline, "sysconf") == 0)
	{
		if (voidsysconf(cline) == 0)
		{
		}
		else
		{
			printf("sysconf error");
		}
	}
	if (strcmp(*cline, "getuid") == 0)
	{
		if (voidgetuid(cline) == 0)
		{
		}
		else
		{
			printf("getuid error");
		}
	}
}

voiduname(cline)
char** cline;
{
	struct utsname uts;
	if (uname(&uts) == -1)
	{
		perror("uname");
		exit(1);
	}

	printf("OSname : %s\n", uts.sysname);
	printf("NodeName : %s\n", uts.nodename);
	printf("Release : %s\n", uts.release);
	printf("Version : %s\n", uts.version);
	printf("Machine : %s\n", uts.machine);

	return 0;
}


voidctime(cline)
char** cline;
{
	time_t t;
	time(&t);
	printf("Time(sec) : %d\n", (int)t);
	printf("Time(date) : %s\n", ctime(&t));	
	return 0;	
}

voidsysconf(cline)
char** cline;
{
	printf("Clock Tick : %ld\n", sysconf(_SC_CLK_TCK));
	printf("Max Open File : %ld\n", sysconf(_SC_OPEN_MAX));
	printf("Max Login Name Length : %ld\n", sysconf(_SC_LOGNAME_MAX));
	printf("Max Password Length : %ld\n", sysconf(_SC_PASS_MAX));
	printf("Max Process per 1 UID : %ld\n", sysconf(_SC_CHILD_MAX));
	printf("POSIX Version : %ld\n", sysconf(_SC_VERSION));	     
	return 0;	
}

voidgetuid(cline)
char** cline;

{
	uid_t uid, euid;
	char* name, * cname;	
	uid = getuid();
	euid = geteuid();	
	name = getlogin();
	cname = cuserid(NULL);
	printf("Login Name = %s,%s UID = %d ,EUID = %d\n", name, cname, (int)uid, (int)euid);
	return 0;
	
}