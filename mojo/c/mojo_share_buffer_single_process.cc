#include "base/command_line.h"
#include "base/message_loop/message_pump_type.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/functions.h"
#include "mojo/public/c/system/types.h"

/**
 *      用于演示在单进程中，如何使用 Share Buffer
 *
 * */
int main(int argc, char const* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::Thread mojo_thread("mojo_thread");
  mojo_thread.StartWithOptions({base::MessagePumpType::IO, 0});

  mojo::core::Init();
  mojo::core::ScopedIPCSupport(
      mojo_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);

  MojoCreateSharedBufferOptions options{sizeof(MojoCreateSharedBufferOptions),
                                        MOJO_CREATE_SHARED_BUFFER_FLAG_NONE};

  uint64_t num_bytes = 512;
  MojoHandle share_buffer_handle;
  CHECK_EQ(MojoCreateSharedBuffer(num_bytes, &options, &share_buffer_handle),
           MOJO_RESULT_OK);

  void* buffer = nullptr;
  MojoMapBufferOptions map_buffer_options{sizeof(MojoMapBufferOptions),
                                          MOJO_MAP_BUFFER_FLAG_NONE};
  CHECK_EQ(MojoMapBuffer(share_buffer_handle, 0, num_bytes, &map_buffer_options,
                         &buffer),
           MOJO_RESULT_OK);
	std::string message = "hi,share buffer";
	mempcpy(buffer,message.c_str(),sizeof(message));
	LOG(INFO)<<"Get share buffer " << (const char*)buffer;

  return 0;
}
