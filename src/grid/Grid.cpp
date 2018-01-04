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
#include <iostream>
#include <iomanip>
#include <fstream>
#include "Grid.h"
#include "Lua.h"
#include "nulib_interface.h"
#include "global_options.h"
#include "Transport.h"
#include "H5Cpp.h"
#include "Neutrino_GR1D.h"
#include "MomentSpectrumArray.h"
#include "RadialMomentSpectrumArray.h"
#include "GR1DSpectrumArray.h"

using namespace std;
namespace pc = physical_constants;

//------------------------------------------------------------
// initialize the grid
//------------------------------------------------------------
void Grid::init(Lua* lua, Transport* insim)
{
	// MPI stuff
	int MPI_myID;
	MPI_Comm_rank( MPI_COMM_WORLD, &MPI_myID );
	bool rank0 = (MPI_myID==0);

	// set the transport pointer
	sim = insim;

	// read the model file or fill in custom model
	do_GR = lua->scalar<int>("do_GR");
	read_model_file(lua);

	// read some parameters
	int do_relativity = lua->scalar<int>("do_relativity");
	int write_zones_every = lua->scalar<int>("write_zones_every");
	do_annihilation = lua->scalar<int>("do_annihilation");

	// complain if the grid is obviously not right
	if(rho.size()==0){
		cout << "Error: there are no grid zones." << endl;
		exit(5);
	}

	// calculate integrated quantities to check
	double total_rest_mass = 0.0;
	double total_KE        = 0.0;
	double total_TE        = 0.0;
	double total_hvis      = 0.0;
	double Tbar            = 0.0;
	double Yebar           = 0.0;
	int do_visc = lua->scalar<int>("do_visc");
    #pragma omp parallel for reduction(+:total_rest_mass,total_KE,total_TE,total_hvis,Tbar,Yebar)
	for(unsigned z_ind=0;z_ind<rho.size();z_ind++){
		// calculate cell rest mass
		double rest_mass   = zone_rest_mass(z_ind);

		// sanity checks
		PRINT_ASSERT(rho[z_ind],>=,0.0);
		PRINT_ASSERT(zone_com_3volume(z_ind),>=,0.0);
		PRINT_ASSERT(rest_mass,>=,0);

		// calculating totals and averages
		total_rest_mass += rest_mass;
		Tbar            += T[z_ind] * rest_mass;
		Yebar           += Ye[z_ind] * rest_mass;
		total_KE        += (zone_lorentz_factor(z_ind) - 1.0) * rest_mass * pc::c*pc::c;
		total_TE        += rest_mass   / pc::m_n * pc::k * T[z_ind];
		if(do_visc) total_hvis += H_vis[z_ind] * rho[z_ind] * zone_com_3volume(z_ind);
	}

	// write out useful info about the grid
	if (rank0){
		cout << "#   mass = " << total_rest_mass/pc::M_sun << " Msun" << endl;
		cout << "#   <T> = " << Tbar/total_rest_mass*pc::k_MeV << " MeV" << endl;
		cout << "#   <Ye> = " << Yebar/total_rest_mass << endl;
		cout << "#   KE = " << total_KE << " erg" << endl;
		cout << "#   TE = " << total_TE << " erg" << endl;
		if(do_visc) cout << "#   hvis(nonrel) = " << total_hvis << " erg/s" << endl;
	}

    // get the frequency grid
	string neutrino_type = lua->scalar<string>("neutrino_type");
	if(neutrino_type=="NuLib") nulib_get_nu_grid(nu_grid_axis);
	else if(neutrino_type=="GR1D") Neutrino_GR1D::set_nu_grid(lua,&nu_grid_axis);
	else{
        double minval = 0;
        double trash, tmp=0;
        vector<double> bintops = vector<double>(0);
    	int n_nu   = lua->scalar<int>("nugrid_n");
    	if(n_nu>0){
    		double nu_start = lua->scalar<double>("nugrid_start");
    		double nu_stop  = lua->scalar<double>("nugrid_stop");
    		PRINT_ASSERT(nu_stop,>,nu_start);
    		nu_grid_axis = Axis(nu_start/pc::h_MeV,nu_stop/pc::h_MeV,n_nu);
    	}
    	else{
    		string nugrid_filename = lua->scalar<string>("nugrid_filename");
    		ifstream nugrid_file;
    		nugrid_file.open(nugrid_filename.c_str());
    		nugrid_file >> trash >> minval;
    		minval /= pc::h_MeV;
    		vector<double> binmid;
    		while(nugrid_file >> trash >> tmp){
    			tmp /= pc::h_MeV;
    			double last = bintops.size()>0 ? bintops[bintops.size()-1] : minval;
    			PRINT_ASSERT(tmp,>,last);
    			bintops.push_back(tmp);
    			binmid.push_back(0.5 * (last + tmp));
    		}
    		nugrid_file.close();
            nu_grid_axis = Axis(minval,bintops,binmid);
    	}
	}
	PRINT_ASSERT(nu_grid_axis.size(),>,0);

    //==================================//
    // set up the spectrum in each zone //
    //==================================//
    if(rank0) cout << "#   Setting up the distribution function...";
    string distribution_type = lua->scalar<string>("distribution_type");

    double minval = 0;
    double trash, tmp=0;
    vector<double> bintops = vector<double>(0);
    distribution.resize(insim->species_list.size());
    vector<Axis> spatial_axes;
    axis_vector(spatial_axes);

    for(int s=0; s<insim->species_list.size(); s++){

    	//-- POLAR SPECTRUM -----------------
    	if(distribution_type == "Polar"){
    		Axis tmp_mugrid, tmp_phigrid;
     		int n_mu = lua->scalar<int>("distribution_nmu");
    		int n_phi = lua->scalar<int>("distribution_nphi");
    		if(n_mu>0 && n_phi>0){
    			tmp_mugrid = Axis(-1,1,n_mu);
    			tmp_phigrid = Axis(-pc::pi, pc::pi, n_phi);
    		}
    		else{
    			// mu ---------------
    			string mugrid_filename = lua->scalar<string>("distribution_mugrid_filename");
    			ifstream mugrid_file;
    			mugrid_file.open(mugrid_filename.c_str());
    			mugrid_file >> trash >> minval;
    			bintops = vector<double>(0);
    			vector<double> binmid;
    			while(mugrid_file >> trash >> tmp){
    				if(bintops.size()>0) PRINT_ASSERT(tmp,>,bintops[bintops.size()-1]);
    				else PRINT_ASSERT(tmp,>,minval);
    				bintops.push_back(tmp);
    				double last = bintops.size()==1 ? minval : bintops[bintops.size()-2];
    				binmid.push_back(0.5 * (last + bintops[bintops.size()-1]));
    			}
    			bintops[bintops.size()-1] = 1.0;
    			PRINT_ASSERT(minval,==,-1);
    			tmp_mugrid = Axis(minval, bintops, binmid);

    			// phi --------------
    			string phigrid_filename = lua->scalar<string>("distribution_phigrid_filename");
    			ifstream phigrid_file;
    			phigrid_file.open(phigrid_filename.c_str());
    			phigrid_file >> trash >> minval;
    			minval -= pc::pi;
    			bintops = vector<double>(0);
    			binmid = vector<double>(0);
    			while(phigrid_file >> trash >> tmp){
    				tmp -= pc::pi;
    				if(bintops.size()>0) PRINT_ASSERT(tmp,>,bintops[bintops.size()-1]);
    				else PRINT_ASSERT(tmp,>,minval);
    				bintops.push_back(tmp);
    				double last = bintops.size()==1 ? minval : bintops[bintops.size()-2];
    				binmid.push_back(0.5 * (last + bintops[bintops.size()-1]));
    			}
    			bintops[bintops.size()-1] = pc::pi;
    			PRINT_ASSERT(minval,==,-pc::pi);
    			tmp_phigrid = Axis(minval, bintops, binmid);
    		}
    		distribution[s] = new PolarSpectrumArray<NDIMS>;
    		((PolarSpectrumArray<NDIMS>*)distribution[s])->init(spatial_axes,nu_grid_axis, tmp_mugrid, tmp_phigrid);
    	}

    	//-- MOMENT SPECTRUM --------------------
    	else if(distribution_type == "Moments"){
    		distribution[s] = new MomentSpectrumArray<NDIMS>;
    		((MomentSpectrumArray<NDIMS>*)distribution[s])->init(spatial_axes,nu_grid_axis);
    	}

    	//-- RADIAL MOMENT SPECTRUM --------------------
    	else if(distribution_type == "RadialMoments"){
    		int order = lua->scalar<int>("distribution_moment_order");
    		distribution[s] = new RadialMomentSpectrumArray<NDIMS>;
    		((RadialMomentSpectrumArray<NDIMS>*)distribution[s])->init(spatial_axes,nu_grid_axis, order);
    	}

    	//-- RADIAL MOMENT SPECTRUM --------------------
    	else if(distribution_type == "GR1D"){
    		distribution[s] = new GR1DSpectrumArray;
    		((GR1DSpectrumArray*)distribution[s])->init(spatial_axes,nu_grid_axis);
    	}

    	//-- CATCH ------------------
    	else{
    		cout << "Distribution type not found." << endl;
    		assert(0);
    	}
    	int distribution_polar_basis = lua->scalar<int>("distribution_polar_basis");
    	distribution[s]->rotated_basis = distribution_polar_basis;

    }

    if(rank0) cout << "finished." << endl;

	// set up the data structures
	abs_opac.resize(sim->species_list.size());
	scat_opac.resize(sim->species_list.size());
	BB.resize(sim->species_list.size());
	scattering_delta.resize(sim->species_list.size());
	scattering_phi0.resize(sim->species_list.size());
	spectrum.resize(sim->species_list.size());
	vector<Axis> axes;
	axis_vector(axes);
	if(do_annihilation) Q_annihil.set_axes(axes);
	fourforce_abs.set_axes(axes);
	fourforce_emit.set_axes(axes);
	l_abs.set_axes(axes);
	l_emit.set_axes(axes);

	axes.push_back(nu_grid_axis);
	for(int s=0; s<sim->species_list.size(); s++){
		BB[s].set_axes(axes);
		abs_opac[s].set_axes(axes);
		scat_opac[s].set_axes(axes);

	    //===========================//
		// intialize output spectrum // only if child didn't
		//===========================//
		Axis tmp_mugrid, tmp_phigrid;
		if(spectrum[s].size()==0){
			int nmu  = lua->scalar<int>("spec_n_mu");
			int nphi = lua->scalar<int>("spec_n_phi");
			tmp_mugrid = Axis(-1,1,nmu);
			tmp_phigrid = Axis(-pc::pi,pc::pi, nphi);
			vector<Axis> dummy_spatial_axes(0);
			spectrum[s].init(dummy_spatial_axes, nu_grid_axis, tmp_mugrid, tmp_phigrid);
		}
		PRINT_ASSERT(spectrum[s].size(),>,0);
	}

	if(sim->use_scattering_kernels==1){
		axes.push_back(nu_grid_axis);
		for(int s=0; s<sim->species_list.size(); s++){
			scattering_delta[s].set_axes(axes);
			scattering_phi0[s].set_axes(axes);
		}
	}
}

void Grid::write_zones(const int iw) const
{
	PRINT_ASSERT(rho.size(),>,0);

	// output all zone data in hdf5 format
	PRINT_ASSERT(dimensionality(),>,0);
	string filename = Transport::filename("fluid",iw,".h5");
	H5::H5File file(filename, H5F_ACC_TRUNC);

	// write coordinates to the hdf5 file (implemented in each grid type)
	distribution[0]->write_hdf5_coordinates(file, "distribution");
	write_hdf5_coordinates(file);
	write_hdf5_data(file);

	nu_grid_axis.write_HDF5("nu_grid(Hz)",file);
	spectrum[0].write_hdf5_coordinates(file,"spectrum");
}

// returning 0 causes the min distance to take over in propagate.cpp::which_event
double Grid::zone_cell_dist(const double x_up[3], const int z_ind) const{
	return 0;
}

double Grid::zone_rest_mass(const int z_ind) const{
	return rho[z_ind] * zone_com_3volume(z_ind);
}

double Grid::zone_com_3volume(const int z_ind) const{
	// assumes v is orthonormal in cm/s
	if(z_ind<0) return 0;
	else return zone_lab_3volume(z_ind) * zone_lorentz_factor(z_ind);
}

void Grid::write_hdf5_data(H5::H5File file) const{
	// useful quantities
	H5::DataSet dataset;
	H5::DataSpace dataspace;
	vector<float> tmp(rho.size(),0.0);

	// SET UP SCALAR DATASPACE
	hsize_t zdims[dimensionality()];
	dims(zdims,dimensionality());
	dataspace = H5::DataSpace(dimensionality(),&zdims[0]);

	// write comoving volume, assumes last index varies fastest.
	dataset = file.createDataSet("comoving_volume(ccm)",H5::PredType::IEEE_F32LE,dataspace);
	for(unsigned z_ind=0; z_ind<rho.size(); z_ind++) tmp[z_ind] = zone_com_3volume(z_ind);
	dataset.write(&tmp[0],H5::PredType::IEEE_F32LE);
	dataset.close();

	// write fluid quantities
	rho.write_HDF5(file,"rho(g|ccm,com)");
	T.write_HDF5(file,"T_gas(MeV,com)");
	Ye.write_HDF5(file,"Ye");
	H_vis.write_HDF5(file,"H_vis(erg|s|g,com)");
	fourforce_abs.write_HDF5(file,"four-force[abs](erg|s|g,tetrad)");
	fourforce_emit.write_HDF5(file,"four-force[emit](erg|s|g,tetrad)");
	l_abs.write_HDF5(file,"l_abs(1|s|ccm,lab)");
	l_emit.write_HDF5(file,"l_emit(1|s|ccm,lab)");

	// write annihilation_rate
	if(do_annihilation) Q_annihil.write_HDF5(file,"annihilation_rate(erg|ccm|s,lab)");

	// write distribution function
	vector<unsigned> dir_ind(dimensionality());
	for(unsigned s=0; s<distribution.size(); s++){
		distribution[s]->write_hdf5_data(file, "distribution"+to_string(s));
		spectrum[s].write_hdf5_data(file,"spectrum"+to_string(s));
	}
}

double Grid::total_rest_mass() const{
	double mass = 0;
	#pragma omp parallel for reduction(+:mass)
	for(unsigned z_ind=0; z_ind<rho.size(); z_ind++) mass += zone_rest_mass(z_ind);
	return mass;
}

// radius given coordinates
double Grid::radius(const double x[4]){
	return sqrt(Metric::dot_Minkowski<3>(x,x));
}

void Grid::integrate_geodesic(EinsteinHelper *eh) const{
	const double* kold = eh->p.kup;
	const double* xold = eh->p.xup;

	// get connection coefficients
	double gamma[4][4][4];
	connection_coefficients(xold,gamma);

	double lambda = - eh->ds_com / eh->g.dot<4>(kold, eh->u);

	double dk_dlambda[4] = {0,0,0,0};
	double dx_dlambda[4] = {0,0,0,0};
	for(int a=0; a<4; a++){
		for(int mu=0; mu<4; mu++) for(int nu=0; nu<4; nu++)
			dk_dlambda[a] -= gamma[a][mu][nu] * kold[mu] * kold[nu];
		dx_dlambda[a]  = kold[a];
	}

	// get new x,k
	double knew[4],xnew[4];
	for(int i=0; i<4; i++){
		eh->p.kup[i] += dk_dlambda[i]*lambda;
		eh->p.xup[i] += dx_dlambda[i]*lambda;
	}
}



// isotropic scatter, done in COMOVING frame
void Grid::isotropic_direction(double D[3], ThreadRNG *rangen) const
{
	// Randomly generate new direction isotropically in comoving frame
	double mu  = 1 - 2.0*rangen->uniform();
	double phi = 2.0*pc::pi*rangen->uniform();
	double smu = sqrt(1 - mu*mu);

	D[0] = smu*cos(phi);
	D[1] = smu*sin(phi);
	D[2] = mu;
	Metric::normalize_Minkowski<3>(D);
}

void Grid::isotropic_kup_tet(const double nu, double kup_tet[4], const double xup[4], ThreadRNG *rangen) const{
	double D[3];
	isotropic_direction(D,rangen);

	kup_tet[0] = nu * D[0] * 2.0*pc::pi / pc::c;
	kup_tet[1] = nu * D[1] * 2.0*pc::pi / pc::c;
	kup_tet[2] = nu * D[2] * 2.0*pc::pi / pc::c;
	kup_tet[3] = nu        * 2.0*pc::pi / pc::c;
}


void Grid::random_core_x_D(const double r_core, ThreadRNG *rangen, double x3[3], double D[3]) const{
	PRINT_ASSERT(do_GR,==,false);

	double phi_core   = 2*pc::pi*rangen->uniform();
	double cosp_core  = cos(phi_core);
	double sinp_core  = sin(phi_core);
	double cost_core  = 1 - 2.0*rangen->uniform();
	double sint_core  = sqrt(1-cost_core*cost_core);

	double a_phot = r_core + r_core*1e-10;
	x3[0] = a_phot*sint_core*cosp_core;
	x3[1] = a_phot*sint_core*sinp_core;
	x3[2] = a_phot*cost_core;

	// pick photon propagation direction wtr to local normal
	double phi_loc = 2*pc::pi*rangen->uniform();
	// choose sqrt(R) to get outward, cos(theta) emission
	double cost_loc  = sqrt(rangen->uniform());
	double sint_loc  = sqrt(1 - cost_loc*cost_loc);
	// local direction vector
	double D_xl = sint_loc*cos(phi_loc);
	double D_yl = sint_loc*sin(phi_loc);
	double D_zl = cost_loc;
	// apply rotation matrix to convert D vector into overall frame
	D[0] = cost_core*cosp_core*D_xl-sinp_core*D_yl+sint_core*cosp_core*D_zl;
	D[1] = cost_core*sinp_core*D_xl+cosp_core*D_yl+sint_core*sinp_core*D_zl;
	D[2] = -sint_core*D_xl+cost_core*D_zl;
	Metric::normalize_Minkowski<3>(D);
}


double Grid::zone_4volume(const int z_ind) const{
	return zone_lab_3volume(z_ind) * zone_lapse(z_ind);
}

void Grid::interpolate_metric(const double xup[4], Metric* g, const int z_ind){
	g->alpha = 1.0;
	for(int i=0; i<3; i++) g->betaup[i] = 0;
	g->gammalow.xx = 1.0;
	g->gammalow.yy = 1.0;
	g->gammalow.zz = 1.0;
	g->gammalow.xy = 0.0;
	g->gammalow.xz = 0.0;
	g->gammalow.yz = 0.0;
	g->update();
}



//-----------------------------------------------------------------
// get opacity at the frequency
//-----------------------------------------------------------------
void Grid::get_opacity(EinsteinHelper *eh, const int z_ind) const
{
	PRINT_ASSERT(z_ind,>=,-1);

	// absorption and scattering opacities
	unsigned nu_bin = nu_grid_axis.bin(eh->nu());
	unsigned dir_ind[NDIMS+1];
	sim->grid->rho.indices(z_ind,dir_ind);
	dir_ind[NDIMS] = nu_bin;
	unsigned global_index = abs_opac[eh->p.s].direct_index(dir_ind);

	double a = abs_opac[eh->p.s][global_index];
	double s = scat_opac[eh->p.s][global_index];

	PRINT_ASSERT(a,>=,0);
	PRINT_ASSERT(s,>=,0);
	PRINT_ASSERT(a,<,INFINITY);
	PRINT_ASSERT(s,<,INFINITY);

	eh->absopac  = a;
	eh->scatopac = s;
}
