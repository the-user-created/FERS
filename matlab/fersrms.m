function r = fersrms(x)
    r = sqrt(sum(x.*x)/length(x));
    