% Load an HDF5 file produced by FERS
function [I Q times scales] = loadfersHDF5(name)
    hinfo = hdf5info(name);
    cnt = size(hinfo.GroupHierarchy.Datasets, 2);
    for ii = 1:(cnt/2)    
        
        [Idata attr] = hdf5read(hinfo.GroupHierarchy.Datasets(ii*2-1), 'ReadAttributes', true);
        [Qdata attr] = hdf5read(hinfo.GroupHierarchy.Datasets(ii*2), 'ReadAttributes', true);
        datasize = floor(size(Idata,1));
        if ii == 1
            I = zeros(cnt/2, datasize);
            Q = zeros(cnt/2, datasize);
        end;
        for jj = 1:datasize
           I(ii,jj) = Idata(jj);
           Q(ii,jj) = Qdata(jj);
        end;
        [time rate scale] = attr.Value;
      %  attr.Value;
        times(ii) = time;
        scales(ii) = scale;
    end;
