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
	int    get_index(const double z) const;    // sample index from the CDF, when passed a random #
	void   print() const;
	void   wipe();
	unsigned size() const;

};

#endif
