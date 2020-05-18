load "scan_fits_0.dat";
load "scan_fits_7.dat";
load "scan_fits_15.dat";
load "scan_fits_23.dat";
load "scan_fits_31.dat";
load "scan_fits_39.dat";
load "scan_fits_47.dat";
load "scan_fits_55.dat";
load "scan_fits_63.dat";


x=[0,7,15,23,31,39,47,55,63];
sx=sum(x);
sx2=sum(x.^2);
n=9;
delta=n*sx2-sx^2;

for k=0:3
 for j=1:384
  a(j,:)=[fit0(j,3+4*k),fit7(j,3+4*k),fit15(j,3+4*k),fit23(j,3+4*k),fit31(j,3+4*k),fit39(j,3+4*k),fit47(j,3+4*k),fit55(j,3+4*k),fit63(j,3+4*k)];
  sy=0;
  sxy=0;
  for i=1:9
    sy=sy+a(j,i);
    sxy=sxy+a(j,i)*x(i);
  endfor
  m(k+1,j)=(n*sxy-sx*sy)/delta;
 endfor
endfor
b=-1./m;
save factors.dat b;
