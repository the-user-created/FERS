function Y = freqshift(x, X, phi)
    sz = size(X,2);
    Y = X;
    Y(round(sz/2):sz)=0;
    ps = floor(phi);
    d=phi-ps
    if rem(ps,2) == 0
        YL=circshift(Y, [0 ps]);
        YH=-circshift(Y, [0 ps+1]);
    else
        YL=-circshift(Y, [0 ps]);
        YH=circshift(Y, [0 ps+1]);
    end;
    Y=YL+e^d*YH;
    
    Y=2*fft(real(ifft(Y)));
    
    
    
    plot(x, abs(YL), 'r', x, abs(YH), 'b', x, abs(Y), 'g');
    
