#ifndef BOOST_BEAST_EXAMPLE_COMMON_ROOT_CERTIFICATES_HPP
#define BOOST_BEAST_EXAMPLE_COMMON_ROOT_CERTIFICATES_HPP

#include <boost/asio/ssl.hpp>
#include <string>
#include <vector>

namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>

namespace detail {

inline void load_root_certificates(ssl::context& ctx, boost::system::error_code& ec, std::string&& certbuffer)
{
    std::string cert(certbuffer);
    // printf("%s\n",cert.data());

    ctx.add_certificate_authority(
        boost::asio::buffer(cert.data(), cert.size()), ec);
    if(ec)
        return;
}

} // detail

// Load the root certificates into an ssl::context

inline void load_root_certificates(ssl::context& ctx, boost::system::error_code& ec,std::string&& certbuffer)
{
    detail::load_root_certificates(ctx, ec, std::move(certbuffer));
}

inline void load_root_certificates(ssl::context& ctx,std::string&& certbuffer)
{
    boost::system::error_code ec;
    detail::load_root_certificates(ctx, ec,std::move(certbuffer));
    if(ec)
        throw boost::system::system_error{ec};
}

#endif