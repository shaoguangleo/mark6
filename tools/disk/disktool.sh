#!/bin/bash

# Author:	del@haystack.mit.edu
# Description:	Disable journaling.

# Executables
TUNE2FS=/sbin/tune2fs
E2FSCK=/sbin/e2fsck
DUMPE2FS=/sbin/dumpe2fs
MOUNT=/bin/mount
MEGACLI=/usr/sbin/megacli
PARTED=/sbin/parted
MKFS=/sbin/mkfs.ext4
FIO=/usr/bin/fio
CONFIG=jobs.fio
OUTPUT=jobs.out
SEQ=/usr/bin/seq


init_dev_map() {
	DEV_MAP=( $( < disktool.rc ) )
	#for i in $(${SEQ} 0 $((${#DEV_MAP[@]} - 1)))
	#do
		#${DEV_MAP[$i]}
	#done
}

tune_devs() {
	for d in ${DEVS}
	do
		IFS=':' read -ra a <<< "$p"
		dev=${a[0]}
		mnt=${a[1]}

		echo Configuring ${dev}1
		# Enable writeback mode. This mode will typically provide the best ext4 performance.
		${TUNE2FS} -o journal_data_writeback ${dev}1

		# Delete has_journal option
		${TUNE2FS} -O ^has_journal  ${dev}1
	
		# Required fsck
		${E2FSCK} -f ${dev}1

		# Check fs options
		${DUMPE2FS} ${dev}1
	done
}

mount_devs() {
	MOUNT_OPTS=defaults,data=writeback,noatime,nodiratime
	for p in ${DEV_MAP[@]}
	do
		IFS=':' read -ra a <<< "$p"
		dev=${a[0]}
		mnt=${a[1]}

		echo ${MOUNT} -t ext4 -o ${MOUNT_OPTS} ${dev}1 ${mnt}
		mkdir -p ${mnt}
		${MOUNT} -t ext4 -o ${MOUNT_OPTS} ${dev}1 ${mnt}
	done
}

mk_raid() {
	${MEGACLI}  -CfgClr -aALL
	# ${MEGACLI} -CfgLdAdd -R0[245:0,245:1,245:2,245:3,245:4,245:5,245:6,245:7,245:8,245:9,245:10,245:11] WT NORA -strpsz 256 -a0
	# ${MEGACLI} -CfgLdAdd -R0[245:12,245:13,245:14,245:15,245:16,245:17,245:18,245:19,245:20,245:21,245:22,245:23] WT NORA -strpsz 256 -a0
	# ${MEGACLI} -CfgLdAdd -R0[245:0,245:1,245:2,245:3,245:4,245:5,245:6,245:7,245:8,245:9,245:10,245:11] WT NORA -strpsz 256 -a1
	# ${MEGACLI} -CfgLdAdd -R0[245:12,245:13,245:14,245:15,245:16,245:17,245:18,245:19,245:20,245:21,245:22,245:23] WT NORA -strpsz 256 -a1

	# Individual disk testing
  	# ${MEGACLI} -CfgForeign -Clear -aALL
  	# ${MEGACLI} -CfgEachDiskRaid0 -aALL
}

mk_part() {
	for DEV in ${DEV_MAP[@]}
	do
		IFS=':' read -ra a <<< "$DEV"
		dev=${a[0]}
		mnt=${a[1]}

		echo ${PARTED} ${dev} --script mklabel gpt
		${PARTED} ${dev} --script mklabel gpt
	
		echo ${PARTED} ${dev} --script rm 1
		${PARTED} ${dev} --script rm 1

		echo ${PARTED} ${dev} --script mkpart ext4 1049k -- -1
		${PARTED} ${dev} --script mkpart ext4 1049k -- -1
	done
}

mk_fs() {
	for DEV in ${DEV_MAP[@]}
	do
		IFS=':' read -ra a <<< "$DEV"
		dev=${a[0]}
		mnt=${a[1]}

		echo ${MKFS} ${dev}1
		${MKFS} ${dev}1
	done
}

perf_test() {
	${FIO}	--output=${OUTPUT} \
		--minimal \
		${CONFIG}

	# Field description.
	# jobname, groupid, error, 
	# Read status:
	# KB I/O, bandwidth (KB/s), runtime (ms)

	# Submission latency:
	# min, max, mean, standard deviation

	# Completion latency:
	# min, max, mean, standard deviation
	# 
	# Bandwidth:
	# min, max, aggregate percentage of total, mean, standard deviation

	# Write status:
	# KB I/O, bandwidth (KB/s), runtime (ms)

	# Submission latency:
	# min, max, mean, standard deviation
	# Completion latency:
	# min, max, mean, standard deviation
	# Bandwidth:
	# min, max, aggregate percentage of total, mean, standard deviation

	# CPU usage:
	# user, system, context switches, major page faults, minor page faults

	# IO depth distribution:
	# <=1, 2, 4, 8, 16, 32, >=64
	# 
	# IO latency distribution (ms):
	# <=2, 4, 10, 20, 50, 100, 250, 500, 750, 1000, >=2000
	# 
	# text description
}


usage() {
    echo "$0: [-r] [-p] [-f] [-t] [-m] [-a] [-T] [-c] [-C] [-h]"
    echo "  -r	Configure RAID"
    echo "  -p	Create partitions"
    echo "  -f	Create file systems"
    echo "  -t	Tune file systems"
    echo "  -m	Mount file systems"
    echo "  -a	Do everything"
    echo "  -T	Test disk performance"
    echo "  -c	Device configuration file (default disktool.rc)."
    echo "  -C	FIO/performance configuration file (default disktool.fio)."
    echo "  -h	Display help message"
}

init_dev_map

main() {
	if [ $# -eq 0 ] ; then
	    usage
	    exit
	fi
	
	DEV_CONFIG="disktool.rc"
	FIO_CONFIG="disktool.fio"
	MK_RAID=0
	MK_PART=0
	MK_FS=0
	TUNE_DEVS=0
	MOUNT_DEVS=0
	PERF_TEST=0
	ALL=0

	echo Welcome to the Mark6 disk management program
	echo
	echo This software has been developed by MIT Haystack Observatory and
	echo is released under the terms fo the GPL \(see LICENSE file\)
	echo 
	echo Please direct any questions to del@haystack.mit.edu
	echo

	while getopts ":c:C:rpftmTah" opt
	do
    		case ${opt} in
		c )	DEV_CONFIG=$OPTARG
			echo DEV_CONFIG: ${DEV_CONFIG}
			;;
		C )	FIO_CONFIG=$OPTARG
			;;
		r )	MK_RAID=1
			;;
		p )	MK_PART=1
			;;
		f )	MK_FS=1
			;;
		t )	TUNE_DEVS=1
			;;
		m )	MOUNT_DEVS=1
			;;
		T )	PERF_TEST=1
			;;
		a )	ALL=1
			;;
		h )	usage
			exit
			;;
		\? )	echo "Invalid option: -$OPTARG"
			usage
			exit 1
			;;
		: )	echo "Option -$OPTARG requires an argument"
			usage
			exit 1
			;;
		esac
	done

	echo CONFIGURATION PARAMETERS
	echo dev_config: ${DEV_CONFIG}
	echo fio_config: ${FIO_CONFIG}
	
	if [ $MK_RAID -ne 0 ]; then
		echo mk_raid
		# mk_raid
	fi

	if [ $MK_PART -ne 0 ]; then
		echo mk_part
		# mk_part
	fi

	if [ $MK_FS -ne 0 ]; then
		echo mk_fs
		# mk_fs
	fi

	if [ $TUNE_DEVS -ne 0 ]; then
		echo tune_devs
		# tune_devs
	fi

	if [ $MOUNT_DEVS -ne 0 ]; then
		echo mount_devs
		# mount_devs
	fi

	if [ $PERF_TEST -ne 0 ]; then
		echo perf_test
		# perf_test
	fi

	if [ $ALL -ne 0 ]; then
		echo All
		mk_raid
		mk_part
		mk_fs
		tune_devs
		mount_devs
	fi
    }


# Kick off setup.
echo $*
main $*
