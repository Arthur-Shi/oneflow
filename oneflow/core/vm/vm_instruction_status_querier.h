#ifndef ONEFLOW_CORE_VM_VPU_INSTRUCTION_STATUS_QUERIER_H_
#define ONEFLOW_CORE_VM_VPU_INSTRUCTION_STATUS_QUERIER_H_

namespace oneflow {

class VmInstructionStatusQuerier {
 public:
  virtual ~VmInstructionStatusQuerier() = default;
  virtual bool Done() const;

 protected:
  VmInstructionStatusQuerier() = default;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_VM_VPU_INSTRUCTION_STATUS_QUERIER_H_