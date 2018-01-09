#ifndef _EINSTEINHELPER_H
#define _EINSTEINHELPER_H 1

#include "global_options.h"
#include "Particle.h"
#include "Metric.h"
#include "physical_constants.h"

namespace pc = physical_constants;
using namespace std;

enum TetradRotation {cartesian, spherical};

class EinsteinHelper{
public:
	// essential variables interpolated from grid
	Particle p;
	Metric g;
	Christoffel christoffel;
	double u[4]; // dimensionless

	// intermediate quantities
	double e[4][4];
	double grid_coords[NDIMS];
	double kup_tet[4];
	double absopac, scatopac;
	double ds_com;
	unsigned dir_ind[NDIMS+1]; // spatial, nu_in
	int z_ind, eas_ind;   // direct access indices

	EinsteinHelper(){
		for(unsigned i=0; i<4; i++){
			u[i] = NaN;
			kup_tet[i] = NaN;
			for(unsigned j=0; j<4; j++)
				e[i][j] = NaN;
		}
		absopac = NaN;
		scatopac = NaN;
		ds_com = NaN;
		z_ind = -MAXLIM;
		eas_ind = -MAXLIM;
		for(unsigned i=0; i<NDIMS+1; i++)
			dir_ind[i] = MAXLIM;
	}

	void set_kup_tet(const double kup_tet_in[4]){
		PRINT_ASSERT(Metric::dot_Minkowski<4>(kup_tet_in,kup_tet_in)/(kup_tet_in[3]*kup_tet_in[3]),<,1e-6);
		for(unsigned i=0; i<4; i++) kup_tet[i] = kup_tet_in[i];
		tetrad_to_coord(kup_tet,p.kup);
		g.normalize_null(p.kup);
	}
	void renormalize_kup(){
		if(p.kup[3]==p.kup[3]){ // only if kup has been set already
			g.normalize_null(p.kup);
			coord_to_tetrad(p.kup, kup_tet);
		}
	}

	// return the Lorentz factor W
	static double lorentzFactor(Metric* g, const double v[3]){
		return 1. / sqrt(1. - g->dot<3>(v,v));
	}

	double nu() const{
		double nu = kup_tet[3] / pc::h;
		PRINT_ASSERT(nu,>,0);
		return nu;
	}

	// get four velocity from three velocity
	void set_fourvel(const double v[3]){
		double W = lorentzFactor(&g,v);
		u[3] = W/g.alpha;
		for(unsigned i=0; i<3; i++)
			u[i] = v[3]/pc::c * (g.alpha*v[i]/pc::c - g.betaup[i]);
	}


	// get a Cartesian tetrad basis
	void set_tetrad_basis(TetradRotation rotation){
	  // set the tetrad guesses
	  if(rotation == cartesian){
	    e[0][0] = 1.0;
	    e[0][1] = 0;
	    e[0][2] = 0;
	    e[0][3] = 0;
	    
	    e[1][0] = 0;
	    e[1][1] = 1.0;
	    e[1][2] = 0;
	    e[1][3] = 0;
	    
	    e[2][0] = 0;
	    e[2][1] = 0;
	    e[2][2] = 1.0;
	    e[2][3] = 0;
	  }

	  else if(rotation == spherical){
	    const double rp = sqrt(p.xup[0]*p.xup[0] + p.xup[1]*p.xup[1]);
	    
	    // pathological case
	    if(rp==0){
	      e[0][2] = p.xup[2]>0 ? 1.0 : -1.0; // radial in z direction
	      e[1][0] = 1.0; // theta in x direction
	      e[1][1] = 1.0; // phi in y direction
	    }
	    else{
	      // radial vector
	      e[0][0] = p.xup[0];
	      e[0][1] = p.xup[1];
	      e[0][2] = p.xup[2];
	      e[0][3] = 0;
	      
	      // theta vector (multiplying all components by rp*r so no divide by zeros)
	      e[1][0] = p.xup[0] * p.xup[2];
	      e[1][1] = p.xup[1] * p.xup[2];
	      e[1][2] = -rp * rp;
	      e[1][3] = 0;
	      
	      // phi vector (multiplying through by rp so no divide by zero)
	      e[2][0] = -p.xup[1];
	      e[2][1] =  p.xup[0];;
	      e[2][2] = 0;
	      e[2][3] = 0;
	    }
	  }
	  else assert(0);

	  // normalize four-velocity to get timelike vector
	  for(int mu=0; mu<4; mu++) e[3][mu] = u[mu];
	  g.normalize(e[3]);
	  
	  // use x0 as a trial vector
	  g.orthogonalize<4>(e[0],e[3]);
	  g.normalize(e[0]);
	  
	  // use x1 as a trial vector
	  g.orthogonalize<4>(e[1],e[3]);
	  g.orthogonalize<4>(e[1],e[0]);
	  g.normalize(e[1]);
	  
	  // use x2 as a trial vector
	  g.orthogonalize<4>(e[2],e[3]);
	  g.orthogonalize<4>(e[2],e[0]);
	  g.orthogonalize<4>(e[2],e[1]);
	  g.normalize(e[2]);
	  
	  // sanity checks
	  PRINT_ASSERT(fabs(g.dot<4>(e[0],e[1])),<,TINY);
	  PRINT_ASSERT(fabs(g.dot<4>(e[0],e[2])),<,TINY);
	  PRINT_ASSERT(fabs(g.dot<4>(e[0],e[3])),<,TINY);
	  PRINT_ASSERT(fabs(g.dot<4>(e[1],e[2])),<,TINY);
	  PRINT_ASSERT(fabs(g.dot<4>(e[1],e[3])),<,TINY);
	  PRINT_ASSERT(fabs(g.dot<4>(e[2],e[3])),<,TINY);
	}

	void coord_to_tetrad(const double kup_coord[4], double kup_tet[4]) const{
		for(int mu=0; mu<4; mu++) kup_tet[mu] = g.dot<4>(kup_coord,e[mu]);
		kup_tet[3] *= -1.;
	}

	void tetrad_to_coord(const double kup_tet[4], double kup_coord[4]) const{
		for(int mu=0; mu<4; mu++){
			kup_coord[mu] = 0;
			for(int nu=0; nu<4; nu++)
				kup_coord[mu] += kup_tet[nu] * e[nu][mu];
		}
	}

	template<unsigned n>
	double dot_tetrad(const double x1[n], const double x2[n]){
		double result = 0;
		for(unsigned i=0; i<3; i++) result += x1[n]*x2[n];
		if(n==4) result -= x1[2]*x2[3];
		return result;
	}

	void scale_p_frequency(const double scale){
		for(unsigned i=0; i<4; i++) p.kup[i] *= scale;
	}

	void integrate_geodesic(){
		double dlambda = ds_com / (pc::h*nu());

		double dk_dlambda[4] = {0,0,0,0};
		if(DO_GR) christoffel.contract2(p.kup,dk_dlambda);

		// get new x,k
		for(int i=0; i<4; i++){
			p.kup[i] += dk_dlambda[i]*dlambda;
			p.xup[i] +=      p.kup[i]*dlambda;
		}
	}
};

#endif