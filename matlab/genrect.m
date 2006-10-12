%Generate a rect pulse of the specified length at the specified frequency
function X = genrect(length, sr)
    plen = length*3*sr
    X = 0:(plen-1);
    X = sinc(X/plen*sr*length).*exp(-j*2*pi*X/2)*2/3*plen;
    X(1) = sr*length;
   
    X = fft(real(ifft(X)));
    