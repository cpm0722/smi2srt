clear
echo "smi2srt 자동화 스크립트 생성기"

function GET_ABS_PATH()		# volume까지 포함한 절대 경로 리턴하는 함수
{
	FIRST_DIR_NAME=${1%%${1#*/}}	# 인자로 받은 경로에서 첫번째 디렉터리 명 획득
	FIRST_DIR_NAME=${FIRST_DIR_NAME:0:-1}	# 앞뒤 slash 제거

	LS=`ls /`	# root directory의 파일 목록 획득

	VOLUMES=	# volume 저장 list

	for FILE in $LS; do		# root directory의 파일 목록 탐색
		if [[ ${FILE:0:6} == "volume" ]]; then	# 파일명이 volume으로 시작하는 경우
			VOLUMES="$VOLUMES $FILE"	# VOLUMES에 추가
		fi
	done

	for VOLUME in $VOLUMES; do		# VOLUMES 탐색
		LS=`ls /$VOLUME`			# VOLUME 내부 파일 목록 획득
		for FILE in $LS; do			# VOLUME 내부 파일 목록 탐색
			if [[ $FILE == $FIRST_DIR_NAME ]]; then	# 인자로 받은 경로에서의 첫번째 디렉터리 명과 FILE이 같은 경우
				RESULT=/$VOLUME/$1	# VOLUME 포함 절대 경로 생성
			fi
		done
	done

	echo $RESULT	# VOLUME 포함 절대 경로 return
}

while true; do

	#####################
	#        DIRs       #
	#####################

	echo "탐색을 수행하고 싶은 디렉터리의 경로들을 공백으로 구분하여 입력하세요."
	echo "ex) video1/drama, video1/movie, video2/lecture에 대해 수행하고 싶은 경우: video1/drama video1/movie video2/lecture"
	read DIRS	# 디렉터리 목록 입력 받기

	DIR_PATHS=	# 입력받은 디렉터리들의 절대경로 저장

	for DIR in $DIRS; do	# DIRS 탐색
		RESULT=`GET_ABS_PATH $DIR`	# GET_ABS_PATH 함수 호출
		echo "$DIR의 절대경로: $RESULT"
		DIR_PATHS="$DIR_PATHS $RESULT"	# DIR_PATHS에 추가
	done

	#####################
	#     -b option     #
	#####################

	echo "smi 파일을 일괄 이동하시겠습니까? (y/n)"
	read B_OPTION	# smi 파일 backup 옵션 입력 받기 (y/n)

	if [[ ${B_OPTION,,} == "y" ]]; then	# smi 파일 backup하는 경우
		echo "smi 파일을 이동시킬 경로를 입력하세요."
		echo "ex) video1/smi_backup으로 이동시키고 싶은 경우: video1/smi_backup"
		read SMI_DIR	# smi 파일 backup 경로
		SMI_DIR=`GET_ABS_PATH $SMI_DIR`	# GET_ABS_PATH 함수 호출
	fi

	#####################
	#       Check       #
	#####################

	echo "입력하신 디렉터리 목록은 다음과 같습니다."
	echo "$DIR_PATHS"	
	if [[ ${B_OPTION,,} == "y" ]]; then
		echo "smi 파일 일괄 이동 옵션을 선택하셨습니다. 경로는 다음과 같습니다."
		echo "$SMI_DIR"
	else
		echo "smi 파일 일괄 이동 옵션을 선택하지 않으셨습니다."
	fi
	echo "위의 내용이 정확합니까? (y/n)"
	read CORRECT

	if [[ ${CORRECT,,} == "y" ]]; then	# CORRECT가 y인 경우에만 다음으로 넘어감 (while문 break)
		echo "스크립트를 생성합니다..."
		break
	fi

done

#####################
#    Make log dir   #
#####################

if [[ ! -e log ]]; then
	mkdir log 
	echo "log 디렉터리 생성 완료"
fi

#####################
#    Make run.sh    #
#####################

RUN_CMD="sudo docker run -i -t -w /smi2srt -v `pwd`/log:/smi2srt_log"	# docker container 실행 명령어

VOLUMES=

for DIR_PATH in $DIR_PATHS; do	# DIR_PATHS 탐색

	PATH_VOLUME=${DIR_PATH%%${DIR_PATH#/*/}} # DIR_PATH에서 volume명 획득
	PATH_VOLUME=${PATH_VOLUME:0:-1}	 # 마지막 slash / 제거

	FIND=0
	for VOLUME in $VOLUMES; do	# VOLUMES 탐색
		if [[ $VOLUME == ${PATH_VOLUME} ]]; then	# PATH_VOLUME이 이미 VOLUMES에 있는 경우
			FIND=1
			break
		fi
	done

	if [[ $FIND == 0 ]]; then		# PATH_VOLUME이 VOLUMES에 없는 경우
		VOLUMES="$VOLUMES ${PATH_VOLUME}"	# PATH_VOLUME을 VOLUMES에 추가
	fi

done

for VOLUME in $VOLUMES; do	# VOLUMES 탐색
	RUN_CMD="$RUN_CMD -v $VOLUME:$VOLUME"	# 각 volume mount 위해 RUN_CMD에 추가
done

RUN_CMD="$RUN_CMD smi2srt /bin/bash"

echo "#!/bin/bash" > run.sh		# run.sh 작성
echo "" >> run.sh
echo "$RUN_CMD" >> run.sh

sudo chmod +x run.sh	# run.sh에 execute 권한 부여

echo "run.sh 생성 완료"

#####################
#   Make exec.sh    #
#####################

START_CMD="/smi2srt/smi2srt "	# docker 내에서 smi2srt 실행하는 명령어

if [[ ${B_OPTION,,} == "y" ]]; then		# smi 파일 backup 옵션 추가
	START_CMD="$START_CMD -b $SMI_DIR"	# smi backup 경로 추가
fi

START_CMD="$START_CMD $DIR_PATHS"		# 디렉터리 목록 추가

echo '#!/bin/bash' > exec.sh	# exec.sh 작성
echo '' >> exec.sh
echo 'PS=`sudo docker ps` # get docker list' >> exec.sh
echo '' >> exec.sh
echo 'TMP=0		# before words' >> exec.sh
echo 'ID=0		# smi2srt container id' >> exec.sh
echo '' >> exec.sh
echo 'for word in $PS;	do # docker list search' >> exec.sh
echo '	if [[ $word == "smi2srt" ]]; then' >> exec.sh
echo '		ID=$TMP	# save smi2srt before token to ID' >> exec.sh
echo '	fi' >> exec.sh
echo '	TMP=$word	# save before token to TMP' >> exec.sh
echo 'done' >> exec.sh
echo '' >> exec.sh
echo 'echo $ID' >> exec.sh
echo '' >> exec.sh
echo "CMD='$START_CMD'	# command for run in container" >> exec.sh
echo '' >> exec.sh
echo 'sudo docker exec $ID $CMD	# docker exec command' >> exec.sh

sudo chmod +x exec.sh	# exec.sh에 execute 권한 부여

echo "exec.sh 생성 완료"
