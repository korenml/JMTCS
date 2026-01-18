#!/bin/sh

echo "Is boundaryField in volPower?"
echo

if grep -q "boundaryField" constant/fuel/volPower; then
	echo found
else
	echo not found
fi
#------------------------------------------------------------------------------
