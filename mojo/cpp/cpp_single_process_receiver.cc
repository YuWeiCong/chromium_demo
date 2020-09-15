#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_type.h"
#include "base/posix/global_descriptors.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "hope_demo/mojo/cpp/mojom/log.mojom-forward.h"
#include "hope_demo/mojo/cpp/logger_impl.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"


class LoggerImpReceiver : public LoggerImp<mojo::PendingReceiver,mojo::Receiver>  {
 public:
  LoggerImpReceiver(mojo::PendingReceiver<demo::mojom::Logger> pending_receiver)
      : LoggerImp<mojo::PendingReceiver,mojo::Receiver>(std::move(pending_receiver)) {}
 
  void ContentToLoggerImp(mojo::PendingReceiver<demo::mojom::Logger> request) override {
    receivers_.Add(this, std::move(request));
  }

 private:
  mojo::ReceiverSet<demo::mojom::Logger> receivers_;
};

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
  const std::string kMessage = "hi,mojo";


  // 使用 mojo::Receiver
  mojo::MessagePipe pipe;
  LoggerImpReceiver logger_receiver(
      demo::mojom::LoggerRequest(std::move(pipe.handle0)));
  mojo::Remote<demo::mojom::Logger> remote(
      mojo::PendingRemote<demo::mojom::Logger>(demo::mojom::LoggerPtrInfo(
          std::move(pipe.handle1), demo::mojom::Logger::Version_)));
  remote->Log(kMessage, "receiver1");

  mojo::Remote<demo::mojom::Logger> remote2;
  LoggerImpReceiver logger_receiver2(remote2.BindNewPipeAndPassReceiver());
  remote2->Log(kMessage, "receiver2");

  // 使用 mojo::ReceiverSet
  mojo::Remote<demo::mojom::Logger> remote3;
  logger_receiver2.ContentToLoggerImp(remote3.BindNewPipeAndPassReceiver());
  remote3->Log(kMessage, "receiverSet3");

  run_loop.Run();
  return 0;
}