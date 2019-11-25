#include "smallsh.h"
#include "kimjunho.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr, *tok;
static char special[] = {' ', '\t', '&', ';', '\n', '\0'};
char *prompt = "Command>";      /* Prompt */
static int npipe; /* 파이프 라인 사용시 카운트 될 것 */
static int noutredirect; /* redirct 사용시 카운트 */
static int ninredirect;
static char *pipearg[MAXARG+1]; /* 파이프 라인 사용시 저장할 공간 */
static FILE *fhistory;
static FILE *falias;
static struct termios initial_settings, new_settings;
static int peek_character = -1;
char *printHis(int h_count);

userin(char *p) {
        /* **변수 정리 ?: */
        int c, count, i;
        ptr = inpbuf;
        tok = tokbuf;
        npipe = 0, noutredirect = 0, ninredirect = 0, count = 0;
        int enter_count = 0;
        int ch = 0;
        char enter_char[BUFSIZ];
        char count_his[BUFSIZ];
        strcpy(count_his, printHis(1));
        int history_count = 0;
        history_count = atoi(count_his);
        /* **변수 정리  */
    printf("%s ", p);
    for(i=0; i<MAXARG; i++) {
        pipearg[i] = NULL;
    }
        for(i=0; i<BUFSIZ; i++) {
                enter_char[i] = '\0';
        }
        for(i=0; i<BUFSIZ; i++) {
                count_his[i] = '\0';
        }
    init_keyboard();
    while(ch != '\n') {
                if(kbhit()) {
        ch = readch();
                        if (ch == 91) {
                                ch = readch();
                                switch(ch) {
                                        case 65:
                                                strcpy(count_his, printHis(history_count));
                                                for(i=0; i < enter_count; i++) {
                                                        putch(127);
                                                }
                                                enter_count = 0;
                                                for(i=0; count_his[i] != '\n'; i++) {
                                                        enter_char[enter_count] = putch(count_his[i]);
                                                        enter_count++;
                                                }
                                                history_count--;
                                                break;
                                        default:
                                                enter_char[enter_count] = putch(65);
                                                enter_count++;
                                                break;
                                }
                        } else if (ch > 31 && ch < 127 || ch == '\n') {
                                enter_char[enter_count] = putch(ch);
                                enter_count++;
                        } else if (ch == 127 || ch == 9) {
                                if (enter_count > 0) {
                                        enter_char[--enter_count] = '\0';
                                        putch(ch);
                                }
                        }
                }
        }
        close_keyboard();

        while(1) {
                if ((c = enter_char[count]) == EOF) {
                        printf("null값 이니까 EOF 처리\n");
                        return(EOF);
                }
                if (strcmp(enter_char, "quit\n") == 0) {
                        return(EOF);
                }
                if (count < MAXBUF) inpbuf[count++] = c;
                if (c == '\n' && count < MAXBUF) {
                        inpbuf[count] = '\0';
                        addHis(inpbuf);
                        return(count);
                }

                if (c == '\n') {
                        printf(" smallsh : input line too long\n");
                        count = 0;
                        npipe = 0;
                        noutredirect = 0;
                        ninredirect = 0;
                        printf("%s ", p);
                }
        }
}

addHis(his)
char *his;
{
        init();
        fprintf(fhistory, "%s", inpbuf);
        fclose(fhistory);
}

gettok(char **outptr) {
        int type;
        *outptr = tok;
        while( *ptr == ' ' || *ptr == '\t') ptr++;
        *tok++ = *ptr;
        switch(*ptr++) {
                case '\n' :
                        type = EOL;
                        break;
                case '&'  :
                        type = AMPERSAND;
                        break;
                case '|' :
                        type = PIPE;
                        break;
                case '>' :
                        type = OUTREDIRECT;
                        break;
                case '<' :
                        type = INREDIRECT;
                        break;
                default   :
                        type = ARG;
                        while(inarg(*ptr)) *tok++ = *ptr++;
        }
        *tok++ = '\0';
        return type;
}

inarg(c)
char c;
{
        char *wrk;
        for(wrk = special; *wrk != '\0'; wrk++) {
                if (c == *wrk) {
                        printf("special arg : %c inarg()\n", *wrk);
                        return(0);
                }
        }
        return(1);
}

procline() {
        char *arg[MAXARG+1];    /* runcommand를 위한 포인터 배열 */
        int toktype;                    /* 명령내의 토근의 유형 */
        int narg;                       /* 지금까지의 인수 수 */
        int type;                       /* FOREGROUND or BACKGROUND */
        int i;
        for (narg = 0;;) {      /* loop FOREVER */
                switch(toktype = gettok(&arg[narg])) {
                        case ARG        : if (narg < MAXARG) narg++;
                                                break;
                        case PIPE       :
                                                        arg[narg] = NULL;
                                                        for(i=0; i<narg; i++) {
                                                                pipearg[i] = arg[i];
                                                                printf("%s\n", arg[i]);
                                                                npipe++;
                                                        }
                                                        printf("pipe LIne : %s\n 인수 수 : %d\n" , *pipearg, narg);
                                                        narg = 0;
                                                        break;
                        case OUTREDIRECT:
                                                        arg[narg] = NULL;
                                                        *pipearg = *arg;
                                                        noutredirect++;
                                                        narg = 0;
                                                        break;
                        case INREDIRECT:
                                                        arg[narg] = NULL;
                                                        *pipearg = *arg;
                                                        ninredirect++;
                                                        narg = 0;
                                                        break;
                        case EOL        :
                        case SEMICOLON  :
                        case AMPERSAND  :
                                                  type = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;


                                                  if (narg != 0) {
                                                          arg[narg] = NULL;
                                                          runcommand(arg, type);
                                                  }
                                                  if (toktype == EOL) return;
                                                  narg = 0;
                                                  break;
                }
        }
}

runcommand(cline, where)
char **cline;
int where;
{
        int pid, exitstat, ret, status;
        int fd[2]; /* pipe LINE */
        pid_t pid1;

        if ((pid = fork()) < 0) {
                perror("smallsh");
                return(-1);
        }
        if (pid == 0) { /* child */
                if (npipe != 0) {
                        pipeline(pipearg, cline);
                } else if (noutredirect != 0) {
                        redirect_out(pipearg, cline);
                } else if (ninredirect != 0) {
                        redirect_in(pipearg, cline);
                } else {
                        /* pipeline과 redirection 종료후에 구현한 것들 */
                        if (develop(cline) == 0) {
                                /* 구현된 것 */
                        } else {
                                printf("구현할 것 \n");
                                execvp(*cline, cline);
                                perror(*cline);
                                exit(127);
                        }
                }
        }
        if (where == BACKGROUND) {
                printf("[Process id %d]\n",pid);
                return(0);
        }
        if (waitpid(pid, &status, 0) == -1) {
                return (-1);
        } else {
                return (status);
        }
}

char *printHis(h_count)
int h_count;
{
        static char temp[MAXBUF];
        int count = 0;
        fhistory = fopen("./history.bashrc", "rt");
        if (h_count == 0) {
                while(fgets(temp, sizeof(temp), fhistory) != NULL) {
                        printf("%d : %s", count++, temp);
                }
        fclose(fhistory);
                return "yes";
        } else if (h_count == 1) {
                while(fgets(temp, sizeof(temp), fhistory) != NULL) {
                        count++;
                }
        fclose(fhistory);
                static char buff[BUFSIZ];
                sprintf(buff, "%d", count);
                return buff;
        } else {
        while(fgets(temp, sizeof(temp), fhistory) != NULL) {
                        count ++;
                        if (h_count == count) break;
                }
                fclose(fhistory);
                return temp;
        }
}

addAlias(alias_temp)
char *alias_temp;
{
        char buf[256];
        if (strcmp(alias_temp, "alias") == 10) {
                falias = fopen("./alias.bashrc", "r");
                while(fgets(buf, sizeof(buf), falias) != NULL) {
                        printf("%s", buf);
                }
                fclose(falias);
                return 0;
        } else {
                char operation[1];
                char *delim = "=";
                operation[0] = strpbrk(alias_temp, delim)[0];
                operation[1] = '\0';
                if (strcmp(operation, "=") == 0) {
                        falias = fopen("./alias.bashrc", "awr");
                        fprintf(falias, "%s", alias_temp);
                        fclose(falias);
                        return 0;
                } else {
                        fclose(falias);
                        return -1;
                }
        }
}

listAlias(cline)
char **cline;
{
        char buf[256];
        int find = 0;
        char *op1, *op2, *result;
        char *delim = "=";
        falias = fopen("./alias.bashrc", "awr");
        fclose(falias);

        falias = fopen("./alias.bashrc", "rt");

        while(fgets(buf, sizeof(buf), falias) != NULL) {
                op1 = strtok(buf, delim);
                result = op1;
                op2 = strtok(NULL,delim);
                result = strtok(result, " ");
                result = strtok(NULL, " ");
                if(strcmp(result, *cline) == 0) {
                        find = 1;
                        break;
                }
        }
        if (find == 1) {
                Eliminate(op2, '"');
                printf("::alias:: 명령어를 실행시킵니다.%s\n", op2);
                system(op2);
                fclose(falias);
                perror(op2);
                return 0;
        } else {
                fclose(falias);
                return -1;
        }
        fclose(falias);
}
/* str 중에 ch인자가 있을시 삭제하는 코드 */
Eliminate(str,ch)
char *str;
char ch;
{
        for(; *str != '\0'; str++) {
                if(*str == ch) {
                        strcpy(str, str+1);
                        str--;
                }
        }
}

develop(cline)
char **cline;
{
        int i;
        /* history 명령어 구현 */
        if(strcmp(*cline, "history") == 0) {
                if (strcmp(printHis(0), "yes") == 0){
                        return 0;
                } else {
                        return -1;
                }
        }
        /* alias add 명령어 구현 */
        if(strcmp(*cline, "alias") == 0) {
                if (addAlias(inpbuf) == 0) {
                        return 0;
                } else {
                        return -1;
                }
        }
        /* alias list 있을 시 명령어 실행 */
        if(listAlias(cline) == 0) {
                return 0;
        }
        /* 김준호가 개발한 명령어 */
        for(i=0; i<DEVELOPKIMCOUNT; i++) {
                if(strcmp(DEVELOPKIM[i], *cline) == 0) {
                        voiddevkim(cline);
                        return 0;
                }
        }
        return -1;
}

redirect_in(fcline, bcline)
char **fcline;
char **bcline;
{
        pid_t pid1;
        int fd;
        /* Redirect 처리 */
        switch(pid1 = fork()) {
                case -1:
                        perror("fork");
                        break;
                case 0:
                        printf("값 :: %s\n", *fcline);
                        fd = open(*fcline, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                        if(fd == -1) {
                                perror("open");
                                exit(1);
                        }
                        if(dup2(fd, 1) == -1) {
                                perror("fd");
                        }
                        close(fd);
                        execvp(*bcline, bcline);
                        exit(0);
                        break;
                default:
                        wait(NULL);
        }
}

redirect_out(fcline, bcline)
char **fcline;
char **bcline;
{
        pid_t pid1;
        int fd;
        /* Redirect 처리 */
        switch(pid1 = fork()) {
                case -1:
                        perror("fork");
                        break;
                case 0:
                        printf("값 :: %s\n", *bcline);
                        fd = open(*bcline, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                        if(fd == -1) {
                                perror("open");
                                exit(1);
                        }
                        if(dup2(fd, 1) == -1) {
                                perror("dup");
                        }
                        close(fd);
                        execvp(*fcline, fcline);
                        exit(1);
                        break;
                default:
                        wait(NULL);
        }
}

pipeline(fcline, bcline)
char **fcline;
char **bcline;
{
        int fd[2]; /* pipe LINE */
        pid_t pid1;

        /* pipe line 처리 */
        pipe(fd); /* pipe 생성 */
        switch(pid1 = fork()) { /* process 생성 */
                case -1:
                                perror("fork");
                                exit(1);
                                break;
                case 0: /* chlid */
                                close(fd[1]);
                                if (fd[0] != 0) {
                                        dup2(fd[0],0);
                                        close(fd[0]);
                                }
                                execvp(*bcline, bcline);
                                exit(127);
                                break;
                default:
                                close(fd[0]);
                                if(fd[1] != 1) {
                                        dup2(fd[1], 1);
                                        close(fd[1]);
                                }
                                execvp(*fcline, fcline);
                                break;
        }
}

init(void)
{
        fhistory = fopen("./history.bashrc", "awr");
}

main() {
        while(userin(prompt) != EOF) {
                procline();
        }
}

init_keyboard()
{
    tcgetattr(0, &initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
}

close_keyboard()
{
    tcsetattr(0, TCSANOW, &initial_settings);
}

kbhit()
{
    char ch;
    int nread;

    if (peek_character != -1) return -1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1) {
        peek_character = ch;
        return 1;
    }
    return 0;
}

readch()
{
    char ch;
    if(peek_character != -1) {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }

    read(0,&ch,1);
    return ch;
}

putch(c)
int c;
{
    putchar(c);
    fflush(stdout);
    return c;
}
