/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2015 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "cellZone.H"

int main(int argc, char *argv[])
{
	timeSelector::addOptions();

    argList::addNote
    (
        "Optional argument is a name from the list of regions from constant/regionProperties"
    );

	argList::noParallel();
	argList::addOption
	(
		"region",
		"word"
	);
	argList::addOption
	(
		"alphaRelax",
		"scalar"
	);
	word myRegion = "empty";
    #include "setRootCase.H"
	// These two create the time system (instance called runTime) and fvMesh (instance called mesh).
    #include "createTime.H"
	instantList timeDirs = timeSelector::select0(runTime, args);
	#include "createMesh.H"

	// It creates mesh if the parameter "region" is used
	if (args.found("region") )
	{
		args.readIfPresent("region", myRegion);
		fvMesh mesh
		(
    		Foam::IOobject
    		(
    		    myRegion,
    		    runTime.timeName(),
    		    runTime,
    		    IOobject::MUST_READ
    		)

		);
		Info << "Region name: " << myRegion << nl << endl;
	}
	
	
	scalar alphaRelax, alphaRelaxRead;
	

	// It creates mesh if the parameter "region" is used
	if (args.found("alphaRelax") )
	{

		args.readIfPresent("alphaRelax", alphaRelaxRead);
		Info << "Found alphaRelax\n";
		Info << "Relaxing parameter for power field: " << alphaRelaxRead << endl;
		alphaRelax = alphaRelaxRead;
	}
	else
	{
		alphaRelax = 0.25;
	}
	
	Info << "Relaxing parameter for power field: " << alphaRelax << endl;
	if (args.found("constant") )
	{
		Info << "I'm reading only constant." << endl;
		#include "createFields.H"

    	    Info<< "Time = " << runTime.timeName() << endl;
			volPower = volPower_0*(1-alphaRelax)+volPower_S*alphaRelax;
			volPower_0 = volPower;
			volPower.write();
			volPower_0.write();
	}
	else
	{
		forAll(timeDirs, timeI)
    	{
			#include "createFields.H"
    	    runTime.setTime(timeDirs[timeI], timeI);
    	    Info<< "Time = " << runTime.timeName() << endl;
			volPower = volPower_0*(1-alphaRelax)+volPower_S*alphaRelax;
			volPower_0 = volPower;
			volPower.write();
			volPower_0.write();
		
		}
	}


    Info<< "Relaxing finished!" << endl;
    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
