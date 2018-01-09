#ifndef _METRIC_H
#define _METRIC_H 1

#include "global_options.h"

const unsigned ixx=0,iyy=1,izz=2,ixy=3,ixz=4,iyz=5,itt=6,ixt=7,iyt=8,izt=9;

class ThreeMetric{
// both indices are down
public:
	Tuple<double,6> data;

	ThreeMetric(){
		data = NaN;
	}
};

class Christoffel{
public:
	// first index is up, others are down
	// 0-9 = 0 first index, 10-19 = 1 first index, 20-29 = 2 first index
	// 3 first index not included
	Tuple<double, 30> data;

	Christoffel(){
		data = NaN;
	}

	void contract2(const double kup[4], double result[3]){
		for(unsigned i=0; i<3; i++){
			const unsigned offset = i*10;
			result[i] = 0;
			result[i] += data[offset+ixx] * kup[0]*kup[0];
			result[i] += data[offset+iyy] * kup[1]*kup[1];
			result[i] += data[offset+izz] * kup[2]*kup[2];
			result[i] += data[offset+itt] * kup[3]*kup[3];

			result[i] += data[offset+ixy] * kup[0]*kup[1]*2.0;
			result[i] += data[offset+ixz] * kup[0]*kup[2]*2.0;
			result[i] += data[offset+ixt] * kup[0]*kup[3]*2.0;
			result[i] += data[offset+iyz] * kup[1]*kup[2]*2.0;
			result[i] += data[offset+iyt] * kup[1]*kup[3]*2.0;
			result[i] += data[offset+izt] * kup[2]*kup[3]*2.0;
		}
	}
};

//========//
// METRIC //
//========//
class Metric{
private:
	double gtt, betalow[3];

public:
	double alpha, betaup[3];
	ThreeMetric gammalow;

	Metric(){
		gtt=NaN;
		alpha = NaN;
		for(unsigned i=0; i<3; i++){
			betaup[i] = NaN;
			betalow[i] = NaN;
		}
	}

	// fill in values for gtt and betalow
	// assumes alpha, betaup, and gammalow have been set.
	void update(){
		lower<3>(betaup, betalow);
		gtt = DO_GR ? -alpha*alpha + contract<3>(betaup, betalow) : -1.0;
	}

	template<unsigned n>
	void lower(const double xup[], double xdown[]) const{
		if(DO_GR){
			xdown[0] = gammalow.data[ixx]*xup[0] +
					   gammalow.data[ixy]*xup[1] +
					   gammalow.data[ixz]*xup[2];

			xdown[1] = gammalow.data[ixy]*xup[0] +
					   gammalow.data[iyy]*xup[1] +
					   gammalow.data[iyz]*xup[2];

			xdown[2] = gammalow.data[ixz]*xup[0] +
					   gammalow.data[iyz]*xup[1] +
					   gammalow.data[izz]*xup[2];

			if(n==4){
				xdown[3]  = xup[3] * gtt;
				for(unsigned i=0; i<3; i++){
					xdown[3] += xup[i] * betalow[i];
					xdown[i] += xup[3] * betalow[i];
				}
			}
		}
		else{
			for(unsigned i=0; i<3; i++) xdown[i] = xup[i];
			if(n==4) xdown[3] = -xup[3];
		}
	}

	template<unsigned n>
	double contract(const double xup[n], const double xdown[n]) const{
		double result = 0;
		for(int i=0; i<n; i++)result += xup[i]*xdown[i];
		return result;
	}

	// dot product
	template<unsigned n>
	double dot(const double x1up[n], const double x2up[n]) const{
		if(DO_GR){
			double x2low[n];
			lower<n>(x2up, x2low);
			double result = contract<n>(x1up, x2low);
			return result;
		}
		else return dot_Minkowski<n>(x1up, x2up);
	}

	// dot the normal observer's four-velocity with a four vector
	double ndot(const double x[4]) const{
		return -alpha * x[3];
	}

	// normalize a four vector to have a norm of +/-1
	void normalize(double x[4]) const{
		double invnorm = sqrt(fabs(1./dot<4>(x,x)));
		for(int i=0; i<4; i++) x[i] *= invnorm;
	}

	// make a vector null
	void normalize_null(double x[4]) const{
		const double invA = 1./gtt;
		const double B = (x[0]*betalow[0] + x[1]*betalow[1] + x[2]*betalow[2]) * invA;
		const double C = dot<3>(x,x) * invA;
		const double result = 0.5 * (B + sqrt(B*B - 4.*C));
		PRINT_ASSERT(result,>,0);
		x[3] = result;
	}

	// make four vector v orthogonal to four vector v
	template<unsigned n>
	void orthogonalize(double v[n], const double e[n]) const{
		double projection = dot<n>(v,e);
		for(int mu=0; mu<n; mu++) v[mu] -= projection * e[mu];
	}

	// vector operations
	template<unsigned s>
	static double dot_Minkowski(const double a[], const double b[]){
		double product = 0;
		for(unsigned i=0; i<3; i++) product += a[i]*b[i];
		if(s==4) product -= a[3]*b[3];
		return product;
	}

	// normalize a vector
	template<unsigned s>
	static void normalize_Minkowski(double a[]){
		double inv_magnitude = 1./sqrt(fabs( dot_Minkowski<s>(a,a) ));
		PRINT_ASSERT(inv_magnitude,<,INFINITY);
		for(unsigned i=0; i<s; i++) a[i] *= inv_magnitude;
	}

	static void normalize_null_Minkowski(double a[4]){
		double spatial_norm = dot_Minkowski<3>(a,a);
		a[3] = sqrt(spatial_norm);
	}

};

#endif