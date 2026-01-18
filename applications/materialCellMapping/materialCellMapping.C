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


// Function to get index of actual point layer
int layer_index(const scalar& z_loc, const int& n_layers, const List<scalar>& layers)
{
	/*
		This function returns index of actual layer where the cell falls
	*/
	
	for (int i=0;i<n_layers;i++)
	{
		if ((z_loc >= layers[i]) and (z_loc < layers[i+1]+VSMALL) )
		{
			return i;
		}
		else if ((z_loc == layers[i]) and (i == (n_layers-1)))
		{
			return i;
		}
	}
	return n_layers-1;
}


//	Function to get cell index from pointList
int pin_index(const point& p, const List<vector>& pointList, const double& diameter)
{
	/*
		This function returns index of actual rod where the cell falls
	*/

	double r = diameter /2;
	double distance;
	
	for (int i=0; i<pointList.size(); i++)
	{
		// Distance between cell center and rod position from list
		distance = std::sqrt( pow(p[0]-pointList[i][0],2) + pow(p[1]-pointList[i][1], 2));

		// If the point is inside the radius than return the index of rod
		if (distance <= r) {
			//Info << i << " " << distance <<endl;
			return i;
		}
		/*
		if ((p[0] >= (pointList[i][0]-0.5*pitch)) && (p[0] < (pointList[i][0]+0.5*pitch)) && (p[1] >= (pointList[i][1]-0.5*pitch)) && (p[1] < (pointList[i][1]+0.5*pitch)))
		{
			return i;
		}
		*/
		
	}
	return pointList.size()-1;
}



// Main function to create material list

int main(int argc, char *argv[])
{
	timeSelector::addOptions();

    argList::addNote
    (
        "Optional argument is a name from the list of regions from constant/regionProperties"
    );

	argList::noParallel();
	
	// Option just to specifie region for cases with multiregion simulations
	argList::addOption
	(
		"region",
		"word"
	);
	
	word myRegion = "empty";

    #include "setRootCase.H"

	// These two create the time system (instance called runTime) and fvMesh (instance called mesh).
    #include "createTime.H"
    #include "createMesh.H"
    
    // Number of cells of the computational grid
    label nCells = mesh.C().size();

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
	
	// Name of the field for cell map
	volScalarField cellMap 
	(
		IOobject
		(
		    "cellMap", // name of the field
		    runTime.timeName(), // name of the current time, i.e. the time folder to read from
		    mesh,
		    IOobject::NO_READ, // always gets imported, will throw an error if the field is missing
		    IOobject::AUTO_WRITE // will get saved automatically when the controlDict parameters will request it
		),
		mesh,
		dimensionedScalar("cellMap", dimensionSet(0,0,0,0,0,0,0), mesh.C().size())  // initialises the field to match the size of the mesh with default (0) values
	);
	
	
	// Name of the field for pin id
	volScalarField pinId
	(
		IOobject
		(
		    "pinId", // name of the field
		    runTime.timeName(), // name of the current time, i.e. the time folder to read from
		    mesh,
		    IOobject::NO_READ, // always gets imported, will throw an error if the field is missing
		    IOobject::AUTO_WRITE // will get saved automatically when the controlDict parameters will request it
		),
		mesh,
		dimensionedScalar("pinId", dimensionSet(0,0,0,0,0,0,0), mesh.C().size())  // initialises the field to match the size of the mesh with default (0) values
	);
	
	// Get access to a custom dictionary materialProperties
    dictionary customDict;
    const word dictName("materialProperties");

    // Create input-output object - this holds the path to the dict and its name
    IOobject dictIO
    (
        dictName, // name of the file
        mesh.time().constant(), // path to where the file is
        mesh, // reference to the mesh needed by the constructor
        IOobject::MUST_READ // indicate that reading this dictionary is compulsory
    );

    // Check the if the dictionary is present and follows the OF format
    if (!dictIO.typeHeaderOk<dictionary>(true))
        FatalErrorIn(args.executable()) << "Cannot open specified dictionary "
            << dictName << exit(FatalError);

    // Initialise the dictionary object
    customDict = IOdictionary(dictIO);
	
	// Lists of rod points
	List<vector> pointList (customDict.lookup("pointList"));
	// Lists of materials at each rod position
	List<List<word>> materialList (customDict.lookup("materialList"));
	
	// List of material in each point
	List<word> pointMaterial(nCells);
	// Number of points
	int n_points = pointList.size();
	// Number of materials
	int n_matPoints = materialList.size();
	
	Info << "Number of rod points " << n_points << endl;
	Info << "Number of material " << n_matPoints << endl;
	//
	// pitch
	double diameter;
	customDict.lookup("diameter") >> diameter;
	
	for (int i = 0; i<n_points; i++)
	{	
		Info << "Point " << i << " " << pointList[i] << endl;
	}
	
	for (int i = 0; i<n_matPoints; i++)
	{	
		Info << "Rod " << i << " Mat ";
		for (int j = 0; j < materialList[i].size(); j++)
		{
			Info << j << " " << materialList[i][j] << " ";
		}
		Info  << endl;
	}
	
	// Lists of values may also be read in the same way
    List<scalar> axialList ( customDict.lookup("axialList") );
    // Number of axial surfaces
	int n_axial_surf = axialList.size();
	// Number of axial layers
	int n_axial = n_axial_surf -1;
	int actual_layer;

	Info << nl;
	for (int i=0; i<n_axial; i++)
	{	
		Info << "Axial " << i << " bottom: " << axialList[i] << " top: " << axialList[i+1] << endl;
	}
	

    // Tolerance for area calculation
    scalar cell_tolerance;
    
    
    customDict.lookup("tol") >> cell_tolerance;
    
    
   	fileName outputDir = mesh.time().constant();
   	autoPtr<OFstream> outputFilePtr;
	word	materialName= "materials.dat";
	

    // creates outputFile
    if (myRegion == "empty")
    {
    	fileName outputDir = mesh.time().constant();
   		outputFilePtr.reset(new OFstream(outputDir/materialName));
    }
    else
    {
		fileName outputDir = mesh.time().constant();
		outputFilePtr.reset(new OFstream(outputDir/myRegion/materialName));

	}
	Info << "Output name: " << outputDir << "\n";
	

	// Insert number of cell in the first line
	outputFilePtr() << mesh.C().size() << nl;
    
    // Loop over all cell centers
    for (label cellI = 0; cellI < mesh.C().size(); cellI++)
    {
    		actual_layer = layer_index(mesh.C()[cellI][2],n_axial,axialList);
			label p_id;
			p_id = pin_index(mesh.C()[cellI],pointList,diameter);
			pinId[cellI] = p_id;
			pointMaterial[cellI] = materialList[p_id][actual_layer];
			//Info << cellI << " " << pointMaterial[cellI] << nl;
			// 
	    	outputFilePtr() << pointMaterial[cellI] << nl;			
			/*
			cellMap[cellI] = actual_layer*n_angular*n_radius*n_points
			+ n_angular*n_radius*p_id 
			+ n_angular*radius_index(mesh.C()[cellI],pointList[p_id],radiusList)
			+ angular_index(mesh.C()[cellI],pointList[p_id],angleList) + 1;
	    	
	    	outputFilePtr() << cellMap[cellI] << nl;
	    	*/
    }
	cellMap.write();
	pinId.write();

	
	
    Info << endl; // spacer
    Info<< "End\n" << endl;
	
    return 0;
}


// ************************************************************************* //
