#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_type.h"
#include "base/posix/global_descriptors.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"

void CheckMojoResult(MojoResult result) {
  CHECK_EQ(MOJO_RESULT_OK, result);
}

int main(int argc, char const* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager at_exit_manager;

  base::MessageLoop message_loop;
  base::RunLoop run_loop;

  base::Thread mojo_thread("mojo_thread");
  mojo_thread.StartWithOptions({base::MessagePumpType::IO, 0});

  mojo::core::Init();
  mojo::core::ScopedIPCSupport(
      mojo_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);

  mojo::ScopedMessagePipeHandle producer;
  mojo::ScopedMessagePipeHandle comsumer;

  CheckMojoResult(mojo::CreateMessagePipe(nullptr, &producer, &comsumer));
  std::string message = "hi,cpp message pipe!";
  CheckMojoResult(WriteMessageRaw(producer.get(), message.c_str(),
                                  strlen(message.c_str()), nullptr, 0,
                                  MOJO_WRITE_MESSAGE_FLAG_NONE));

  std::vector<uint8_t> payload;
  std::vector<mojo::ScopedHandle> handles;
  CheckMojoResult(ReadMessageRaw(comsumer.get(), &payload, &handles,
                                 MOJO_READ_MESSAGE_FLAG_NONE));

  LOG(INFO) << "read message raw " << reinterpret_cast<uint8_t*>(&payload[0]);
  run_loop.Run();
  return 0;
}
