#include "oneflow/core/kernel/softmax_kernel.h"
#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/kernel/transpose_kernel.h"
#include "oneflow/core/ndarray/ndarray_util.h"

namespace oneflow {

namespace {

template<DeviceType device_type, typename T>
void SoftmaxComputeDiff(DeviceCtx* ctx, const int64_t n, const int64_t w, const T* out_diff,
                        const T* out, T* sum_vec, T* in_diff, void* temp_storage,
                        const size_t temp_storage_bytes) {
  auto Val = NdarrayUtil<device_type, T>::GetValNdarrayBuilder();
  auto Var = NdarrayUtil<device_type, T>::GetVarNdarrayBuilder();
  // it's safe to use in_diff as tmp
  T* tmp = in_diff;
  // dot product | get dot product sum_vec[i] from out[i] * out_diff[i]
  NdarrayUtil<device_type, T>::Mul(ctx, Var({n, w}, tmp), Val({n, w}, out), Val({n, w}, out_diff));
  NdarrayUtil<device_type, T>::ReduceSum(ctx, Var({n, 1}, sum_vec), Val({n, w}, tmp),
                                         Var({static_cast<int64_t>(temp_storage_bytes / sizeof(T))},
                                             reinterpret_cast<T*>(temp_storage)));
  // copy out_diff to in_diff
  NdarrayUtil<device_type, T>::Assign(ctx, Var({n, w}, in_diff), Val({n, w}, out_diff));
  // sub | in_diff[i][j] -= sum_vec[i]
  NdarrayUtil<device_type, T>::InplaceBroadcastSub(ctx, Var({n, w}, in_diff), Val({n, 1}, sum_vec));
  // elementwise multiplication | in_diff[i][j] *= out[i][j]
  NdarrayUtil<device_type, T>::InplaceMul(ctx, Var({n, w}, in_diff), Val({n, w}, out));
}

}  // namespace

template<DeviceType device_type, typename T>
void SoftmaxComputeProb(DeviceCtx* ctx, const int64_t n, const int64_t w, const T* in, T* tmp,
                        T* prob, void* temp_storage, const size_t temp_storage_bytes) {
  auto Val = NdarrayUtil<device_type, T>::GetValNdarrayBuilder();
  auto Var = NdarrayUtil<device_type, T>::GetVarNdarrayBuilder();
  // copy in blob to prob blob
  NdarrayUtil<device_type, T>::Assign(ctx, Var({n, w}, prob), Val({n, w}, in));
  // max | calculate max of every sample vector prob[i], store in tmp[i]
  //       the prob[i] now is store the data of in[i]
  NdarrayUtil<device_type, T>::ReduceMax(ctx, Var({n, 1}, tmp), Val({n, w}, prob),
                                         Var({static_cast<int64_t>(temp_storage_bytes / sizeof(T))},
                                             reinterpret_cast<T*>(temp_storage)));
  // sub | every element of prob blob subract the max value of the same sample
  NdarrayUtil<device_type, T>::InplaceBroadcastSub(ctx, Var({n, w}, prob), Val({n, 1}, tmp));
  // exp | exponentiation every element
  NdarrayUtil<device_type, T>::InplaceExp(ctx, Var({n, w}, prob));
  // sum | calculate sum of every sample vector prob[i], store in tmp[i]
  //       the prob[i] now is store the tmp data after exp
  NdarrayUtil<device_type, T>::ReduceSum(ctx, Var({n, 1}, tmp), Val({n, w}, prob),
                                         Var({static_cast<int64_t>(temp_storage_bytes / sizeof(T))},
                                             reinterpret_cast<T*>(temp_storage)));
  // div | every element of prob[i] divided by the data of tmp[i] (the sum
  // value)
  NdarrayUtil<device_type, T>::InplaceBroadcastDiv(ctx, Var({n, w}, prob), Val({n, 1}, tmp));
}

template<DeviceType device_type, typename T>
void SoftmaxKernel<device_type, T>::ForwardDataContent(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  const Blob* in_blob = BnInOp2Blob(this->op_attribute().input_bns(0));
  Blob* out_blob = BnInOp2Blob(this->op_attribute().output_bns(0));
  Blob* tmp_blob = BnInOp2Blob("fw_softmax_num");
  Blob* buf_blob = BnInOp2Blob("fw_buf");
  auto conf = this->kernel_conf().softmax_conf();
  const int64_t n = conf.transpose_rows();
  const int64_t w = conf.transpose_cols();
  T* tmp = tmp_blob->mut_dptr<T>();
  if (conf.need_transpose()) {
    Blob* transpose_in_blob = BnInOp2Blob("transpose_in");
    Blob* transpose_out_blob = BnInOp2Blob("transpose_out");
    Transpose<device_type, T>(ctx.device_ctx, in_blob, transpose_in_blob, conf.perm());
    SoftmaxComputeProb<device_type, T>(ctx.device_ctx, n, w, transpose_in_blob->dptr<T>(), tmp,
                                       transpose_out_blob->mut_dptr<T>(), buf_blob->mut_dptr(),
                                       buf_blob->ByteSizeOfDataContentField());
    Transpose<device_type, T>(ctx.device_ctx, transpose_out_blob, out_blob, conf.perm());
  } else {
    SoftmaxComputeProb<device_type, T>(ctx.device_ctx, n, w, in_blob->dptr<T>(), tmp,
                                       out_blob->mut_dptr<T>(), buf_blob->mut_dptr(),
                                       buf_blob->ByteSizeOfDataContentField());
  }
}

REGISTER_KERNEL_HELPER_GPU_FLOATING(OperatorConf::kSoftmaxConf, SoftmaxKernel);
REGISTER_KERNEL_HELPER_GPU_HALF(OperatorConf::kSoftmaxConf, SoftmaxKernel);

}  // namespace oneflow
