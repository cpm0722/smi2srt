clear
echo "*********************************************************"
echo "*                                                       *"
echo "*          [  smi2srt 자동화 스크립트 생성기 ]          *"
echo "*                                                       *"
echo "*                    Made by cpm0722                    *" 
echo "*                                                       *"
echo "*********************************************************"
echo ""

while true; do

	#####################
	#        DIRs       #
	#####################

	echo "탐색을 수행하고 싶은 디렉터리의 경로들을 공백으로 구분하여 입력하세요."
	echo "ex) video1/drama, video1/movie, video2/lecture에 대해 수행하고 싶은 경우: video1/drama video1/movie video2/lecture"
	read DIR_PATHS	# 디렉터리 목록 입력 받기

	#####################
	#     -b option     #
	#####################

	echo ""
	echo "smi 파일을 일괄 이동하시겠습니까? (y/n)"
	read B_OPTION	# smi 파일 backup 옵션 입력 받기 (y/n)

	if [[ ${B_OPTION,,} == "y" ]]; then	# smi 파일 backup하는 경우
		echo ""
		echo "smi 파일을 이동시킬 경로를 입력하세요."
		echo "ex) video1/smi_backup으로 이동시키고 싶은 경우: video1/smi_backup"
		read SMI_DIR	# smi 파일 backup 경로
	fi

	#####################
	#       Check       #
	#####################

	echo ""
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
		break
	fi

done

#####################
#    Make log dir   #
#####################

if [[ ! -e log ]]; then
	mkdir log 
	echo ""
	echo "log 디렉터리 생성 완료"
fi

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
echo '	if [[ $word == "cpm0722/smi2srt:latest" ]]; then' >> exec.sh
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

echo ""
echo "exec.sh 생성 완료"

echo ""
echo "프로그램을 종료합니다."
