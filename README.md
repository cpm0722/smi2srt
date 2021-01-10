# smi2srt for plex in synology v2.0

Made by Hansu Kim (cpm0722@kakao.com)

## 주요 기능
- 특정 디렉터리 내부를 탐색하여 smi 파일을 찾아 srt 파일로 변환
- 변환된 srt 파일의 파일명 끝에 .ko를 추가해 PLEX에서 한국어 자막으로 인식하도록 변경
- -b 옵션을 사용해 smi 파일을 지정한 디렉터리로 일괄 이동 가능, 사용하지 않을 경우 smi파일은 원래 위치에 그대로 유지
- 실행 내역에 대한 로그 파일 log.txt 생성

## 실행 조건
- Docker 사용이 가능한 Synology 제품

## 사용 방법
1. 스크립트 파일 NAS에 업로드

    [make_script.sh 다운로드](https://github.com/cpm0722/smi2srt/raw/main/make_script.sh)

    위의 파일을 다운로드해 NAS에 업로드한다. homes/계정명/smi2srt 디렉터리에 넣는다. smi2srt 디렉터리가 없다면 새로 생성한다.

2. DSM → 제어판 → 터미널 및 SNMP → 터미널 → SSH 서비스 활성화

    ssh 접속 시 사용할 원하는 포트 번호를 지정한다. 이미 지정이 되어있는 경우 다음 단계로 넘어간다.

3. ssh로 NAS 터미널에 접속한다. PC 운영체제에 따라 방법이 다르다.
    - Windows
        1. PuTTY 설치

            [PuTTY 다운로드](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html)

        2. PuTTY 실행
        3. Host Name에 `[DSM계정명]@[NAS Ipv4 주소]`를 입력
            - ex) `admin@192.168.255.255`
        4. Port에 2번에서 지정한 ssh 포트 번호 입력
        5. Open
        6. DSM 계정의 비밀번호 입력

    - Mac
        1. Mac 터미널에서 명령어 입력

            ```bash
            ssh [DSM계정명]@[NAS Ipv4 주소] -p [포트번호]
            ```

            만약 admin 계정으로 192.168.255.255에 22번 포트로 접속하는 경우라면 `ssh admin@192.168.255.255 -p 22`가 될 것이다.

        2. DSM 계정의 비밀번호 입력


4. 터미널에서 다음 명령어를 한 줄 씩 입력한다. 스크립트가 실행된다.

    ```
    cd ~/smi2srt

    ./make_script.sh
    ```

    스크립트의 안내문을 따라서 경로들을 입력한다. 이 때 주의할 점은 2가지가 있다.

    - 스크립트에 입력하는 모든 경로들에는 공백이 포함되지 않아야 한다.
    - smi 파일을 백업하고자 할 때 smi 파일의 백업 디렉터리는 탐색을 수행할 디렉터리 내부에 있어서는 안된다.
5. DSM → 패키지 센터 → 커뮤니티 → Docker 설치

    Docker가 없는 경우 설치됨 탭에서 이미 Docker가 설치되어 있는지 확인한다.
6. DSM → Docker 실행
7. 레지스트리 탭 → cpm0722/smi2srt 검색 → latest 선택
8. 이미지 탭 → cpm0722/smi2srt:latest 선택 -> 실행

    컨테이너 이름을 **smi2srt**로 변경한다. 이후 고급 설정에 들어간다.

    - 고급 설정 탭: 자동 재시작 활성화
    - 볼륨 탭

        위에서 파일을 넣었던 smi2srt 디렉터리 내 log 디렉터리를 추가한다. `homes/계정명/smi2srt/log` 가 추가될 것이다. 이 때 마운트 경로는 `/smi2srt_log`로 작성한다.

        실행하고자 하는 디렉터리들을 추가한다. smi 파일을 일괄 백업하고 싶은 경우 smi 파일을 저장하고 싶은 디렉터리 역시 포함된다. 이 때 실행하고자 하는 디렉터리들의 전체 경로가 아닌, 해당 디렉터리가 포함된 시놀로지 상의 "**공유폴더**"를 추가한다. 공유폴더란, 시놀로지 파일 탐색기에서 최상단에 위치하는 디렉터리들을 의미한다. 아래의 경우에서는 docker, home, homes, main, .... 등이 공유폴더이다.

        <img src="https://user-images.githubusercontent.com/18459502/104104561-3df29680-52ec-11eb-91a5-014724c26dca.jpg" height="500">

        각각의 공유폴더들의 마운트 경로는 아래와 같이 작성한다. 공유폴더명 앞에 '/'만을 붙여 `/공유폴더명`의 형태가 된다.

        아래는 video1, video2에 대해서 실행하고 싶은 경우에 대한 예시이다.

        <img src="https://user-images.githubusercontent.com/18459502/104104565-434fe100-52ec-11eb-9b7b-de0ff7ca6d4f.jpg" height="500">

    적용 버튼을 눌러 설정을 마무리한다.

9. 제어판 → 작업 스케줄러 → 생성
    - 일반 탭
        - 작업: **smi2srt**
        - 사용자: **root**
    - 스케줄 탭

        원하는 실행 주기에 맞게 적절하게 작성한다. 실행 주기는 최소 20분 이상을 권장한다.

    - 작업 설정

        사용자 정의 스크립트를 아래와 같이 작성한다.

        ```bash
        /var/services/homes/[DSM계정명]/smi2srt/exec.sh
        ```

    확인 버튼을 누른다. 이후 생성한 smi2srt 작업을 활성화 체크를 한 후 꼭 **저장** 버튼을 누른다.

## 로그 확인

smi2srt 디렉터리 내 log 디렉터리의 log.txt을 열어 확인할 수 있다.

## 라이센스
    
cpm0722

axfree [axfree/smi2srt](https://github.com/axfree/smi2srt)
