% Compute the allan deviation of the given vector
function ad=fersallan(y,taus)
ad=zeros(1,length(taus));
ii = 1;
for tau = taus
n=length(y);
p=ones(1,tau);
D=filter(p, 1, y)/tau;
D=D(tau:tau:end);
av = 0.5*sum(diff(D).^2)/(length(D)-1);
ad(ii) = sqrt(av);
ii = ii+1;
end
    
    
    