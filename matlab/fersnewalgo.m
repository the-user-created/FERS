function y=fersnewalgo(x, Fs, doppler, delay, flen)
    xs = size(x, 2);
    y = zeros(1, xs);
    Fd = Fs*doppler;
    for ii = 1:xs
        jj = 1:xs;
        y(ii) = sum(x(jj).*hs(ii/Fd-jj/Fs+delay/Fs, Fs, flen));        
    end;
    
function w = hs(t, Fs, flen)
    t = t * Fs;
    hlen = flen/2;
    w = zeros(size(t));
    w(t >= -hlen & t <= hlen) = sinc(t(t >= -hlen & t <= hlen));
    
    
