#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define PATH_LEN 2048
#define FILE_LEN 128
#define CMD_LEN PATH_LEN * 2 + 20
#define TIME_LEN 40

#define CONVERT "smi2srt -n \""		//CONVERT 명령어
#define LOGDIR	"/smi2srt_log"		//log를 저장하는 디렉터리, docker 외부에서 mount
#define TMPFILE "/smi2srt/tmp.tmp"	//출력 결과를 임시 저장하는 임시 파일의 경로
#define LOGFILE LOGDIR "/log.txt"	//로그 저장하는 파일의 경로			
#define TIMEFILE LOGDIR "/time.bin"	//최종 수행 시각을 저장하는 파일의 경로

#define STDOUT_SAVE 100			//임시로 stdout을 저장하는 fd
#define STDERR_SAVE 101			//임시로 stderr을 저장하는 fd

typedef enum {false, true} bool;

int system_rename(char src[PATH_LEN], char dst[PATH_LEN]);
int redirection(char *cmd, const char *tmpFile);
int rename_to_ko(char path[PATH_LEN]);
int rename_to_non_ko(char path[PATH_LEN]);
int smi2srt(char path[PATH_LEN]);
int mkdir_recursive(char path[PATH_LEN]);
int search_directory(char *path);
int get_abs_path(char result[PATH_LEN], char *path);

bool backup = false;			//-b 옵션 여부 저장
bool korean = false;			//-ko 옵션 여부 저장

char cmd[CMD_LEN] = CONVERT;	//CONVERT 명령어 저장

char pwd[PATH_LEN];				//현재 실행 경로 저장

char nowRootDir[PATH_LEN];		//명령어 인자로 넘겨받은 경로의 절대 경로 저장
char backupDir[PATH_LEN];		//-b 옵션 시 인자로 넘겨받은 backup 디렉터리의 절대 경로 저장

time_t startTime;		//시작 시간
time_t lastTime;		//프로그램이 가장 최근에 실행됐던 시간
char strTime[TIME_LEN];	//시작 시간을 문자열로 저장

//system 함수 사용해 mv 명령어 실행하는 함수
//다른 device 간 파일 이동을 위해 rename 함수 호출 시 cross-device error 발생하기 때문
int system_rename(char src[PATH_LEN], char dst[PATH_LEN])
{
	char cmd[PATH_LEN*2+10];
	strcpy(cmd, "mv \"");
	strcat(cmd, src);
	strcat(cmd, "\" \"");
	strcat(cmd, dst);
	strcat(cmd, "\"");
	system(cmd);
	return 0;
}

//cmd를 수행하면서 출력 결과를 stdout이 아닌 tmpFile에 저장하는 함수
int redirection(char *cmd, const char *tmpFile)
{
	int tmpFd;
	if((tmpFd = open(tmpFile, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0){	//tmpFile open
		fprintf(stderr, "open error for %s\n", tmpFile);
		return 1;
	}
	dup2(STDOUT_FILENO, STDOUT_SAVE);	//stdout 임시 저장
	dup2(tmpFd, STDOUT_FILENO);			//tmpFd를 stdout으로
	system(cmd);						//명령어 실행
	dup2(STDOUT_SAVE, STDOUT_FILENO);	//stdout 복원
	close(tmpFd);
	return 0;
}

//srt, ass 파일에 .ko를 추가하는 함수
int rename_to_ko(char path[PATH_LEN])
{
	if(*(path + strlen(path) - strlen(".xx.srt")) == '.')	//이미 파일명에 언어가 명시된 경우 종료
		return 0;
	char postfix[10] = ".ko";
	strcat(postfix, path + strlen(path) - strlen(".srt"));	//.ko.srt 또는 .ko.ass 생성
	char dstPath[PATH_LEN];
	memset(dstPath, PATH_LEN, '\0');
	strcpy(dstPath, path);
	*(dstPath + strlen(dstPath) - strlen(".srt")) = '\0';	//확장자 제외
	strcat(dstPath, postfix);	//언어 명시 및 확장자 부여
	if(rename(path, dstPath) < 0){	//rename 수행
		fprintf(stderr, "rename error for %s to %s\n", path, dstPath);
		return 1;
	}
	else{
		fprintf(stderr, "rename %s to %s\n", path, dstPath);
	}
	return 0;
}

//srt, ass 파일에 .ko를 제거하는 함수
int rename_to_non_ko(char path[PATH_LEN])
{
	if(*(path + strlen(path) - strlen(".xx.srt")) != '.')	//이미 파일명에 언어가 명시되지 않은 경우 종료
		return 0;
	char postfix[10];
	strcpy(postfix, path + strlen(path) - strlen(".srt"));	//언어 명시 없는 .srt 또는 .ass 생성
	char dstPath[PATH_LEN];
	memset(dstPath, PATH_LEN, '\0');
	strcpy(dstPath, path);
	*(dstPath + strlen(dstPath) - strlen(".ko.srt")) = '\0';	//언어 명시자, 확장자 제외
	strcat(dstPath, postfix);	//확장자 부여
	if(rename(path, dstPath) < 0){	//rename 수행
		fprintf(stderr, "rename error for %s to %s\n", path, dstPath);
		return 1;
	}
	return 0;
}

//smi를 srt로 변환하는 함수
int smi2srt(char path[PATH_LEN])
{
	strcpy(cmd + strlen(CONVERT), path);	//CONVERT 명령어 완성
	strcat(cmd, "\"");

	redirection(cmd, TMPFILE);				//CONVERT 수행 후 출력 결과 TMPFILE에 저장

	struct stat statbuf;
	if(stat(TMPFILE, &statbuf) < 0){			//TMPFILE stat 획득
		fprintf(stderr, "stat error for %s\n", TMPFILE);
		return 1;
	}
	if(statbuf.st_size > 0)						//TMPFILE의 size 0 이상인 경우 (CONVERT 정상 수행된 경우)
		fprintf(stderr, "%s\n", path + strlen(nowRootDir));	//log.txt에 추가
	else										//TMPFILE의 size 0인 경우 (CONVERT error 발생한 경우)
		fprintf(stderr, "*****[CONVERT ERROR]***** %s\n", path);			//log.txt에 error 기록 추가
	
	char jaPath[PATH_LEN];					//.ja.srt의 경로
	strcpy(jaPath, path);
	*(jaPath + strlen(jaPath) - strlen(".smi")) = '\0';	// .ja.srt 경로 생성
	strcat(jaPath, ".ja.srt");
	char koPath[PATH_LEN];			//.ko.srt 경로 생성
	strcpy(koPath, jaPath);
	char *str = koPath + strlen(koPath) - strlen(".ja.srt"); 
	str[1] = 'k';
	str[2] = 'o';
	if(access(jaPath, F_OK) == 0 && access(koPath, F_OK) < 0){	//.ja.srt가 존재하면서 .ko.srt는 존재하지 않는 경우
		rename(jaPath, koPath);
	}

	if(!korean){	// -ko 옵션이 아닌 경우
		rename_to_non_ko(koPath);
	}

	return 0;
}

//디렉터리를 재귀적으로 모두 mkdir하는 함수
int mkdir_recursive(char path[PATH_LEN])
{
	int len = strlen(path);
	if(len < strlen(backupDir))
		return 0;
	int i;
	for(i = len; i >= 0; i--){				//부모 경로가 끝나는 idx를 i에 저장
		if(path[i] == '/'){
			break;
	}
	}
	if(access(path, F_OK) < 0){				//path 디렉터리가 없을 경우
		char parentPath[PATH_LEN];			//parentPath에 부모 디렉터리 경로 저장
		memset(parentPath, '\0', PATH_LEN);
		strncpy(parentPath, path, i);
		if(access(parentPath, F_OK) < 0)	//부모 디렉터리 없을 경우
			mkdir_recursive(parentPath);	//재귀 호출
	}
	if(mkdir(path, 0755) < 0){				//path 디렉터리 mkdir
		fprintf(stderr, "mkdir error for %s\n", path);
		return 1;
	}
	else
		fprintf(stderr, "mkdir %s\n", path);

	return 0;
}

//재귀적으로 디렉터리 탐색하며 smi 파일 찾아 smi2srt 호출하는 함수
int search_directory(char *path)
{
	DIR *dirp;
	if((dirp = opendir(path)) == NULL){			//디렉터리 open
		fprintf(stderr, "opendir error for %s\n", path);
		return 1;
	}
	struct dirent *dentry;
	while((dentry = readdir(dirp)) != NULL){	//디렉터리 탐색

		if(!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, "..") || (dentry->d_name[0] == '@'))	// . .. @로 시작하는 디렉터리 skip
			continue;

		char nowPath[PATH_LEN];					//현재 탐색중인 파일에 대한 절대 경로 완성
		strcpy(nowPath, path);
		strcat(nowPath, "/");
		strcat(nowPath, dentry->d_name);

		struct stat statbuf;
		if(stat(nowPath, &statbuf) < 0){		//현재 파일에 대한 stat 획득
			fprintf(stderr, "stat error for %s\n", nowPath);
			return 1;
		}

		if(S_ISDIR(statbuf.st_mode)){			//현재 파일이 디렉터리인 경우
			if(statbuf.st_mtime > lastTime)		//해당 디렉터리의 최종 수정 시각이 프로그램이 가장 최근에 실행됐던 시각보다 이후일 경우
				search_directory(nowPath);		//해당 디렉터리에 대해 재귀 호출
		}

		else{	//현재 파일이 디렉터리가 아닌 일반 파일인 경우
			if( !strcmp(nowPath+strlen(nowPath)-4, ".srt") ||	//확장자가 srt인 경우
				!strcmp(nowPath+strlen(nowPath)-4, ".SRT") ||	
				!strcmp(nowPath+strlen(nowPath)-4, ".ass") || 
				!strcmp(nowPath+strlen(nowPath)-4, ".ASS")){	//확장자가 ass인 경우
				if(korean){	//-ko 옵션이 true인 경우
					rename_to_ko(nowPath);	//파일명의 끝에 .ko 추가
				}
			}
			else if(!strcmp(nowPath+strlen(nowPath)-4, ".smi") || !strcmp(nowPath+strlen(nowPath)-4, ".SMI")){	//확장자가 smi 또는 SMI인 경우

				char srtPath[PATH_LEN];			//srt 경로 생성
				strcpy(srtPath, nowPath);
				strcpy(srtPath + strlen(srtPath)-3, "srt");

				if(access(srtPath, F_OK) < 0)	//srt 파일이 없을 경우에만
					smi2srt(nowPath);				//smi2srt 호출
				else
					fprintf(stderr, "%s already exists.\n", srtPath);

				if(backup){						//-b옵션일 경우

					char backupPath[PATH_LEN];	//backup 경로 완성
					strcpy(backupPath, backupDir);
					strcat(backupPath, nowPath + strlen(nowRootDir));

					int i;
					for(i = strlen(backupPath); i>=0; i--){	//부모 디렉터리 경로가 끝나는 idx 찾아 i에 저장
						if(backupPath[i] == '/')
							break;
					}
					if(i != 0)
						backupPath[i] = '\0';			//부모 디렉터리 경로로 수정

					char fname[FILE_LEN];				//전체 경로가 아닌 실제 파일명 획득
					strcpy(fname, backupPath+i+1);

					if(access(backupPath, F_OK) < 0){	//부모 디렉터리가 없을 경우
						char tmp[PATH_LEN];
						strncpy(tmp, backupPath, i + 1);
						mkdir_recursive(tmp);			//mkdir_recursive 호출
					}

					strcat(backupPath, "/");			//부모 디렉터리 경로가 아닌 전체 경로로 복원
					strcat(backupPath, fname);

					system_rename(nowPath, backupPath);	//backup 디렉터리로 mv
				}
			}
		}
	}
	return 0;
}

//상대 경로를 절대경로로 변경해 result에 저장하는 함수
int get_abs_path(char result[PATH_LEN], char *path)
{
	if(path[0] == '/' || path[0] == '~'){
		strcpy(result, path);
		return 0;
	}

	strcpy(result, pwd);
	if (strcmp(pwd, "/"))
		strcat(result, "/");
	strcat(result, path);

	return 0;
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
	struct timeval start_tv, end_tv;
	gettimeofday(&start_tv, NULL);			//시작 시간 저장
#endif
	getcwd(pwd, PATH_LEN);					//현재 경로 획득해 pwd에 저장

	for(int i = 1; i < argc; i++){			//옵션 있는지 탐색
		if(!strcmp(argv[i], "-b")){			//-b 옵션일 경우
			backup = true;					//backup = true
			get_abs_path(backupDir, argv[++i]);	//-b 직후 인자의 절대 경로 구해 backupDir에 저장
			fprintf(stderr, "backup: %s\nbackupDir: %s\n", backup?"true":"false", backupDir);
		}
		if(!strcmp(argv[i], "-ko")){		//-ko 옵션일 경우
			korean = true;					//korean = true
		}
	}

	startTime = time(NULL);			//시작 시간 startTime에 저장
	ctime_r(&startTime, strTime);	//시작 시간 문자열로 strTime에 저장
	strTime[strlen(strTime)-1] = '\0';

	if(access(TIMEFILE, F_OK) < 0)	//TIMEFILE이 없을 경우
		lastTime = 0;				//가장 최근에 프로그램 실행됐던 시간을 0으로 지정
	else{							//TIMEFILE이 있을 경우
		int timeFd;
		if((timeFd = open(TIMEFILE, O_RDONLY)) < 0){	//TIMEFILE read 권한으로 open
			fprintf(stderr, "open error for %s\n", TIMEFILE);
			exit(1);
		}

		if(read(timeFd, &lastTime, sizeof(time)) <= 0){	//TIMEFILE에 저장된 시간 lastTIme에 읽어옴
			fprintf(stderr, "read error for %s\n", TIMEFILE);
			exit(1);
		}
	}

	int logFd;
	if((logFd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){	//LOGFILE write(append) 권한으로 open
		fprintf(stderr, "open error for %s\n", LOGFILE);
		exit(1);
	}
	dup2(STDERR_FILENO, STDERR_SAVE);		//STDERR을 LOGFILE으로 변경
	dup2(logFd, STDERR_FILENO);

	fprintf(stderr, "\n*** [%s] start ***\n\n", strTime);	//LOGFILE에 현재 시각 write

	for(int i = 1; i < argc; i++){			//인자 탐색하며 search_directory 호출
		if(!strcmp(argv[i], "-b")){			//-b 옵션일 경우
			i++;							//직후 인자도 skip
			continue;
		}
		else if(!strcmp(argv[i], "-ko")){	//-ko 옵션일 경우 skip
			continue;
		}
		char nowPath[PATH_LEN];							//현재 인자의 절대 경로 저장
		get_abs_path(nowPath, argv[i]);
		strcpy(nowRootDir, nowPath);					//현재 인자의 절대 경로 nowRootDir에 저장
		for(int i = strlen(nowRootDir); i >= 0; i--)	//현재 인자의 부모 디렉터리 경로로 변경
			if(nowRootDir[i] == '/'){
				nowRootDir[i] = '\0';
				break;
			}

		search_directory(nowPath);						//search_directory 호출

	}

	close(logFd);

	int timeFd;
	if((timeFd = open(TIMEFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){	//TIMEFILE write 권한으로 open
		fprintf(stderr, "open error for %s\n", TIMEFILE);
		exit(1);
	}
	time_t now = time(NULL);
	if(write(timeFd, &now, sizeof(time_t)) <= 0){			//현재 시각 TIMEFILE에 write
		fprintf(stderr, "write error for %s\n", TIMEFILE);
	}
	close(timeFd);

	dup2(STDERR_SAVE, STDERR_FILENO);							//stderr 복원

#ifdef DEBUG
	gettimeofday(&end_tv, NULL);			//종료 시간 저장
	fprintf(stderr, "process time: %lld\n", end_tv.tv_sec - start_tv.tv_sec);
#endif

	exit(0);
}
