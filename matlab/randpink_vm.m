%Generate approximate pink noise using the voss-mccartney algorithm
function n = randpink_vm(sz)
    rows = randn(1, 32);
    n = zeros(1, sz);
    for ii = 1:sz
        n(ii) = sum(rows)+randn();
        p=ii/2;
        q=1;
        while (floor(p) == p)
            p = p/2;
            q = q+1;
        end;        
        rows(q) = randn();        
    end;