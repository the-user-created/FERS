function [I Q] = updownmixsim(bbtx, t, freq_error, phase_error, phaseshift)
    I = bbtx.*cos(2*pi*t.*freq_error-phase_error-phaseshift);
    Q = -bbtx.*sin(2*pi*t.*freq_error-phase_error-phaseshift);
    