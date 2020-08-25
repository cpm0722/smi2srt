smi2srt: smi2srt.o convert_v1
	gcc -o smi2srt smi2srt.o

smi2srt.o: smi2srt.c convert_v1
	gcc -c smi2srt.c

init: smi_bak
	rm -rf log.txt error.txt smi test tmp.tmp grep.tmp
	cp -r smi_bak smi

start: smi2srt convert_v1 smi
	./smi2srt smi -b test

init2: smi_bak
	rm -rf log.txt error.txt smi smi_1 smi_2 test tmp.tmp grep.tmp
	cp -r smi_bak smi
	cp -r smi_bak/dir1 smi_1
	cp -r smi_bak/dir2 smi_2

start2: smi2srt convert_v1 smi smi_1 smi_2
	./smi2srt smi smi_1 smi_2 -b test
