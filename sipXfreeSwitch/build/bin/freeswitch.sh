#!/bin/bash
#
# Copyright (C) 2004 SIPfoundry Inc.
# Licensed by SIPfoundry under the LGPL license.
#
# Copyright (C) 2004 Pingtel Corp.
# Licensed to SIPfoundry under a Contributor Agreement.

Action=RUN
Status=0
Args=""

. /opt/sipx-0.0.4.5.2/libexec/sipXecs/sipx-utils.sh

iam=`whoami`

while [ $# -ne 0 ]
do
    case ${1} in
        --configtest)
            Action=CONFIGTEST
            ;;

        --config)
            Action=CONFIG
            ;;

        --stop)
            Action=STOP
            ;;

        *)
            Args="$Args $1"
            ;;
    esac           

    shift # always consume 1
done

. /opt/sipx-0.0.4.5.2/libexec/sipXecs/sipx-utils.sh || exit 1

SIPX_USER=dhubler

FS_LOGDIR=/opt/sipx-0.0.4.5.2/var/log/sipxpbx
FS_DBDIR=/opt/sipx-0.0.4.5.2/var/sipxdata/tmp/freeswitch
FS_CONFDIR=/opt/sipx-0.0.4.5.2/etc/sipxpbx/freeswitch/conf
FS_EXEC="/opt/freeswitch/bin/freeswitch \
   -conf $FS_CONFDIR \
   -db $FS_DBDIR \
   -log $FS_LOGDIR \
   -htdocs $FS_CONFDIR/htdocs"

SIPXDIRS="$FS_DBDIR $FS_CONFDIR"
pidFile=/opt/sipx-0.0.4.5.2/var/run/sipxpbx/freeswitch.pid

case ${Action} in
   RUN)
     ulimit -s 240  # Keep stack size small for maximum number of threads
     echo $$ > $pidFile
     exec $FS_EXEC -nc -nf -nonat $Args
     exit 0
     ;;

   STOP)
     PID=`cat ${pidFile} 2> /dev/null`

     # Tell FS to stop the nice way
     $FS_EXEC -stop

     # Give it some time to die...
     # (FreeSWITCH normally shuts down in about 5 seconds)
     for ticks in `seq 10 -1 0`
     do
         sleep 1
         proc_alive "$PID" && echo -n "." || break
     done

     # If still alive, kill it the hard way,
     # else just do the normal cleanup
     sipx_stop freeswitch $pidFile
     
     exit $?
     ;;

   CONFIG)
     if [ $iam = root ]
     then
        Status=0
        echo -e "\n Installing FreeSWITCH folder \n"
        rm -rf $SIPXDIRS
        /opt/sipx-0.0.4.5.2/libexec/sipXecs/setup.d/freeswitch_setup.sh
     else
        Status=1
        echo -e "\n must be root to run this \n" >&2
     fi  
     ;;

   CONFIGTEST)
     WRONGPERM=`find $SIPXDIRS -not -user $SIPX_USER -or -not -group $SIPX_USER 2> /dev/null | wc -l`
     if [ $WRONGPERM -gt 0 ] 
     then
        if [ $iam = root ]
        then
           Status=0
           echo -e "\n Fixing FreeSWITCH folder permissions \n"
           rm -rf $SIPXDIRS
           /opt/sipx-0.0.4.5.2/libexec/sipXecs/setup.d/freeswitch_setup.sh
        else
           Status=1
           echo -e "\n Invalid FreeSWITCH folders & files permissions, run 'freeswitch.sh --configtest' as root to fix this \n" >&2
        fi  
     fi

     # check validity of xml routing rules
#     /opt/sipx-0.0.4.5.2/bin/sipx-validate-xml /opt/freeswitch/conf/freeswitch.xml
#     Status=$((${Status} + $?))

     if [ ! -x /opt/freeswitch/bin/freeswitch ]
     then
	 echo "Error: FreeSWITCH executable is not at /opt/freeswitch/bin/freeswitch" >&2
	 Status=$((${Status} + 1))
     fi

     ;;
esac

exit $Status
