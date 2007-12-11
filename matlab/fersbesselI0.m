%Polynomial approximation to zeroth order modified Bessel
% from "Numerical Recipes in C" and "Handbook of Mathematical Functions" by
% Abramowitz and Stegun
function y = fersbesselI0(x)
y = zeros(size(x));
for ii = 1:size(x,2)
    ax = abs(x(ii));
    if ax < 3.75
        p = (ax/3.75)^2;
        y(ii) = 1.0 + p*3.5156229+p^2*3.0899424+p^3*1.2067492+p^4*0.2659732+p^5*0.0360768+p^6*0.0045813;
    else
        p=3.75/ax;
        r = 0.39894228+p*0.01328592+p^2*0.00225319-p^3*0.00157565+p^4*0.00916281;
        r = r - p^5*0.02057706 + p^6*0.02635537 - p^7*0.01647633 + p^8*0.00392377;
        y(ii) = r*exp(ax)/sqrt(ax);
    end;
    %y(ii)-besseli(0,x(ii))
end;
        
      
