#!/bin/bash
PWD=${0%/*}

$PWD/smi2srt -nts -pwd $PWD /volume3/video2/movie /volume3/video2/drama

$PWD/smi2srt -nts -pwd $PWD /volume4/video3/lecture

$PWD/smi2srt -pwd $PWD /volume5/camera/2018 /volume5/camera/2019 /volume5/camera/2020
