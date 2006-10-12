%Export a vector in the format expected by FERS
function ferscsvwrite(filename, M, Fs)
    M = [ size(M,2) Fs M ];
    csvwrite(filename, M');