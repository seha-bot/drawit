#include "drawit.hpp"
#include <algorithm>
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

static drw::Key mapKey(uint32_t key) {
    if (key == 113) {
        return drw::Key::left;
    } else if (key == 114) {
        return drw::Key::right;
    } else if (key == 111) {
        return drw::Key::up;
    } else if (key == 116) {
        return drw::Key::down;
    } else if (key == 65) {
        return drw::Key::space;
    } else if (key == 66) {
        return drw::Key::escape;
    }

    return drw::Key::none;
}

void drw::Window::pollEvents() noexcept {
    auto eventCount = XPending(handle->display);

    Key key;
    XEvent event, peekEvent;
    while (eventCount != 0) {
        XNextEvent(handle->display, &event);
        eventCount--;

        if (event.type == UnmapNotify) {
            _isClosed = true;
        } else if (event.type == KeyRelease) {
            if (eventCount != 0) {
                XPeekEvent(handle->display, &peekEvent);
                if (peekEvent.type == KeyPress && event.xkey.time == peekEvent.xkey.time && event.xkey.keycode == peekEvent.xkey.keycode) {
                    XNextEvent(handle->display, &peekEvent);
                    eventCount--;
                    continue;
                }
            }

            key = mapKey(event.xkey.keycode);
            if (key != drw::Key::none) {
                const auto it = std::find(downKeys.begin(), downKeys.end(), key);
                if (it != downKeys.end()) {
                    downKeys.erase(it);
                }
            }
        } else {
            key = mapKey(event.xkey.keycode);
            if (key != drw::Key::none) {
                const auto it = std::find(downKeys.begin(), downKeys.end(), key);
                if (it == downKeys.end()) {
                    downKeys.push_back(key);
                }
            }
        }
    }
}

bool drw::Window::isKeyDown(Key key) const noexcept {
    return std::find(downKeys.begin(), downKeys.end(), key) != downKeys.end();
}

bool drw::Window::isKeyDownOnce(Key key) noexcept {
    const auto it = std::find(downKeys.begin(), downKeys.end(), key);
    if (it != downKeys.end()) {
        downKeys.erase(it);
        return true;
    }
    return false;
}

bool drw::Window::isClosed() const noexcept {
    return _isClosed;
}

drw::Window::Window(uint32_t width, uint32_t height) : handle(nullptr), _isClosed(false), width(width), height(height) {
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

    XSelectInput(handle->display, handle->window, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
}

drw::Window::~Window() {
    delete[] framebuffer;
    XCloseDisplay(handle->display);
    delete handle;
}
