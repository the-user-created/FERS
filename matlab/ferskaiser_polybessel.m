%Compute the value of the kaiser window of length N at sample t (with t not
%necessarily integer
%This one uses the polynomial approximation of the bessel function
function w = ferskaiser_polybessel(t, N, beta)
    alpha = N/2;
    w = zeros(size(t));
    w(t >= 0 & t <= N) = fersbesselI0(beta*sqrt(1-((t(t >= 0 & t <= N)-alpha)/alpha).^2))/fersbesselI0(beta);
    