k_b = 1.3806488e-16 # erg/K
c = 2.99e10         #cm/s
h = 6.62606957e-27  #erg/Hz
sigma = 5.67051e-5  #erg/cm^2/K^4/s
pi = 3.14159265359

r = 1                      #cm
T = 3.5e11                 #K
L = 1e43                   #erg/s
P = 4*pi*r*r*sigma*T*T*T*T #erg/s
N = L/P

set xlabel "Neutrino Frequency (Hz) (2.5e20 Hz/MeV)"
set ylabel "Energy Flux (erg/s/Hz)"

#set xrange [0:3e15]
set term pdf
set output "compare_spectrum_0.pdf"
set title "Electron Neutrinos"
plot 2*pi*4*pi*r*r*2*x*x*x*h/c/c*1/(exp(h*x/(k_b*T))+1)/6, './species0_I2.spec' using 1:2
set output

set term pdf
set output "compare_spectrum_1.pdf"
set title "Electron Anti-Neutrinos"
plot 2*pi*4*pi*r*r*2*x*x*x*h/c/c*1/(exp(h*x/(k_b*T))+1)/6, './species1_I2.spec' using 1:2
set output

set term pdf
set output "compare_spectrum_2.pdf"
set title "Mu/Tau Anti/Neutrinos"
plot 2*pi*4*pi*r*r*2*x*x*x*h/c/c*1/(exp(h*x/(k_b*T))+1)*2/3, './species2_I2.spec' using 1:2
set output

# planck function has units of erg/s/cm^2/Hz/ster
# 2*pi assuming isotropic emission from surface
# 4*pi*r*r is the surface area