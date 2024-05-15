
#pragma once

#include <string>

namespace proxy {

struct credentials {
  std::string username;
  std::string password;

  bool empty() const {
    return username.empty() && password.empty();
  }

  credentials(
    const std::string& _username = "",
    const std::string& _password = "")
    :
    username(_username),
    password(_password)
  {
  }
};

}
