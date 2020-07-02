import numpy as np
import oneflow as flow


def test_bernoulli(test_case):
    func_config = flow.FunctionConfig()
    func_config.default_distribute_strategy(flow.distribute.consistent_strategy())
    func_config.default_data_type(flow.float)

    @flow.global_function(func_config)
    def BernoulliJob(a=flow.FixedTensorDef((10, 10))):
        return flow.random.bernoulli(a)

    x = np.ones((10, 10), dtype=np.float32)
    y = BernoulliJob(x).get().ndarray()
    test_case.assertTrue(np.array_equal(y, x))

    x = np.zeros((10, 10), dtype=np.float32)
    y = BernoulliJob(x).get().ndarray()
    test_case.assertTrue(np.array_equal(y, x))

    x = np.ones((10, 10), dtype=np.float32) * 0.5
    y = BernoulliJob(x).get().ndarray()
    test_case.assertTrue(np.allclose(y.sum(), 50.0, rtol=0.1, atol=5.0))


def test_bernoulli_mirrored(test_case):
    func_config = flow.FunctionConfig()
    func_config.default_data_type(flow.float)

    @flow.global_function(func_config)
    def BernoulliJob(a=flow.MirroredTensorDef((10, 10))):
        return flow.random.bernoulli(a)

    x = np.ones((10, 10), dtype=np.float32)
    y = BernoulliJob([x]).get().ndarray_list()[0]
    test_case.assertTrue(np.array_equal(y, x))

    x = np.zeros((10, 10), dtype=np.float32)
    y = BernoulliJob([x]).get().ndarray_list()[0]
    test_case.assertTrue(np.array_equal(y, x))

    x = np.ones((10, 10), dtype=np.float32) * 0.5
    y = BernoulliJob([x]).get().ndarray_list()[0]
    test_case.assertTrue(np.allclose(y.sum(), 50.0, rtol=0.1, atol=5.0))