FROM		ubuntu:16.04
MAINTAINER	cpm0722@gmail.com

### SET NONINTERACTIVE
ARG		DEBIAN_FRONTEND=noninteractive

### UPDATE
RUN		apt-get update
RUN		apt-get upgrade -y

### LOCALE
RUN		apt-get install locales
RUN		locale-gen ko_KR.UTF-8
ENV		LANG ko_KR.UTF-8
ENV		LANGUAGE ko_KR:en
ENV		LC_ALL ko_KR.UTF-8
RUN		update-locale LANG=ko_KR.UTF-8

### TIMEZONE ENV		TZ=Asia/Seoul
RUN		apt-get install -y tzdata

### DEV_TOOLS
RUN		apt-get install -y curl vim gcc git 
RUN		apt-get install -y build-essential

### NODE.JS
RUN		apt-get install -y apt-transport-https lsb-release > /dev/null 2>&1
RUN		curl -sL https://deb.nodesource.com/setup_15.x -o /nodesource_setup.sh ; bash /nodesource_setup.sh
RUN		rm /nodesource_setup.sh
RUN		apt-get install -y nodejs

### axfree/smi2srt
RUN		npm install smi2srt -g

### cpm0722/smi2srt
RUN		git clone https://github.com/cpm0722/smi2srt.git
RUN		gcc -o /smi2srt/smi2srt /smi2srt/smi2srt.c
