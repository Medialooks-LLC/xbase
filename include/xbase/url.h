#pragma once

#include "xbase/symbols.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace xsdk::xbase::url {

struct Parts {
    std::string                        protocol;
    std::string                        server;
    std::string                        path;
    std::string                        user_or_adapter;
    std::string                        password;
    uint16_t                           port = 0;
    std::map<std::string, std::string> params;
};

XBASE_API bool IsProtocol(std::string_view _text,
                          std::string*     _protocol_p = nullptr,
                          std::string*     _name_p     = nullptr);

XBASE_API size_t IsAnyProtocol(std::string_view              _text,
                               std::vector<std::string_view> _protocols,
                               std::string*                  _name_p = nullptr);

XBASE_API Parts Parse(const std::string& _url);

XBASE_API std::string Make(Parts _url);

} // namespace xsdk::xbase::url
