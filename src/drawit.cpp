#include "drawit.hpp"
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <X11/Xlib.h>

struct drw::WindowHandle {
    Display *display;
    ::Window window;
    GC graphicsContext;
    XImage *image;
};

void drw::Window::drawPixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= width || y >= height) {
        throw std::runtime_error("Trying to draw outside of window borders");
    }
    framebuffer[x + y * width] = color;
}

void drw::Window::drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t yi = 0; yi < height; yi++) {
        for (uint32_t xi = 0; xi < width; xi++) {
            drawPixel(x + xi, y + yi, color);
        }
    }
}

void drw::Window::flush() noexcept {
    XPutImage(handle->display, handle->window, handle->graphicsContext, handle->image, 0, 0, 0, 0, width, height);
}

void drw::Window::clearFramebuffer() noexcept {
    std::memset(framebuffer, 0, sizeof(uint32_t) * width * height);
}

std::optional<drw::Event> drw::Window::nextEvent() const noexcept {
    if (XPending(handle->display) == 0) {
        return std::nullopt;
    }
    XEvent event;
    XNextEvent(handle->display, &event);

    if (event.type == UnmapNotify) {
        return drw::Event(EventType::windowUnmapped, 0);
    } else {
        return drw::Event(EventType::keyPress, event.xkey.keycode);
    }
}

drw::Window::Window(uint32_t width, uint32_t height) : handle(nullptr), width(width), height(height) {
    handle = new WindowHandle;

    handle->display = XOpenDisplay(nullptr);
    if (!handle->display) {
        throw std::runtime_error("Can't open display");
    }

    const auto root = XDefaultRootWindow(handle->display);
    handle->window = XCreateSimpleWindow(handle->display, root, 0, 0, width, height, 0, 0, 0x0);
    handle->graphicsContext = XCreateGC(handle->display, handle->window, 0, nullptr);
    XMapWindow(handle->display, handle->window);
    XFlush(handle->display);

    framebuffer = new uint32_t[width * height];
    clearFramebuffer();

    XWindowAttributes xwa;
    XGetWindowAttributes(handle->display, handle->window, &xwa);
    handle->image = XCreateImage(handle->display, xwa.visual, xwa.depth, ZPixmap, 0, (char *)framebuffer, width, height, 32, width * sizeof(uint32_t));
    if (handle->image == nullptr) {
        throw std::runtime_error("Can't create framebuffer");
    }

    XSelectInput(handle->display, handle->window, StructureNotifyMask | KeyPressMask);
}

drw::Window::~Window() {
    delete[] framebuffer;
    XCloseDisplay(handle->display);
    delete handle;
}
