# smi2srt for plex in synology

from Hansu Kim (cpm0722@kakao.com)

## 주요 기능
- 특정 디렉터리를 재귀적으로 탐색(하위 폴더 전부)하여 smi 파일을 찾아 srt 파일로 변경
- -b 옵션을 사용해 smi 파일들을 백업 가능
- 주기적 실행할 때 연산량 감소 위해 폴더 수정 시각과 이전 실행 시각 비교해 이후 갱신되었는지 판단해 실행
- 실행 내역에 대한 로그 파일 log.txt 생성
- 에러 발생 내역에 대한 로그 파일 error.txt 생성

## 환경 설정
1. opkg 설치

    ```bash
    wget http://pkg.entware.net/binaries/x86-64/installer/entware_install.sh
    ./entware_install.sh
    ```

    [참고](https://www.sysnet.pe.kr/2/0/11706)

1. gcc vim make 설치

    ```bash
    sudo opkg install gcc vim make
    ```

1. git 설치

    1. DSM → 패키지 센터 → Git Server 제거
    
    1. 패키지 센터 → 설정 → 패키지 소스 → 추가

        `이름: SynoCommunity`
        `위치: http://packages.synocommunity.com`

    1. 패키지 센터 → 커뮤니티 → 새로 고침
    
    1. Git 설치
        
        - 설치 오류 발생할 경우 패키지 센터 → 설정 → 신뢰 수준 Synology Inc. 및 신뢰할 수 있는 게시자로 변경
    
        [참고](https://blog.acidpop.kr/228)

1. node_js 설치

    - DSM → 패키지 센터 → 커뮤니티 → Node.js v8 설치
    
1. npm 실행 설정

    1. 권한 변경

        ```bash
        sudo chown root:users /volume1/@appstore/Node.js_v8/usr/local/lib/node_modules -R
        sudo chmod 775 /volume1/@appstore/Node.js_v8/usr/local/lib/node_modules
        sudo chmod 775 /volume1/@appstore/Node.js_v8/usr/local/bin
        sudo chown root:users  /volume1/@appstore/Node.js_v8/usr/local/bin
        ```

    1. /etc/profile 을 vim으로 연 뒤, PATH = 로 시작하는 행 뒤에 :/volume1/@appstore/Node.js_v8/usr/local/bin 추가
    
    1. ssh 재접속 (터미널 종료 후 재시작)
    
        [참고](https://community.synology.com/enu/forum/1/post/124087)
        
        [참고](http://blog.naver.com/PostView.nhn?blogId=takakobj&logNo=110149113938)

1. smi2srt(convert_v2) 설치

    ```bash
    sudo npm install smi2srt -g
    ```

## 실행 방법

1. 실행 파일 생성
    - gcc를 통해 smi2srt.c 파일을 complie해 smi2srt 실행 파일 생성 (실행 환경 종속성 회피하기 위함)

    ```bash
    make start
    ```

1. 실행
    1. smi 파일 백업 X
        - 탐색경로들을 모두 탐색하며 smi 파일을 찾아 srt파일로 변경
        - smi 파일은 삭제되지 않음

        ```bash
        ./smi2srt [탐색경로1] [탐색경로2] ... [탐색경로n]
        ```

    1. smi 파일 백업 O
        - 탐색 경로들을 모두 탐색하며 smi 파일을 찾아 srt 파일로 변경
        - smi 파일들은 백업 디렉터리 하위에 같은 경로로 디렉터리가 생성되어 모두 이동됨

        ```bash
        ./smi2srt -b [백업경로] [탐색경로1] [탐색경로2] ... [탐색경로n]
        ```

## 구현

1. 이전 실행 내역 있는지 판단
2. -b 옵션 여부 판단
3. 인자로 받은 경로들에 대해 재귀 탐색 (search_directory)
    1. 디렉터리인 경우 재귀 호출
    2. smi 파일인 경우
        1.  변환 수행 (smi2srt)
            - grep <body> 수행
                1. grep <body>의 결과가 있는 경우
                    1. convert_v1 실행
                    2. 실행 후 출력 결과에서 grep <Error> 수행
                        1. grep <Error>의 결과가 없는 경우
                            - log.txt에 성공 log 추가
                        2. grep <Error>의 결과가 있는 경우
                            - error.txt에 에러 log 추가
                2. grep <body>의 결과가 없는 경우
                    - convert_v2 실행
                        1. 실행 후 출력 결과가 있는 경우
                            - log.txt에 성공 log 추가
                        2. 실행 후 출력 결과가 없는 경우
                            - error.txt에 에러 log 추가
        2. -b 옵션 판단
            - -b 옵션인 경우
                1. 백업 경로 생성
                2. 백업 경로의 부모 디렉터리에 대해 재귀적으로 생성 (mkdir_recursive)
                3. 백업 경로로 smi 파일 이동
  
## 라이센스

George Shuklin

axfree [axfree/smi2srt](https://github.com/axfree/smi2srt)
