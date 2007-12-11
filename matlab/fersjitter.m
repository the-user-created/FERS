function y = fersjitter(x, fs, jitter)
    l = size(x, 2);
    for ii = 1:l
        sstart = max(1, ii - 16);
        send = min(l, ii+16);
        t = ii/fs+jitter(ii);
        n = sstart:send;                   
        y(ii) = sum(x(n).*interp_func((t-n/fs)*fs, 32));
    end;
    

    
    function s = interp_func(t, sz)
        w = ferskaiser_polybessel(t+sz/2, sz, 11);
        s = sinc(t).*w;
        
    
    
        
           