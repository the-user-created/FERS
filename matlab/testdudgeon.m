% Test the Dudgeon IIR design method

%Design a low pass
w = linspace(0, 1, 256);
alpha = 5
H = (20*w)./(exp(20*w)-1);

Hmask = [ones(1,256) ]; % 30 sample transition band
W = [ ones(1,5) ones(1,251)];


[b a iter dk] = dudgeoniir(17, 17, H, 20, Hmask, W);

dk
iter

[h w] = freqz(b, a, 256);
plot(w, H, 'g', w, abs(h), 'b');
%semilogx(w, 20*log10(H), 'g', w, 20*log10(h), 'b');
grid on;
%plot(w, 20*log10(H), 'g');

