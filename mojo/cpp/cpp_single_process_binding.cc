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
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"


class LoggerImpBinding : public LoggerImp<mojo::InterfaceRequest,mojo::Binding> {
 public:
  LoggerImpBinding(demo::mojom::LoggerRequest request)
      : LoggerImp<mojo::InterfaceRequest,mojo::Binding>(std::move(request)) {}

  void ContentToLoggerImp(demo::mojom::LoggerRequest request) override {
    binding_set_.AddBinding(this, std::move(request));
  }
 private:
  mojo::BindingSet<demo::mojom::Logger> binding_set_;
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

  // 使用 mojo::Binding
  // hope_demo/mojo/cpp/mojom/log.mojom-forward.h
  mojo::MessagePipe pipe;
  LoggerImpBinding logger_imp(
      demo::mojom::LoggerRequest(std::move(pipe.handle0)));
  demo::mojom::LoggerPtr logger_ptr;
  logger_ptr.Bind(demo::mojom::LoggerPtrInfo(std::move(pipe.handle1),
                                             demo::mojom::Logger::Version_));
  logger_ptr->Log(kMessage, "logger_imp1");

  // mojo::MakeRequest 定义在 mojo/public/cpp/bindings/interface_request.h
  demo::mojom::LoggerPtr logger_ptr2;
  auto request2 = mojo::MakeRequest(&logger_ptr2);
  LoggerImpBinding logger_imp2(std::move(request2));
  logger_ptr2->Log(kMessage, "logger_imp2");

  // 使用 Mojo::BindingSet
  mojo::MessagePipe pipe3;
  logger_imp2.ContentToLoggerImp(
      demo::mojom::LoggerRequest(std::move(pipe3.handle0)));
  demo::mojom::LoggerPtr logger_ptr3;
  logger_ptr3.Bind(demo::mojom::LoggerPtrInfo(std::move(pipe3.handle1),
                                              demo::mojom::Logger::Version_));
  logger_ptr3->Log(kMessage, "BindingSet1");

  demo::mojom::LoggerPtr logger_ptr4;
  auto request4 = mojo::MakeRequest(&logger_ptr4);
  logger_imp2.ContentToLoggerImp(std::move(request4));
  logger_ptr4->Log(kMessage, "BindingSet2");

  run_loop.Run();
  return 0;
}