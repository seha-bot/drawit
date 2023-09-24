#ifndef DRAWITHPP
#define DRAWITHPP

#include <cstdint>
#include <optional>

namespace drw {
    enum EventType { keyPress, windowUnmapped };

    class Event {
    public:
        EventType type;
        uint32_t keycode;

        Event(EventType type, uint32_t keycode) : type(type), keycode(keycode) {}
    };

    struct WindowHandle;

    class Window {
        WindowHandle *handle;
        uint32_t *framebuffer;
    public:
        const uint32_t width;
        const uint32_t height;

        void drawPixel(uint32_t x, uint32_t y, uint32_t color);
        void drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

        void flush() noexcept;

        void clearFramebuffer() noexcept;

        std::optional<Event> nextEvent() const noexcept;

        Window(uint32_t width, uint32_t height);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        ~Window();
    };
}

#endif /* DRAWITHPP */
