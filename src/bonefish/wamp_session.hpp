#ifndef BONEFISH_WAMP_SESSION_HPP
#define BONEFISH_WAMP_SESSION_HPP

#include <bonefish/identifier/session_id.hpp>
#include <bonefish/websocket_config.hpp>
#include <memory>
#include <string>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/server.hpp>

namespace bonefish {

class wamp_message;
class wamp_transport;

class wamp_session
{
public:
    wamp_session();
    wamp_session(const session_id& id,
            std::unique_ptr<wamp_transport> transport);
    ~wamp_session();
    wamp_session(wamp_session const&) = delete;
    wamp_session& operator=(wamp_session const&) = delete;
    const session_id& get_session_id() const;
    bool send_message(const wamp_message* message);

private:
    session_id m_session_id;
    std::unique_ptr<wamp_transport> m_transport;
};

} // namespace bonefish

#endif // BONEFISH_WAMP_SESSION_HPP