%Use a polyphase filter to upsample by the factor N
function y = fers_upsample_p(x, N, filter_size, cutoff_factor)
    %Design the required filter
    b = fir1(filter_size, (1/N)*cutoff_factor);
    %Decompose the filter into a polyphase bank
    b_bank = zeros(N, ceil(filter_size/N));
    for ii = 1:N
        b_branch = b(ii:N:end);
        b_bank(ii, 1:length(b_branch)) = b_branch;
    end;
    %Apply the multirate bank
    y = zeros(1, length(x)*N);
    for ii = 1:N
        y_branch = filter(b_bank(ii, :), 1, x);
        y_branch = upsample(y_branch, N, ii-1);
        y = y + y_branch;
    end;
    