#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_type.h"
#include "base/posix/global_descriptors.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "hope_demo/mojo/cpp/pipe_reader.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/wait.h"

void CheckMojoResult(MojoResult result) {
  CHECK_EQ(MOJO_RESULT_OK, result);
}

void CreateProducer() {
  logging::SetLogPrefix("producer");
  mojo::PlatformChannel channel;
  mojo::OutgoingInvitation invitation;
  mojo::ScopedMessagePipeHandle pipe =
      invitation.AttachMessagePipe("arbitrary pipe name");

  // use commandline to translate th fd
  base::LaunchOptions options;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  channel.PrepareToPassRemoteEndpoint(&options, command_line);
  LOG(INFO) << "commandLine is " << command_line->GetArgumentsString();
  base::Process child_process = base::LaunchProcess(*command_line, options);

  channel.RemoteProcessLaunchAttempted();
  mojo::OutgoingInvitation::Send(std::move(invitation), child_process.Handle(),
                                 channel.TakeLocalEndpoint());
  
}
void CreateConsumer() {
  logging::SetLogPrefix("consumer");
  LOG(INFO) << "commandLine is "
            << base::CommandLine::ForCurrentProcess()->GetArgumentsString();
  mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(
      mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
          *base::CommandLine::ForCurrentProcess()));
  mojo::ScopedMessagePipeHandle pipe =
      invitation.ExtractMessagePipe("arbitrary pipe name");
  CheckMojoResult(mojo::Wait(pipe.get(), MOJO_HANDLE_SIGNAL_READABLE));

  // 1. get message from MessagePipe
  std::vector<uint8_t> payload;
  std::vector<mojo::ScopedHandle> handles;
  CheckMojoResult(ReadMessageRaw(pipe.get(), &payload, &handles,
                                 MOJO_READ_MESSAGE_FLAG_NONE));
  LOG(INFO) << "read message from message pipe " << (char*)(&payload[0]);

  // 2. get DataPipe handle,ShareBuffer handle by MessagePipe
  CheckMojoResult(ReadMessageRaw(pipe.get(), &payload, &handles,
                                 MOJO_READ_MESSAGE_FLAG_NONE));
  LOG(INFO) << "read message from message pipe " << (char*)(&payload[0]);
  LOG(INFO) << "get handle size " << handles.size();

  // 3. get DataPipe handle,ShareBuffer handle by MessagePipe
  MessagePipeReader message_pipe_reader(std::move(pipe));
  DataPipeReader data_pipe_reader(mojo::ScopedDataPipeConsumerHandle(
      mojo::DataPipeConsumerHandle(handles[0].release().value())));

  SharedBufferReader shared_buffer_reader(mojo::ScopedSharedBufferHandle(
      mojo::SharedBufferHandle(handles[1].release().value())));
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

  if (argc < 3) {
    CreateProducer();
  } else {
    CreateConsumer();
  }
  run_loop.Run();
  return 0;
}
