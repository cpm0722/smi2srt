smi2srt: smi2srt.o convert_v1
	gcc -o smi2srt smi2srt.o

smi2srt.o: smi2srt.c convert_v1
	gcc -c smi2srt.c

init: smi_bak
	rm -rf log.txt error.txt smi test tmp.tmp grep.tmp
	cp -r smi_bak smi

start: smi2srt convert_v1 smi
	./smi2srt smi -b test
