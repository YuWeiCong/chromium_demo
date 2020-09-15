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
#include "hope_demo/mojo/cpp/logger_impl.h"
#include "hope_demo/mojo/cpp/mojom/log.mojom-forward.h"
#include "hope_demo/mojo/cpp/mojom/log.mojom.h"
#include "hope_demo/mojo/cpp/pipe_reader.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
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

class LoggerImpReceiver
    : public LoggerImp<mojo::PendingReceiver, mojo::Receiver> {
 public:
  LoggerImpReceiver(mojo::PendingReceiver<demo::mojom::Logger> pending_receiver)
      : LoggerImp<mojo::PendingReceiver, mojo::Receiver>(
            std::move(pending_receiver)) {}

  void ContentToLoggerImp(
      mojo::PendingReceiver<demo::mojom::Logger> request) override {
    receivers_.Add(this, std::move(request));
  }

 private:
  mojo::ReceiverSet<demo::mojom::Logger> receivers_;
};

void CreateProducer() {
  logging::SetLogPrefix("producer");
  mojo::PlatformChannel channel;
  mojo::OutgoingInvitation invitation;
  mojo::ScopedMessagePipeHandle binding_pipe =
      invitation.AttachMessagePipe("binding");

  // use commandline to translate th fd
  base::LaunchOptions options;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  channel.PrepareToPassRemoteEndpoint(&options, command_line);
  LOG(INFO) << "commandLine is " << command_line->GetArgumentsString();
  base::Process child_process = base::LaunchProcess(*command_line, options);

  channel.RemoteProcessLaunchAttempted();
  mojo::OutgoingInvitation::Send(std::move(invitation), child_process.Handle(),
                                 channel.TakeLocalEndpoint());

  mojo::Remote<demo::mojom::Logger> remote(
      mojo::PendingRemote<demo::mojom::Logger>(
          demo::mojom::LoggerPtrInfo(std::move(binding_pipe), demo::mojom::Logger::Version_)));

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](mojo::Remote<demo::mojom::Logger> remote) {
            remote->Log("hi,mojo", "multiple process ");
          },
          std::move(remote)),
      base::TimeDelta::FromSeconds(1));
}

void CreateConsumer() {
  logging::SetLogPrefix("consumer");
  LOG(INFO) << "commandLine is "
            << base::CommandLine::ForCurrentProcess()->GetArgumentsString();
  mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(
      mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
          *base::CommandLine::ForCurrentProcess()));
  mojo::ScopedMessagePipeHandle pipe = invitation.ExtractMessagePipe("binding");
  new LoggerImpReceiver(demo::mojom::LoggerRequest(std::move(pipe)));
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

  if (argc < 2) {
    CreateProducer();
  } else {
    CreateConsumer();
  }
  run_loop.Run();
  return 0;
}
