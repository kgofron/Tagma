function enable ( chan )

  disa=caget("x6b:det1.CHEN");
  disa(chan)=0;
  caput("x6b:det1.CHEN",disa);

endfunction

