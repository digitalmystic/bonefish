/**
 *  Copyright (C) 2015 Topology LP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <bonefish/common/wamp_message_processor.hpp>
#include <bonefish/identifiers/wamp_session_id.hpp>
#include <bonefish/identifiers/wamp_session_id_generator.hpp>
#include <bonefish/messages/wamp_abort_message.hpp>
#include <bonefish/messages/wamp_call_message.hpp>
#include <bonefish/messages/wamp_error_message.hpp>
#include <bonefish/messages/wamp_goodbye_message.hpp>
#include <bonefish/messages/wamp_hello_details.hpp>
#include <bonefish/messages/wamp_hello_message.hpp>
#include <bonefish/messages/wamp_message.hpp>
#include <bonefish/messages/wamp_publish_message.hpp>
#include <bonefish/messages/wamp_register_message.hpp>
#include <bonefish/messages/wamp_subscribe_message.hpp>
#include <bonefish/messages/wamp_unregister_message.hpp>
#include <bonefish/messages/wamp_unsubscribe_message.hpp>
#include <bonefish/messages/wamp_yield_message.hpp>
#include <bonefish/router/wamp_router.hpp>
#include <bonefish/router/wamp_routers.hpp>
#include <bonefish/session/wamp_session.hpp>
#include <bonefish/trace/trace.hpp>
#include <bonefish/transport/wamp_transport.hpp>
#include <bonefish/common/wamp_connection_base.hpp>

namespace bonefish {

void wamp_message_processor::process_message(
        const std::unique_ptr<wamp_message>& message,
        std::unique_ptr<wamp_transport>&& transport,
        wamp_connection_base* connection)
{
    BONEFISH_TRACE("processing message: %1%", message_type_to_string(message->get_type()));
    switch (message->get_type())
    {
        case wamp_message_type::AUTHENTICATE:
            break;
        case wamp_message_type::CALL:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_call_message* call_message = static_cast<wamp_call_message*>(message.get());
                router->process_call_message(connection->get_session_id(), call_message);
            }
            break;
        }
        case wamp_message_type::CANCEL:
            break;
        case wamp_message_type::ERROR:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_error_message* error_message = static_cast<wamp_error_message*>(message.get());
                router->process_error_message(connection->get_session_id(), error_message);
            }
            break;
        }
        case wamp_message_type::GOODBYE:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_goodbye_message* goodbye_message = static_cast<wamp_goodbye_message*>(message.get());
                router->process_goodbye_message(connection->get_session_id(), goodbye_message);
                router->detach_session(connection->get_session_id());
            }
            connection->clear_data();
            break;
        }
        case wamp_message_type::HELLO:
        {
            wamp_hello_message* hello_message = static_cast<wamp_hello_message*>(message.get());
            std::shared_ptr<wamp_router> router = m_routers->get_router(hello_message->get_realm());
            if (!router) {
                std::unique_ptr<wamp_abort_message> abort_message(new wamp_abort_message);
                abort_message->set_reason("wamp.error.no_such_realm");
                transport->send_message(std::move(*abort_message));
            } else {
                wamp_session_id id;
                auto generator = router->get_session_id_generator();
                do {
                    id = generator->generate();
                } while(router->has_session(id));

                connection->set_session_id(id);
                connection->set_realm(hello_message->get_realm());

                // We need to setup the sessions roles before attaching the session
                // so that we know whether or not to also attach the session to the
                // dealer and the broker.
                wamp_hello_details hello_details;
                hello_details.unmarshal(hello_message->get_details());

                auto session = std::make_shared<wamp_session>(
                        id, hello_message->get_realm(), std::move(transport));
                session->set_roles(hello_details.get_roles());

                router->attach_session(session);
                router->process_hello_message(id, hello_message);
            }
            break;
        }
        case wamp_message_type::PUBLISH:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_publish_message* publish_message = static_cast<wamp_publish_message*>(message.get());
                router->process_publish_message(connection->get_session_id(), publish_message);
            }
            break;
        }
        case wamp_message_type::REGISTER:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_register_message* register_message = static_cast<wamp_register_message*>(message.get());
                router->process_register_message(connection->get_session_id(), register_message);
            }
            break;
        }
        case wamp_message_type::SUBSCRIBE:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_subscribe_message* subscribe_message = static_cast<wamp_subscribe_message*>(message.get());
                router->process_subscribe_message(connection->get_session_id(), subscribe_message);
            }
            break;
        }
        case wamp_message_type::UNREGISTER:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_unregister_message* unregister_message = static_cast<wamp_unregister_message*>(message.get());
                router->process_unregister_message(connection->get_session_id(), unregister_message);
            }
            break;
        }
        case wamp_message_type::UNSUBSCRIBE:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_unsubscribe_message* unsubscribe_message = static_cast<wamp_unsubscribe_message*>(message.get());
                router->process_unsubscribe_message(connection->get_session_id(), unsubscribe_message);
            }
            break;
        }
        case wamp_message_type::YIELD:
        {
            std::shared_ptr<wamp_router> router = m_routers->get_router(connection->get_realm());
            if (router) {
                wamp_yield_message* yield_message = static_cast<wamp_yield_message*>(message.get());
                router->process_yield_message(connection->get_session_id(), yield_message);
            }
            break;
        }
        default:
            break;
    }
}

} // namespace bonefish
