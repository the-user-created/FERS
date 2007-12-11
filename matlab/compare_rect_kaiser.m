function p = compare_rect_kaiser(y, Fs, doppler, delay, N)
tic
    r1 = ferswinalgo(y, Fs, doppler, delay, N);
    toc
    tic
    r2 = ferslimitalgo(y, Fs, doppler, delay, N);
    toc
    x = 1:size(y,2);
    figure(1);
    plot(x, y, 'b', x, r1, 'k', x, r2, 'r');
    figure(2);
    plot(x, abs(fft(y)), 'b', x, abs(fft(r1)), 'k', x, abs(fft(r2)), 'r');