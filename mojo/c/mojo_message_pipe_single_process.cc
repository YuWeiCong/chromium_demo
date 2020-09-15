#include "base/command_line.h"
#include "base/message_loop/message_pump_type.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/c/system/functions.h"

/**
 *    用于演示在单进程中，如何使用MessagePipe传输数据或者MojoHandle
 **/

int main(int argc, char const* argv[]) {

  base::Thread mojo_thread("mojo_thread");
  mojo_thread.StartWithOptions({base::MessagePumpType::IO, 0});

  mojo::core::Init();
  mojo::core::ScopedIPCSupport(
      mojo_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  MojoHandle mojo_handle_a, mojo_handle_b;
  MojoCreateMessagePipeOptions options = {0};
  options.struct_size = sizeof(MojoCreateMessageOptions);
  options.flags = MOJO_CREATE_MESSAGE_PIPE_FLAG_NONE;
  MojoResult result =
      MojoCreateMessagePipe(&options, &mojo_handle_a, &mojo_handle_b);
  CHECK_EQ(result, MOJO_RESULT_OK) << "Create message pipe failed.";

  MojoMessageHandle message_handle;
  result = MojoCreateMessage(nullptr, &message_handle);
  CHECK_EQ(result, MOJO_RESULT_OK) << "Create message failed.";

  void* buffer;
  uint32_t buffer_size;
  std::string message_str = "hello";
  MojoAppendMessageDataOptions append_message_data_options;
  append_message_data_options.struct_size = sizeof(MojoAppendMessageDataOptions);
  append_message_data_options.flags = MOJO_APPEND_MESSAGE_DATA_FLAG_COMMIT_SIZE;
  result = MojoAppendMessageData(message_handle, sizeof(message_str), nullptr,
                                 0, &append_message_data_options, &buffer, &buffer_size);
  CHECK_EQ(result, MOJO_RESULT_OK) << "Append message data failed.";

  memcpy(buffer, message_str.c_str(), buffer_size);
  MojoWriteMessageOptions write_message_options;
  write_message_options.struct_size = sizeof(MojoAppendMessageDataOptions);
  write_message_options.flags = MOJO_WRITE_MESSAGE_FLAG_NONE;
  result = MojoWriteMessage(mojo_handle_a, message_handle, &write_message_options);
  CHECK_EQ(result, MOJO_RESULT_OK) << "write message failed.";
  MojoClose(mojo_handle_a);

  MojoMessageHandle read_message_handle;
  result = MojoReadMessage(mojo_handle_b, nullptr, &read_message_handle);
  CHECK_EQ(result, MOJO_RESULT_OK) << "read message failed.";

  void* read_buffer;
  uint32_t read_buffer_size;
  MojoGetMessageData(read_message_handle, nullptr, &read_buffer,
                     &read_buffer_size, nullptr, nullptr);
  LOG(INFO) << "receive message " << (char*)read_buffer;

  MojoClose(mojo_handle_b);
  return 0;
}
