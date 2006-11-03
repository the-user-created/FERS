function [pulses times] = loadfersHDF5(name)
    hinfo = hdf5info(name);
    cnt = size(hinfo.GroupHierarchy.Datasets, 2);
    for ii = 1:cnt
        [data attr] = hdf5read(hinfo.GroupHierarchy.Datasets(ii), 'ReadAttributes', true);
        pulses(ii, :) = data;
        [time rate] = attr.Value;
        times(ii) = time;
    end;