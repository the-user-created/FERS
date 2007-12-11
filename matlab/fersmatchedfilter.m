function outdata = fersmatchedfilter(data, refsig)
%Matched filter the given data (column wise) with the reference signal
ft = zeros(size(data));
Fdata = fft(data, size(data,2), 2);
Fref = conj(fft(refsig, size(data,2), 2));
Ffilt = repmat(Fref, size(data, 1), 1);
size(Ffilt)
size(data)
ft = Fdata.*Ffilt;
size(ft)
outdata = ifft(ft, size(data, 2), 2);

%function odata = fersmatchedfilter(data, pulse)
% Fdata = fft(data, size(data, 2), 2);
% Fpulse = conj(fft(pulse, size(data, 2), 2));
% for ii = 1:size(data,1)
%   Fdata(ii, :) = Fdata(ii, :).*Fpulse;
% end;
% odata = ifft(Fdata);