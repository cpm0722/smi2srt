#!/bin/bash
PWD=${0%/*}

$PWD/smi2srt -nts -pwd $PWD -b /volume3/video2/smi_bak /volume3/video2/movie /volume3/video2/drama

$PWD/smi2srt -nts -pwd $PWD -b /volume4/video3/smi_bak /volume4/video3/lecture

$PWD/smi2srt -pwd $PWD -b /volume5/camera/smi_bak /volume5/camera/2018 /volume5/camera/2019 /volume5/camera/2020
