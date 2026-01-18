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



int layer_index(const scalar& z_loc, const int& n_layers, const List<scalar>& layers)
{
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


int pin_index(const point& p, const List<vector>& pointList, const double& diameter)
{
	/*
		This function returns index of actual rod where the cell falls
	*/

	double r = diameter / 2;
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
	}
	return pointList.size()-1;
}

double actual_radius(const point& p, const point& p_c)
{
	return Foam::sqrt(Foam::pow(p[0] - p_c[0],2) + Foam::pow(p[1] - p_c[1],2));
}

int radius_index(const point& p, const point& p_c, const List<scalar>& radiuses)
{
	double rad;
	rad = actual_radius(p,p_c);
	for (int i=0; i<(radiuses.size()-1); i++)
	{
		if ((rad >= radiuses[i]) && (rad < radiuses[i+1]))
		{
			return i;
		}
	}
	return radiuses.size()-2;
}

double actual_angle(const point& p, const point& p_c)
{
	return Foam::atan2((p[1] - p_c[1]),(p[0] - p_c[0]));
}

int angular_index(const point& p, const point& p_c, const List<scalar>& angulars)
{
	double angle;
	angle = actual_angle(p,p_c)*180.0/M_PI;
	if (angle < 0.0)
	{
		angle = 360.0 + angle;
	}
	for (int i=0; i<(angulars.size()-1); i++)
	{
		if ((angle >= angulars[i]) && (angle < angulars[i+1]))
		{
			return i;
		}
	}
	return angulars.size()-2;
}

double tri_area(const point& p1, const point& p2, const point& p3)
{
	return Foam::mag(((p1[0]-p3[0])*(p2[1]-p3[1])
	+ (p2[0]-p3[0])*(p3[1]-p1[1]))/2.0);
}

void print_points(const List<point>& ps)
{
	forAll(ps, psI)
	{
		Info << ps[psI] << " ";
	}
	Info << nl;
}

void print_point(const point& p)
{
	forAll(p, pI)
	{
		Info << p[pI];
	}
	Info << nl;
}

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
	word myRegion = "empty";

    #include "setRootCase.H"

	// These two create the time system (instance called runTime) and fvMesh (instance called mesh).
    #include "createTime.H"
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
	
	volScalarField radId
	(
		IOobject
		(
		    "radId", // name of the field
		    runTime.timeName(), // name of the current time, i.e. the time folder to read from
		    mesh,
		    IOobject::NO_READ, // always gets imported, will throw an error if the field is missing
		    IOobject::AUTO_WRITE // will get saved automatically when the controlDict parameters will request it
		),
		mesh,
		dimensionedScalar("radId", dimensionSet(0,0,0,0,0,0,0), mesh.C().size())  // initialises the field to match the size of the mesh with default (0) values
	);
	
	volScalarField angId
	(
		IOobject
		(
		    "angId", // name of the field
		    runTime.timeName(), // name of the current time, i.e. the time folder to read from
		    mesh,
		    IOobject::NO_READ, // always gets imported, will throw an error if the field is missing
		    IOobject::AUTO_WRITE // will get saved automatically when the controlDict parameters will request it
		),
		mesh,
		dimensionedScalar("angId", dimensionSet(0,0,0,0,0,0,0), mesh.C().size())  // initialises the field to match the size of the mesh with default (0) values
	);
	
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
	
	// Get access to a custom dictionary
    dictionary customDict;
    const word dictName("mappingProperties");

    // Create and input-output object - this holds the path to the dict and its name
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
	
	// Lists of points
	List<vector> pointList (customDict.lookup("pointList"));
	// Number of points
	int n_points = pointList.size();
	// pitch
	double pitch;
	customDict.lookup("pitch") >> pitch;
	
	for (int i=0; i<n_points; i++)
	{	
		Info << "Point " << i << " " << pointList[i] << endl;
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
	
	// Lists of radiuses
	List<scalar> radiusList ( customDict.lookup("radiusList") );
    // Number of radiuses
	int n_radius = radiusList.size() -1;

	Info << nl;
    for (int i=0; i<n_radius; i++)
	{	
		Info << "Radius " << i << " rIn " << radiusList[i] << " rOut " << radiusList[i+1] << endl;
	}
	
	// Lists of angles
	List<scalar> angleList ( customDict.lookup("angleList") );
    // Number of angles
	int n_angular = angleList.size() -1;

	Info << nl;
    for (int i=0; i<n_radius; i++)
	{	
		Info << "Radius " << i << " rIn " << radiusList[i] << " rOut " << radiusList[i+1] << endl;
	}
	
    // Tolerance for area calculation
    scalar cell_tolerance;
    customDict.lookup("tol") >> cell_tolerance;
    
    
    fileName outputDir = mesh.time().constant();
   	autoPtr<OFstream> outputFilePtr;
	word	materialName= "mapping.dat";
	

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

	
	outputFilePtr() << mesh.C().size() << nl;
    
    for (label cellI = 0; cellI < mesh.C().size(); cellI++)
    {
    		actual_layer = layer_index(mesh.C()[cellI][2],n_axial,axialList);
			label p_id;
			p_id = pin_index(mesh.C()[cellI],pointList,pitch);
			pinId[cellI] = p_id;
			radId[cellI] = radius_index(mesh.C()[cellI],pointList[p_id],radiusList);
			angId[cellI] = angular_index(mesh.C()[cellI],pointList[p_id],angleList);
			cellMap[cellI] = actual_layer*n_angular*n_radius*n_points
			+ n_angular*n_radius*p_id 
			+ n_angular*radius_index(mesh.C()[cellI],pointList[p_id],radiusList)
			+ angular_index(mesh.C()[cellI],pointList[p_id],angleList) + 1;
	    	
	    	outputFilePtr() << cellMap[cellI] << nl;
    }
	cellMap.write();
	pinId.write();
	radId.write();
	angId.write();
	
	/*
	
	Info << "Output file name: " << mappingName << nl;
	Info << mesh.C().size() <<nl;
	for (label cellI = 0; cellI < mesh.C().size(); cellI++)
    {   
    	bool inside=false;
    	actual_layer = layer_index(mesh.C()[cellI][2],n_axial,axialList);
		//mesh.C()[cellI]
		//stl_label = i + 1 + actual_layer*n_stl;
		
		
			if (inside == true)
			{
				break;
			}
			
		}
		if (inside == true)
		{
			outputFilePtr() << stl_label << nl;
		}
		else
		{
			outputFilePtr() << non_stl << nl;
		}
		
	}
	*/
	
    Info << endl; // spacer
    Info<< "End\n" << endl;
	
    return 0;
}


// ************************************************************************* //
