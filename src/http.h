#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <unordered_map>
#include <type_traits>

namespace crawler
{
class Response
{
public:
    class Block
    {
    public:
        Block(std::string& d): data(d) { }
        Block(std::string& d, const Block& b)
            : data(d), start(b.start), finish(b.finish), length(b.length)
        {
        }

        ~Block() = default;

        void init(std::size_t start, std::size_t finish);
        const char* c_str() const;

        const char* begin() const;
        char* begin();
        const char* end() const;
        char* end();
        std::string::size_type size() const;
        std::string::reference operator[](std::size_t pos);
        std::string::const_reference operator[](std::size_t pos) const;
        bool start_with(const char* prefix, std::size_t& pos) const;
        void pass_until_meet(const char* suffix, std::size_t& pos) const;
    private:
        std::string &data;
        std::size_t start;
        std::size_t finish;
        std::size_t length;
    };
public:
    Response(std::string&& cont);
    Response(Response&& res)
        : raw_content(std::move(res.raw_content)), head(raw_content, res.head),
                                                   body(raw_content, res.head)
    {
    }

    ~Response() = default;

    const std::string& get_data() const
    {
        return raw_content;
    }

    Block& get_head();
    Block& get_body();

private:
    void remove_unrelated();
    void generate_blocks();
    bool start_with(const char *prefix, std::size_t& pos) const;
    void pass_until_meet(const char *suffix, std::size_t& pos);
    void reverse_pass_until_meet(const char* prefix, std::size_t& pos);
private:
    std::string raw_content;
    std::size_t start;
    std::size_t finish;
    std::size_t length;
    Block head;
    Block body;
}; // class Response

class Request
{
    using Header = std::unordered_map<std::string, std::string>;
public:
    enum Method
    {
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        TRACE,
        CONNECT
    };
    Request(const std::string& url, Method method = Method::GET, const Header& header = {
        {"Content-type", "text/html"},
        {"Connection", "Close"},
        {"User-Agent", "Simple_curl"}
    });

    ~Request() = default;

    // 第一次调用，完成DNS解析
    Response slow_get_response() const;

    // 之后调用，不用DNS解析
    Response fast_get_response() const;

private:
    Response get_response() const;

    std::string content;
    std::string host_url;

    static struct in_addr host_ip;
}; // class Request

} // namespace crawler
#endif // HTTP_H