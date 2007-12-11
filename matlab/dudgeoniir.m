%Design an IIR filter using the differential correction method described by Dudgeon
% N: Denominator Order
% M: Numerator Order
% H: Desired Magnitude-Squared response
% ep: initial guess of maximum error
% Hmask: Mask for design of filters with disjoint intervals
% W: Weights for design of filters with weighted errors
function [b a iter_cnt dk] = dudgeoniir(N, M, H, ep, Hmask, W)
  lH = length(H);
  %Number of system constraints. More for better filters at the cost of computation time.
  nC = lH;

  Co = [zeros(1, N+1) zeros(1, M+1) 1]';
  rn = linspace(0, pi, nC);
  dk = ep;
  dkl = ep;

  %Initial guesses of B and A
  B=[1 zeros(1,M)]';
  A = zeros(N+1,1);
  fmin = -1;
  iter_cnt = 0;

while ((fmin < 0) && ((iter_cnt < 2) || (dk <= dkl)))
  iter_cnt = iter_cnt + 1;
  cons = [];
  ctype_str = ''; 
  b = [];
  for ii = 1:nC    
    w = rn(ii);
    if (Hmask(ii) == 0)
      continue;
    end;
    c1 = [-W(ii)];
    c2 = [W(ii)];
    c4 = [1];
    for jj = 1:N
      c1 = [c1, -2*W(ii)*cos(jj*w) ];
      c2 = [c2, 2*W(ii)*cos(jj*w) ];
      c4 = [c4, 2*cos(jj*w) ];
    end;
    c1 = [c1, (dk+W(ii)*H(ii))];
    c2 = [c2, (dk-W(ii)*H(ii))];
    c4 = [c4, 0];
    for jj = 1:M
      c1 = [c1, 2*cos(jj*w)*(dk+W(ii)*H(ii))];
      c2 = [c2, 2*cos(jj*w)*(dk-W(ii)*H(ii))];
      c4 = [c4, 0];
    end;
    c1 = [c1, Bf(B, w)];
    c2 = [c2, Bf(B, w)];
    c4 = [c4, 0];
    cons = [cons; c1; c2;  ];
    b = [b; 0; 0;  ];
    ctype_str = [ctype_str, 'LL'];
  end;
  %Constrain d(0) to 1
  cd0 = [zeros(1, N+1) 1 zeros(1, M+1) ];
  cons = [cons; cd0];
  ctype_str = [ ctype_str, 'S'];
  %Create the goal vector
  b=[b; 1];

  % Minima and maxima for all variables
  min_v = [zeros(1,N+1) 0 zeros(1,M) -Inf]';
  max_v = [ones(1,N+1) 1 ones(1,M) Inf]';
  
  m.msglev = 0;
  m.dual = 1;
  m.round = 1;
  m.lpsolver = 1;
  [xo fmin] = glpk(Co, cons, b, min_v, max_v, ctype_str, '', 1, m);  
  
  %Get Ak and Bk
  Al = A;
  Bl = B;
  A=xo(1:(N+1));
  B=xo((N+2):(N+M+2));  
  
  %Get a new value for dk, the peak error
  [rk wk]= mfreqz(A, B, lH);  
  dkl = dk
  dk = max(abs(H-rk).*Hmask.*W);
end;


%Use the previous values of A and B if they are better
if (fmin >= 0)
  A = Al;
  B = Bl;
  dk = dkl;
end;

% Perform spectral factorization
a=real(specfac_fft(B));
b=real(specfac_fft(A));


function r = Bf(B, w)
  ii = (2:length(B))';
  r = B(1) + sum(2*B(2:end).*cos((ii-1)*w));

function [h w] = mfreqz(c, d, n)
  w = linspace(0, pi, n);
  C = c(1)*ones(1,n);
  for ii = 2:length(c)
    C = C+2*c(ii)*cos((ii-1)*w);
  end;
  D = d(1)*ones(1,n);
  for ii = 2:length(d)
    D = D+2*d(ii)*cos((ii-1)*w);
  end;
  h = C./D;
  plot(w, 20*log10(C), 'r', w, 20*log10(D), 'b', w, 20*log10(h), 'g');