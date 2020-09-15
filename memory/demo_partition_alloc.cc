#include <string.h>

#include "base/allocator/partition_allocator/partition_alloc.h"
#include "base/logging.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"

int main(int argc, char const* argv[]) {
  const char kPartitionsDemo[] = "PartitionsDemo";
  auto copy_and_log = [](char* p, char const* str, int length,
                         std::string tag) {
    memcpy(p, str, length);
    LOG(INFO) << "the value of " << tag << " is " << p;
  };

  // 通过WTF::Partitions调用
  WTF::Partitions::Initialize();
  char* fast_ptr =
      (char*)WTF::Partitions::FastMalloc(sizeof(kPartitionsDemo), "char");
  copy_and_log(fast_ptr, kPartitionsDemo, sizeof(kPartitionsDemo), "fast_ptr");
  WTF::Partitions::FastFree((void*)fast_ptr);

  char* buffer_ptr = (char*)WTF::Partitions::BufferPartition()->Alloc(
      sizeof(kPartitionsDemo), "char");
  copy_and_log(buffer_ptr, kPartitionsDemo, sizeof(kPartitionsDemo),
               "buffer_ptr");
  WTF::Partitions::BufferPartition()->Free((void*)buffer_ptr);

  // 直接使用PartitionAllocatorGeneric 和 SizeSpecificPartitionAllocator<size>
  base::PartitionAllocatorGeneric partition_allocator_generic;
  base::SizeSpecificPartitionAllocator<1024> size_specific_partition_allocator;
  partition_allocator_generic.init();
  size_specific_partition_allocator.init();

  char* allocator_generic_ptr =
      (char*)partition_allocator_generic.root()->Alloc(sizeof(kPartitionsDemo),
                                                       "char");
  copy_and_log(allocator_generic_ptr, kPartitionsDemo, sizeof(kPartitionsDemo),
               "allocator_generic_ptr");
  partition_allocator_generic.root()->Free(allocator_generic_ptr);
  
  // 因为PartitionRoot 所申请的大小必须与系统指针大小所对齐
  const size_t pointer_size = sizeof(void*);
  int i = 1;
  while (i*pointer_size<sizeof(kPartitionsDemo))
  {
    ++i;
  }
  char* size_specific_partition_allocator_ptr =
      (char*)size_specific_partition_allocator.root()->Alloc(
          i*pointer_size, "char");
  copy_and_log(size_specific_partition_allocator_ptr, kPartitionsDemo, sizeof(kPartitionsDemo)+1,
               "size_specific_partition_allocator_ptr");
  partition_allocator_generic.root()->Free(size_specific_partition_allocator_ptr);

  return 0;
}