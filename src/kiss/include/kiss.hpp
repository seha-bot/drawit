#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_render.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "kiss_sdl.h"

namespace kiss {

struct Component {
    unsigned x, y;
    virtual void init(kiss_window *window, SDL_Renderer *renderer) = 0;
    virtual void process_event(SDL_Event *event, int *is_ready) = 0;
    virtual void draw(SDL_Renderer *renderer) = 0;

    Component(unsigned x_pos, unsigned y_pos) noexcept : x(x_pos), y(y_pos) {}
    Component(const Component&) = default;
    Component& operator=(const Component&) = default;
    virtual ~Component() {}
};

class Window final {
    kiss_array m_objects;
    SDL_Renderer *m_renderer = nullptr;
    kiss_window m_window;
    bool m_is_open = true;
    // TODO rename every is_ready to should_redraw
    int m_is_ready = 0;

    std::vector<std::unique_ptr<Component>> m_components;

public:
    Window(const std::string& title, unsigned w, unsigned h)
        : m_renderer(
              kiss_init(const_cast<char *>(title.data()), &m_objects, w, h)) {
        if (m_renderer == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }

        kiss_window_new(&m_window, nullptr, 1, 0, 0, kiss_screen_width,
                        kiss_screen_height);
        m_window.visible = 1;
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window() { kiss_clean(&m_objects); }

    template <typename T>
    T& register_component(std::unique_ptr<T>&& component) {
        component->init(&m_window, m_renderer);
        m_components.push_back(std::move(component));
        return static_cast<T&>(*m_components.back());
    }

    void process_event(const SDL_Event& event) noexcept {
        if (event.type == SDL_QUIT) {
            m_is_open = false;
        }

        kiss_window_event(&m_window, const_cast<SDL_Event *>(&event),
                          &m_is_ready);

        for (auto& e : m_components) {
            e->process_event(const_cast<SDL_Event *>(&event), &m_is_ready);
        }
    }

    void draw() noexcept {
        kiss_window_draw(&m_window, m_renderer);

        for (auto& e : m_components) {
            e->draw(m_renderer);
        }
    }

    void flush() noexcept {
        SDL_RenderPresent(m_renderer);
        m_is_ready = 0;
    }

    void force_redraw() noexcept { m_is_ready = 1; }

    bool is_open() const noexcept { return m_is_open; }
    bool is_ready() const noexcept { return m_is_ready; }
};

class Label final : public Component {
    kiss_label m_label;

public:
    Label(unsigned x, unsigned y) noexcept : Component(x, y) {}

    void update_text(const std::string& text) noexcept {
        kiss_string_copy(m_label.text, KISS_MAX_LABEL, nullptr,
                         const_cast<char *>(text.data()));
    }

    void init(kiss_window *window, SDL_Renderer *renderer) noexcept override {
        char empty[] = "";
        kiss_label_new(&m_label, window, empty, x, y);
    }

    void process_event(SDL_Event *event, int *is_ready) noexcept override {}

    void draw(SDL_Renderer *renderer) noexcept override {
        kiss_label_draw(&m_label, renderer);
    }
};

class Button final : public Component {
    kiss_button m_button;
    std::string m_text;
    std::function<void()> m_on_click;

public:
    Button(const std::string& text, unsigned x, unsigned y,
           std::function<void()> on_click) noexcept
        : Component(x, y), m_text(text), m_on_click(on_click) {}

    void init(kiss_window *window, SDL_Renderer *renderer) noexcept override {
        kiss_button_new(&m_button, window, const_cast<char *>(m_text.data()), x,
                        y);
    }

    void process_event(SDL_Event *event, int *is_ready) noexcept override {
        if (kiss_button_event(&m_button, event, is_ready)) {
            m_on_click();
        }
    }

    void draw(SDL_Renderer *renderer) noexcept override {
        kiss_button_draw(&m_button, renderer);
    }
};

class Container final : public Component {
    kiss_window m_window;
    SDL_Renderer *m_renderer;
    std::vector<std::unique_ptr<Component>> m_components;

public:
    unsigned w, h;

    Container(unsigned x, unsigned y, unsigned width, unsigned height) noexcept
        : Component(x, y), w(width), h(height) {}

    void init(kiss_window *window, SDL_Renderer *renderer) noexcept override {
        kiss_window_new(&m_window, nullptr, 1, x, y, w, h);
        m_renderer = renderer;
    }

    void process_event(SDL_Event *event, int *is_ready) noexcept override {
        kiss_window_event(&m_window, event, is_ready);

        for (auto& e : m_components) {
            e->process_event(event, is_ready);
        }
    }

    void draw(SDL_Renderer *renderer) noexcept override {
        kiss_window_draw(&m_window, renderer);

        for (auto& e : m_components) {
            e->draw(renderer);
        }
    }

    template <typename T>
    T& register_component(std::unique_ptr<T>&& component) {
        component->init(&m_window, m_renderer);
        m_components.push_back(std::move(component));
        return static_cast<T&>(*m_components.back());
    }

    void set_visibility(bool visibile) noexcept { m_window.visible = visibile; }
};

class Canvas final : public Component {
    std::function<void(Canvas&)> m_on_draw;
    unsigned m_tex_w, m_tex_h, m_scr_w, m_scr_h;

    SDL_Texture *m_texture = nullptr;
    SDL_PixelFormat *m_format = nullptr;

public:
    Canvas(unsigned x, unsigned y, unsigned tex_w, unsigned tex_h,
           unsigned scr_w, unsigned scr_h,
           std::function<void(Canvas&)> on_draw) noexcept
        : Component(x, y),
          m_on_draw(on_draw),
          m_tex_w(tex_w),
          m_tex_h(tex_h),
          m_scr_w(scr_w),
          m_scr_h(scr_h) {}

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    ~Canvas() {
        SDL_DestroyTexture(m_texture);
        SDL_FreeFormat(m_format);
    }

    // unsigned tex_width() const noexcept { return m_tex_w; }
    // unsigned tex_height() const noexcept { return m_tex_h; }

    void init(kiss_window *window, SDL_Renderer *renderer) override {
        m_texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                              SDL_TEXTUREACCESS_STREAMING, m_tex_w, m_tex_h);
        if (m_texture == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }

        m_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
        if (m_format == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }
    }

    void process_event(SDL_Event *event, int *is_ready) override {}

    // TODO organize
    uint32_t *pixels = nullptr;

    void fill(uint32_t color) noexcept {
        std::fill(pixels, pixels + m_tex_w * m_tex_h, color);
    }

    void set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= m_tex_w || y >= m_tex_h) {
            throw std::runtime_error("canvas coordinate out of bounds");
        }
        pixels[x + y * m_tex_w] = SDL_MapRGB(m_format, r, g, b);
    }

    void draw(SDL_Renderer *renderer) override {
        int pitch;
        if (SDL_LockTexture(m_texture, nullptr,
                            reinterpret_cast<void **>(&pixels), &pitch) != 0) {
            throw std::runtime_error(SDL_GetError());
        }

        m_on_draw(*this);

        SDL_UnlockTexture(m_texture);

        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = m_scr_w;
        rect.h = m_scr_h;
        // assume no error :)
        SDL_RenderCopy(renderer, m_texture, nullptr, &rect);
    }
};

class KeyboardListener final : public Component {
    const uint8_t *m_keyboard;
    std::vector<bool> m_key_states;

public:
    KeyboardListener() noexcept : Component(0, 0) {
        int size;
        m_keyboard = SDL_GetKeyboardState(&size);
        m_key_states.resize(size, false);
    }

    bool is_key_down(SDL_Scancode key) const noexcept {
        return m_keyboard[key];
    }

    bool is_key_down_once(SDL_Scancode key) noexcept {
        if (m_keyboard[key] && !m_key_states[key]) {
            return m_key_states[key] = true;
        }
        return false;
    }

    void process_event(SDL_Event *event, int *) noexcept override {
        if (event->type == SDL_KEYUP) {
            m_key_states[event->key.keysym.scancode] = false;
        }
    }

    void init(kiss_window *, SDL_Renderer *) noexcept override {}
    void draw(SDL_Renderer *) noexcept override {}
};

}  // namespace kiss
