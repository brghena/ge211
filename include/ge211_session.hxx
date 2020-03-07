#pragma once

#include <atomic>

namespace ge211 {

namespace detail {

struct PINNED
{
    PINNED() = default;

    PINNED(PINNED&&) = delete;
    PINNED& operator=(PINNED&&) = delete;
    PINNED(const PINNED&) = delete;
    PINNED& operator=(const PINNED&) = delete;
};

struct Sdl_session : PINNED
{
    Sdl_session();
    ~Sdl_session();
};

struct Mix_session : PINNED
{
    Mix_session();
    ~Mix_session();

    bool enabled;
};

struct Img_session : PINNED
{
    Img_session();
    ~Img_session();
};

struct Ttf_session : PINNED
{
    Ttf_session();
    ~Ttf_session();
};

struct Text_input_session : PINNED
{
    Text_input_session();
    ~Text_input_session();
};

class Session
{
public:
    Session();
    ~Session();

    bool is_mixer_enabled() const { return mix_.enabled; }

    static void check_session(const char*);

private:
    Sdl_session        sdl_;
    Mix_session        mix_;
    Img_session        img_;
    Ttf_session        ttf_;
    Text_input_session text_input_;

    static std::atomic<int> session_count_;
};

} // end namespace detail

}
