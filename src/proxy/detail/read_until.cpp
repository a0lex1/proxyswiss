
#include "proxy/detail/read_until.h"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace std;
using namespace boost::asio::ip;
using namespace boost::placeholders;

namespace proxy {
namespace detail {

class read_until_op: public boost::enable_shared_from_this<read_until_op> {
public:
  typedef boost::system::error_code error_code;
  typedef boost::function<void(error_code)> handler_t;

  read_until_op(tcp::socket& s, string& buf, const string& delim,
    size_t max_chars, handler_t handler)
    :
    sock_(s), buf_(buf), delim_(delim), max_chars_(max_chars),
    handler_(handler)
  {
  }

  void start() {
    buf_.clear();
    read_next();
  }

private:
  void read_next() {

    // |max_chars_| is reached ?
    if (buf_.length() == max_chars_) {
      // Done
      handler_(boost::system::error_code());
      return;
    }

    // |buf| ends with |delim| ?
    if (buf_.length() >= delim_.length()
        &&
        !memcmp(&buf_.c_str()[buf_.length() - delim_.length()],
                delim_.c_str(), delim_.length()))
    {
      // Done
      handler_(boost::system::error_code());
      return;
    }

    // Allocate and read next char
    size_t len = buf_.length();
    buf_.resize(len + 1);
    char* p = const_cast<char*>(&buf_.c_str()[len]);

    boost::asio::async_read(
      sock_,
      boost::asio::buffer(p, 1),
      boost::asio::transfer_exactly(1),
      boost::bind(&read_until_op::handle_read, shared_from_this(), _1, _2));
  }

  void handle_read(error_code err, size_t) {
    if (err) {
      handler_(err);
      return;
    }

    read_next();
  }

private:
  tcp::socket&  sock_;
  string&       buf_;
  string        delim_;
  size_t        max_chars_;
  handler_t     handler_;
};

void read_until(
  tcp::socket& sock,
  string& buf,
  const string& delim,
  size_t max_chars,
  boost::function<void(boost::system::error_code)> handler)
{
  boost::shared_ptr<read_until_op> sptr(new read_until_op(
    sock, buf, delim, max_chars, handler));

  sptr->start();
}

}}
