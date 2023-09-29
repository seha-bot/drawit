#ifndef DRAWITHPP
#define DRAWITHPP

#include <cstdint>
#include <vector>

namespace drw {
    enum Key {
        none,
        left,
        right,
        up,
        down,
        space,
        escape
    };

    class Window {
        struct WindowHandle *handle;
        uint32_t *framebuffer;
        std::vector<Key> downKeys;
        bool _isClosed;
    public:
        const uint32_t width;
        const uint32_t height;

        void drawPixel(uint32_t x, uint32_t y, uint32_t color);
        void drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

        void flush() noexcept;
        void clearFramebuffer() noexcept;

        void pollEvents() noexcept;
        bool isKeyDown(Key key) const noexcept;
        bool isKeyDownOnce(Key key) noexcept;

        bool isClosed() const noexcept;

        Window(uint32_t width, uint32_t height);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        ~Window();
    };
}

#endif /* DRAWITHPP */
