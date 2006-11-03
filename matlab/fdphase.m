
%Algorithm from Steven G. Johnson's post at
%http://www.dsprelated.com/showmessage/58183/1.php
function Y=fdphase(X,phi)
sz = size(X,2)
p=0:(sz-1);
%Y=X.*exp(-j*2*pi*p/sz*phi);

for ii=1:floor(sz/2)
Y(ii)=X(ii)*exp(-j*(ii-1)/sz*phi*2*pi);
end;

Y(floor(sz/2)+1)=cos(2*pi*phi/2);

for ii=(floor(sz/2)+1):(sz)
    Y(ii)=X(ii)*exp(-j*(ii-sz-1)/sz*2*pi*phi);
end;
    

%Y=X*exp(i*phi);

%plot(real(ifft(X)), 'r'); hold on; plot(real(ifft(Y)), 'b'); hold off;
