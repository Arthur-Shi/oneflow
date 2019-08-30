#include "oneflow/core/kernel/tanh_grad_kernel.h"
#include "oneflow/core/kernel/new_kernel_util.h"

namespace oneflow {

template<DeviceType device_type, typename T>
void TanHGradKernel<device_type, T>::ForwardDataContent(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  const Blob* y_blob = BnInOp2Blob("y");
  NewKernelUtil<device_type>::TanHBackward(
      ctx.device_ctx, y_blob->shape().elem_cnt(), y_blob->dptr<T>(), y_blob->dptr<T>(),
      BnInOp2Blob("dy")->dptr<T>(), BnInOp2Blob("dx")->mut_dptr<T>());
}

REGISTER_KERNEL_HELPER_GPU_FLOATING(OperatorConf::kTanhGradConf, TanHGradKernel);
REGISTER_KERNEL_HELPER_GPU_HALF(OperatorConf::kTanhGradConf, TanHGradKernel);

}  // namespace oneflow
