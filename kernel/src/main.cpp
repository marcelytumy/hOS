#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "graphics.hpp"
#include "font.hpp"
#include "../../ui/include/ui.hpp"
#include "../../ui/include/cursor.hpp"
#include "../../input/include/mouse.hpp"

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

namespace {

__attribute__((used, section(".limine_requests")))
volatile LIMINE_BASE_REVISION(3);

}

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

namespace {

__attribute__((used, section(".limine_requests")))
volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

}

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .cpp file, as seen fit.

namespace {

__attribute__((used, section(".limine_requests_start")))
volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
volatile LIMINE_REQUESTS_END_MARKER;

}

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .cpp file.

extern "C" {

void *memcpy(void *__restrict dest, const void *__restrict src, std::size_t n) {
    std::uint8_t *__restrict pdest = static_cast<std::uint8_t *__restrict>(dest);
    const std::uint8_t *__restrict psrc = static_cast<const std::uint8_t *__restrict>(src);

    for (std::size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, std::size_t n) {
    std::uint8_t *p = static_cast<std::uint8_t *>(s);

    for (std::size_t i = 0; i < n; i++) {
        p[i] = static_cast<uint8_t>(c);
    }

    return s;
}

void *memmove(void *dest, const void *src, std::size_t n) {
    std::uint8_t *pdest = static_cast<std::uint8_t *>(dest);
    const std::uint8_t *psrc = static_cast<const std::uint8_t *>(src);

    if (src > dest) {
        for (std::size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (std::size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, std::size_t n) {
    const std::uint8_t *p1 = static_cast<const std::uint8_t *>(s1);
    const std::uint8_t *p2 = static_cast<const std::uint8_t *>(s2);

    for (std::size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

}

// Halt and catch fire function.
namespace {

void hcf() {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

}

// The following stubs are required by the Itanium C++ ABI (the one we use,
// regardless of the "Itanium" nomenclature).
// Like the memory functions above, these stubs can be moved to a different .cpp file,
// but should not be removed, unless you know what you are doing.
extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { hcf(); }
    void *__dso_handle;
}

// Extern declarations for global constructors array.
extern void (*__init_array[])();
extern void (*__init_array_end[])();

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain() {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Call global constructors.
    for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == nullptr
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    
    // Initialize graphics system
    Graphics graphics(framebuffer);
    
    // Clear screen to white
    graphics.clear_screen(0x000000);
    
    // Draw centered, scaled text below the logo
    graphics.draw_string_centered_scaled("hOS 0.1", framebuffer->height / 2, 0xFFFFFF, default_font, 4);

    // Draw loading bar outline
    int bar_x = framebuffer->width / 2 - 100;
    int bar_y = framebuffer->height / 2 + 50;
    int bar_width = 200;
    int bar_height = 20;

    graphics.draw_rect(bar_x, bar_y, bar_width, bar_height, 0xFFFFFF);

    // Animate loading bar fill
    for (int fill = 0; fill <= bar_width; fill++) {
        // Fill the bar up to 'fill' width
        graphics.fill_rect(bar_x, bar_y, fill, bar_height, 0xFFFFFF);

        // Simple delay loop (not precise, but enough for a visible animation)
        for (volatile int d = 0; d < 1000000; ++d) { }
    }

    // Enable double-buffering backbuffer
    static uint32_t backbuffer_storage[1920 * 1080];
    graphics.enable_backbuffer(backbuffer_storage, 1920u * 1080u);

    // Compute initial centered window rect
    const uint32_t screen_w = graphics.get_width();
    const uint32_t screen_h = graphics.get_height();
    const uint32_t taskbar_h = ui::get_taskbar_height(screen_h);
    const uint32_t usable_h = screen_h - taskbar_h - 20;
    const uint32_t usable_w = screen_w - 40;
    uint32_t win_w = (usable_w * 3) / 5;
    uint32_t win_h = (usable_h * 3) / 5;
    if (win_w < 320) win_w = 320;
    if (win_h < 200) win_h = 200;
    ui::Rect window_rect{(screen_w - win_w) / 2, (screen_h - taskbar_h - win_h) / 2, win_w, win_h};

    // Draw desktop with window (to backbuffer), then present
    ui::draw_desktop(graphics, window_rect);
    graphics.present();

    // Initialize mouse and cursor
    input::Ps2Mouse mouse;
    mouse.initialize();
    ui::Cursor cursor;
    cursor.set_position(screen_w / 2, screen_h / 2, screen_w, screen_h);
    cursor.draw(graphics);
    graphics.present();

    bool dragging = false;
    uint32_t drag_off_x = 0;
    uint32_t drag_off_y = 0;
    bool prev_left = false;

    // Simple event loop: poll mouse, move cursor, support window dragging
    for (;;) {
        int8_t dx = 0, dy = 0; bool left = false, right = false, middle = false;
        if (!mouse.poll_packet(dx, dy, left, right, middle)) {
            continue;
        }

        // Update cursor position
        cursor.erase(graphics);
        cursor.move_by(dx, dy, screen_w, screen_h);

        // Handle drag begin/end based on left button edge
        if (left && !prev_left) {
            if (ui::point_in_titlebar(window_rect, cursor.x(), cursor.y())) {
                dragging = true;
                drag_off_x = cursor.x() - window_rect.x;
                drag_off_y = cursor.y() - window_rect.y;
            }
        } else if (!left && prev_left) {
            dragging = false;
        }

        // Update window position if dragging
        if (dragging) {
            int64_t new_x = static_cast<int64_t>(cursor.x()) - static_cast<int64_t>(drag_off_x);
            int64_t new_y = static_cast<int64_t>(cursor.y()) - static_cast<int64_t>(drag_off_y);
            if (new_x < 0) new_x = 0;
            if (new_y < 0) new_y = 0;
            if (new_x > static_cast<int64_t>(screen_w - window_rect.w)) new_x = screen_w - window_rect.w;
            uint32_t bottom_limit = screen_h - taskbar_h - window_rect.h;
            if (new_y > static_cast<int64_t>(bottom_limit)) new_y = bottom_limit;
            window_rect.x = static_cast<uint32_t>(new_x);
            window_rect.y = static_cast<uint32_t>(new_y);
        }

        // Redraw desktop and cursor to backbuffer then present once
        ui::draw_desktop(graphics, window_rect);
        cursor.draw(graphics);
        graphics.present();

        prev_left = left;
    }
}
