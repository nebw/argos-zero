#!/bin/bash
# Paths
readonly SOURCES=/root/argos-build/src
readonly WEIGHTP=.
wfile="foo"

while true; do 
	# look for new weights (just one variable)

	newwfile=$(wget -O- -q http://tonic.imp.fu-berlin.de/argos/best-weights) 
	# for two variables: eg file name and threshold
	IFS=';' read -ra VARS <<< "$(wget -O- -q http://tonic.imp.fu-berlin.de/argos/best-weights)"
	newwfile=${VARS[0]}
	thres=${VARS[1]}
	unset IFS
	
	# download new weights if available
	if [ "$newwfile" != "$wfile" ]
	then
		# download
		wget http://tonic.imp.fu-berlin.de/argos/$newwfile-0000.params -P $WEIGHTP
		wget http://tonic.imp.fu-berlin.de/argos/$newwfile-symbol.json -P $WEIGHTP
		# set new filename
		wfile=$newwfile
	fi 
	# run selfplay with the newest weights
	$SOURCES/selfplay -n $WEIGHTP/$wfile --tree-networkRollouts true --tree-trainingMode true
done
