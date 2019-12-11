#include "oneflow/core/thread/gpu_thread.h"
#include "oneflow/core/device/cuda_stream_handle.h"

namespace oneflow {

#ifdef WITH_CUDA

GpuThread::GpuThread(int64_t thrd_id, int64_t dev_id) {
  set_thrd_id(thrd_id);
  mut_actor_thread() = std::thread([this, dev_id]() {
    cpu_set_t new_cpu_set;
    CudaDeviceGetCpuAffinity(dev_id, &new_cpu_set);
    CHECK_EQ(sched_setaffinity(0, sizeof(cpu_set_t), &new_cpu_set), 0);

    CudaCheck(cudaSetDevice(dev_id));
    ThreadCtx ctx;
    ctx.g_cuda_stream.reset(new CudaStreamHandle(&cb_event_chan_));
    ctx.cb_event_chan = &cb_event_chan_;
    PollMsgChannel(ctx);
  });
  cb_event_poller_ = std::thread([this, dev_id]() {
    cpu_set_t new_cpu_set;
    CudaDeviceGetCpuAffinity(dev_id, &new_cpu_set);
    CHECK_EQ(sched_setaffinity(0, sizeof(cpu_set_t), &new_cpu_set), 0);

    CudaCheck(cudaSetDevice(dev_id));
    CudaCBEvent cb_event;
    while (cb_event_chan_.Receive(&cb_event) == kChannelStatusSuccess) {
      CudaCheck(cudaEventSynchronize(cb_event.event));
      cb_event.callback();
      CudaCheck(cudaEventDestroy(cb_event.event));
    }
  });
}

GpuThread::~GpuThread() {
  cb_event_chan_.Close();
  cb_event_poller_.join();
}

#endif

}  // namespace oneflow
