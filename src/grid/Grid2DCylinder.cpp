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
#include "Grid2DCylinder.h"
#include "Transport.h"
#include "H5Cpp.h"

using namespace std;
namespace pc = physical_constants;

//------------------------------------------------------------
// initialize the zone geometry from model file
//------------------------------------------------------------
void Grid2DCylinder::read_model_file(Lua* lua)
{
    	// verbocity
    	int MPI_myID;
    	MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID );
    	const int rank0 = (MPI_myID == 0);

    	// generalHDF5 variables
    	H5::DataSet dataset;
    	H5::DataSpace space;
    	H5::CompType comptype;

    	// open the model files
    	if(rank0) cout << "# Reading the model files..." << endl;
    	string model_filename   = lua->scalar<string>("model_file"  );
    	H5::H5File file(model_filename, H5F_ACC_RDONLY);

    	//=========================//
    	// read in the actual data //
    	//=========================//

    	// check that everything makes sense with one of the datasets
    	const int dataset_rank = 2;
    	hsize_t dims[dataset_rank];
    	dataset = file.openDataSet("/rho");
    	space = dataset.getSpace();
    	space.getSimpleExtentDims(dims);
    	const int nr = dims[0];
    	const int nz = dims[1];
    	PRINT_ASSERT(dataset.getTypeClass(),==,H5T_FLOAT);
    	PRINT_ASSERT(space.getSimpleExtentNdims(),==,dataset_rank);

    	// read the data
    	float    Ye[nr][nz]; //
    	float   eps[nr][nz]; //
    	float press[nr][nz]; //
    	float   rho[nr][nz]; //
    	float  temp[nr][nz]; //
    	float  vphi[nr][nz]; //
    	float vrcyl[nr][nz]; //
    	float    vz[nr][nz]; //
    	float  rcyl[nr][nz]; //
    	float  zcyl[nr][nz]; //
    	dataset = file.openDataSet("/Ye");
    	dataset.read(&(Ye[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/eps");
    	dataset.read(&(eps[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/press");
    	dataset.read(&(press[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/rho");
    	dataset.read(&(rho[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/temp");
    	dataset.read(&(temp[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/vel^phi");
    	dataset.read(&(vphi[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/vel^rho");
    	dataset.read(&(vrcyl[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/vel^z");
    	dataset.read(&(vz[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/x");
    	dataset.read(&(rcyl[0][0]),H5::PredType::IEEE_F32LE);
    	dataset = file.openDataSet("/z");
    	dataset.read(&(zcyl[0][0]),H5::PredType::IEEE_F32LE);
    	dataset.close();
    	file.close();

	// adjust units
	for(int j=0; j<nz; j++)
	  for(int i=0; i<nr; i++){
	    rcyl[i][j] *= 147690.2071535873; // cm
	    zcyl[i][j] *= 147690.2071535873; // cm
	    eps[i][j] *= 8.987551787368177e+20; // erg/g
	    press[i][j] *= 5.548858138674317e+38; // dyn/cm^2
	    rho[i][j] *= 6.173937319029555e+17; // g/cm^3
	    temp[i][j] /= pc::k_MeV; // K
	    vphi[i][j] *= pc::c; // cm/s
	    vrcyl[i][j] *= pc::c; // cm/s
	    vz[i][j] *= pc::c; // cm/s
	  }

    	//=========================//
    	// read in the coordinates // 
    	//=========================//

    	// read x (r) coordinates
    	rcyl_out.resize(nr);
	rcyl_out.min = rcyl[0][0];
    	for(int i=0; i<nr; i++){
 	        if(i==nr-1) rcyl_out[i] = rcyl_out[i-1] + (rcyl_out[i-1] - rcyl_out[i-2]);
		else        rcyl_out[i] = 0.5 * (rcyl[i][0] + rcyl[i+1][0]);
		PRINT_ASSERT(rcyl_out[i],>,(i==0 ? rcyl_out.min : rcyl_out[i-1]) );
    	}

    	// read z coordinates
    	zcyl_out.resize(nz);
	zcyl_out.min = zcyl[0][0];
    	for(int i=0; i<nz; i++){
 	        if(i==nz-1) zcyl_out[i] = zcyl_out[i-1] + (zcyl_out[i-1] - zcyl_out[i-2]);
		else        zcyl_out[i] = 0.5 * (zcyl[0][i] + zcyl[0][i+1]);
		PRINT_ASSERT(zcyl_out[i],>,(i==0 ? zcyl_out.min : zcyl_out[i-1]) );
    	}

    	//===============//
    	// fill the grid //
    	//===============//
	const int n_zones = nr*nz;
	z.resize(n_zones);

    	double newtonian_eint_total = 0;
    	double newtonian_mass = 0;
    	const double gamma_max = 2.0;
    	const double speed_max = pc::c * sqrt(1.0 - 1.0/gamma_max);
    #pragma omp parallel for collapse(2) reduction(+:newtonian_eint_total,newtonian_mass)
	for(unsigned j=0; j<dims[1]; j++)
	  for(unsigned i=0; i<dims[0]; i++){

	    // indices. moving by one proc in the x direction increases proc by 1
	    const int z_ind = zone_index(i,j);
	    PRINT_ASSERT(z_ind,<,n_zones);
	    
	    // zone position
	    double r[2];
	    zone_coordinates(z_ind,r,2);
	    
	    // zone values
	    z[z_ind].rho =   rho[i][j];
	    z[z_ind].T   =  temp[i][j];
	    z[z_ind].Ye  =    Ye[i][j];
	    double vr_tmp   = vrcyl[i][j];
	    double vphi_tmp =  vphi[i][j];
	    double vz_tmp   =    vz[i][j];
	    double speed = sqrt(vr_tmp*vr_tmp + vphi_tmp*vphi_tmp + vz_tmp*vz_tmp);
	    if(speed > speed_max){
	      vr_tmp   *= speed_max / speed;
	      vphi_tmp *= speed_max / speed;
	      vz_tmp   *= speed_max / speed;
	      if(rank0) cout << "WARNING: velocity of superluminal cell at {r,z}={" << rcyl_out[i] << "," << zcyl_out[j] << "} set to gamma=" << gamma_max << endl;
	    }
	    z[z_ind].u[0] = vr_tmp;
	    z[z_ind].u[1] = vz_tmp;
	    z[z_ind].u[2] = vphi_tmp;
	    PRINT_ASSERT(z[z_ind].rho,>=,0.0);
	    PRINT_ASSERT(z[z_ind].T,>=,0.0);
	    PRINT_ASSERT(z[z_ind].Ye,>=,0.0);
	    PRINT_ASSERT(z[z_ind].Ye,<=,1.0);
	    newtonian_eint_total += eps[i][j] * z[z_ind].rho * zone_lab_volume(z_ind);
	    newtonian_mass += z[z_ind].rho * zone_lab_volume(z_ind);
	  }
    	if(rank0){
    		cout << "#   Newtonian total internal energy: " << newtonian_eint_total << " erg" << endl;
    		cout << "#   Newtonian total mass: "            << newtonian_mass       << " g" << endl;		
    	}


	exit(1);
}

//------------------------------------------------------------
// Return the zone index containing the position x
//------------------------------------------------------------
int Grid2DCylinder::zone_index(const double x[3], const int xsize) const
{
	PRINT_ASSERT(xsize,==,3);
	double rcyl = sqrt(x[0]*x[0] + x[1]*x[1]);
	double zcyl = x[2];
	PRINT_ASSERT(rcyl,>=,0);

	// check if off the boundaries
	if(rcyl <  rcyl_out.min               ) return -1;
	if(rcyl >= rcyl_out[rcyl_out.size()-1]) return -2;
	if(zcyl <  zcyl_out.min               ) return -2;
	if(zcyl >= zcyl_out[zcyl_out.size()-1]) return -2;

	// find in zone array using stl algorithm upper_bound and subtracting iterators
	const int i = rcyl_out.locate(rcyl);
	const int j = zcyl_out.locate(zcyl);
	const int z_ind = zone_index(i,j);
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)z.size());
	return z_ind;
}

//----------------------------------------------------------------
// Return the zone index corresponding to the directional indices
//----------------------------------------------------------------
int Grid2DCylinder::zone_index(const int i, const int j) const
{
	PRINT_ASSERT(i,>=,0);
	PRINT_ASSERT(j,>=,0);
	PRINT_ASSERT(i,<,(int)rcyl_out.size());
	PRINT_ASSERT(j,<,(int)zcyl_out.size());
	const int z_ind = i*zcyl_out.size() + j;
	PRINT_ASSERT(z_ind,<,(int)z.size());
	return z_ind;
}


//------------------------------------------------------------
// return volume of zone
//------------------------------------------------------------
double Grid2DCylinder::zone_lab_volume(const int z_ind) const
{
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)z.size());
	int dir_ind[2];
	zone_directional_indices(z_ind,dir_ind,2);
	const unsigned i = dir_ind[0];
	const unsigned j = dir_ind[1];
	const double rcyl0 = rcyl_out.bottom(i);
	const double zcyl0 = zcyl_out.bottom(j);
	const double rcyl1 = rcyl_out[i];
	const double zcyl1 = zcyl_out[j];
	const double vol = pc::pi * (rcyl1*rcyl1 - rcyl0*rcyl0) * (zcyl1 - zcyl0);
	PRINT_ASSERT(vol,>=,0);
	return vol;
}


//------------------------------------------------------------
// return length of zone
//------------------------------------------------------------
double Grid2DCylinder::zone_min_length(const int z_ind) const
{
	int dir_ind[2];
	zone_directional_indices(z_ind,dir_ind,2);
	const unsigned i = dir_ind[0];
	const unsigned j = dir_ind[1];

	// the 'minimum lengts' are just approximate.
	const double r_len = (rcyl_out[i] - rcyl_out.bottom(i));
	const double z_len = (zcyl_out[j] - zcyl_out.bottom(j));
	return min(r_len, z_len);

}

//------------------------------------------------------------
// Return the cell-center spherical coordinates of the cell
//------------------------------------------------------------
void Grid2DCylinder::zone_coordinates(const int z_ind, double r[2], const int rsize) const
{
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)(rcyl_out.size()*zcyl_out.size()));
	PRINT_ASSERT(rsize,==,2);

	int dir_ind[2];
	zone_directional_indices(z_ind, dir_ind,2);
	const unsigned i = dir_ind[0];
	const unsigned j = dir_ind[1];

	const double rcyl0 = rcyl_out.bottom(i);
	const double zcyl0 = zcyl_out.bottom(j);
	r[0] = 0.5 * (rcyl0 + rcyl_out[i]);
	r[1] = 0.5 * (zcyl0 + zcyl_out[j]);
}


//-------------------------------------------
// get directional indices from zone index
//-------------------------------------------
void Grid2DCylinder::zone_directional_indices(const int z_ind, int dir_ind[2], const int size) const
{
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)z.size());
	PRINT_ASSERT(size,==,2);
	dir_ind[0] = z_ind / zcyl_out.size(); // rcyl index
	dir_ind[1] = z_ind % zcyl_out.size(); // zcyl index
	PRINT_ASSERT(dir_ind[0],>=,0);
	PRINT_ASSERT(dir_ind[1],>=,0);
	PRINT_ASSERT(dir_ind[0],<,(int)rcyl_out.size());
	PRINT_ASSERT(dir_ind[1],<,(int)zcyl_out.size());
}


//------------------------------------------------------------
// sample a random cartesian position within the spherical shell
//------------------------------------------------------------
void Grid2DCylinder::sample_in_zone(const int z_ind, const double rand[3], const int randsize, double x[3], const int xsize) const
{
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)z.size());
	PRINT_ASSERT(randsize,==,3);
	PRINT_ASSERT(xsize,==,3);

	// radius and theta indices
	int dir_ind[3];
	zone_directional_indices(z_ind,dir_ind,2);
	int i = dir_ind[0];
	int j = dir_ind[1];

	// inner and outer coordinates of shell
	double rcyl0 = rcyl_out.bottom(i);
	double zcyl0 = zcyl_out.bottom(j);
	double rcyl1 = rcyl_out[i];
	double zcyl1 = zcyl_out[j];

	// sample radial position in shell using a probability integral transform
	double radius = pow( rand[0]*(rcyl1*rcyl1 - rcyl0*rcyl0) + rcyl0*rcyl0, 1./2.);
	PRINT_ASSERT(radius,>=,rcyl0*(1.-TINY));
	PRINT_ASSERT(radius,<=,rcyl1*(1.+TINY));
	radius = max(rcyl0,radius);
	radius = min(rcyl1,radius);

	// sample cos(theta) uniformily
	double zcyl = zcyl0 + (zcyl1-zcyl0)*rand[1];
	zcyl = max(zcyl0, zcyl);
	zcyl = min(zcyl1, zcyl);

	// sample phi uniformily
	double phi = 2.0*pc::pi*rand[2];

	// set the real 3-d coordinates. remember, z is along the symmetry axis
	x[0] = radius*cos(phi);
	x[1] = radius*sin(phi);
	x[2] = zcyl;
}


//------------------------------------------------------------
// get the cartesian velocity vector (cm/s)
//------------------------------------------------------------
void Grid2DCylinder::interpolate_fluid_velocity(const double x[3], const int xsize, double v[3], const int vsize, int z_ind) const
{
	PRINT_ASSERT(xsize,==,3);
	PRINT_ASSERT(vsize,==,3);
	if(z_ind < 0) z_ind = zone_index(x,xsize);
	PRINT_ASSERT(z_ind,>=,-1);

	// if within inner sphere, z_ind=-1. Leave velocity at 0.
	if(z_ind >= 0){

		// radius in zone
		double r    = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
		double rcyl = sqrt(x[0]*x[0] + x[1]*x[1]);
		int along_axis = (rcyl/r < TINY);

		// Based on position, calculate what the 3-velocity is
		double vrcyl = z[z_ind].u[0];
		double vphi  = z[z_ind].u[1];
		double vz    = z[z_ind].u[2];

		double vrcyl_cart[3];
		vrcyl_cart[0] = vrcyl * x[0]/rcyl;
		vrcyl_cart[1] = vrcyl * x[1]/rcyl;
		vrcyl_cart[2] = 0;

		double vphi_cart[3];
		vphi_cart[0] = (along_axis ? 0 : -vphi * x[1]/rcyl );
		vphi_cart[1] = (along_axis ? 0 :  vphi * x[0]/rcyl );
		vphi_cart[2] = 0;

		double vz_cart[3];
		vz_cart[0] = 0;
		vz_cart[1] = 0;
		vz_cart[2] = vz;

		// remember, symmetry axis is along the z-axis
		for(int i=0; i<3; i++) v[i] = vrcyl_cart[i] + vz_cart[i] + vphi_cart[i];
	}
}


//------------------------------------------------------------
// Write the grid information out to a file
//------------------------------------------------------------
void Grid2DCylinder::write_rays(int iw) const
{
	PRINT_ASSERT(iw,>=,0);
	ofstream outf;
	unsigned i=0,j=0;
	double r[3];
	string filename = "";

	// along equator
	filename = Transport::filename("ray_z.5",iw,".dat");
	outf.open(filename.c_str());
	write_header(outf);
	j = zcyl_out.size()/2;
	for(i=0; i<rcyl_out.size(); i++){
		int z_ind = zone_index(i,j);
		zone_coordinates(z_ind,r,2);
		write_line(outf,z_ind);
	}
	outf.close();

	// along z
	filename = Transport::filename("ray_r.5",iw,".dat");
	outf.open(filename.c_str());
	write_header(outf);
	i = rcyl_out.size()/2;
	for(j=0; j<zcyl_out.size(); j++){
		int z_ind = zone_index(i,j);
		zone_coordinates(z_ind,r,2);
		write_line(outf,z_ind);
	}
	outf.close();
}


//------------------------------------------------------------
// Reflect off the outer boundary
//------------------------------------------------------------
void Grid2DCylinder::symmetry_boundaries(LorentzHelper *lh) const{
// does nothing - not implemented
}

//------------------------------------------------------------
// Find distance to outer boundary
//------------------------------------------------------------
double Grid2DCylinder::lab_dist_to_boundary(const LorentzHelper *lh) const{
	const Particle* p = lh->particle_readonly(lab);
	double Dlab[3];
	lh->p_D(lab,Dlab,3);

	// calculate some geometric quantities
	double rcyl = sqrt(p->xup[0]*p->xup[0] + p->xup[1]*p->xup[1]);
	double xcyl_dot_dcyl = p->xup[0]*Dlab[0] + p->xup[1]*Dlab[1];
	double mucyl = ( rcyl==0 ? 0 : xcyl_dot_dcyl/rcyl );

	// Phi   = Pi - Theta (angle on the triangle) (0 if outgoing)
	double Rout  = rcyl_out[rcyl_out.size()-1];
	double Rin   = rcyl_out.min;
	double costheta = sqrt(Dlab[0]*Dlab[0] + Dlab[1]*Dlab[1]);
	double d_outer_boundary = numeric_limits<double>::infinity();
	double d_inner_boundary = numeric_limits<double>::infinity();
	PRINT_ASSERT(rcyl,<,Rout);
	PRINT_ASSERT(zone_index(p->xup,3),>=,-1);

	// distance to inner boundary
	double radical = rcyl*rcyl*(mucyl*mucyl-1.0) + Rin*Rin;
	if(rcyl >= Rin){
		if(Rin>0 && mucyl<0 && radical>=0){
			double dcyl = -rcyl*mucyl - sqrt(radical);
			PRINT_ASSERT(dcyl,<=,sqrt(Rout*Rout-Rin*Rin)*(1.0+TINY));
			d_inner_boundary = dcyl / costheta;
		}
	}
	else{
	        double dcyl = -rcyl*mucyl + sqrt(radical);
		PRINT_ASSERT(dcyl,<=,2.*Rin);
		d_inner_boundary = dcyl / costheta;
	}
	if(d_inner_boundary<0 && fabs(d_inner_boundary/Rin)<TINY*(rcyl_out[0]-Rin)) d_inner_boundary = TINY*(rcyl_out[0]-Rin);
	PRINT_ASSERT(d_inner_boundary,>=,0);

	// distance to outer boundary
	double dcyl = -rcyl*mucyl + sqrt(rcyl*rcyl*(mucyl*mucyl-1.0) + Rout*Rout);
	if(dcyl<=0 && fabs(dcyl/Rin)<TINY*(Rout-rcyl_out[rcyl_out.size()-2])) dcyl = TINY*(Rout-rcyl_out[rcyl_out.size()-2]);
	PRINT_ASSERT(dcyl,>,0);
	PRINT_ASSERT(dcyl,<=,2.*Rout);
	d_outer_boundary = dcyl / costheta;

	// distances to the z boundaries
	double sintheta = Dlab[2];
	double z_dist = INFINITY;
	if(sintheta>0)      z_dist = (zcyl_out[zcyl_out.size()-1] - p->xup[2]) / sintheta;
	else if(sintheta<0) z_dist = (p->xup[2] - zcyl_out.min               ) / sintheta;
	PRINT_ASSERT(z_dist,>,0);

	// make sure the particle ends up in a reasonable place
	const double r_dist = min(d_inner_boundary, d_outer_boundary);
	return min(r_dist,z_dist);
}


double Grid2DCylinder::zone_radius(const int z_ind) const{
	PRINT_ASSERT(z_ind,>=,0);
	PRINT_ASSERT(z_ind,<,(int)z.size());

	// radius and theta indices
	int dir_ind[2];
	zone_directional_indices(z_ind,dir_ind,2);
	int i = dir_ind[0];
	int j = dir_ind[1];

	double r = rcyl_out[i];
	double zhigh = zcyl_out[j];
	double zlow = ( j==0 ? zcyl_out.min : zcyl_out[j-1] );
	double z = max(zhigh, -zlow);
	return sqrt(r*r + z*z);
}

//-----------------------------
// Dimensions of the grid
//-----------------------------
void Grid2DCylinder::dims(hsize_t dims[2], const int size) const{
	PRINT_ASSERT(size,==,(int)dimensionality());
	dims[0] = rcyl_out.size();
	dims[1] = zcyl_out.size();
}

//----------------------------------------------------
// Write the coordinates of the grid points to the hdf5 file
//----------------------------------------------------
void Grid2DCylinder::write_hdf5_coordinates(H5::H5File file) const
{
	// useful quantities
	H5::DataSet dataset;
	H5::DataSpace dataspace;

	// get dimensions
	hsize_t coord_dims[2];
	dims(coord_dims,2);
	for(unsigned i=0; i<2; i++) coord_dims[i]++; //make room for min value

	// write r coordinates
	float tmp0[coord_dims[0]];
	dataspace = H5::DataSpace(1,&coord_dims[0]);
	dataset = file.createDataSet("grid_rcyl(cm)",H5::PredType::IEEE_F32LE,dataspace);
	tmp0[0] = rcyl_out.min;
	for(unsigned i=1; i<rcyl_out.size()+1; i++) tmp0[i] = rcyl_out[i-1];
	dataset.write(&tmp0[0],H5::PredType::IEEE_F32LE);
	dataset.close();

	// write theta coordinates
	float tmp1[coord_dims[1]];
	dataspace = H5::DataSpace(1,&coord_dims[1]);
	dataset = file.createDataSet("grid_zcyl(cm)",H5::PredType::IEEE_F32LE,dataspace);
	tmp1[0] = zcyl_out.min;
	for(unsigned i=1; i<zcyl_out.size()+1; i++) tmp1[i] = zcyl_out[i-1];
	dataset.write(&tmp1[0],H5::PredType::IEEE_F32LE);
	dataset.close();
}

double Grid2DCylinder::g_down(const double xup[4], const int mu, const int nu) const{
	cout << "g_down not implemented" << endl;
	exit(1);
}
double Grid2DCylinder::connection_coefficient(const double xup[4], const int a, const int mu, const int nu) const{
	cout << "g_down not implemented" << endl;
	exit(1);
}