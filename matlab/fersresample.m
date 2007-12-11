%Adjust the sample rate of a signal by factor using bandlimited
%interpolation algorithm
function y = fersresample(x, Fs, factor)
    sz = size(x, 2);
    nsz = floor(sz*factor);
    Fn = Fs*factor;
    for jj = 1:nsz
        y(jj) = 0;
        for ii = 1:sz
            y(jj) = y(jj)+x(ii)*hs(jj/Fn-ii/Fs, Fs);
        end;
    end;
    
    function p = hs(x, Fs)
        p = sinc(Fs*x);
        