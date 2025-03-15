#ifndef NNGPP_HPP
#define NNGPP_HPP

#include <cstring>
#include <nng/nng.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/pipeline0/push.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/pair1/pair.h>
#include <string>

#define NNG_NEW_METHODS(clazz)              \
  static Result<clazz> New() {              \
    clazz $tmp$;                            \
    if (const auto r = $tmp$.init(); !r) {  \
      return r.propagateError<clazz>();     \
    }                                       \
    return $tmp$;                           \
  }                                         \
  static Result<clazz *> New(Heap) {        \
    auto $tmp$ = new clazz;                 \
    if (const auto r = $tmp$->init(); !r) { \
      return r.propagateError<clazz *>();   \
    }                                       \
    return $tmp$;                           \
  }

#define NNG_CONSTRUCTORS(clazz)            \
  clazz(const clazz&) = delete;            \
  clazz& operator=(const clazz&) = delete; \
  clazz& operator=(clazz&&) = delete;      \
  clazz() = default;

#define NNG_CLASS_DECL(clazz) \
  class clazz final : protected virtual Socket

#define NNG_INIT(op) \
  Result<> init() {  \
    return op;       \
  }

#define NNG_MOVE_CONSTRUCTOR(clazz) \
  clazz(clazz&& other) noexcept : Socket{std::move(other)} {}

namespace NNG {
  constexpr int NNG_ERSLTMVD{0x30000001};
  constexpr int SOCK_ALREADY_OPEN{0x30000002};
  constexpr int EMPTY_RESULT{0x7FFFFFFF};

  struct Void {};

  struct Heap {};

  extern const Heap heap;

  template<typename T = Void>
  class Result final {
    template<typename U>
    friend class Result;

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) = delete;

  public:
    Result() : error{EMPTY_RESULT} {}

    Result(Result&& other) noexcept
      : error{other.error}, value{std::move(other.value)} {
      other.error = NNG_ERSLTMVD;
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    Result(T&& value) : error{0}, value{std::move(value)} {}

    void ignore() const noexcept {}
    bool empty() const { return error == EMPTY_RESULT; }
    bool ok() const { return error == 0; }
    bool err() const { return error != 0; }
    const T& get() const { return value; }
    T& get() { return value; }
    T&& takeOwnership() { return std::move(value); }

    const char* what() const {
      switch (error) {
        case NNG_ERSLTMVD:
          return "result object moved; this instance is no longer valid";
        case SOCK_ALREADY_OPEN:
          return "socket is already opened";
        case EMPTY_RESULT:
          return "Result is empty";
        default:
          return nng_strerror(error);
      }
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator bool() const { return ok(); }
    const T* operator->() const { return &value; }
    T* operator->() { return &value; }

    template<typename U = Void>
    Result<U> propagateError() const {
      return Result<U>::Error(error);
    }

    static Result Empty() { return Error(EMPTY_RESULT); }

    static Result Success(T&& value) {
      Result result;
      result.value = std::forward<T>(value);
      result.error = 0;
      return result;
    }

    static Result Success()
      requires std::is_same_v<T, Void> {
      Result result;
      result.error = 0;
      return result;
    }

    static Result Error(const int error) {
      Result result;
      result.error = error;
      return result;
    }

  private:
    int error;
    T value;
  };

  class Socket {
    friend class Context;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

  public:
    Socket() = default;
    virtual ~Socket() { nng_close(sock_); }

    Socket(Socket&& other) noexcept : sock_{other.sock_} {
      memset(&other.sock_, 0, sizeof(other.sock_));
    }

    [[nodiscard]]
    Result<> listen(const std::string_view url) const {
      if (const int err{nng_listen(sock_, url.data(), nullptr, 0)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> dial(const std::string_view url) const {
      if (const int err{nng_dial(sock_, url.data(), nullptr, 0)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> pull0_open() {
      if (const int err{nng_pull0_open(&sock_)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> push0_open() {
      if (const int err{nng_push0_open(&sock_)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> req0_open() {
      if (const int err{nng_req0_open(&sock_)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> rep0_open() {
      if (const int err{nng_rep0_open(&sock_)}; err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> pair1_open() {
      if (const int err{nng_pair1_open(&sock_)}; err != 0) {
        return Result<>::Error(err);
      }
      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> send(const std::string_view message) const {
      if (const int err{
          nng_send(sock_, // ReSharper disable once CppCStyleCast
                   (void *)message.data(),
                   message.size(),
                   0)
        };
        err != 0) {
        return Result<>::Error(err);
      }

      return Result<>::Success();
    }

    [[nodiscard]]
    Result<> _receive(char** buffer, size_t* n, const int flags) const {
      if (const int err{nng_recv(sock_, buffer, n, flags)}; err != 0) {
        return Result<>::Error(err);
      }
      return Result<>::Success();
    }

    [[nodiscard]]
    Result<std::string> receive() const {
      char* buffer;
      size_t n;
      if (const int err{nng_recv(sock_, &buffer, &n, NNG_FLAG_ALLOC)}; err != 0) {
        return Result<std::string>::Error(err);
      }

      std::string message(buffer, n);
      nng_free(buffer, n);
      return message;
    }

    Result<> setReceiveTimeout(const int durationMillis) const {
      if (const int err{nng_socket_set_ms(sock_, NNG_OPT_RECVTIMEO, durationMillis)}; err != 0) {
        return Result<>::Error(err);
      }
      return Result<>::Success();
    }

    Result<> close() const {
      if (const int err{nng_close(sock_)}; err != 0) {
        return Result<>::Error(err);
      }
      return Result<>::Success();
    }

  protected:
    nng_socket sock_{};
  };

  class Message {
    friend class Result<Message>;
    friend class Context;
    Message() = default;
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = delete;

  public:
    Message(Message&& other) noexcept : msg_(other.msg_) {
      other.msg_ = nullptr;
    }

    static Result<Message> New() {
      Message msg;
      if (const auto r = nng_msg_alloc(&msg.msg_, 0); r != 0) {
        return Result<Message>::Error(r);
      }
      return msg;
    }

    static Result<Message> New(const std::string_view message) {
      Message msg;
      if (const auto r = nng_msg_alloc(&msg.msg_, message.size()); r != 0) {
        return Result<Message>::Error(r);
      }

      memcpy(nng_msg_body(msg.msg_), message.data(), message.size());
      return msg;
    }

    ~Message() {
      nng_msg_free(msg_);
    }

    Result<> append(const std::string_view message) const {
      if (const auto r = nng_msg_append(msg_, message.data(), message.size()); r != 0) {
        return Result<>::Error(r);
      }
      return Result<>::Success();
    }

  private:
    nng_msg* msg_;
  };

  namespace ReqRep {
    class Request;
  }

  class Context {
    friend class Result<Context>;
    friend class Message;
    friend class ReqRep::Request;
    Context() = default;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

  public:
    Context(Context&& other) noexcept: ctx_(other.ctx_) {
      memset(&other.ctx_, 0, sizeof(other.ctx_));
    }
    Context& operator=(Context&& other) noexcept {
      ctx_ = other.ctx_;
      memset(&other.ctx_, 0, sizeof(other.ctx_));
      return *this;
    }

    static Result<Context> New(const Socket& socket) {
      Context ctx;
      if (const auto r = nng_ctx_open(&ctx.ctx_, socket.sock_); r != 0) {
        return Result<Context>::Error(r);
      }
      return ctx;
    }

    ~Context() {
      nng_ctx_close(ctx_);
    }

    Result<> sendMessage(const Message& message) const {
      if (const auto r = nng_ctx_sendmsg(ctx_, message.msg_, 0); r != 0) {
        return Result<>::Error(r);
      }
      return Result<>::Success();
    }

    Result<std::string> receiveMessage() const {
      nng_msg* msg;
      if (const auto r = nng_ctx_recvmsg(ctx_, &msg, 0); r != 0) {
        return Result<std::string>::Error(r);
      }

      std::string message{static_cast<const char *>(nng_msg_body(msg)), nng_msg_len(msg)};
      nng_msg_free(msg);
      return message;
    }

  private:
    nng_ctx ctx_;
  };

  namespace Pipeline {
    NNG_CLASS_DECL(Receiver) {
      friend class Result<Receiver>;
      NNG_CONSTRUCTORS(Receiver)
      NNG_INIT(pull0_open())

    public:
      NNG_MOVE_CONSTRUCTOR(Receiver)
      NNG_NEW_METHODS(Receiver)

      using Socket::listen;
      using Socket::dial;
      using Socket::receive;
      using Socket::setReceiveTimeout;
    };

    NNG_CLASS_DECL(Sender) {
      friend class Result<Sender>;
      NNG_CONSTRUCTORS(Sender)
      NNG_INIT(push0_open())

    public:
      NNG_MOVE_CONSTRUCTOR(Sender)
      NNG_NEW_METHODS(Sender)

      using Socket::listen;
      using Socket::dial;
      using Socket::send;
    };
  }

  namespace ReqRep {
    NNG_CLASS_DECL(Client) {
      friend class Result<Client>;
      NNG_CONSTRUCTORS(Client)
      NNG_INIT(req0_open())

    public:
      NNG_MOVE_CONSTRUCTOR(Client)
      NNG_NEW_METHODS(Client)

      using Socket::dial;
      using Socket::setReceiveTimeout;

      [[nodiscard]]
      Result<std::string> request(const std::string_view message) const {
        if (const auto result{send(message)}; !result) {
          return result.propagateError<std::string>();
        }
        return receive();
      }

      [[nodiscard]]
      Result<std::string> requestInParallel(const std::string_view message) const {
        const auto ctx{Context::New(*this)};
        if (!ctx) {
          return ctx.propagateError<std::string>();
        }

        const auto msg{Message::New(message)};
        if (!msg) {
          return msg.propagateError<std::string>();
        }

        if (const auto result = ctx->sendMessage(msg.get()); !result) {
          return result.propagateError<std::string>();
        }

        return ctx->receiveMessage();
      }
    };

    class Request {
      friend class Result<Request>;
      friend class Server;

      Request(const Request&) = delete;
      Request& operator=(const Request&) = delete;

    public:
      Request() = default;

      Request(Request&& other) noexcept
        : ctx(std::move(other.ctx)), msg(std::move(other.msg)) {}
      Request& operator=(Request&& other) noexcept {
        ctx = std::move(other.ctx);
        msg = std::move(other.msg);
        return *this;
      }

      Request(Context&& context, std::string&& message)
        : ctx(std::move(context)), msg(std::move(message)) {}

      [[nodiscard]]
      const std::string& message() const { return msg; }

      [[nodiscard]]
      Result<> reply(const std::string_view message) const {
        const auto m = Message::New(message);
        if (!m) {
          return m.propagateError();
        }

        return ctx.sendMessage(m.get());
      }

    private:
      Context ctx;
      std::string msg;
    };

    NNG_CLASS_DECL(Server) {
      friend class Result<Server>;
      NNG_CONSTRUCTORS(Server)
      NNG_INIT(rep0_open())

    public:
      NNG_MOVE_CONSTRUCTOR(Server)
      NNG_NEW_METHODS(Server)

      using Socket::listen;
      using Socket::receive;
      using Socket::send;
      using Socket::setReceiveTimeout;

      Result<Request> receiveInParallel() const {
        auto ctx{Context::New(*this)};
        if (!ctx) {
          return ctx.propagateError<Request>();
        }

        auto message = ctx->receiveMessage();
        if (!message) {
          return message.propagateError<Request>();
        }

        return Request{ctx.takeOwnership(), message.takeOwnership()};
      }
    };
  } // namespace ReqRep

  namespace Radio {
    NNG_CLASS_DECL(Peer) {
      friend class Result<Peer>;
      NNG_CONSTRUCTORS(Peer)
      NNG_INIT(pair1_open())

    public:
      NNG_MOVE_CONSTRUCTOR(Peer)
      NNG_NEW_METHODS(Peer)

      using Socket::listen;
      using Socket::dial;
      using Socket::receive;
      using Socket::send;
      using Socket::setReceiveTimeout;
    };
  }
}

#endif
