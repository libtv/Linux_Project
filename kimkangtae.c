#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#define MAX_STR_LENGTH 256
#define MAX_TIME_LENGTH 20

void ls(char** cline, int narg);
void pwd();
void mstat(char** cline);
void mcat(char** cline, int narg);

void listDir(char*, bool, bool);
void listFile(char*, char*, bool);
void getPermissions(struct stat, char*);
void getUserName(struct stat, char*);
void getGroupName(struct stat, char*);
void getModTime(struct stat, char*);

void kangtae(char** cline, int narg){
        if(strcmp(*cline, "ls") == 0 ){
                ls(cline,narg);
        }

        else if(strcmp(*cline, "pwd") == 0){
                pwd();
        }
        else if(strcmp(*cline, "stat") == 0){
                mstat(cline);
        }
        else if(strcmp(*cline, "cat") == 0){
                mcat(cline, narg);
        }
}

void ls(char **cline, int narg){

        struct stat sb;
        int i;
        bool listAll = false, listLong = false;

        char filename[MAX_STR_LENGTH];

        //extract options from the command line arguments
        //set boolean variables listAll and listLong to encode whether those options were present

        //if there were no arguments other than options, then list what's in the current directory
        if(narg == 1){
                listDir(".", listAll, listLong);
        }else if(narg > 1){
                for(i=1 ; i < narg ; i++){
                        if(cline[i][1] == 'a')
                                listAll = true;
                        else if(cline[i][1] == 'l')
                                listLong = true;
                        else
                                printf("fuck\n");
                }
                listDir(".",listAll,listLong);
        }

}

//extract the file's type and permissions from the file's stats and formats the string to encode it
void getPermissions(struct stat sb, char* permissionString){

        //determine the filetype and add the corresponding character
        permissionString[0] = '-';
        if(S_ISDIR(sb.st_mode)){
                permissionString[0] = 'd';
        }
        if(S_ISCHR(sb.st_mode)){
                permissionString[0] = 'r';
        }
        if(S_ISBLK(sb.st_mode)){
                permissionString[0] = 'b';
        }
        if(S_ISFIFO(sb.st_mode)){
                permissionString[0] = 'p';
        }
        if(S_ISLNK(sb.st_mode)){
                permissionString[0] = 'l';
        }
        if(S_ISSOCK(sb.st_mode)){
                permissionString[0] = 's';
        }


        //check each permission and set the appropriate character
        permissionString[1] = (sb.st_mode & S_IRUSR) == S_IRUSR ? 'r': '-';
        permissionString[2] = (sb.st_mode & S_IWUSR) == S_IWUSR ? 'w': '-';
        permissionString[3] = (sb.st_mode & S_IXUSR) == S_IXUSR ? 'x': '-';

        permissionString[4] = (sb.st_mode & S_IRGRP) == S_IRGRP ? 'r': '-';
        permissionString[5] = (sb.st_mode & S_IWGRP) == S_IWGRP ? 'w': '-';
        permissionString[6] = (sb.st_mode & S_IXGRP) == S_IXGRP ? 'x': '-';

        permissionString[7] = (sb.st_mode & S_IROTH) == S_IROTH ? 'r': '-';
        permissionString[8] = (sb.st_mode & S_IWOTH) == S_IWOTH ? 'w': '-';
        permissionString[9] = (sb.st_mode & S_IXOTH) == S_IXOTH ? 'x': '-';

        permissionString[10] = '\0';
}

//list a file's info (just the filename if listLong is false)
void listFile(char* dirname, char* filename, bool listLong){
        struct stat sb;

        char fullPath[2*MAX_STR_LENGTH];
        char permissionString[11];

        char username[MAX_STR_LENGTH], groupname[MAX_STR_LENGTH];

        char timestring[MAX_TIME_LENGTH];



        //check if the -l option was used
        if(listLong){
                strcpy(fullPath, dirname);
                strcat(fullPath, "/");
                strcat(fullPath, filename);

                if(stat(fullPath, &sb) == -1){
                        perror(filename);
                        return;
                }
                getPermissions(sb, permissionString);

                getUserName(sb, username);
                getGroupName(sb, groupname);

                getModTime(sb, timestring);

                printf("%s %i %s %s %i %s %s\n",permissionString, (int)sb.st_nlink, username, groupname, (int)sb.st_size, timestring, filename);
        }
        else{
                printf("%s\n",filename);
        }
}

//extracts the last modified time from the file stats
void getModTime(struct stat sb, char* timestring){
        struct tm  *timestamp;
        struct tm *currentTime;
        time_t currentTimeRaw;

        int currentYear, fileYear;


        //get the current year
        time(&currentTimeRaw);
        currentTime = localtime(&currentTimeRaw);
        currentYear = (int)currentTime->tm_year;

        timestamp = localtime((time_t*)&sb.st_mtim); //time the file was last modified
        fileYear = (int)timestamp->tm_year; //year the file was last modified

        //if the file was last modified during the current year, show the time, else show the year
        if(currentYear == fileYear){
                strftime(timestring, MAX_TIME_LENGTH, "%b %d %R", timestamp);
        }
        else{
                strftime(timestring, MAX_TIME_LENGTH, "%b %d %Y", timestamp);
        }
}

//returns the group name of the file's owner from the stats
void getGroupName(struct stat sb, char* groupname){
        struct group *groupInfo;

        groupInfo = getgrgid(sb.st_gid);

        strncpy(groupname, groupInfo->gr_name, MAX_STR_LENGTH);
}

//returns the username of the file's owner from the file's stats
void getUserName(struct stat sb, char* username){
        struct passwd *userInfo;

        userInfo = getpwuid(sb.st_uid);
        strncpy(username, userInfo->pw_name, MAX_STR_LENGTH);
}

//reads a directory to read each file it contains
void listDir(char* dirname, bool listAll, bool listLong){
        DIR* dirp;
        struct dirent *dirEntry;
        char* filename;

        //check if the directory opens successfully
        if((dirp = opendir(dirname)) == NULL){
                perror(dirname);
        }
        else{
                //go through each item in the directory
                while((dirEntry = readdir(dirp)) != NULL){
                        filename = dirEntry->d_name;

                        //only list hidden files (. files) if the -a option was used (ie listAll is true)
                        if(filename[0] == '.'){
                                if(listAll){
                                        listFile(dirname, filename, listLong);
                                }
                        }else{
                                listFile(dirname, filename, listLong);
                        }

                }
                //cleanup
                closedir(dirp);
        }
}

void pwd(){
        printf("pwd\n");
        char *dir;
        getcwd(dir,512);
        printf("ssibal my pid : %d, parent pid : %d\n",
                        (int)getpid, (int)getppid);
        //if(dir == null)perror("error");
}

void mstat(char** cline){
        struct stat sbuf;
        char permissionString[11];

        stat(cline[1] , &sbuf);

        getPermissions(sbuf, permissionString);

        printf("  File: `%s\'\n",cline);
        printf("  Size: %d\t\tBlocks: %d\tIO Block:%d\n",(long int)sbuf.st_size,(long)sbuf.st_blocks,(long int)sbuf.st_blksize);
        printf("Device: %lld\t\tInode: %ld\tLinks: %d\n",(long long)sbuf.st_dev,(long int)sbuf.st_ino,(int)sbuf.st_nlink);
        printf("Access: (%o/%s)\t Uid: ( %d)\t Gid: ( %d)\n"
                        ,(unsigned int)sbuf.st_mode, permissionString, (int)sbuf.st_uid,(int)sbuf.st_gid);
        printf("Access: %ld:%s",(long int )sbuf.st_atime,ctime(&sbuf.st_atime));
        printf("Modify: %ld:%s",(long int )sbuf.st_mtime,ctime(&sbuf.st_mtime));
        printf("Change: %ld:%s",(long int )sbuf.st_ctime,ctime(&sbuf.st_ctime));

}

void mcat(char** cline, int narg)
{
        char temp[MAX_STR_LENGTH];
        static FILE *fp;
        int count = 0;
        if (narg > 2)
        {
                switch (cline[1][1])
                {
                        case 'n':
                                if ((fp = fopen(cline[2], "rt")) == NULL)
                                {
                                        perror("fopen\n");
                                }
                                while (fgets(temp, sizeof(temp), fp) != NULL)
                                {
                                        printf("%d : %s", count++, temp);
                                }
                                fclose(fp);
                                break;
                        case 'e':
                                if ((fp = fopen(cline[2], "rt")) == NULL)
                                {
                                        perror("fopen\n");
                                }
                                while (fgets(temp, sizeof(temp), fp) != NULL)
                                {
                                        temp[strlen(temp) - 1] = '$';
                                        printf("%s\n", temp);
                                }
                                fclose(fp);
                                break;
                        default :
                                printf("인자를'n' or 'e' 로  똑바로 입력하시오\n");
                                break;
                }
        }
        else if(narg == 2 | cline[1][1] != 'n')
        {
                if ((fp = fopen(cline[1], "rt")) == NULL)
                {
                        perror("fopen\n");
                }
                while (fgets(temp, sizeof(temp), fp) != NULL)
                {
                        printf("%s", temp);
                }
                fclose(fp);
        }
}
