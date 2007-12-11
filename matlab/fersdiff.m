%Calculate the first difference of a vector
function d = fersdiff(x)
    d = x(1:length(x)-1)-x(2:length(x));
    