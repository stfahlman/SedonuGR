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

#include <mpi.h>
#include <fstream>
#include "global_options.h"
#include "physical_constants.h"
#include "Species.h"
#include "PolarSpectrumArray.h"
#include "MomentSpectrumArray.h"
#include "RadialMomentSpectrumArray.h"
#include "GR1DSpectrumArray.h"
#include "Transport.h"
#include "Grid.h"

using namespace std;
namespace pc = physical_constants;

Species::Species(){
	weight = NaN;
	lepton_number = MAXLIM;
	sim = NULL;
	ID = MAXLIM;
	T_core = NaN;
	mu_core = NaN;
	core_lum_multiplier = NaN;
}

void Species::init(Lua* lua, Transport* simulation)
{
	// initialize MPI parallelism
	int MPI_myID;
	MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID );
	const int rank0 = (MPI_myID == 0);

	// set the pointer to see the simulation info
	sim = simulation;
	PRINT_ASSERT(sim->grid->rho.size(),>,0);


	//============================//
	// CALL CHILD'S INIT FUNCTION //
	//============================//
	myInit(lua);

	if(rank0) cout << "finished." << endl;
}


//-----------------------------------------------------------------
// get opacity at the frequency
//-----------------------------------------------------------------
void Species::get_opacity(const double com_nu, const int z_ind, double* a, double* s) const
{
	PRINT_ASSERT(z_ind,>=,-1);
	PRINT_ASSERT(com_nu,>,0);

	// absorption and scattering opacities
	unsigned nu_bin = sim->grid->nu_grid_axis.bin(com_nu);
	unsigned dir_ind[NDIMS+1];
	sim->grid->rho.indices(z_ind,dir_ind);
	dir_ind[NDIMS] = nu_bin;
	unsigned global_index = sim->grid->abs_opac[ID].direct_index(dir_ind);

	*a = sim->grid->abs_opac[ID][global_index];
	*s = sim->grid->scat_opac[ID][global_index];

	PRINT_ASSERT(*a,>=,0);
	PRINT_ASSERT(*s,>=,0);
	PRINT_ASSERT(*a,<,INFINITY);
	PRINT_ASSERT(*s,<,INFINITY);
}

