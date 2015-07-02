load "thermistor10k.dat";
# Load resistor
R1=100000;
R2=rvst(:,2);
V(:,2)=rvst(:,1);
V(:,1)=4095.*R2./(R1.+R2);
save 'ADC_therm10k.dat' V;
