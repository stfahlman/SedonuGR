/*
//  Copyright (c) 2015, California Institute of Technology and the Regents
//  of the University of California, based on research sponsored by the
//  United States Department of Energy. All rights reserved.
//
//  This file is part of Sedonu.
//
//  Sedonu is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Neither the name of the California Institute of Technology (Caltech)
//  nor the University of California nor the names of its contributors 
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  Sedonu is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Sedonu.  If not, see <http://www.gnu.org/licenses/>.
//
*/

#ifndef _SPECIES_H
#define _SPECIES_H

#include <vector>
#include "SpectrumArray.h"
#include "PolarSpectrumArray.h"
#include "Lua.h"
#include "CDFArray.h"

class Transport;

class Species
{

public:

	// pointer to the simulation info (one level up)
	Transport* sim;

	// species-specific initialization stuff
	virtual void myInit(Lua* lua) = 0;

public:

	Species();
	virtual ~Species() {}

	// name
	std::string name;
	int ID;

	// lepton number (the particle property, {-1,0,1})
	int lepton_number;

	// the numbers of species this represents
	double weight;

	// core properties
	double T_core, mu_core;
	double core_lum_multiplier;

	// set everything up
	void init(Lua* lua, Transport* sim);

	// set the emissivity, absorption opacity, and scattering opacity
	virtual void set_eas(const int zone_index) = 0;
};




#endif
