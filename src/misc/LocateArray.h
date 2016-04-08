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

#ifndef _LOCATE_ARRAY_H
#define _LOCATE_ARRAY_H 

#include <vector>

enum InterpolationMethod {constant, linear, logarithmic, power};

class LocateArray {

public:

	// where applicable, these values are the right bin wall (i.e. not the left)
	std::vector<double> x;
	double min;

	// other parameters
	InterpolationMethod interpolation_method;

	// constructors
	LocateArray(int n=0);
	//locate_array(int n) {init(n);}

	// Return size of array (also, # of bins)
	unsigned size() const {return (int)x.size();}

	void init(const int size);
	void init(const double start,const double stop,const double del, const InterpolationMethod imeth=constant);
	void init(const double start,const double stop,const int n, const InterpolationMethod imeth=constant);
	void init(const std::vector<double> a, const double minval, const InterpolationMethod imeth=constant);
	void swap(LocateArray& new_array);
	void copy(const LocateArray& new_array);

	// bottom of the bin left of i
	double bottom(const int i) const{
		return (i==0 ? min : x[i-1]);
	}

	// center of the bin left of i
	double center(const int i) const{
		return 0.5 * (bottom(i) + x[i]);
	}

	// width of the bin left of i
	double delta(const int i) const{
		return x[i] - bottom(i);
	}


	int    locate(const double) const;
	void   print() const;
	double value_at(const double nu, const std::vector<double>& array) const;

	// operators for easy access
	double  operator[] (const int i) const {return x[i];};
	double& operator[] (const int i)       {return x[i];};
	void resize(int i) {x.resize(i);};
};

#endif