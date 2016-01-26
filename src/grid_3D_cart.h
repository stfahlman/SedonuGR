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

#ifndef _GRID_3D_CART_H
#define _GRID_3D_CART_H 1

#include "global_options.h"
#include "grid_general.h"
#include <vector>
#include "Lua.h"

//*******************************************
// 1-Dimensional Spherical geometry
//*******************************************
class grid_3D_cart: public grid_general
{

private:
	// specifics to this geometry

	vector<int>    nx; // number of zones in each dimension
	vector<double> dx; // length of each zone in each dimension
	vector<double> x0; // leftmost points
	vector<double> x1; // next-to leftmost points
	vector<double> xmax;
	vector<int> reflect;
	vector<int> rotate_hemisphere;
	int rotate_quadrant;

public:

	grid_3D_cart();
	virtual ~grid_3D_cart() {}

	void get_deltas(const int z_ind, vector<double>& delta) const;

	void read_model_file(Lua* lua);
	void read_SpEC_file(Lua* lua);
	void read_David_file(Lua* lua);

	double zone_left_boundary(const unsigned dir, const unsigned dir_ind) const;
	double zone_right_boundary(const unsigned dir, const unsigned dir_ind) const;

	// required functions
	int    zone_index               (const vector<double>& x                                       ) const;
	int    zone_index               (const int i, const int j, const int k                         ) const;
	double zone_lab_volume              (const int z_ind                                               ) const;
	double zone_min_length          (const int z_ind                                               ) const;
	void   zone_coordinates         (const int z_ind, vector<double>& r                            ) const;
	void   zone_directional_indices (const int z_ind, vector<int>& dir_ind                         ) const;
	void   cartesian_sample_in_zone (const int z_ind, const vector<double>& rand, vector<double>& x) const;
	void   cartesian_velocity_vector(const vector<double>& x, vector<double>& v, int z_ind      ) const;
	void   write_rays               (const int iw                                                  ) const;
	void   reflect_outer            (particle *p                                                   ) const;
	void   symmetry_boundaries      (particle *p                                                   ) const;
	double lab_dist_to_boundary         (const particle *p                                             ) const;
	double zone_radius              (const int z_ind) const;
	void dims(vector<hsize_t>& dims) const;
	hsize_t dimensionality() const {return 3;};
	void write_hdf5_coordinates(H5::H5File file) const;
};


#endif
