#!/bin/sh

echo "Is boundaryField properly set in volPower?"
echo

if grep -q "compressible::turbulentTemperatureCoupledBaffleMixed" constant/fuel/volPower; then
	echo "found"
	sed -i "s/compressible::turbulentTemperatureCoupledBaffleMixed/zeroGradient/" constant/fuel/volPower
else
	echo "not found"
fi
#------------------------------------------------------------------------------
