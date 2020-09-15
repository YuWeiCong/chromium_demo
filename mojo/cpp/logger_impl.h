#ifndef HOPE_DEMO_MOJO_CPP_LOGGER_IMPL_H
#define HOPE_DEMO_MOJO_CPP_LOGGER_IMPL_H

#include <string>

#include "hope_demo/mojo/cpp/mojom/log.mojom.h"

template <template <typename R> class Request,
          template <typename R,
                    class ImplRefTraits = mojo::RawPtrImplRefTraits<R>>
          class Binding>
class LoggerImp : public demo::mojom::Logger {
 public:
#ifndef ONLY_BINDING_SET
  LoggerImp(Request<demo::mojom::Logger> request)
      : binding_(this, std::move(request)) {}
#else
  LoggerImp(){}
#endif // ONLY_BINDING_SET
  void Log(const std::string& message, const std::string& tag) override {
    LOG(INFO) << tag + " " + message;
  }
  virtual void ContentToLoggerImp(Request<demo::mojom::Logger> request) = 0;

#ifndef ONLY_BINDING_SET
 private:
  Binding<demo::mojom::Logger> binding_;
#endif // BINDING_SET
};

#endif  // HOPE_DEMO_MOJO_CPP_LOGGER_IMPL_H