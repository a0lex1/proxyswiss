
#include "proxy/destination.h"

#include "common/net/url_parser.h"

#include <stdint.h>

namespace proxy {

template <typename T>
void destination_from_url_parser(proxy::destination& dst,
  const typename common::net::host_address& host, uint16_t port);

}
