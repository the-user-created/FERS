import tables;

#Load a FERS result file in HDF5 format
def load(name):
    nodes = []
    h5f = tables.openFile(name, mode="r")
    for node in h5f:
        nodes.append(node)
    nodes = nodes[1:]
    arrays = []
    for array in nodes:
        arrays.append(array.read());    
    h5f.close();
    return arrays;

