#include "ge211_sprites.h"
#include "ge211_error.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cmath>

namespace ge211 {

using namespace detail;

Sprite_set::Sprite_set() {}

Sprite_set&
Sprite_set::add_sprite(const Sprite& sprite, Position xy, int z,
                       const Transform& t)
{
    sprites_.emplace_back(sprite, xy, z, t);
    return *this;
}

Sprite_set& Sprite_set::add_sprite(const Sprite& sprite, Position xy, int z)
{
    return add_sprite(sprite, xy, z, Transform{});
}

namespace detail {

Placed_sprite::Placed_sprite(const Sprite& sprite, Position xy,
                             int z, const Transform& transform) noexcept
        : sprite{&sprite}, xy{xy}, z{z}, transform{transform}
{ }

void Placed_sprite::render(Renderer& dst) const
{
    sprite->render(dst, xy, transform);
}

bool operator<(const Placed_sprite& s1, const Placed_sprite& s2) noexcept
{
    return s1.z > s2.z;
}

Dimensions Texture_sprite::dimensions() const
{
    return get_texture_().dimensions();
}

void Texture_sprite::render(Renderer& renderer,
                            Position position,
                            const Transform& transform) const
{
    if (transform.is_identity())
        renderer.copy(get_texture_(), position);
    else
        renderer.copy(get_texture_(), position, transform);
}

void Texture_sprite::prepare(const Renderer& renderer) const
{
    renderer.prepare(get_texture_());
}

delete_ptr<SDL_Surface> Render_sprite::create_surface_(Dimensions dimensions)
{
    SDL_Surface* surface =
            SDL_CreateRGBSurfaceWithFormat(0,
                                           dimensions.width,
                                           dimensions.height,
                                           32,
                                           SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        return {surface, &SDL_FreeSurface};
    }

    throw Host_error{"Could not create sprite surface"};
}

Render_sprite::Render_sprite(Dimensions dimensions)
        : texture_{create_surface_(dimensions)}
{ }

const Texture& Render_sprite::get_texture_() const
{
    return texture_;
}

SDL_Surface* Render_sprite::as_surface()
{
    SDL_Surface* result = texture_.as_surface();
    if (result) return result;

    throw Ge211_logic_error{"Render_sprite::as_surface: already a texture"};
}

void Render_sprite::fill_surface(Color color)
{
    auto surface = as_surface();
    SDL_FillRect(surface, nullptr, color.to_sdl_(surface->format));
}

void Render_sprite::fill_rectangle(Rectangle rect, Color color)
{
    auto surface = as_surface();
    SDL_Rect rect_buf = rect;
    SDL_FillRect(surface, &rect_buf, color.to_sdl_(surface->format));
}

void Render_sprite::set_pixel(Position xy, Color color)
{
    fill_rectangle({xy.x, xy.y, 1, 1}, color);
}

} // end namespace detail

namespace sprites {

static Dimensions check_rectangle_dimensions(Dimensions dims)
{
    if (dims.width <= 0 || dims.height <= 0) {
        throw Client_logic_error(
                "Rectangle_sprite: width and height must both be positive");
    }

    return dims;
}

Rectangle_sprite::Rectangle_sprite(Dimensions dims, Color color)
        : Render_sprite{check_rectangle_dimensions(dims)}
{
    fill_surface(color);
}

void Rectangle_sprite::recolor(Color color)
{
    *this = Rectangle_sprite{dimensions(), color};
}

static Dimensions compute_circle_dimensions(int radius)
{
    if (radius <= 0) {
        throw Client_logic_error("Circle_sprite: radius must be positive");
    }

    return {radius * 2, radius * 2};
}

Circle_sprite::Circle_sprite(int radius, Color color)
        : Render_sprite{compute_circle_dimensions(radius)}
{
    const int cx = radius;
    const int cy = radius;

    for (int y = 0; y < radius; ++y) {
        for (int x = 0; x < radius; ++x) {
            if (x * x + y * y < radius * radius) {
                set_pixel({cx + x, cy + y}, color);
                set_pixel({cx + x, cy - y - 1}, color);
                set_pixel({cx - x - 1, cy + y}, color);
                set_pixel({cx - x - 1, cy - y - 1}, color);
            }
        }
    }
}

void Circle_sprite::recolor(Color color)
{
    *this = Circle_sprite{radius_(), color};
}

int Circle_sprite::radius_() const
{
    return dimensions().width >> 1;
}

Texture
Image_sprite::load_texture_(const std::string& filename)
{
    File_resource file(filename);
    SDL_Surface* raw = IMG_Load_RW(file.get_raw_(), 0);
    if (raw) return Texture(raw);

    throw Image_error::could_not_load(filename);
}

Image_sprite::Image_sprite(const std::string& filename)
        : texture_{load_texture_(filename)} {}

const Texture& Image_sprite::get_texture_() const
{
    return texture_;
}

Texture
Text_sprite::create_texture(const Builder& config)
{
    SDL_Surface* raw;

    std::string message = config.message();

    if (message.empty())
        return Texture{};

    if (config.word_wrap() > 0) {
        raw = TTF_RenderUTF8_Blended_Wrapped(
                config.font().get_raw_(),
                message.c_str(),
                config.color().to_sdl_(),
                static_cast<uint32_t>(config.word_wrap()));
    } else {
        auto render = config.antialias() ?
                      &TTF_RenderUTF8_Blended :
                      &TTF_RenderUTF8_Solid;
        raw = render(config.font().get_raw_(),
                     message.c_str(),
                     config.color().to_sdl_());
    }

    if (!raw)
        throw Host_error{"Could not render text: “" + message + "”"};
    else
        return Texture{raw};
}

Text_sprite::Text_sprite(const Text_sprite::Builder& config)
        : texture_{create_texture(config)} {}

Text_sprite::Text_sprite()
        : texture_{} {}

Text_sprite::Text_sprite(const std::string& message,
                         const Font& font)
        : Text_sprite{Builder{font}.message(message)} {}

const Texture& Text_sprite::get_texture_() const
{
    assert_initialized_();
    return texture_;
}

void Text_sprite::assert_initialized_() const
{
    if (texture_.empty())
        throw Client_logic_error{"Attempt to render empty Text_sprite"};
}

Text_sprite::Builder::Builder(const Font& font)
        : message_{}, font_{&font}, color_{Color::white()}, antialias_{true},
          word_wrap_{0} {}

Text_sprite::Builder& Text_sprite::Builder::message(const std::string& message)
{
    message_.str(message);
    return *this;
}

Text_sprite::Builder& Text_sprite::Builder::font(const Font& font)
{
    font_ = &font;
    return *this;
}

Text_sprite::Builder& Text_sprite::Builder::color(Color color)
{
    color_ = color;
    return *this;
}

Text_sprite::Builder& Text_sprite::Builder::antialias(bool antialias)
{
    antialias_ = antialias;
    return *this;
}

Text_sprite::Builder& Text_sprite::Builder::word_wrap(int word_wrap)
{
    if (word_wrap < 0) word_wrap = 0;
    word_wrap_ = static_cast<uint32_t>(word_wrap);
    return *this;
}

Text_sprite Text_sprite::Builder::build() const
{
    return Text_sprite{*this};
}

std::string Text_sprite::Builder::message() const
{
    return message_.str();
}

const Font& Text_sprite::Builder::font() const
{
    return *font_;
}

Color Text_sprite::Builder::color() const
{
    return color_;
}

bool Text_sprite::Builder::antialias() const
{
    return antialias_;
}

int Text_sprite::Builder::word_wrap() const
{
    return static_cast<int>(word_wrap_);
}

void Text_sprite::reconfigure(const Text_sprite::Builder& config)
{
    texture_ = create_texture(config);
}

bool Text_sprite::empty() const
{
    return texture_.empty();
}

} // end namespace sprites

}
