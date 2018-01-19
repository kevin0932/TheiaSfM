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


# with h5py.File('/home/kevin/JohannesCode/south-building-demon/fuse_southbuilding_demon.h5', 'r') as f:
# with h5py.File('/home/kevin/DeMoN_Prediction/south_building/south_building_predictions.h5', 'r') as f:
# with h5py.File('/home/kevin/ThesisDATA/gerrard-hall/demon_prediction/gerrard_hall_predictions.h5', 'r') as f:

# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_1.6m_to_infm.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_0.8m_to_1.6m.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_0.4m_to_0.8m.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/SUN3D_Train_mit_ne47_huge_office.ne47_floor2_office_1/demon_prediction/demon_sun3d_train_mit_ne47_huge_office_ne47_floor2_office_1.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_0.2m_to_0.4m.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_0.1m_to_0.2m.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/sun3d_train_0.01m_to_0.1m.h5', 'r') as f:
# with h5py.File('/home/kevin/JohannesCode/KevinProcessedData_southbuilding/kevin_southbuilding_demon.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/sun3d_test.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/rgbd_test.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/nyu2_test.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/mvs_test.h5', 'r') as f:
# with h5py.File('/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/scenes11_test.h5', 'r') as f:

with h5py.File("/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/SUN3D_Train_mit_w85_lounge1.wg_lounge1_1/demon_prediction/images_demon/dense/1/kevin_exhaustive_demon.h5", 'r') as f:
# with h5py.File("/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/SUN3D_Train_hotel_beijing~beijing_hotel_2/demon_prediction/kevin_beijing_hotel_2_exhaustive_demon.h5", 'r') as f:
#with h5py.File("/home/kevin/anaconda_tensorflow_demon_ws/demon/datasets/traindata/SUN3D_Train_hotel_beijing~beijing_hotel_2/demon_prediction/View128ColmapFilter_demon_sun3d_train_hotel_beijing~beijing_hotel_2.h5", 'r') as f:

    # print(f['/IMG_2430.JPG---IMG_2429.JPG/']['scale'].value)
    for (path, dset) in h5py_dataset_iterator(f):
        print(path, dset)
    #     # print(dset['scale'])
    # for h5key in f.keys():
    #     print(h5key)
