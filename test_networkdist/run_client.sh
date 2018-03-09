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
		if [ ! -f "$WEIGHTP/$newwfile-0000.params" ]; then
			wget http://tonic.imp.fu-berlin.de/argos/$newwfile-0000.params -P $WEIGHTP
		fi
		if [ ! -f "$WEIGHTP/$newwfile-symbol.json" ]; then
			wget http://tonic.imp.fu-berlin.de/argos/$newwfile-symbol.json -P $WEIGHTP
		fi
		# set new filename
		wfile=$newwfile
	fi 
	# run selfplay with the newest weights
	$SOURCES/selfplay -n $WEIGHTP/$wfile --tree-networkRollouts true --tree-trainingMode true --tree-randomizeFirstNMoves 10 --engine-resignThreshold thres
done
