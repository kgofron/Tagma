of=fopen("bptTherm10kdegC.dbd","w");

fprintf(of,"breaktable(Therm10kdegC) {\n");

load "ADC_therm10k.dat";
s=size(V);

for i=s(1):-5:1;
  fprintf(of,"\t%g %g\n",V(i,1),V(i,2));
endfor;

fprintf(of,"}\n");

fclose(of);

