#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_type.h"
#include "base/posix/global_descriptors.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "hope_demo/mojo/cpp/logger_impl.h"
#include "hope_demo/mojo/cpp/mojom/log.mojom-forward.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/generic_pending_receiver.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"

class LoggerBindMapReceiver
    : public LoggerImp<mojo::InterfaceRequest, mojo::Binding> {
 public:
  LoggerBindMapReceiver(scoped_refptr<base::SequencedTaskRunner> task_runner) {
    binder_map_.Add<demo::mojom::Logger>(
        base::BindRepeating(
            [](LoggerBindMapReceiver* logger,
               mojo::InterfaceRequest<demo::mojom::Logger> request) {
              new mojo::Receiver<demo::mojom::Logger>(
                  logger, mojo::PendingReceiver<demo::mojom::Logger>(
                              std::move(request)));
            },
            this),
        task_runner);
  }

  void ContentToLoggerImp(demo::mojom::LoggerRequest request) override {
    binder_map_.Bind(new mojo::GenericPendingReceiver(
        mojo::PendingReceiver<demo::mojom::Logger>(std::move(request))));
  }

 private:
  mojo::BinderMap binder_map_;
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

  // 使用 mojo::BinderMap
  mojo::MessagePipe pipe;
  LoggerBindMapReceiver logger_imp(mojo_thread.task_runner());
  logger_imp.ContentToLoggerImp(demo::mojom::LoggerRequest(std::move(pipe.handle0)));
  demo::mojom::LoggerPtr logger_ptr;
  logger_ptr.Bind(demo::mojom::LoggerPtrInfo(std::move(pipe.handle1),
                                             demo::mojom::Logger::Version_));
  logger_ptr->Log(kMessage, "BinderMap");

  run_loop.Run();
  return 0;
}