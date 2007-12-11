%Calculate SNR based on amount of frequency noise in clock
function [snr stdev] = freqnoisesnr(signal, t, amount, noise)
    phase_error = 0;
    phase_shift = 0;
    stdev = zeros(1, length(signal));
    snr = zeros(1, length(signal));
    noise = noise-mean(noise);
    for ii = 1:length(amount)
        freq_error = noise*amount(ii);
        stdev(ii) = std(freq_error);
        [I Q] = updownmixsim(signal, t, phase_error, freq_error, phase_shift);
        snr(ii) = (fersrms(signal)/fersrms(I-signal))^2;
    end;
        