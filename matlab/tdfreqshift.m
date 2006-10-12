function Y = tdfreqshift(X, phi, Fs)
    sz = size(X,2);
    p = 0:(sz-1);
    x=hilbert(ifft(X));
    
    phi=phi*sz/Fs;
    ephi = (phi-floor(phi))*Fs/sz;

    y = x.*exp(j*2*pi*(p/sz*phi-ephi));   
    
   % y=y.*exp(j*2*pi*p/sz*ephi+j*theta);
    
    
  
    
    Y = fft(real(y));
   %Y=fft(y);
    