# python script to check content in the hdf5 file .h5

import h5py

def h5py_dataset_iterator(g, prefix=''):
    for key in g.keys():
        item = g[key]
        path = '{}/{}'.format(prefix, key)
        if isinstance(item, h5py.Dataset): # test for dataset
            yield (path, item)
        elif isinstance(item, h5py.Group): # test for group (go down)
            yield from h5py_dataset_iterator(item, path)

# with h5py.File('/home/kevin/JohannesCode/south-building-demon/south_building_predictions.h5', 'r') as f:
with h5py.File('/home/kevin/JohannesCode/KevinProcessedData_southbuilding/kevin_southbuilding_demon.h5', 'r') as f:
    for (path, dset) in h5py_dataset_iterator(f):
        print(path, dset)
