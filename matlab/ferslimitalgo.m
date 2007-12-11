function y=ferslimitalgo(x, Fs, doppler, delay, flen)
    xs = size(x, 2);
    y = zeros(1, xs);
    Fd = Fs*doppler;
    for ii = 1:xs
        mid = round(ii/doppler + delay);
        hlen = flen/2;
        if (mid <= hlen)
            mid = hlen+1;
        end;
        if (mid+hlen > xs)
            mid = xs-hlen;
        end;        
        jj = (mid-hlen):(mid+hlen);
    
        y(ii) = sum(x(jj).*hs(ii/Fd-jj/Fs+delay/Fs, Fs, flen));        
    end;
    
function w = hs(t, Fs, flen)
    t = t * Fs;
    hlen = floor(flen/2);
    w = zeros(size(t));
    w(t >= -hlen & t <= hlen) = ferskaiser(t(t >= -hlen & t <= hlen)+hlen, flen, 9).*sinc(t(t >= -hlen & t <= hlen));
    
    
