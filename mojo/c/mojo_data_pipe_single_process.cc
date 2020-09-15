#include <string.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/functions.h"
#include "mojo/public/c/system/types.h"

/**
 *    用于演示在单进程中，如何使用 DataPipe 传输数据
 **/

int main(int argc, char const* argv[]) {
  // 创建 PlatformShareMemory 时，会使用到
  // base::CommandLine::ForCurrentProcess() 所以要初始化 CommandLine
  base::CommandLine::Init(argc, argv);

  base::Thread mojo_thread("mojo_thread");
  mojo_thread.StartWithOptions({base::MessagePumpType::IO, 0});
  mojo::core::Init();
  mojo::core::ScopedIPCSupport(
      mojo_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);

  MojoHandle producer_handle, comsumer_handle;
  MojoCreateDataPipeOptions options{sizeof(MojoCreateDataPipeOptions),
                                    MOJO_CREATE_DATA_PIPE_FLAG_NONE, 1, 100};
  CHECK_EQ(MOJO_RESULT_OK,
           MojoCreateDataPipe(&options, &producer_handle, &comsumer_handle));

  std::string message = "hi,data pipe.";
  uint32_t msg_size = sizeof(message);

#ifndef TOW_PHASE_WRITE
  MojoWriteDataOptions write_data_options{sizeof(MojoWriteDataOptions),
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE};
  CHECK_EQ(MojoWriteData(producer_handle, message.c_str(), &msg_size,
                         &write_data_options),
           MOJO_RESULT_OK);

#ifdef KNOW_NUM_BYTES
  uint32_t num_bytes = msg_size;
#else
  // 如果你不知道当前的 DataPipe存储了多少数据，那先进行查询
  MojoReadDataOptions query_data_options{sizeof(MojoReadDataOptions),
                                         MOJO_READ_DATA_FLAG_QUERY};
  char* query_buff = new char();
  uint32_t num_bytes = 0;
  CHECK_EQ(MojoReadData(comsumer_handle, &query_data_options, query_buff,
                        &num_bytes),
           MOJO_RESULT_OK);
  LOG(INFO) << "the num_bytes of this data pipe is " << num_bytes;
#endif  // KNOW_NUM_BYTES
  MojoReadDataOptions read_data_options{sizeof(MojoReadDataOptions),
                                        MOJO_READ_DATA_FLAG_ALL_OR_NONE};
  char* buff = new char();
  CHECK_EQ(MojoReadData(comsumer_handle, &read_data_options, buff, &num_bytes),
           MOJO_RESULT_OK);
  LOG(INFO) << "receive data pipe " << (char*)buff;
  return 0;

#else
  {
    void* buff = nullptr;
    uint32_t buffer_num_bytes = msg_size;
    MojoBeginWriteDataOptions begin_write_data_options{
        sizeof(MojoBeginWriteDataOptions), MOJO_BEGIN_WRITE_DATA_FLAG_NONE};
    CHECK_EQ(MojoBeginWriteData(producer_handle, &begin_write_data_options,
                                &buff, &buffer_num_bytes),
             MOJO_RESULT_OK);
    memcpy(buff, message.c_str(), msg_size);

    MojoEndWriteDataOptions end_write_data_options{
        sizeof(MojoEndWriteDataOptions), MOJO_END_WRITE_DATA_FLAG_NONE};
    CHECK_EQ(MojoEndWriteData(producer_handle, buffer_num_bytes,
                              &end_write_data_options),
             MOJO_RESULT_OK);
  }

  {
    MojoBeginReadDataOptions begin_read_data_options{
        sizeof(MojoBeginReadDataOptions), MOJO_BEGIN_READ_DATA_FLAG_NONE};
    const void* buff = nullptr;
    uint32_t buffer_num_bytes = msg_size;
    CHECK_EQ(MojoBeginReadData(comsumer_handle, &begin_read_data_options, &buff,
                               &buffer_num_bytes),
             MOJO_RESULT_OK);
    LOG(INFO) << "with TOW_PHASE_WRITE, receive data pipe " << (char*)buff;

    MojoEndReadDataOptions end_read_data_options{sizeof(MojoEndReadDataOptions),
                                                 MOJO_END_READ_DATA_FLAG_NONE};
    CHECK_EQ(MojoEndReadData(comsumer_handle, buffer_num_bytes,
                             &end_read_data_options),
             MOJO_RESULT_OK);
    return 0;
  }

#endif  // TOW_PHASE_WRITE
}
