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

#ifndef _CDF_H
#define _CDF_H 1

#include <vector>
#include "locate_array.h"

//**********************************************************
// CDF == Cumulative Distribution Function
//
// This simple class just holds a vector which should be
// monitonically increasing and reaches unity
// We can sample from it using a binary search.
// the CDF value at locate_array's "min" is assumed to be 0
//**********************************************************

class cdf_array
{

private:

	std::vector<double> y;
	double inverse_tangent(const int i, const locate_array* xgrid) const;
	double inverse_secant(const int i, const int j, const locate_array* xgrid) const;


public:

	cdf_array(const int iorder = 1);
	int interpolation_order;

	double N;
	void resize(const int n)  {y.resize(n);}

	double get(const int i)const             {return y[i];}   // Get local CDF value
	void   set(const int i, const double f)  {y[i] = f;}      // Set cell CDF value

	void   set_value(const int i, const double f);     // set the actual (not CDF) value
	double get_value(const int i) const;               // Get the actual (not CDF) value

	void   normalize(double cutoff=0.0);         // normalize the cdf, so that final value = 1. Sets N.
	double invert_cubic(const double z, const locate_array* xgrid, const int i_in=-1) const;    // sample value from the CDF, when passed a random #
	double invert_linear(const double z, const locate_array* xgrid, const int i_in=-1) const;
	double invert_piecewise(const double z, const locate_array* xgrid, const int i_in=-1) const;
	int    get_index(const double z) const;    // sample index from the CDF, when passed a random #
	void   print() const;
	void   wipe();
	unsigned size() const;

};

#endif
