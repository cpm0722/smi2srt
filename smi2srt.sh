#!/bin/sh

DAT=`date +%Y%m%d-%H%M%S`
echo "-----------------------------------------------------"
echo  Start $DAT
echo "-----------------------------------------------------"

targetdir='/volume1/smi'
# targetdir2='/volume1/inbox/download/_smi2srt_'
smidir='/volume1/smi2srt'

# find $targetdir $targetdir2 -name "*.smi" | grep -v @eaDir | while read oldfile
find $targetdir $targetdir2 -name "*.smi" | grep -v @eaDir | while read oldfile
do
	newfile=${oldfile%smi}srt
	#newfile1=${oldfile%smi}kor.srt
       	if [ -f "$newfile" ] 
	then
		echo "$oldfile" "Skip!!"
	else
	        echo "$oldfile => $newfile" converted
	        $smidir/smi2srt "$oldfile" "$newfile" -d1
	        #$smidir/smi2srt "$oldfile" "$newfile1" -d1
fi
done   

DAT2=`date +%Y%m%d-%H%M%S`
echo "-----------------------------------------------------"
echo Start Time : $DAT   
echo End Time : $DAT2 
echo "-----------------------------------------------------"
