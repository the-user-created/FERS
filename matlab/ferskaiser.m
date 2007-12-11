%Compute the value of the kaiser window of length N at sample t (with t not
%necessarily integer
function w = ferskaiser(t, N, beta)
    alpha = N/2;
    w = zeros(size(t));
    w(t >= 0 & t <= N) = besseli(0, beta*sqrt(1-((t(t >= 0 & t <= N)-alpha)/alpha).^2))/besseli(0,beta);
    