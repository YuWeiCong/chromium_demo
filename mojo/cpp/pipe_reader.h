#ifndef HOPE_DEMO_MOJO_CPP_PIPE_READER_H
#define HOPE_DEMO_MOJO_CPP_PIPE_READER_H

#include "base/logging.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

template <class T>
class PipeReader {
 public:
  PipeReader(T pipe)
      : pipe_(std::move(pipe)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC) {
    // NOTE: base::Unretained is safe because the callback can never be run
    // after SimpleWatcher destruction.
    watcher_.Watch(
        pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        base::BindRepeating(&PipeReader::OnReadable, base::Unretained(this)));
  }

  ~PipeReader() {}

 protected:
  virtual void OnReadableImpl() = 0;
  void print(char* str) { LOG(INFO) << "get message " << str; }
  T pipe_;

 private:
  void OnReadable(MojoResult result) {
    while (result == MOJO_RESULT_OK) {
      OnReadableImpl();
    }
  }

  mojo::SimpleWatcher watcher_;
};

class MessagePipeReader : public PipeReader<mojo::ScopedMessagePipeHandle> {
 public:
  MessagePipeReader(mojo::ScopedMessagePipeHandle pipe)
      : PipeReader(std::move(pipe)) {}

 protected:
  void OnReadableImpl() override {
    std::vector<uint8_t> payload;
    std::vector<mojo::ScopedHandle> scoped_handles;
    auto result = mojo::ReadMessageRaw(pipe_.get(), &payload, &scoped_handles,
                                       MOJO_READ_MESSAGE_FLAG_NONE);
    CHECK_EQ(result, MOJO_RESULT_OK);
    print((char*)&payload[0]);
  }
};

class DataPipeReader : public PipeReader<mojo::ScopedDataPipeConsumerHandle> {
 public:
  DataPipeReader(mojo::ScopedDataPipeConsumerHandle pipe)
      : PipeReader(std::move(pipe)) {}

 protected:
  void OnReadableImpl() override {
    char* message = nullptr;
    std::vector<mojo::ScopedHandle> scoped_handles;
    uint32_t num_bytes;
    auto result = pipe_.get().ReadData(message, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE);
    CHECK_EQ(result, MOJO_RESULT_OK);
    print(message);
  }
};

class SharedBufferReader : public PipeReader<mojo::ScopedSharedBufferHandle> {
 public:
  SharedBufferReader(mojo::ScopedSharedBufferHandle pipe)
      : PipeReader(std::move(pipe)) {}

 protected:
  void OnReadableImpl() override {
    std::vector<mojo::ScopedHandle> scoped_handles;
    auto mapping = pipe_.get().Map(1024);
    print((char*)mapping.get());
  }
};


#endif  // HOPE_DEMO_MOJO_CPP_PIPE_READER_H