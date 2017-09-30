function[B] = householder(A)
    [m, n] = size(A);
    bvec   = zeros(n,1);
    for j = 1:n-1
        [u, b] = house(A(j:m, j));
        b
        u
        %bvec(j,1) = b;
        A(j:m, j:n) = (eye(m-j+1)-b*u*transpose(u))*A(j:m,j:n);
        A(j:m, j:n)
        %if j < m
        %    A(j+1:m, j) = u(2:m-j+1);
        %end
    end
    B = A;
    % Q = getq(A, bvec)
