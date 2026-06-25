#include "xbase/url.h"

#include "xbase/strings.h"

#include <cstdlib>

namespace xsdk::xbase::url {

bool IsProtocol(const std::string_view _text, std::string* const _protocol_p, std::string* const _name_p)
{
    const size_t proto_pos = _text.find("://");
    if (proto_pos == std::string::npos)
        return false;

    if (_protocol_p)
        *_protocol_p = _text.substr(0, proto_pos);
    if (_name_p)
        *_name_p = _text.substr(proto_pos + 3);

    return true;
}

size_t IsAnyProtocol(const std::string_view              _text,
                     const std::vector<std::string_view> _protocols,
                     std::string* const                  _name_p)
{
    const size_t proto_pos = _text.find("://");
    if (proto_pos == std::string::npos)
        return 0;

    const auto protocol = _text.substr(0, proto_pos);
    for (size_t i = 0; i < _protocols.size(); ++i) {
        if (strings::IsSame(_protocols[i], protocol, false)) {
            if (_name_p)
                *_name_p = _text.substr(proto_pos + 3);
            return i + 1;
        }
    }

    return 0;
}

namespace {

std::string ParseAuthority(const std::string&                  _url,
                           std::string* const                  _path_p,
                           uint16_t* const                     _port_p,
                           std::string* const                  _user_or_adapter_p,
                           std::string* const                  _password_p,
                           std::string* const                  _protocol_p,
                           std::map<std::string, std::string>* _params_p)
{
    if (_protocol_p)
        *_protocol_p = "";
    if (_path_p)
        *_path_p = "";
    if (_port_p)
        *_port_p = 0;
    if (_user_or_adapter_p)
        *_user_or_adapter_p = "";
    if (_password_p)
        *_password_p = "";

    std::string url              = _url;
    const size_t folder_pos      = _url.find('/');
    const size_t protocol_pos    = _url.find("://");
    const bool   protocol_before = protocol_pos != std::string::npos && protocol_pos < folder_pos;
    if (protocol_before) {
        if (_protocol_p)
            *_protocol_p = url.substr(0, protocol_pos + 3);
        url = url.substr(protocol_pos + 3);
    }

    const size_t params_pos = url.find('?');
    if (params_pos != std::string::npos) {
        if (_params_p) {
            const auto params_text = std::string_view(url).substr(params_pos + 1);
            const auto params      = strings::SplitAny(params_text, "&;", false, true);
            for (const auto param : params) {
                const auto eq_pos = param.find('=');
                if (eq_pos != std::string_view::npos)
                    (*_params_p)[std::string(param.substr(0, eq_pos))] = std::string(param.substr(eq_pos + 1));
            }
        }
        url = url.substr(0, params_pos);
    }

    const size_t path_pos = url.find_first_of("\\/");
    if (path_pos != std::string::npos) {
        if (_path_p)
            *_path_p = url.substr(path_pos + 1);
        url = url.substr(0, path_pos);
    }

    auto parts = strings::SplitAnyCopy(url, ":@");
    if (parts.empty())
        return "";

    if (!url.empty() && url.back() == '@')
        parts.emplace_back();

    if (parts.size() > 1) {
        char*    end  = nullptr;
        uint16_t port = static_cast<uint16_t>(std::strtol(parts.back().c_str(), &end, 10));
        if (end && *end == 0) {
            if (_port_p)
                *_port_p = port;
            parts.pop_back();
        }
    }

    if (parts.size() > 1) {
        if (_user_or_adapter_p)
            *_user_or_adapter_p = parts[0];
        if (_password_p && parts.size() > 2)
            *_password_p = parts[1];
    }

    return parts.back();
}

} // namespace

Parts Parse(const std::string& _url)
{
    Parts result;
    result.server = ParseAuthority(_url,
                                   &result.path,
                                   &result.port,
                                   &result.user_or_adapter,
                                   &result.password,
                                   &result.protocol,
                                   &result.params);
    return result;
}

std::string Make(Parts _url)
{
    std::string result = _url.protocol;
    if (!_url.user_or_adapter.empty() || !_url.password.empty()) {
        result += _url.user_or_adapter;
        if (!_url.password.empty())
            result += ":" + _url.password;
        result += "@";
    }

    result += _url.server;
    if (_url.port > 0)
        result += ":" + std::to_string(_url.port);
    if (!_url.path.empty())
        result += "/" + _url.path;
    if (!_url.params.empty()) {
        std::string params;
        for (const auto& [key, val] : _url.params) {
            if (!params.empty())
                params += "&";
            params += key + "=" + val;
        }
        result += "?" + params;
    }

    return result;
}

} // namespace xsdk::xbase::url
