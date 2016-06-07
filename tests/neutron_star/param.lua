-- Included Physics

do_visc       = 0
do_relativity = 0
do_annihilation = 0
radiative_eq  = 1
reflect_outer = 0

-- Equilibrium Solving

equilibrium_T  = 0
equilibrium_Ye = 1
equilibrium_damping = 0.5
equilibrium_itmax = 100
equilibrium_tolerance = 1e-5

-- Opacity and Emissivity

grey_opacity  = -1
grey_abs_frac = -1
nulib_table = "/home/srichers/software/sedonu-devel/external/tables/NuLib/NuLib_noscat.h5"
nulib_eos = "/home/srichers/software/sedonu-devel/external/tables/EOS/SFHo.h5"
opac_interp_method = 0
cdf_interpolation_order = 1
cdf_cutoff    = 0

-- Escape Spectra

spec_n_mu       = 1
spec_n_phi      = 1

-- Distribution Function

distribution_nmu = 2
distribution_nphi = 2

-- Grid and Model

grid_type = "Grid1DSphere"
model_file = "neutron_star.mod"

-- Output

write_zones_every   = 1
write_rays_every    = 1
write_spectra_every = 1
output_zones_distribution = 0
output_hdf5 = 0

-- Particle Creation

max_particles  = 2e7
n_subcycles = 1
do_emit_by_bin = 0
n_emit_core    = 1e5
n_emit_therm   = 0

-- Inner Source

r_core = 9e5
core_emit_method = 1
T_core = 10
core_nue_chem_pot = 0
core_lum_multiplier = 1.0

-- General Controls

verbose       = 1
max_n_iter =  20
step_size = 0.4

-- Biasing

importance_bias = 0
bias_path_length = 0
min_packet_energy = 1e24
max_packet_energy = 1e99
max_path_length_boost = 2
min_importance = 1e-4
